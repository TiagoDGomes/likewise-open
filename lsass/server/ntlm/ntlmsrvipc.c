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
 *        ntlmsrvipc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication API (NTLM Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmSrvApiInit(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;

    dwError = NtlmInitializeContextDatabase();
    BAIL_ON_LW_ERROR(dwError);

    LsaInitializeCredentialsDatabase();

error:
    return dwError;
}

DWORD
NtlmSrvApiShutdown(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    NtlmShutdownContextDatabase();

    return dwError;
}

LWMsgDispatchSpec*
NtlmSrvGetDispatchSpec(
    VOID
    )
{
    return gNtlmMessageHandlers;
}

DWORD
NtlmSrvIpcCreateError(
    DWORD dwErrorCode,
    PNTLM_IPC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pError), (PVOID*)(PVOID)&pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = dwErrorCode;

    *ppError = pError;

error:
    return dwError;
}

LWMsgStatus
NtlmSrvIpcAcceptSecurityContext(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ACCEPT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerAcceptSecurityContext(
        &pReq->hCredential,
        &pReq->hContext,
        pReq->pInput,
        pReq->fContextReq,
        pReq->TargetDataRep,
        &pReq->hNewContext,
        pReq->pOutput,
        &(pNtlmResp->fContextAttr),
        &(pNtlmResp->tsTimeStamp)
        );

    if(!dwError)
    {
        pNtlmResp->hContext = pReq->hContext;
        pReq->hContext = NULL;

        pNtlmResp->hNewContext = pReq->hNewContext;
        pReq->hNewContext = NULL;

        memcpy(&(pNtlmResp->Output), pReq->pOutput, sizeof(SecBufferDesc));
        pReq->pOutput = NULL;

        pResponse->tag = NTLM_R_ACCEPT_SEC_CTXT_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACCEPT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcAcquireCredentialsHandle(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ACQUIRE_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ACQUIRE_CREDS_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_ACQUIRE_CREDS_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerAcquireCredentialsHandle(
        pAssoc,
        pReq->pszPrincipal,
        pReq->pszPackage,
        pReq->fCredentialUse,
        pReq->pvLogonID,
        pReq->pAuthData,
        &pNtlmResp->hCredential,
        &pNtlmResp->tsExpiry
        );

    if(!dwError)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_register_handle(
                                      pAssoc,
                                      "LSA_CRED_HANDLE",
                                      pNtlmResp->hCredential,
                                      NtlmSrvFreeCredHandle));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_retain_handle(pAssoc, pNtlmResp->hCredential));
        BAIL_ON_LSA_ERROR(dwError);


        pResponse->tag = NTLM_R_ACQUIRE_CREDS_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcDecryptMessage(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_DECRYPT_MSG_REQ pReq = pRequest->object;
    PNTLM_IPC_DECRYPT_MSG_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_DECRYPT_MSG_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerDecryptMessage(
        &pReq->hContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &pNtlmResp->bEncrypted
        );

    if(!dwError)
    {
        memcpy(&(pNtlmResp->Message), pReq->pMessage, sizeof(SecBufferDesc));
        pReq->pMessage = NULL;

        pResponse->tag = NTLM_R_DECRYPT_MSG_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_DECRYPT_MSG_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcDeleteSecurityContext(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_DELETE_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;

    //dwError = NtlmServerDeleteSecurityContext(
    //    &pReq->hContext
    //    );

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_unregister_handle(pAssoc, pReq->hContext));
    BAIL_ON_NTLM_ERROR(dwError);

    if(!dwError)
    {
        pResponse->tag = NTLM_R_DELETE_SEC_CTXT_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_DELETE_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcEncryptMessage(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ENCRYPT_MSG_REQ pReq = pRequest->object;
    PNTLM_IPC_ENCRYPT_MSG_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_ENCRYPT_MSG_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerEncryptMessage(
        &pReq->hContext,
        pReq->bEncrypt,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    if(!dwError)
    {
        memcpy(&(pNtlmResp->Message), pReq->pMessage, sizeof(SecBufferDesc));
        pReq->pMessage = NULL;

        pResponse->tag = NTLM_R_ENCRYPT_MSG_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ENCRYPT_MSG_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcExportSecurityContext(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_EXPORT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerExportSecurityContext(
        &pReq->hContext,
        pReq->fFlags,
        &(pNtlmResp->PackedContext),
        &(pNtlmResp->hToken)
        );

    if(!dwError)
    {
        pResponse->tag = NTLM_R_EXPORT_SEC_CTXT_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_EXPORT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcFreeCredentialsHandle(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_FREE_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;

    // TODO: Remove this during code cleanup... leave it here as a reminder of
    // why we're not calling this function... when we unregister_handle, the
    // free function we set earlier will be called for us.  Just deref here,
    // and the lwmsg system takes care of it for us.
    //dwError = NtlmServerFreeCredentialsHandle(
    //    &pReq->hCredential
    //    );

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_unregister_handle(pAssoc, pReq->hCredential));
    BAIL_ON_NTLM_ERROR(dwError);

    if(!dwError)
    {
        pResponse->tag = NTLM_R_FREE_CREDS_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_FREE_CREDS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcImportSecurityContext(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_IMPORT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerImportSecurityContext(
        pReq->pszPackage,
        pReq->pPackedContext,
        pReq->pToken,
        &(pNtlmResp->hContext)
        );

    if(!dwError)
    {
        pResponse->tag = NTLM_R_IMPORT_SEC_CTXT_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_IMPORT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcInitializeSecurityContext(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_INIT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_INIT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    DWORD dwStatus = LW_ERROR_SUCCESS;

    if(!pReq->hContext)
    {
        dwStatus = LW_WARNING_CONTINUE_NEEDED;
    }

    dwError = LsaAllocateMemory(sizeof(*pNtlmResp), OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerInitializeSecurityContext(
        &pReq->hCredential,
        &pReq->hContext,
        pReq->pszTargetName,
        pReq->fContextReq,
        pReq->Reserved1,
        pReq->TargetDataRep,
        pReq->pInput,
        pReq->Reserved2,
        &pReq->hNewContext,
        pReq->pOutput,
        &pNtlmResp->fContextAttr,
        &pNtlmResp->tsExpiry
        );

    if(!dwError)
    {
        pNtlmResp->hNewContext = pReq->hNewContext;
        pReq->hNewContext = NULL;

        pNtlmResp->dwStatus = dwStatus;
        memcpy(&(pNtlmResp->Output), pReq->pOutput, sizeof(SecBufferDesc));

        pReq->pOutput = NULL;

        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_register_handle(
                                      pAssoc,
                                      "LSA_CONTEXT_HANDLE",
                                      pNtlmResp->hNewContext,
                                      NtlmSrvFreeContextHandle));
        BAIL_ON_LW_ERROR(dwError);

        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_retain_handle(pAssoc, pNtlmResp->hNewContext));
        BAIL_ON_LSA_ERROR(dwError);


        pResponse->tag = NTLM_R_INIT_SEC_CTXT_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_INIT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcMakeSignature(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_MAKE_SIGN_REQ pReq = pRequest->object;
    PNTLM_IPC_MAKE_SIGN_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_MAKE_SIGN_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerMakeSignature(
        &pReq->hContext,
        pReq->bEncrypt,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    if(!dwError)
    {
        memcpy(&(pNtlmResp->Message), pReq->pMessage, sizeof(SecBufferDesc));
        pReq->pMessage = NULL;

        pResponse->tag = NTLM_R_MAKE_SIGN_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_MAKE_SIGN_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcQueryCredentialsAttributes(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_QUERY_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_QUERY_CREDS_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_QUERY_CREDS_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerQueryCredentialsAttributes(
        &pReq->hCredential,
        pReq->ulAttribute,
        pNtlmResp->pBuffer
        );

    if(!dwError)
    {
        // we only support getting the user name for now, so this should be
        // safe until that changes.
        pNtlmResp->dwBufferSize = strlen(pNtlmResp->pBuffer);

        pResponse->tag = NTLM_R_QUERY_CREDS_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_QUERY_CREDS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcQueryContextAttributes(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_QUERY_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_QUERY_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_QUERY_CTXT_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerQueryContextAttributes(
        &pReq->hContext,
        pReq->ulAttribute,
        pNtlmResp->pBuffer
        );

    if(!dwError)
    {
        // For now we only support getting the sizes of the context components
        pNtlmResp->dwBufferSize = sizeof(SecPkgContext_Sizes);

        pResponse->tag = NTLM_R_QUERY_CTXT_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_QUERY_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcVerifySignature(
    LWMsgAssoc* pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_VERIFY_SIGN_REQ pReq = pRequest->object;
    PNTLM_IPC_VERIFY_SIGN_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(
        sizeof(NTLM_IPC_VERIFY_SIGN_RESPONSE),
        (PVOID*)(PVOID)&pNtlmResp
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmServerVerifySignature(
        &pReq->hContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &(pNtlmResp->bVerified),
        &(pNtlmResp->bEncrypted)
        );

    if(!dwError)
    {
        pResponse->tag = NTLM_R_VERIFY_SIGN_SUCCESS;
        pResponse->object = pNtlmResp;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_VERIFY_SIGN_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    if(pNtlmResp)
    {
        LsaFreeMemory(pNtlmResp);
    }
    goto cleanup;
}

VOID
NtlmSrvFreeCredHandle(
    PVOID pData
    )
{
    LsaReleaseCredential((LSA_CRED_HANDLE)pData);
}

VOID
NtlmSrvFreeContextHandle(
    PVOID pData
    )
{
    NtlmReleaseContext((LSA_CONTEXT_HANDLE)pData);
}
