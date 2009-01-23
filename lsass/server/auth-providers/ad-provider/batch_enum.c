/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        batch_enum.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "adprovider.h"
#include "batch_common.h"
#include "batch_gather.h"
#include "batch_marshal.h"

typedef struct __AD_CELL_COOKIE_DATA
{
    // Initially, this is set to NULL to indicate that the primary cell
    // should be searched.
    const DLINKEDLIST* pCurrentCell;
    LSA_SEARCH_COOKIE LdapCookie;

    // This hash table is used to ensure that the same user/group is not
    // returned twice through linked cells. It is only allocated if linked
    // cells are used in the computer's cell.
    PLSA_HASH_TABLE pEnumeratedSids;
} AD_CELL_COOKIE_DATA, *PAD_CELL_COOKIE_DATA;

static
VOID
LsaAdBatchFreeCellCookie(
    IN OUT PVOID pvCookie
    )
{
    PAD_CELL_COOKIE_DATA pData = (PAD_CELL_COOKIE_DATA)pvCookie;

    if (pData != NULL)
    {
        LsaFreeCookieContents(&pData->LdapCookie);
        LsaHashSafeFree(&pData->pEnumeratedSids);

        LSA_SAFE_FREE_MEMORY(pData);
    }
}

static
DWORD
LsaAdBatchEnumGetNoMoreError(
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    DWORD dwError = 0;

    switch (ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LSA_ERROR_NO_MORE_USERS;
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            dwError = LSA_ERROR_NO_MORE_GROUPS;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
    }

    return dwError;
}

static
DWORD
LsaAdBatchEnumGetScopeRoot(
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsByRealObject,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    OUT PSTR* ppszScopeRoot
    )
{
    DWORD dwError = 0;
    PSTR pszScopeRoot = NULL;

    if (bIsByRealObject)
    {
        dwError = LsaLdapConvertDomainToDN(pszDnsDomainName, &pszScopeRoot);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        PCSTR pszContainer = NULL;
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszContainer = "Users";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszContainer = "Groups";
                break;
            default:
                LSA_ASSERT(FALSE);
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                        &pszScopeRoot,
                        "CN=%s,%s",
                        pszContainer,
                        pszCellDn);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppszScopeRoot = pszScopeRoot;
    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszScopeRoot);
    goto cleanup;
}


static
PCSTR
LsaAdBatchEnumGetRealQuery(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    PCSTR pszQuery = NULL;

    if (bIsSchemaMode)
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=User)(!(objectClass=Computer))(sAMAccountName=*)(uidNumber=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=Group)(!(objectClass=Computer))(sAMAccountName=*)(gidNumber=*))";
                break;
        }
    }
    else
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=User)(!(objectClass=Computer))(sAMAccountName=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=Group)(!(objectClass=Computer))(sAMAccountName=*))";
                break;
        }
    }

    return pszQuery;
}

static
PCSTR
LsaAdBatchEnumGetPseudoQuery(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    PCSTR pszQuery = NULL;

    if (bIsSchemaMode)
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=posixAccount)(keywords=objectClass=centerisLikewiseUser)(uidNumber=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=posixGroup)(keywords=objectClass=centerisLikewiseGroup)(gidNumber=*))";
                break;
        }
    }
    else
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uidNumber=*))";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=gidNumber=*))";
                break;
        }
    }

    return pszQuery;
}

static
PCSTR
LsaAdBatchEnumGetQuery(
    IN BOOLEAN bIsByRealObject,
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    PCSTR pszQuery = NULL;

    if (bIsByRealObject)
    {
        pszQuery = LsaAdBatchEnumGetRealQuery(bIsSchemaMode, ObjectType);
    }
    else
    {
        pszQuery = LsaAdBatchEnumGetPseudoQuery(bIsSchemaMode, ObjectType);
    }

    return pszQuery;
}

