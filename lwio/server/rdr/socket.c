/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        socket.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Socket Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

static
int
SMBSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSocketHashSessionByUID(
    PCVOID vp
    );

static
int
SMBSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSocketHashSessionByKey(
    PCVOID vp
    );

static
void *
SMBSocketReaderMain(
    void *pData
    );

static
NTSTATUS
SMBSocketFindAndSignalResponse(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
NTSTATUS
SMBSocketFindSessionByUID(
    PSMB_SOCKET   pSocket,
    uint16_t      uid,
    PSMB_SESSION* ppSession
    );

static
int
SMBSocketGetFd(
    PSMB_SOCKET pSocket
    );

static
NTSTATUS
RdrEaiToNtStatus(
    int eai
    );

NTSTATUS
SMBSocketCreate(
    IN PCSTR pszHostname,
    IN BOOLEAN bUseSignedMessagesIfSupported,
    OUT PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    SMB_SOCKET *pSocket = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;

    ntStatus = SMBAllocateMemory(
                sizeof(SMB_SOCKET),
                (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->bUseSignedMessagesIfSupported = bUseSignedMessagesIfSupported;

    pthread_mutex_init(&pSocket->mutex, NULL);
    bDestroyMutex = TRUE;

    ntStatus = pthread_cond_init(&pSocket->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pSocket->refCount = 1;

    /* @todo: find a portable time call which is immune to host date and time
       changes, such as made by ntpd */
    pSocket->lastActiveTime = time(NULL);

    pSocket->fd = -1;

    /* Hostname is trusted */
    ntStatus = SMBStrndup(
                    (char *) pszHostname,
                    strlen((char *) pszHostname) + sizeof(NUL),
                    (char **) &pSocket->pszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->maxBufferSize = 0;
    pSocket->maxRawSize = 0;
    pSocket->sessionKey = 0;
    pSocket->capabilities = 0;
    pSocket->pSecurityBlob = NULL;
    pSocket->securityBlobLen = 0;

    pSocket->hPacketAllocator = gRdrRuntime.hPacketAllocator;

    ntStatus = SMBHashCreate(
                    19,
                    SMBSocketHashSessionCompareByKey,
                    SMBSocketHashSessionByKey,
                    NULL,
                    &pSocket->pSessionHashByPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashCreate(
                    19,
                    SMBSocketHashSessionCompareByUID,
                    SMBSocketHashSessionByUID,
                    NULL,
                    &pSocket->pSessionHashByUID);
    BAIL_ON_NT_STATUS(ntStatus);

    /* The reader thread will immediately block waiting for initialization */
    ntStatus = pthread_create(
                    &pSocket->readerThread,
                    NULL,
                    &SMBSocketReaderMain,
                    (void *) pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->pSessionPacket = NULL;

    *ppSocket = pSocket;

cleanup:

    return ntStatus;

error:

    if (pSocket)
    {
        SMBHashSafeFree(&pSocket->pSessionHashByUID);
        SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);

        LWIO_SAFE_FREE_MEMORY(pSocket->pszHostname);

        if (bDestroyCondition)
        {
            pthread_cond_destroy(&pSocket->event);
        }

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSocket->mutex);
        }

        SMBFreeMemory(pSocket);
    }

    *ppSocket = NULL;

    goto cleanup;
}

static
int
SMBSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    )
{
    uint16_t uid1 = *((uint16_t *) vp1);
    uint16_t uid2 = *((uint16_t *) vp2);

    if (uid1 == uid2)
    {
        return 0;
    }
    else if (uid1 > uid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
SMBSocketHashSessionByUID(
    PCVOID vp
    )
{
   return *((uint16_t *) vp);
}

static
int
SMBSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    )
{
    const struct _RDR_SESSION_KEY* pKey1 = (struct _RDR_SESSION_KEY*) vp1;
    const struct _RDR_SESSION_KEY* pKey2 = (struct _RDR_SESSION_KEY*) vp2;

    return !(pKey1->uid == pKey2->uid &&
             !strcmp(pKey1->pszPrincipal, pKey2->pszPrincipal));
}

static
size_t
SMBSocketHashSessionByKey(
    PCVOID vp
    )
{
    const struct _RDR_SESSION_KEY* pKey = (struct _RDR_SESSION_KEY*) vp;

    return SMBHashCaselessString(pKey->pszPrincipal) ^ (pKey->uid ^ (pKey->uid << 16));
}

BOOLEAN
SMBSocketTimedOut(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTimedOut = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    bTimedOut = SMBSocketTimedOut_InLock(pSocket);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return bTimedOut;
}

BOOLEAN
SMBSocketTimedOut_InLock(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bTimedOut = FALSE;
    DWORD   dwDiffSeconds = 0;

    /* Because we don't compare with the time of last write, only last read, a
       socket can become stale immediately after a request has been sent if
       the socket was previously idle.  We rely on the large timeout in
       SMBTreeReceiveResponse to smooth this over; we don't want to block
       forever on requests just because other threads are writing to the
       (possibly dead) socket. */
    dwDiffSeconds = difftime(time(NULL), pSocket->lastActiveTime);
    if (dwDiffSeconds > 30)
    {
        LWIO_LOG_DEBUG("Socket timed out and was stale for [%d] seconds", dwDiffSeconds);
        bTimedOut = TRUE;
    }

    return bTimedOut;
}

BOOLEAN
SMBSocketIsSignatureRequired(
    IN PSMB_SOCKET pSocket
    )
{
    BOOLEAN bIsRequired = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* We need to sign outgoing packets if we negotiated it and the
       socket is in the ready state -- that is, we have completed the
       negotiate stage.  This causes the initial session setup packet to
       be signed, even though we do not yet have a session key.  This results
       in signing with a zero-length key, which matches Windows behavior.
       Note that no known SMB server actually verifies this signature since it
       is meaningless and probably the result of an implementation quirk. */
    if (pSocket->state == RDR_SOCKET_STATE_READY &&
        (pSocket->bSignedMessagesRequired ||
         (pSocket->bSignedMessagesSupported && pSocket->bUseSignedMessagesIfSupported)))
    {
        bIsRequired = TRUE;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return bIsRequired;
}

static
ULONG
SMBSocketGetNextSequence_inlock(
    PSMB_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    dwSequence = pSocket->dwSequence;
    // Next for response
    // Next for next message
    pSocket->dwSequence += 2;

    return dwSequence;
}

ULONG
SMBSocketGetNextSequence(
    PSMB_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwSequence = SMBSocketGetNextSequence_inlock(pSocket);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwSequence;
}

VOID
SMBSocketUpdateLastActiveTime(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->lastActiveTime = time(NULL);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

NTSTATUS
SMBSocketSend(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    /* @todo: signal handling */
    NTSTATUS ntStatus = 0;
    ssize_t  writtenLen = 0;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bSemaphoreAcquired = FALSE;
    BOOLEAN bIsSignatureRequired = FALSE;

    if (pSocket->maxMpxCount)
    {
        ntStatus = SMBSemaphoreWait(&pSocket->semMpx);
        BAIL_ON_NT_STATUS(ntStatus);

        bSemaphoreAcquired = TRUE;
    }

    if (pPacket->allowSignature)
    {
        bIsSignatureRequired = SMBSocketIsSignatureRequired(pSocket);
    }

    if (bIsSignatureRequired)
    {
        pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;
    }

    SMBPacketHTOLSmbHeader(pPacket->pSMBHeader);

    pPacket->haveSignature = bIsSignatureRequired;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    if (pPacket->pSMBHeader->command != COM_NEGOTIATE)
    {
        pPacket->sequence = SMBSocketGetNextSequence_inlock(pSocket);
    }

    if (bIsSignatureRequired)
    {
        ntStatus = SMBPacketSign(
                        pPacket,
                        pPacket->sequence,
                        pSocket->pSessionKey,
                        pSocket->dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    writtenLen = write(
                    pSocket->fd,
                    pPacket->pRawBuffer,
                    pPacket->bufferUsed);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    if (writtenLen < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    if (bSemaphoreAcquired)
    {
        NTSTATUS localStatus = SMBSemaphorePost(&pSocket->semMpx);
        if (localStatus)
        {
            LWIO_LOG_ERROR("Failed to post semaphore (status = 0x%08X)", localStatus);
        }
    }

    if (pSocket != NULL)
    {
        SMBSocketInvalidate(pSocket, ntStatus);
    }

    goto cleanup;
}

NTSTATUS
SMBSocketReceiveAndUnmarshall(
    IN PSMB_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    /* @todo: handle timeouts, signals, buffer overflow */
    /* This logic would need to be modified for zero copy */
    uint32_t len = sizeof(NETBIOS_HEADER);
    uint32_t readLen = 0;
    uint32_t bufferUsed = 0;

    /* @todo: support read threads in the daemonized case */
    ntStatus = SMBSocketRead(pSocket, pPacket->pRawBuffer, len, &readLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if (len != readLen)
    {
        ntStatus = STATUS_END_OF_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
    bufferUsed += len;

    pPacket->pNetBIOSHeader->len = htonl(pPacket->pNetBIOSHeader->len);

    if ((uint64_t) pPacket->pNetBIOSHeader->len + (uint64_t) bufferUsed > (uint64_t) pPacket->bufferLen)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBSocketRead(
                    pSocket,
                    pPacket->pRawBuffer + bufferUsed,
                    pPacket->pNetBIOSHeader->len,
                    &readLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if(pPacket->pNetBIOSHeader->len != readLen)
    {
        ntStatus = STATUS_END_OF_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket->pSMBHeader = (SMB_HEADER *) (pPacket->pRawBuffer + bufferUsed);
    bufferUsed += sizeof(SMB_HEADER);

    if (SMBIsAndXCommand(SMB_LTOH8(pPacket->pSMBHeader->command)))
    {
        pPacket->pAndXHeader = (ANDX_HEADER *)
            (pPacket->pSMBHeader + bufferUsed);
        bufferUsed += sizeof(ANDX_HEADER);
    }

    pPacket->pParams = pPacket->pRawBuffer + bufferUsed;
    pPacket->pData = NULL;
    pPacket->bufferUsed = pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER);

error:

    return ntStatus;
}

static
PVOID
SMBSocketReaderMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SOCKET pSocket = (PSMB_SOCKET) pData;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pPacket = NULL;

    LWIO_LOG_INFO("Spawning socket reader thread for [%s]",
                 SMB_SAFE_LOG_STRING((char *) pSocket->pszHostname));

    /* Wait for thread to become ready */
    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    while (pSocket->state < RDR_SOCKET_STATE_NEGOTIATING)
    {
        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    /* When the ref. count drops to zero, shutdown() breaks out of this loop */
    while (SMBSocketGetState(pSocket) < RDR_SOCKET_STATE_TEARDOWN)
    {
        int ret = 0;
        fd_set fdset;
        int fd = SMBSocketGetFd(pSocket);

        FD_ZERO(&fdset);
        FD_SET(fd, &fdset);

        ret = select(fd + 1, &fdset, NULL, &fdset, NULL);
        if (ret == -1)
        {
            ntStatus = UnixErrnoToNtStatus(errno);
        }
        else if (ret != 1)
        {
            ntStatus = STATUS_ASSERTION_FAILURE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);
        if (pSocket->bShutdown)
        {
            goto cleanup;
        }
        LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

        SMBSocketUpdateLastActiveTime(pSocket);

        ntStatus = SMBPacketAllocate(pSocket->hPacketAllocator, &pPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBPacketBufferAllocate(
                    pSocket->hPacketAllocator,
                    1024*64,
                    &pPacket->pRawBuffer,
                    &pPacket->bufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Read whole messages */
        ntStatus = SMBSocketReceiveAndUnmarshall(pSocket, pPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pSocket->maxMpxCount)
        {
            ntStatus = SMBSemaphorePost(&pSocket->semMpx);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        /* @todo: the client thread is responsible for calling FreePacket(),
           which will lock the socket and add the memory back to the free
           list, if appropriate. */
        /* This function should free packet and socket memory on error */
        ntStatus = SMBSocketFindAndSignalResponse(pSocket, pPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        pPacket = NULL;
    }

cleanup:

    if (pPacket)
    {
        SMBPacketRelease(pSocket->hPacketAllocator, pPacket);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return NULL;

error:

    LWIO_LOG_ERROR("Error when handling SMB socket[code:%d]", ntStatus);

    goto cleanup;
}

/* Ref. counting intermediate structures is not strictly necessary because
   there are currently no blocking operations in the response path; one could
   simply lock hashes all the way up the path */
static
NTSTATUS
SMBSocketFindAndSignalResponse(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SESSION  pSession  = NULL;
    PSMB_TREE     pTree     = NULL;
    PSMB_RESPONSE pResponse = NULL;
    BOOLEAN bSocketInLock = FALSE;
    uint8_t command = SMB_LTOH8(pPacket->pSMBHeader->command);
    uint16_t uid = SMB_LTOH16(pPacket->pSMBHeader->uid);
    uint16_t tid = SMB_LTOH16(pPacket->pSMBHeader->tid);
    uint16_t mid = SMB_LTOH16(pPacket->pSMBHeader->mid);

    /* If any intermediate object has an error status, then the
       waiting thread has (or will soon) be awoken with an error
       condition by the thread which set the original error. */

    if (command == COM_NEGOTIATE ||
        command == COM_SESSION_SETUP_ANDX ||
        command == COM_LOGOFF_ANDX)
    {
        LWIO_LOCK_MUTEX(bSocketInLock, &pSocket->mutex);

        assert(!pSocket->pSessionPacket);

        pSocket->pSessionPacket = pPacket;

        pthread_cond_broadcast(&pSocket->event);

        LWIO_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

        goto cleanup;
    }

    ntStatus = SMBSocketFindSessionByUID(
                    pSocket,
                    uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    /* COM_TREE_DISCONNECT has a MID and is handled by the normal MID path. */
    if (command == COM_TREE_CONNECT_ANDX)
    {
        BOOLEAN bSessionInLock = FALSE;

        LWIO_LOCK_MUTEX(bSessionInLock, &pSession->mutex);

        assert(!pSession->pTreePacket);
        pSession->pTreePacket = pPacket;

        pthread_cond_broadcast(&pSession->event);

        LWIO_UNLOCK_MUTEX(bSessionInLock, &pSession->mutex);

        goto cleanup;
    }

    ntStatus = SMBSessionFindTreeById(
                    pSession,
                    tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeFindLockedResponseByMID(
                    pTree,
                    mid,
                    &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOG_DEBUG("Found response [mid: %d] in Tree [0x%x] Socket [0x%x]", mid, pTree, pSocket);

    pResponse->pPacket = pPacket;
    pResponse->state = SMB_RESOURCE_STATE_VALID;

    pthread_cond_broadcast(&pResponse->event);

cleanup:

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    if (pResponse)
    {
        LWIO_LOG_DEBUG("Unlocking response [mid: %d] in Tree [0x%x]", pResponse->mid, pTree);

        SMBResponseUnlock(pResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SMBSocketFindSessionByUID(
    PSMB_SOCKET   pSocket,
    uint16_t      uid,
    PSMB_SESSION* ppSession
    )
{
    /* It is not necessary to ref. the socket here because we're guaranteed
       that the reader thread dies before the socket is destroyed */
    NTSTATUS ntStatus = 0;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashGetValue(
                    pSocket->pSessionHashByUID,
                    &uid,
                    (PVOID *) &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refCount++;

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    goto cleanup;
}

VOID
SMBSocketAddReference(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    pSocket->refCount++;

    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
}

NTSTATUS
SMBSocketConnect(
    PSMB_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = 0;
    int fd = -1;
    fd_set fdset;
    struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };
    int ret = 0;
    BOOLEAN bInLock = FALSE;
    struct addrinfo *ai = NULL;
    struct addrinfo *pCursor = NULL;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    ntStatus = RdrEaiToNtStatus(
        getaddrinfo(pSocket->pszHostname, "445", &hints, &ai));
    BAIL_ON_NT_STATUS(ntStatus);

    for (pCursor = ai; pCursor; pCursor = pCursor->ai_next)
    {
        fd = socket(pCursor->ai_family, pCursor->ai_socktype, pCursor->ai_protocol);

        if (fd < 0)
        {
#ifdef EPROTONOSUPPORT
            if (errno == EPROTONOSUPPORT)
            {
                continue;
            }
#endif
            ntStatus = UnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            break;
        }
    }

    if (fd < 0)
    {
        ntStatus = STATUS_BAD_NETWORK_NAME;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        ntStatus = UnixErrnoToNtStatus(errno);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    if (connect(fd, ai->ai_addr, ai->ai_addrlen) && errno != EINPROGRESS)
    {
        ntStatus = UnixErrnoToNtStatus(errno);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ret = select(fd + 1, NULL, &fdset, &fdset, &tv);
    if (ret == 0)
    {
        ntStatus = STATUS_IO_TIMEOUT;
    }
    else if (ret == -1)
    {
        ntStatus = UnixErrnoToNtStatus(errno);
    }
    else if (ret != 1)
    {
        ntStatus = STATUS_ASSERTION_FAILURE;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* We are done connecting -- go to negotiation stage */
    pSocket->state = RDR_SOCKET_STATE_NEGOTIATING;
    pSocket->fd = fd;
    fd = -1;
    memcpy(&pSocket->address, &ai->ai_addr, ai->ai_addrlen);

    pthread_cond_broadcast(&pSocket->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

cleanup:

    if (ai)
    {
        freeaddrinfo(ai);
    }

    return ntStatus;

error:

    if (fd >= 0)
    {
        close(fd);
    }

    SMBSocketInvalidate(pSocket, ntStatus);

    goto cleanup;
}

NTSTATUS
SMBSocketRead(
    PSMB_SOCKET pSocket,
    uint8_t *buffer,
    uint32_t len,
    uint32_t *actualLen
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t totalRead = 0;
    ssize_t nRead = 0;
    fd_set fdset;

    FD_ZERO(&fdset);

    while (totalRead < len)
    {
        int ret = 0;
        /* Always ten seconds from last data read */
        struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };

        FD_SET(pSocket->fd, &fdset);

        ret = select(pSocket->fd + 1, &fdset, NULL, &fdset, &tv);
        if (ret == 0)
        {
            ntStatus = STATUS_IO_TIMEOUT;
        }
        else if (ret == -1)
        {
            ntStatus = UnixErrnoToNtStatus(errno);
        }
        else if (ret != 1)
        {
            ntStatus = STATUS_ASSERTION_FAILURE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        nRead = read(pSocket->fd, buffer + totalRead, len - totalRead);
        if(nRead < 0)
        {
            ntStatus = UnixErrnoToNtStatus(errno);
        }
        else if (nRead == 0)
        {
            /* EOF */
            goto cleanup;
        }

        totalRead += nRead;
    }

cleanup:

    *actualLen = totalRead;

    return ntStatus;

error:

    SMBSocketInvalidate(pSocket, ntStatus);

    goto cleanup;
}

VOID
SMBSocketInvalidate(
    PSMB_SOCKET pSocket,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    SMBSocketInvalidate_InLock(pSocket, ntStatus);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

VOID
SMBSocketInvalidate_InLock(
    PSMB_SOCKET pSocket,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInGlobalLock = FALSE;
    pSocket->state = RDR_SOCKET_STATE_ERROR;
    pSocket->error = ntStatus;

    LWIO_LOCK_MUTEX(bInGlobalLock, &gRdrRuntime.socketHashLock);
    if (pSocket->bParentLink)
    {
        SMBHashRemoveKey(gRdrRuntime.pSocketHashByName,
                         pSocket->pszHostname);
        pSocket->bParentLink = FALSE;
    }
    LWIO_UNLOCK_MUTEX(bInGlobalLock, &gRdrRuntime.socketHashLock);

    pthread_cond_broadcast(&pSocket->event);
}

VOID
SMBSocketSetState(
    PSMB_SOCKET        pSocket,
    RDR_SOCKET_STATE   state
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->state = state;

    pthread_cond_broadcast(&pSocket->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

RDR_SOCKET_STATE
SMBSocketGetState(
    PSMB_SOCKET        pSocket
    )
{
    BOOLEAN bInLock = FALSE;
    RDR_SOCKET_STATE socketState = RDR_SOCKET_STATE_ERROR;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    socketState = pSocket->state;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return socketState;
}

static
int
SMBSocketGetFd(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;
    int fd = -1;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    fd = pSocket->fd;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return fd;
}

NTSTATUS
SMBSocketReceiveResponse(
    IN PSMB_SOCKET pSocket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    OUT PSMB_PACKET* ppPacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    struct timespec ts = { 0, 0 };
    PSMB_PACKET pPacket = NULL;
    int err = 0;

    // TODO-The pSocket->pSessionPacket stuff needs to go away
    // so that this function can go away.

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* @todo: always check socket state for error */

    while (!pSocket->pSessionPacket)
    {

        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

retry_wait:

        /* @todo: always verify non-error state after acquiring mutex */
        err = pthread_cond_timedwait(
            &pSocket->event,
            &pSocket->mutex,
            &ts);
        if (err == ETIMEDOUT)
        {
            if (time(NULL) < ts.tv_sec)
            {
                err = 0;
                goto retry_wait;
            }

            /* As long as the socket is active, continue to wait.
             * otherwise, mark the socket as bad and return
             */
            if (SMBSocketTimedOut_InLock(pSocket))
            {
                ntStatus = STATUS_IO_TIMEOUT;
                SMBSocketInvalidate_InLock(pSocket, ntStatus);
            }
            else
            {
                ntStatus = STATUS_SUCCESS;
            }
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket = pSocket->pSessionPacket;
    pSocket->pSessionPacket = NULL;

    ntStatus = SMBPacketDecodeHeader(
                    pPacket,
                    bVerifySignature && !pSocket->bIgnoreServerSignatures,
                    dwExpectedSequence,
                    pSocket->pSessionKey,
                    pSocket->dwSessionKeyLength);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    *ppPacket = pPacket;

    return ntStatus;

error:
    if (pPacket)
    {
        SMBPacketRelease(pSocket->hPacketAllocator, pPacket);
        pPacket = NULL;
    }

    goto cleanup;
}

NTSTATUS
SMBSocketFindSessionByPrincipal(
    IN PSMB_SOCKET pSocket,
    IN PCSTR pszPrincipal,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SESSION pSession = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashGetValue(
                    pSocket->pSessionHashByPrincipal,
                    pszPrincipal,
                    (PVOID *) &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBSessionAddReference(pSession);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

VOID
SMBSocketRelease(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    assert(pSocket->refCount > 0);

    /* If the socket is no longer referenced and
       it is not usable, free it immediately.
       Otherwise, allow the reaper to collect it
       asynchronously */
    if (--pSocket->refCount == 0)
    {
        if (pSocket->state != RDR_SOCKET_STATE_READY)
        {
            if (pSocket->bParentLink)
            {
                SMBHashRemoveKey(gRdrRuntime.pSocketHashByName,
                                 pSocket->pszHostname);
                pSocket->bParentLink = FALSE;
            }
            LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
            SMBSocketFree(pSocket);
        }
        else
        {
            LWIO_LOG_VERBOSE("Socket %p is eligible for reaping", pSocket);
            LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
            RdrReaperPoke(&gRdrRuntime, pSocket->lastActiveTime);
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
    }

}

NTSTATUS
SMBSocketWaitReady(
    PSMB_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pSocket->state != RDR_SOCKET_STATE_READY)
    {
        if (pSocket->state == RDR_SOCKET_STATE_ERROR)
        {
            ntStatus = pSocket->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

error:

    return ntStatus;
}

NTSTATUS
SMBSocketWaitSessionSetup(
    PSMB_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pSocket->bSessionSetupInProgress)
    {
        if (pSocket->state == RDR_SOCKET_STATE_ERROR)
        {
            ntStatus = pSocket->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

error:

    return ntStatus;
}

VOID
SMBSocketFree(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    assert(!pSocket->refCount);

    /* shutdown read side of socket to wake up thread to join */
    if (pSocket->fd >= 0)
    {
        LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);
        pSocket->bShutdown = TRUE;
        LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
        shutdown(pSocket->fd, SHUT_RD);
    }

    pthread_join(pSocket->readerThread, NULL);

    if ((pSocket->fd >= 0) && (close(pSocket->fd) < 0))
    {
        LWIO_LOG_ERROR("Failed to close socket [fd:%d]", pSocket->fd);
    }

    if (pSocket && pSocket->maxMpxCount)
    {
        SMBSemaphoreDestroy(&pSocket->semMpx);
    }

    pthread_cond_destroy(&pSocket->event);

    LWIO_SAFE_FREE_MEMORY(pSocket->pszHostname);
    LWIO_SAFE_FREE_MEMORY(pSocket->pSecurityBlob);

    /* @todo: assert that the session hashes are empty */
    SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);
    SMBHashSafeFree(&pSocket->pSessionHashByUID);

    assert(!pSocket->pSessionPacket);

    pthread_mutex_destroy(&pSocket->mutex);

    LWIO_SAFE_FREE_MEMORY(pSocket->pSessionKey);

    /* @todo: use allocator */
    SMBFreeMemory(pSocket);
}

VOID
RdrSocketSetIgnoreServerSignatures(
    PSMB_SOCKET pSocket,
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->bIgnoreServerSignatures = bValue;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
NTSTATUS
RdrEaiToNtStatus(
    int eai
    )
{
    switch (eai)
    {
    case 0:
        return STATUS_SUCCESS;
    case EAI_NONAME:
#ifdef EAI_NODATA
    case EAI_NODATA:
#endif
        return STATUS_BAD_NETWORK_NAME;
    case EAI_MEMORY:
        return STATUS_INSUFFICIENT_RESOURCES;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

