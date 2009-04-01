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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "localprovider.h"

DWORD
LsaInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable)
{
    DWORD dwError = 0;
    LOCAL_CONFIG config = {0};

    pthread_rwlock_init(&gProviderLocalGlobalDataLock, NULL);

    dwError = LsaDnsGetHostInfo(&gProviderLocal_Hostname);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {

        dwError = LsaLPParseConfigFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLPTransferConfigContents(
                        &config,
                        &gLocalConfig);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLPSetConfigFilePath(pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);

    }

    LsaLPDbInitGlobals();

    dwError = LsaLPDbCreate();
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled())
    {
        LsaLocalProviderLogServiceStartEvent(dwError);
    }

    *ppszProviderName = (PSTR)gpszLocalProviderName;
    *ppFunctionTable = &gLocalProviderAPITable;

cleanup:

    return dwError;

error:

    if (LsaLPEventlogEnabled())
    {
        LsaLocalProviderLogServiceStartEvent(dwError);
    }

    LsaLPFreeConfigContents(&config);

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

DWORD
LsaLPOpenHandle(
    uid_t uid,
    gid_t gid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(LOCAL_PROVIDER_CONTEXT),
                    (PVOID*)&pContext
                    );
    BAIL_ON_LSA_ERROR(dwError);

    pContext->uid = uid;
    pContext->gid = gid;

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *(phProvider) = (HANDLE)NULL;

    if (pContext) {
        LsaLPCloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
LsaLPCloseHandle(
    HANDLE hProvider
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    if (pContext) {
        LsaFreeMemory(pContext);
    }
}

BOOLEAN
LsaLPServicesDomain(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    if (IsNullOrEmptyString(pszDomain) ||
        !strcasecmp(pszDomain, "localhost") ||
        !strcasecmp(pszDomain, gProviderLocal_Hostname))
    {
        bResult = TRUE;
    }

    return bResult;
}

static DWORD
CheckAccountFlags(
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;

    BAIL_ON_INVALID_POINTER(pUserInfo2);

    if (pUserInfo2->bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LSA_ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaLPAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PBYTE pHash = NULL;
    DWORD dwHashLen = 0;
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    BOOLEAN bAuthenticated = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPFindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    /* Check for disable, expired, etc..  accounts */

    dwError = CheckAccountFlags(pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvComputeNTHash(pszPassword, &pHash, &dwHashLen);
    BAIL_ON_LSA_ERROR(dwError);

    // Either both hashes are null, or they must match
    bAuthenticated =
        (((pUserInfo->dwNTHashLen) &&
         (pUserInfo->dwNTHashLen == dwHashLen) &&
         (pHash) &&
         (pUserInfo->pNTHash) &&
         !memcmp(pUserInfo->pNTHash, pHash, dwHashLen)) ||
         (!pUserInfo->dwNTHashLen && !dwHashLen));

    if (!bAuthenticated) {
        dwError = LSA_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    LSA_SAFE_FREE_MEMORY(pHash);

    return dwError;

error:

    goto cleanup;
}


DWORD
LsaLPAuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;
    PLSA_USER_INFO_2 pUserInfo2 = NULL;
    PSTR pszAccountName = NULL;
    PCSTR pszDomain = NULL;
    DWORD dwLen = 0;
    BYTE NTResponse[24] = { 0 };
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PBYTE pChal = NULL;
    PBYTE pNTresp = NULL;
    DWORD dwUserInfoLevel = 2;

    BAIL_ON_INVALID_POINTER(pUserParams->pszAccountName);

    /* Assume the local domain (localhost) if we don't have one */

    if (pUserParams->pszDomain)
        pszDomain = pUserParams->pszDomain;
    else
        pszDomain = "LOCALHOST";

    /* Allow the next provider to continue if we don't handle this domain */

    if (!LsaLPServicesDomain(pszDomain)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* calculate length includeing '\' and terminating NULL */

    dwLen = strlen(pszDomain) + strlen(pUserParams->pszAccountName) + 2;
    dwError = LsaAllocateMemory(dwLen, (PVOID*)&pszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    snprintf(pszAccountName, dwLen,
             "%s\\%s",
             pszDomain,
             pUserParams->pszAccountName);

    /* Find the user */

    dwError = LsaLPFindUserByName(hProvider,
                                              pszAccountName,
                                              dwUserInfoLevel,
                                              (PVOID*)&pUserInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    /* Check Account Status */

    dwError = CheckAccountFlags(pUserInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    /* generate the responses and compare */

    pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);
    BAIL_ON_INVALID_POINTER(pChal);

    ntError = NTLMv1EncryptChallenge(pChal,
                                     NULL,     /* ignore LM hash */
                                     pUserInfo2->info1.pNTHash,
                                     NULL,
                                     NTResponse);
    if (ntError != STATUS_SUCCESS) {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pNTresp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);
    BAIL_ON_INVALID_POINTER(pNTresp);

    if (memcmp(pNTresp, NTResponse, 24) != 0)
    {
        dwError = LSA_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo2) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo2);
    }

    LSA_SAFE_FREE_MEMORY(pszAccountName);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPFindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    /* Map any failures to find the user to 'user not found'. This way if the
     * unknown_ok option is specified for the pam module, this error will be
     * ignored instead of blocking all logins.
     */
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LSA_LOG_DEBUG(
                "Failed to find user '%s' while validating login "
                "[error code:%d]",
                pszLoginId,
                dwError);
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPCheckUserInList(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
LsaLPFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindUserByName(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    pLoginInfo->pszName,
                    dwUserInfoLevel,
                    &pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LsaLPFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pUserInfo = NULL;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    if (uid == 0)
    {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbFindUserById(
                    hDb,
                    uid,
                    dwUserInfoLevel,
                    &pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LsaLPGetGroupsForUser(
    IN HANDLE hProvider,
    IN uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwGroupsFound,
    IN PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;

    if (uid == 0)
    {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbGetGroupsForUser(
                    hDb,
                    uid,
                    dwGroupInfoLevel,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaLPBeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LsaLPCreateUserState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaLPEnumUsers(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNumRecords,
    PDWORD   pdwUsersFound,
    PVOID**  pppUserInfoList
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbEnumUsers(
                    hDb,
                    pEnumState->dwInfoLevel,
                    pEnumState->dwNextStartingId,
                    dwMaxNumRecords,
                    pdwUsersFound,
                    pppUserInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (*pdwUsersFound) {
        pEnumState->dwNextStartingId += *pdwUsersFound;
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    *pdwUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
}

VOID
LsaLPEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    LsaLPFreeUserState(
            hProvider,
            (PLOCAL_PROVIDER_ENUM_STATE)hResume);
}

DWORD
LsaLPFindGroupByName(
    IN HANDLE hProvider,
    IN PCSTR pszGroupName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszGroupName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
    	dwError = LSA_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindGroupByName(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    pLoginInfo->pszName,
                    dwGroupInfoLevel,
                    &pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LsaLPFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;

    if (gid == 0)
    {
    	dwError = LSA_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindGroupById(
                    hDb,
                    gid,
                    dwGroupInfoLevel,
                    &pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LsaLPBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LsaLPCreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaLPEnumGroups(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxGroups,
    PDWORD   pdwGroupsFound,
    PVOID**  pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;
    HANDLE hDb = (HANDLE)NULL;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbEnumGroups(
                    hDb,
                    pEnumState->dwInfoLevel,
                    pEnumState->dwNextStartingId,
                    dwMaxGroups,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (*pdwGroupsFound) {
        pEnumState->dwNextStartingId += *pdwGroupsFound;
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

VOID
LsaLPEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    LsaLPFreeGroupState(
            hProvider,
            (PLOCAL_PROVIDER_ENUM_STATE)hResume);
}

DWORD
LsaLPChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 2;
    HANDLE hDb = (HANDLE)NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindUserByName(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    pLoginInfo->pszName,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if ((pContext->uid != 0) &&
        (pContext->uid != ((PLSA_USER_INFO_2)pUserInfo)->uid)) {
       // Only the user or root can change the password
       dwError = EACCES;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // If the account initiating the password change has super user privileges,
    // don't bother checking the old password
    if (pContext->uid) {

        if (!pUserInfo->bUserCanChangePassword) {
            dwError = LSA_ERROR_USER_CANNOT_CHANGE_PASSWD;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaLPAuthenticateUser(
                        hProvider,
                        pszLoginId,
                        pszOldPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaLPDbChangePassword(
                    hDb,
                    ((PLSA_USER_INFO_2)pUserInfo)->uid,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwUserInfoLevel)
    {
        case 0:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_USER_INFO_0)(pUserInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case 1:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_USER_INFO_1)(pUserInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case 2:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_USER_INFO_2)(pUserInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbAddUser(
                    hDb,
                    dwUserInfoLevel,
                    pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled()){
        LsaLocalProviderLogUserAddEvent(pLoginInfo->pszName, ((PLSA_USER_INFO_0)pUserInfo)->uid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
       dwError = EACCES;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbModifyUser(
                        hDb,
                        pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
       LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbDeleteUser(
                    hDb,
                    uid);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled())
    {
        LsaLocalProviderLogUserDeleteEvent(uid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPAddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    switch(dwGroupInfoLevel)
    {
        case 1:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_GROUP_INFO_1)(pGroupInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbAddGroup(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    dwGroupInfoLevel,
                    pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled()){
        LsaLocalProviderLogGroupAddEvent(pLoginInfo->pszName, ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbDeleteGroup(
                    hDb,
                    gid);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled()){
        LsaLocalProviderLogGroupDeleteEvent(gid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPOpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPFindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    // Allow directory creation only if this is
    //
    if ((pContext->uid != 0) &&
        (pContext->uid != ((PLSA_USER_INFO_0)(pUserInfo))->uid)) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPCreateHomeDirectory(
                    (PLSA_USER_INFO_0)pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPCreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = 022;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    if (IsNullOrEmptyString(pUserInfo->pszHomedir)) {
       LSA_LOG_ERROR("The user's [Uid:%ld] home directory is not defined", (long)pUserInfo->uid);
       dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pUserInfo->pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
       dwError = LsaCreateDirectory(
                    pUserInfo->pszHomedir,
                    perms & (~umask));
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = TRUE;

       dwError = LsaChangeOwner(
                    pUserInfo->pszHomedir,
                    pUserInfo->uid,
                    pUserInfo->gid);
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = FALSE;

       dwError = LsaLPProvisionHomeDir(
                       pUserInfo->uid,
                       pUserInfo->gid,
                       pUserInfo->pszHomedir);
       BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pUserInfo->pszHomedir);
    }

    goto cleanup;
}

DWORD
LsaLPProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD   dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckDirectoryExists(
                    "/etc/skel",
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists) {
        dwError = LsaCopyDirectory(
                    "/etc/skel",
                    ownerUid,
                    ownerGid,
                    pszHomedirPath);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
LsaLPCloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 0;
    PVOID pUserInfo = NULL;

    dwError = LsaLPFindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaShutdownProvider(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    // TODO: Should we grab the global lock?
    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    LSA_SAFE_FREE_STRING(gProviderLocal_Hostname);

    return 0;
}

DWORD
LsaLPGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes)
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
LsaLPFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    *ppNSSArtefactInfo = NULL;

    return LSA_ERROR_NOT_HANDLED;
}


DWORD
LsaLPBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LSA_ERROR_NOT_HANDLED;

    *phResume = (HANDLE)NULL;

    return dwError;
}

DWORD
LsaLPEnumNSSArtefacts(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNSSArtefacts,
    PDWORD   pdwNSSArtefactsFound,
    PVOID**  pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;

    dwError = LSA_ERROR_NOT_HANDLED;

    *pdwNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    return dwError;
}

VOID
LsaLPEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}

DWORD
LsaLPGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;

    dwError = LsaAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    gpszLocalProviderName,
                    &pProviderStatus->pszId);
    BAIL_ON_LSA_ERROR(dwError);

    pProviderStatus->mode = LSA_PROVIDER_MODE_LOCAL_SYSTEM;
    pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;

    *ppProviderStatus = pProviderStatus;

cleanup:

    return dwError;

error:

    *ppProviderStatus = NULL;

    if (pProviderStatus)
    {
        LsaLPFreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
LsaLPRefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;
    LOCAL_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;

    dwError = LsaLPGetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {
        dwError = LsaLPParseConfigFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        ENTER_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

        dwError = LsaLPTransferConfigContents(
                        &config,
                        &gLocalConfig);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaLocalProviderLogConfigReloadEvent();

cleanup:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);

    LEAVE_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LsaLPFreeConfigContents(&config);

    goto cleanup;
}

VOID
LsaLPFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    )
{
    LSA_SAFE_FREE_STRING(pProviderStatus->pszId);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszCell);

    LsaFreeMemory(pProviderStatus);
}

DWORD
LsaLPProviderIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

VOID
LsaLocalProviderLogServiceStartEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    int nUserCount = 0;
    int nGroupCount = 0;
    PSTR pszData = NULL;
    PSTR pszDescription = NULL;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbGetUserCount(
                    hDb,
                    &nUserCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbGetGroupCount(
                    hDb,
                    &nGroupCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider initialization %s.\r\n\r\n" \
                 "     Authentication provider:        %s\r\n\r\n" \
                 "     Current number of local users:  %d\r\n" \
                 "     Current number of local groups: %d",
                 dwErrCode ? "failed" : "succeeded",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 nUserCount,
                 nGroupCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwErrCode)
    {
        dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
        BAIL_ON_LSA_ERROR(dwError);

        LsaSrvLogServiceFailureEvent(
                 LSASS_EVENT_FAILED_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 pszData);
    }
    else
    {
        LsaSrvLogServiceSuccessEvent(
                 LSASS_EVENT_SUCCESSFUL_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 NULL);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}

VOID
LsaLocalProviderLogConfigReloadEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider configuration settings have been reloaded.\r\n\r\n" \
                 "     Authentication provider:       %s\r\n\r\n" \
                 "     Current settings are...\r\n" \
                 "     Password change interval:      %d\r\n" \
                 "     Password change warning time : %d\r\n" \
                 "     Enable event log:              %s",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 gLocalConfig.dwPasswdChangeInterval,
                 gLocalConfig.dwPasswdChangeWarningTime,
                 gLocalConfig.bEnableEventLog ? "true" : "false");
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             LSASS_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

VOID
LsaLocalProviderLogUserAddEvent(
    PCSTR pszUsername,
    uid_t uid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "User account created.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     User name:               %s\r\n" \
                 "     UID:                     %d",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 LSA_SAFE_LOG_STRING(pszUsername),
                 uid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_ADD_USER_ACCOUNT,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:
    goto cleanup;
}

VOID
LsaLocalProviderLogUserDeleteEvent(
    uid_t uid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "User account deleted.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     UID: %d",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 uid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_DELETE_USER_ACCOUNT,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:
    goto cleanup;
}

VOID
LsaLocalProviderLogGroupAddEvent(
    PCSTR pszGroupname,
    gid_t gid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Group created.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     Group name:  %s\r\n" \
                 "     GID: %d",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 LSA_SAFE_LOG_STRING(pszGroupname),
                 gid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_ADD_GROUP,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:
    goto cleanup;
}

VOID
LsaLocalProviderLogGroupDeleteEvent(
    gid_t gid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Group deleted.\r\n\r\n" \
                 "     Authentication provider: %s\r\n\r\n" \
                 "     GID: %d",
                 LSA_SAFE_LOG_STRING(gpszLocalProviderName),
                 gid);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
            LSASS_EVENT_DELETE_GROUP,
            ACCOUNT_MANAGEMENT_EVENT_CATEGORY,
            pszDescription,
            NULL);
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:
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