static
DWORD
LsaAdBatchEnumProcessRealMessages(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessages,
    IN DWORD dwCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LDAPMessage* pCurrentMessage = NULL;
    LDAP* pLd = LsaLdapGetSession(hDirectory);
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;

    dwError = LsaAllocateMemory(
                    dwCount * sizeof(*ppObjects),
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    pCurrentMessage = ldap_first_entry(pLd, pMessages);
    while (pCurrentMessage)
    {
        LSA_AD_BATCH_ITEM batchItem = { { 0 }, 0 };

        dwError = LsaAdBatchGatherRealObject(
                        &batchItem,
                        ObjectType,
                        NULL,
                        hDirectory,
                        pCurrentMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAdBatchMarshal(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        &batchItem,
                        &ppObjects[dwObjectsCount]);
        BAIL_ON_LSA_ERROR(dwError);

        dwObjectsCount++;

        pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
    }

cleanup:
    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    return dwError;

error:
    // set OUT params out cleanup.
    LsaDbSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;

    goto cleanup;
}

static
DWORD
LsaAdBatchEnumProcessPseudoMessages(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessages,
    IN DWORD dwCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    LDAPMessage* pCurrentMessage = NULL;
    LDAP* pLd = LsaLdapGetSession(hDirectory);
    PSTR* ppszSids = NULL;
    DWORD dwSidsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;
    PSTR* ppszKeywordValues = NULL;
    DWORD dwKeywordValuesCount = 0;

    dwError = LsaAllocateMemory(
                    dwCount * sizeof(*ppszSids),
                    (PVOID*)&ppszSids);
    BAIL_ON_LSA_ERROR(dwError);

    pCurrentMessage = ldap_first_entry(pLd, pMessages);
    while (pCurrentMessage)
    {
        PCSTR pszSidFromKeywords = NULL;

        dwError = LsaLdapGetStrings(
                        hDirectory,
                        pCurrentMessage,
                        AD_LDAP_KEYWORDS_TAG,
                        &ppszKeywordValues,
                        &dwKeywordValuesCount);
        BAIL_ON_LSA_ERROR(dwError);

        pszSidFromKeywords = LsaAdBatchFindKeywordAttributeStatic(
                                    dwKeywordValuesCount,
                                    ppszKeywordValues,
                                    AD_LDAP_BACKLINK_PSEUDO_TAG);
        if (IsNullOrEmptyString(pszSidFromKeywords))
        {
            dwError = LSA_ERROR_INVALID_SID;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateString(pszSidFromKeywords,
                                    &ppszSids[dwSidsCount]);
        BAIL_ON_LSA_ERROR(dwError);

        dwSidsCount++;

        LsaFreeStringArray(ppszKeywordValues, dwKeywordValuesCount);
        ppszKeywordValues = NULL;
        dwKeywordValuesCount = 0;

        pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
    }

    dwError = LsaAdBatchFindObjects(
                    LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                    dwSidsCount,
                    ppszSids,
                    NULL,
                    &dwObjectsCount,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Ideally, do extra sanity check on object type.
    // In the future, find should take the object type
    // and restrict the search based on that.
    XXX;

cleanup:
    LsaFreeStringArray(ppszKeywordValues, dwKeywordValuesCount);
    LsaFreeStringArray(ppszSids, dwSidsCount);

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    return dwError;

error:
    // set OUT params out cleanup.
    LsaDbSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;

    goto cleanup;
}

static
DWORD
LsaAdBatchEnumProcessMessages(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsByRealObject,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessages,
    IN DWORD dwMaxCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    LDAP* pLd = LsaLdapGetSession(hDirectory);
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;

    dwCount = ldap_count_entries(pLd, pMessages);
    if ((int)dwCount < 0)
    {
       dwError = LSA_ERROR_LDAP_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount == 0)
    {
        dwError = LsaAdBatchEnumGetNoMoreError(ObjectType);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount > dwMaxCount)
    {
       dwError = LSA_ERROR_LDAP_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (bIsByRealObject)
    {
        dwError = LsaAdBatchEnumProcessRealMessages(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        ObjectType,
                        hDirectory,
                        pMessages,
                        dwCount,
                        &dwObjectsCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchEnumProcessPseudoMessages(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        ObjectType,
                        hDirectory,
                        pMessages,
                        dwCount,
                        &dwObjectsCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    return dwError;

error:
    // set OUT params out cleanup.
    LsaDbSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;

    goto cleanup;
}

// If this function returns with an error, the position stored in pCookie will
// be advanced, even though no results are returned. pCookie will still need
// to be freed outside of this function, even if this function returns with an
// error.
static
DWORD
LsaAdBatchEnumObjectsInCell(
    IN HANDLE hDirectory,
    IN OUT PLSA_SEARCH_COOKIE pCookie,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsByRealObject,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwRemainingObjectsWanted = dwMaxObjectsCount;
    PSTR szBacklinkAttributeList[] =
    {
        AD_LDAP_KEYWORDS_TAG,
        NULL
    };
    PSTR szRealAttributeList[] =
    {
        // AD attributes:
        // - common:
        AD_LDAP_OBJECTCLASS_TAG,
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_DN_TAG,
        // - user-specific:
        AD_LDAP_PRIMEGID_TAG,
        AD_LDAP_UPN_TAG,
        AD_LDAP_USER_CTRL_TAG,
        AD_LDAP_ACCOUT_EXP_TAG,
        AD_LDAP_PWD_LASTSET_TAG,
        // schema mode:
        // - (group alias) or (user gecos in unprovisioned mode):
        AD_LDAP_DISPLAY_NAME_TAG,
        // - unix properties (alias is just user alias):
        AD_LDAP_ALIAS_TAG,
        AD_LDAP_UID_TAG,
        AD_LDAP_GID_TAG,
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_GECOS_TAG,
        AD_LDAP_HOMEDIR_TAG,
        AD_LDAP_SHELL_TAG,
        NULL
    };
    PSTR* pszAttributeList = bIsByRealObject ? szRealAttributeList : szBacklinkAttributeList;
    PSTR pszScopeRoot = NULL;
    PCSTR pszQuery = NULL;
    PSTR pszNetbiosDomainName = NULL;
    LDAPMessage* pMessage = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppTotalObjects = NULL;
    DWORD dwTotalObjectsCount = 0;

    LSA_ASSERT(!pCookie->bSearchFinished);

    dwError = LsaAdBatchEnumGetScopeRoot(
                    ObjectType,
                    bIsByRealObject,
                    pszDnsDomainName,
                    pszCellDn,
                    &pszScopeRoot);
    BAIL_ON_LSA_ERROR(dwError);

    pszQuery = LsaAdBatchEnumGetQuery(bIsByRealObject, bIsSchemaMode, ObjectType);
    LSA_ASSERT(pszQuery);

    dwError = LsaDmWrapGetDomainName(pszDnsDomainName, NULL,
                                     &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    while (!pCookie->bSearchFinished && dwRemainingObjectsWanted)
    {
        dwError = LsaLdapDirectoryOnePagedSearch(
                        hDirectory,
                        pszScopeRoot,
                        pszQuery,
                        pszAttributeList,
                        dwRemainingObjectsWanted,
                        pCookie,
                        bIsByRealObject ? LDAP_SCOPE_SUBTREE : LDAP_SCOPE_ONELEVEL,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAdBatchEnumProcessMessages(
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        ObjectType,
                        bIsByRealObject,
                        hDirectory,
                        pMessage,
                        dwRemainingObjectsWanted,
                        &dwObjectsCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        ldap_msgfree(pMessage);
        pMessage = NULL;

        dwRemainingObjectsWanted -= dwObjectsCount;

        dwError = LsaAppendAndFreePtrs(
                        &dwTotalObjectsCount,
                        (PVOID**)&ppTotalObjects,
                        &dwObjectsCount,
                        (PVOID**)&ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszScopeRoot);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    LsaDbSafeFreeObjectList(dwObjectsCount, &ppObjects);

    *pdwObjectsCount = dwTotalObjectsCount;
    *pppObjects = ppTotalObjects;

    return dwError;

error:
    // set OUT params in cleanup.
    LsaDbSafeFreeObjectList(dwTotalObjectsCount, &ppTotalObjects);
    dwTotalObjectsCount = 0;

    goto cleanup;
}

// If this function returns with an error, the position stored in pCookie will
// be advanced, even though no results are returned. pCookie will still need
// to be freed outside of this function, even if this function returns with an
// error.
static
DWORD
LsaAdBatchEnumObjectsInLinkedCells(
    IN HANDLE hDirectory,
    IN OUT PLSA_SEARCH_COOKIE pCookie,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsByRealObject,
    IN BOOLEAN bIsSchemaMode,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjectsInOneCell = NULL;
    DWORD dwObjectsCountInOneCell = 0;
    PLSA_SECURITY_OBJECT* ppObjects = *pppObjects;
    DWORD dwObjectsCount = *pdwObjectsCount;
    PAD_CELL_COOKIE_DATA pCookieData = NULL;

    LSA_ASSERT(pCookie->pfnFree == LsaAdBatchFreeCellCookie);
    pCookieData = (PAD_CELL_COOKIE_DATA)pCookie->pvData;

    while (dwObjectsCount < dwMaxObjectsCount && pCookieData->pCurrentCell)
    {
        PAD_LINKED_CELL_INFO pCellInfo = (PAD_LINKED_CELL_INFO)
            pCookieData->pCurrentCell->pItem;

        dwError = LsaAdBatchEnumObjectsInCell(
                        hDirectory,
                        &pCookieData->LdapCookie,
                        ObjectType,
                        bIsByRealObject,
                        bIsSchemaMode,
                        pCellInfo->pszDomain,
                        pCellInfo->pszCellDN,
                        dwMaxObjectsCount - dwObjectsCount,
                        &dwObjectsCountInOneCell,
                        &ppObjectsInOneCell);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAppendAndFreePtrs(
                        &dwObjectsCount,
                        (PVOID**)&ppObjects,
                        &dwObjectsCountInOneCell,
                        (PVOID**)&ppObjectsInOneCell);
        BAIL_ON_LSA_ERROR(dwError);

        if (pCookieData->LdapCookie.bSearchFinished)
        {
            pCookieData->pCurrentCell = pCookieData->pCurrentCell->pNext;
            LsaFreeCookieContents(&pCookieData->LdapCookie);
        }
    }

    if (!pCookieData->pCurrentCell)
    {
        pCookie->bSearchFinished = TRUE;
    }

cleanup:
    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

    LsaDbSafeFreeObjectList(dwObjectsCountInOneCell, &ppObjectsInOneCell);

    return dwError;

error:
   // set OUT params in cleanup...
   LsaDbSafeFreeObjectList(dwObjectsCount, &ppObjects);
   dwObjectsCount = 0;

   goto cleanup;
}

// If this function returns with an error, the position stored in pCookie will
// be advanced, even though no results are returned. pCookie will still need
// to be freed outside of this function, even if this function returns with an
// error.
DWORD
LsaAdBatchEnumObjects(
    IN HANDLE hDirectory,
    IN OUT PLSA_SEARCH_COOKIE pCookie,
    IN ADAccountType AccountType,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsByRealObject = FALSE;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    BOOLEAN bIsSchemaMode = (gpADProviderData->adConfigurationMode == SchemaMode) ? TRUE : FALSE;
    PAD_CELL_COOKIE_DATA pCookieData = NULL;
    DWORD dwTotalObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppTotalObjects = NULL;
    DWORD dwInput = 0;
    DWORD dwOutput = 0;
    PSTR pszCopiedSid = NULL;

    if (LsaAdBatchIsDefaultSchemaMode() || LsaAdBatchIsUnprovisionedMode())
    {
        bIsByRealObject = TRUE;
    }

    dwError = LsaAdBatchAccountTypeToObjectType(AccountType, &objectType);
    BAIL_ON_LSA_ERROR(dwError);

    if (pCookie->bSearchFinished)
    {
        dwError = LsaAdBatchEnumGetNoMoreError(objectType);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pCookie->pfnFree == NULL)
    {
        dwError = LsaAllocateMemory(
                        sizeof(AD_CELL_COOKIE_DATA),
                        &pCookie->pvData);
        BAIL_ON_LSA_ERROR(dwError);

        pCookieData = (PAD_CELL_COOKIE_DATA)pCookie->pvData;
        if (gpADProviderData->pCellList != NULL)
        {
            // There are linked cells, so we need to keep track of which
            // sids have been enumerated.
            dwError = LsaHashCreate(
                            10 * 1024,
                            LsaHashCaselessStringCompare,
                            LsaHashCaselessString,
                            LsaHashFreeStringKey,
                            &pCookieData->pEnumeratedSids);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pCookie->pfnFree = LsaAdBatchFreeCellCookie;
    }
    else
    {
        LSA_ASSERT(pCookie->pfnFree == LsaAdBatchFreeCellCookie);
        pCookieData = (PAD_CELL_COOKIE_DATA)pCookie->pvData;
    }

    while (dwTotalObjectsCount < dwMaxObjectsCount &&
            !pCookie->bSearchFinished)
    {
        if (pCookieData->pCurrentCell == NULL)
        {
            // First get the objects from the primary cell
            dwError = LsaAdBatchEnumObjectsInCell(
                            hDirectory,
                            &pCookieData->LdapCookie,
                            objectType,
                            bIsByRealObject,
                            bIsSchemaMode,
                            gpADProviderData->szDomain,
                            gpADProviderData->cell.szCellDN,
                            dwMaxObjectsCount - dwTotalObjectsCount,
                            &dwObjectsCount,
                            &ppObjects);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAppendAndFreePtrs(
                            &dwTotalObjectsCount,
                            (PVOID**)&ppTotalObjects,
                            &dwObjectsCount,
                            (PVOID**)&ppObjects);
            BAIL_ON_LSA_ERROR(dwError);

            if (pCookieData->LdapCookie.bSearchFinished)
            {
                LsaFreeCookieContents(&pCookieData->LdapCookie);
                pCookieData->pCurrentCell = gpADProviderData->pCellList;
                if (pCookieData->pCurrentCell == NULL)
                {
                    pCookie->bSearchFinished = TRUE;
                }
            }
        }
        else
        {
            dwError = LsaAdBatchEnumObjectsInLinkedCells(
                         hDirectory,
                         pCookie,
                         objectType,
                         bIsByRealObject,
                         bIsSchemaMode,
                         dwMaxObjectsCount - dwTotalObjectsCount,
                         &dwObjectsCount,
                         &ppObjects);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAppendAndFreePtrs(
                            &dwTotalObjectsCount,
                            (PVOID**)&ppTotalObjects,
                            &dwObjectsCount,
                            (PVOID**)&ppObjects);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Remove any sids that have already been enumerated
        if (pCookieData->pEnumeratedSids != NULL)
        {
            for (; dwInput < dwTotalObjectsCount; dwInput++)
            {
                dwError = LsaHashGetValue(
                            pCookieData->pEnumeratedSids,
                            ppTotalObjects[dwInput]->pszObjectSid,
                            NULL);
                if (dwError == LSA_ERROR_SUCCESS)
                {
                    // The object is already in the hash
                    LsaDbSafeFreeObject(&ppTotalObjects[dwInput]);
                }
                else if (dwError == ENOENT)
                {
                    // This is a new entry; let's track it in the hash

                    if (pCookieData->pEnumeratedSids->sCount * 2 >
                        pCookieData->pEnumeratedSids->sTableSize)
                    {
                        // Enlarge the hash table to avoid collisions
                        dwError = LsaHashResize(
                                    pCookieData->pEnumeratedSids,
                                    pCookieData->pEnumeratedSids->sCount * 4);
                        BAIL_ON_LSA_ERROR(dwError);
                    }

                    dwError = LsaAllocateString(
                                    ppTotalObjects[dwInput]->pszObjectSid,
                                    &pszCopiedSid);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LsaHashSetValue(
                                pCookieData->pEnumeratedSids,
                                pszCopiedSid,
                                NULL);
                    BAIL_ON_LSA_ERROR(dwError);

                    // This is now owned by the hash table
                    pszCopiedSid = NULL;

                    ppTotalObjects[dwOutput++] = ppTotalObjects[dwInput];
                }
                else
                {
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            // The array is now smaller since duplicates have been removed
            dwTotalObjectsCount = dwOutput;
            dwInput = dwOutput;
        }
    }

cleanup:
    *pdwObjectsCount = dwTotalObjectsCount;
    *pppObjects = ppTotalObjects;
    LSA_SAFE_FREE_STRING(pszCopiedSid);

    return dwError;

error:
    // set OUT params in cleanup...
    LsaDbSafeFreeObjectList(dwObjectsCount, &ppObjects);
    dwObjectsCount = 0;
    LsaDbSafeFreeObjectList(dwTotalObjectsCount, &ppTotalObjects);
    dwTotalObjectsCount = 0;
    goto cleanup;
}
