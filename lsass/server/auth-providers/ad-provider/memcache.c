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
 *        adcache.c
 *
 * Abstract:
 *
 *        This is the public interface for the AD Provider Local Cache
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "adprovider.h"


DWORD
MemCacheSetup(
    IN sqlite3* pSqlHandle
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->Setup(
                    pSqlHandle
                    );

    return dwError;
}

DWORD
MemCacheOpen(
    IN PCSTR pszDbPath,
    OUT PLSA_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->Open(
                        pszDbPath,
                        phDb
                        );
    return dwError;
}


DWORD
MemCacheFreePreparedStatements(
    IN OUT PLSA_DB_CONNECTION pConn
    )
{
}

void
MemCacheSafeClose(
    PLSA_DB_HANDLE phDb
    )
{
    pCacheProvider->SafeClose(
                        phDb
                        );
    return;
}

DWORD
MemCacheFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindUserByName(
                        hDb,
                        pUserNameInfo,
                        ppObject
                        );
    return dwError;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
}

DWORD
MemCacheFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
}

DWORD
MemCacheFindGroupById(
    LSA_DB_HANDLE hDb,
    gid_t gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindGroupById(
                    hDb,
                    gid,
                    ppObject
                    );
    return dwError;
}

DWORD
MemCacheRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->RemoveUserBySid(
                    hDb,
                    pszSid
                    );
    return dwError;
}

DWORD
MemCacheRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->RemoveGroupBySid(
                     hDb,
                     pszSid
                    );
    return dwError;
}

DWORD
MemCacheEmptyCache(
    IN LSA_DB_HANDLE hDb
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->EmptyCache(
                    hDb
                    );
    return dwError;
}


DWORD
MemCacheUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT_VERSION_INFO pResult)
{

    DWORD dwError = 0;

    dwError = pCacheProvider->UnpackCacheInfo(
                    pstQuery,
                    piColumnPos,
                    pResult
                    );

    return dwError;
}


DWORD
MemCacheUnpackObjectInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult)
{
}


DWORD
MemCacheUnpackUserInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult)
{
    return dwError;
}


DWORD
MemCacheUnpackGroupInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult)
{
}


DWORD
MemCacheUnpackGroupMembershipInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PLSA_GROUP_MEMBERSHIP pResult
    )
{
}

DWORD
MemCacheStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->StoreObjectEntry(
                        hDb,
                        pObject
                        );
    return dwError;

}

DWORD
MemCacheStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->StoreObjectEntries(
                        hDb,
                        sObjectCount,
                        ppObjects
                        );
    return dwError;
}

void
MemCacheSafeFreeObject(
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->SafeFreeObject(
                    ppObject
                    );
    return dwError;
}


DWORD
MemCacheCreateCacheTag(
    IN PLSA_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    OUT int64_t *pqwCacheId
    )
{
}


DWORD
MemCacheUpdateMembership(
    IN sqlite3_stmt* pstQuery,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid
    )
{
}


DWORD
MemCacheAddMembership(
    IN PLSA_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid,
    IN BOOLEAN bIsInPac,
    IN BOOLEAN bIsInPacOnly,
    IN BOOLEAN bIsInLdap,
    IN BOOLEAN bIsDomainPrimaryGroup
    )
{
}


DWORD
MemCacheStoreGroupMembershipCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    )
{
}

DWORD
MemCacheStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->StoreGroupMembership(
                        hDb,
                        pszParentSid,
                        sMemberCount,
                        ppMembers
                        );
    return dwError;
}


DWORD
MemCacheStoreUserMembershipCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    )
{
}

DWORD
MemCacheStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->StoreGroupsForUser(
                    hDb,
                    pszChildSid,
                    sMemberCount,
                    ppMembers,
                    bIsPacAuthoritative
                    );
    return dwError;
}


DWORD
MemCacheGetMemberships(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bIsGroupMembers,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->GetMemberships(
                    hDb,
                    pszSid,
                    bIsGroupMembers,
                    bFilterNotInPacNorLdap,
                    psCount,
                    pppResults
                    );

    return dwError;
}

DWORD
MemCacheGetGroupMembers(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->GetGroupMembers(
                        hDb,
                        pszSid,
                        bFilterNotInPacNorLdap,
                        psCount,
                        pppResults
                        );
    return dwError;
}

DWORD
MemCacheGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
}

void
MemCacheSafeFreeGroupMembership(
        PLSA_GROUP_MEMBERSHIP* ppMembership)
{
}

void
MemCacheSafeFreeGroupMembershipList(
        size_t sCount,
        PLSA_GROUP_MEMBERSHIP** pppMembershipList)
{
}

void
MemCacheSafeFreeObjectList(
        size_t sCount,
        PLSA_SECURITY_OBJECT** pppObjectList)
{
}

DWORD
MemCacheQueryObjectMulti(
    IN sqlite3_stmt* pstQuery,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
}

DWORD
MemCacheEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->EnumUsersCache(
                        hDb,
                        dwMaxNumUsers,
                        pszResume,
                        dwNumUsersFound,
                        pppObjects
                        );

    return dwError;
}

DWORD
MemCacheEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->EnumGroupsCache(
                        hDb,
                        dwMaxNumGroups,
                        pszResume,
                        dwNumGroupsFound,
                        pppObjects
                        );

    return dwError;
}


DWORD
MemCacheQueryObject(
    IN sqlite3_stmt* pstQuery,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
}


PCSTR
MemCacheGetObjectFieldList(
    VOID
    )
{
}

DWORD
MemCacheFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindObjectByDN(
                        hDb,
                        pszDN,
                        ppObject
                        );
    return dwError;
}

DWORD
MemCacheFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindObjectsByDNList(
                        hDb,
                        sCount,
                        ppszDnList,
                        pppResults
                        );
    return dwError;
}

DWORD
MemCacheFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindObjectBySid(
                            hDb,
                            pszSid,
                            ppObject
                            );

    return dwError;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// version.
DWORD
MemCacheFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{

}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    )
{
}

void
MemCacheFreePasswordVerifier(
    IN OUT PLSA_PASSWORD_VERIFIER pVerifier
    )
{
}

DWORD
MemCacheStorePasswordVerifier(
    LSA_DB_HANDLE hDb,
    PLSA_PASSWORD_VERIFIER pVerifier
    )
{
}
