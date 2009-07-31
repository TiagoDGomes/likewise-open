#include "includes.h"

NTSTATUS
SrvProcessTransaction2(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT pBytecount = NULL; // Do not free
    PUSHORT pSetup = NULL; // Do not free
    PBYTE   pParameters = NULL; // Do not free
    PBYTE   pData = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;
    ULONG   ulOffset = 0;

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = WireUnmarshallTransactionRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pSetup,
                    &pBytecount,
                    NULL, /* No transaction name */
                    &pParameters,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pSetup == NULL)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (*pSetup)
    {
        case SMB_SUB_COMMAND_TRANS2_OPEN2 :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_FIND_FIRST2 :

            ntStatus = SrvProcessTrans2FindFirst2(
                          pConnection,
                          pSmbRequest,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData,
                          &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS2_FIND_NEXT2 :

            ntStatus = SrvProcessTrans2FindNext2(
                          pConnection,
                          pSmbRequest,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData,
                          &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_FS_INFORMATION :

            ntStatus = SrvProcessTrans2QueryFilesystemInformation(
                          pConnection,
                          pSmbRequest,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData,
                          &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_PATH_INFORMATION :

            ntStatus = SrvProcessTrans2QueryPathInformation(
                          pConnection,
                          pSmbRequest,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData,
                          &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS2_SET_PATH_INFORMATION :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_FILE_INFORMATION :

            ntStatus = SrvProcessTrans2QueryFileInformation(
                          pConnection,
                          pSmbRequest,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData,
                          &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS2_SET_FILE_INFORMATION :

            ntStatus = SrvProcessTrans2SetFileInformation(
                            pConnection,
                            pSmbRequest,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pParameters,
                            pData,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS2_FSCTL :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_IOCTL2 :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_FIRST :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_NEXT :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_CREATE_DIRECTORY :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_SESSION_SETUP :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_GET_DFS_REFERRAL :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS2_REPORT_DFS_INCONSISTENCY :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

