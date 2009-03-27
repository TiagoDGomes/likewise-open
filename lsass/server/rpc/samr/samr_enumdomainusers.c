/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_enumdomainusers.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrEnumDomainUsers function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvEnumDomainUsers(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 account_flags,
    /* [in] */ uint32 max_size,
    /* [out] */ RidNameArray **names,
    /* [out] */ uint32 *num_entries
    )
{
    CHAR szFilter[] = "(objectclass=user)";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    RidNameArray *pNames = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    PWSTR wszAttributes[3];
    PWSTR pwszAttrNameUsername = NULL;
    PWSTR pwszAttrNameUid = NULL;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    DWORD dwEntriesNum = 0;
    DWORD dwSize = 0;
    DWORD i = 0;
    DWORD dwCount = 0;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pConnCtx = pDomCtx->pConnCtx;
    pwszBase = pDomCtx->pwszDn;

    dwError = LsaMbsToWc16s(szFilter, &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_USER_NAME,
                            &pwszAttrNameUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(DIRECTORY_ATTR_TAG_UID,
                            &pwszAttrNameUid);
    BAIL_ON_LSA_ERROR(dwError);

    wszAttributes[0] = pwszAttrNameUsername;
    wszAttributes[1] = pwszAttrNameUid;
    wszAttributes[2] = NULL;

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              TRUE,
                              &pEntries,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateMemory((void**)&pNames,
                                   sizeof(RidNameArray),
                                   pDomCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwSize += sizeof(pNames->count);
    i       = *resume;
    dwCount = 0;

    if (i >= dwEntriesNum) {
        i = 0;
        status = STATUS_NO_MORE_ENTRIES;
    }

    while (dwSize < max_size && i < dwEntriesNum) {
        pEntry   = &(pEntries[i]);
        pAttr    = NULL;
        pAttrVal = NULL;

        if (pEntry->ulNumAttributes == 0) {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                   pwszAttrNameUsername,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr,
                                             &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal->Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING) {
            dwSize += sizeof(uint32);
            dwSize += wc16slen(pAttrVal->pwszStringValue) * sizeof(WCHAR);
            dwSize += 2 * sizeof(uint16);

            if (dwSize < max_size && pNames->entries) {
                status = SamrSrvReallocMemory((void**)&pNames->entries,
                                              sizeof(pNames->entries[0])
                                              * (++dwCount),
                                              pNames->entries);

            } else if (dwSize < max_size && !pNames->entries) {
                status = SamrSrvAllocateMemory((void**)&pNames->entries,
                                               sizeof(pNames->entries[0])
                                               * (++dwCount),
                                               pNames);

            } else {
                status = STATUS_MORE_ENTRIES;
                break;
            }

            BAIL_ON_NTSTATUS_ERROR(status);

            pNames->count = dwCount;

            status = InitUnicodeString(&(pNames->entries[dwCount - 1].name),
                                       pAttrVal->pwszStringValue);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = SamrSrvAddDepMemory(pNames->entries[dwCount - 1].name.string,
                                         pNames->entries);
            BAIL_ON_NTSTATUS_ERROR(status);

        } else {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        pAttr    = NULL;
        pAttrVal = NULL;

        dwError = DirectoryGetEntryAttributeByName(pEntry,
                                                   pwszAttrNameUid,
                                                   &pAttr);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectoryGetAttributeValue(pAttr,
                                             &pAttrVal);
        BAIL_ON_LSA_ERROR(dwError);

        if (pAttrVal->Type == DIRECTORY_ATTR_TYPE_LARGE_INTEGER) {
            pNames->entries[dwCount - 1].rid = pAttrVal->ulValue;

        } else {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        i++;
    }

    *resume      = i;
    *num_entries = dwCount;
    *names       = pNames;

cleanup:
    if (pwszFilter) {
        LSA_SAFE_FREE_MEMORY(pwszFilter);
    }

    if (pwszAttrNameUsername) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameUsername);
    }

    if (pwszAttrNameUid) {
        LSA_SAFE_FREE_MEMORY(pwszAttrNameUid);
    }

    if (pEntries) {
        DirectoryFreeEntries(pEntries, dwEntriesNum);
    }

    return status;

error:
    if (pNames) {
        SamrSrvFreeMemory(pNames);
    }

    *resume      = 0;
    *num_entries = 0;
    *names       = NULL;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
