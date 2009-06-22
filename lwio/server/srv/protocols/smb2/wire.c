/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        wire.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Wire protocol
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SMB2MarshalHeader(
    PSMB_PACKET pSmbPacket,
    USHORT      usCommand,
    USHORT      usCredits,
    ULONG       ulPid,
    ULONG       ulTid,
    ULONG64     ullSessionId,
    NTSTATUS    status,
    BOOLEAN     bIsResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    static uchar8_t smb2Magic[4] = { 0xFE, 'S', 'M', 'B' };
    ULONG    ulBufferUsed = 0;
    ULONG    ulBufferAvailable = pSmbPacket->bufferLen;
    PBYTE pBuffer = pSmbPacket->pRawBuffer;

    if (ulBufferAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->pNetBIOSHeader = (NETBIOS_HEADER *) (pBuffer);
    ulBufferUsed += sizeof(NETBIOS_HEADER);
    ulBufferAvailable -= sizeof(NETBIOS_HEADER);
    pBuffer += sizeof(NETBIOS_HEADER);

    if (ulBufferAvailable < sizeof(SMB2_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->packetType = SMB_PACKET_TYPE_SMB_2;
    pSmbPacket->pSMB2Header = (PSMB2_HEADER)(pBuffer);

    ulBufferUsed += sizeof(SMB2_HEADER);
    ulBufferAvailable -= sizeof(SMB2_HEADER);
    pBuffer += sizeof(SMB2_HEADER);

    memcpy(&pSmbPacket->pSMB2Header->smb[0], &smb2Magic[0], sizeof(smb2Magic));
    pSmbPacket->pSMB2Header->command        = usCommand;
    pSmbPacket->pSMB2Header->usCredits      = usCredits;
    pSmbPacket->pSMB2Header->ulPid          = ulPid;
    pSmbPacket->pSMB2Header->ulTid          = ulTid;
    pSmbPacket->pSMB2Header->ullSessionId   = ullSessionId;
    pSmbPacket->pSMB2Header->error         = status;
    pSmbPacket->pSMB2Header->usHeaderLen = sizeof(SMB2_HEADER);

    if (bIsResponse)
    {
        pSmbPacket->pSMB2Header->ulFlags |= SMB2_FLAGS_SERVER_TO_REDIR;
    }

    pSmbPacket->pParams = pSmbPacket->pRawBuffer + ulBufferUsed;

    pSmbPacket->bufferUsed = ulBufferUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2PacketMarshallFooter(
    PSMB_PACKET pPacket
    )
{
    pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));

    return 0;
}
