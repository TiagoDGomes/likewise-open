/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        createfile.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
DWORD
SMBCreateFileA(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    PCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    )
{
    DWORD dwError = 0;
    PWSTR pwszFileName = NULL;

    BAIL_ON_INVALID_POINTER(phFile);

    dwError = SMBMbsToWc16s(
                    pszFileName,
                    &pwszFileName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateFileW(
        hConnection,
        hAccessToken,
        pwszFileName,
        dwDesiredAccess,
        dwSharedMode,
        pSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile,
        phFile);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszFileName);

    return dwError;

error:

    if (phFile)
    {
        *phFile = (HANDLE)NULL;
    }

    goto cleanup;
}

SMB_API
DWORD
SMBCreateFileW(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    PCWSTR              pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    )
{
    DWORD  dwError = 0;
    SMB_CREATE_FILE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PIO_CONTEXT pConnection = (PIO_CONTEXT) hConnection;

    BAIL_ON_INVALID_WSTRING(pwszFileName);
    BAIL_ON_INVALID_POINTER(phFile);

    dwError = SMBAPIHandleGetSecurityToken(hAccessToken, &request.pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    request.pwszFileName = (PWSTR)pwszFileName;
    request.dwDesiredAccess = dwDesiredAccess;
    request.dwSharedMode = dwSharedMode;
    request.dwCreationDisposition = dwCreationDisposition;
    request.dwFlagsAndAttributes = dwFlagsAndAttributes;
    request.pSecurityAttributes = pSecurityAttributes;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_CREATE_FILE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_CREATE_FILE_SUCCESS:
            break;

        case SMB_CREATE_FILE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);
    
    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    
    pConnection = pConnection;
    pAPIHandle->variant.hIPCHandle = (HANDLE) pResponse;
    
    *phFile = (HANDLE) pAPIHandle;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    if (phFile)
    {
        *phFile = (HANDLE)NULL;
    }

    goto cleanup;
}
