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
 *        tree.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Tree Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 */

#include "rdr.h"

static
NTSTATUS
SMBTreeDestroyContents(
    PSMB_TREE pTree
    );

NTSTATUS
SMBTreeCreate(
    PSMB_TREE* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_TREE pTree = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_t* pMutexAttr = NULL;

    ntStatus = SMBAllocateMemory(
                sizeof(SMB_TREE),
                (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pTree->StateWaiters);

    ntStatus = pthread_mutexattr_init(&mutexAttr);
    BAIL_ON_NT_STATUS(ntStatus);

    pMutexAttr = &mutexAttr;

    ntStatus = pthread_mutexattr_settype(pMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pTree->mutex, pMutexAttr);
    bDestroyMutex = TRUE;

    ntStatus = pthread_cond_init(&pTree->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pTree->refCount = 1;
    pTree->pSession = NULL;
    pTree->tid = 0;
    pTree->pszPath = NULL;

    *ppTree = pTree;

cleanup:

    if (pMutexAttr)
    {
        pthread_mutexattr_destroy(pMutexAttr);
    }

    return ntStatus;

error:

    if (bDestroyCondition)
    {
        pthread_cond_destroy(&pTree->event);
    }

    if (bDestroyMutex)
    {
        pthread_mutex_destroy(&pTree->mutex);
    }

    if (pTree)
    {
        SMBTreeDestroyContents(pTree);
    }
    LWIO_SAFE_FREE_MEMORY(pTree);

    *ppTree = NULL;

    goto cleanup;
}

VOID
RdrTreeRevive(
    PSMB_TREE pTree
    )
{
    if (pTree->pTimeout)
    {
        LwRtlCancelTask(pTree->pTimeout);
        LwRtlReleaseTask(&pTree->pTimeout);
    }
}

VOID
SMBTreeAddReference(
    PSMB_TREE pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->pSession->mutex);

    pTree->refCount++;
    RdrTreeRevive(pTree);

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
}

NTSTATUS
SMBTreeSetState(
    PSMB_TREE pTree,
    SMB_RESOURCE_STATE state
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    pTree->state = state;

    pthread_cond_broadcast(&pTree->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

static
VOID
RdrTreeUnlink(
    PSMB_TREE pTree
    )
{
    if (pTree->bParentLink)
    {
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByPath,
            pTree->pszPath);
        SMBHashRemoveKey(
            pTree->pSession->pTreeHashByTID,
            &pTree->tid);
        pTree->bParentLink = FALSE;
    }
}

NTSTATUS
SMBTreeInvalidate(
    PSMB_TREE pTree,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSessionLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->mutex);

    pTree->state = RDR_TREE_STATE_ERROR;
    pTree->error = ntStatus;

    LWIO_LOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);
    RdrTreeUnlink(pTree);
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pTree->pSession->mutex);

    pthread_cond_broadcast(&pTree->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

static
VOID
RdrTreeReap(
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PSMB_TREE pTree = pContext;
    PSMB_SESSION pSession = pTree->pSession;

    LWIO_LOCK_MUTEX(bLocked, &pSession->mutex);

    if (pTree->refCount == 0)
    {
        RdrTreeUnlink(pTree);
        LWIO_UNLOCK_MUTEX(bLocked, &pSession->mutex);
        status = TreeDisconnect(pTree);
        SMBTreeFree(pTree);
        BAIL_ON_NT_STATUS(status);
    }

error:

    LWIO_UNLOCK_MUTEX(bLocked, &pSession->mutex);
}

static
VOID
RdrTreeTimeout(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    PSMB_TREE pTree = pContext;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = RDR_IDLE_TIMEOUT * 1000000000ll;
    }
    else if (WakeMask & LW_TASK_EVENT_TIME)
    {
        if (LwRtlQueueWorkItem(gRdrRuntime.pThreadPool, RdrTreeReap, pTree, 0) == STATUS_SUCCESS)
        {
            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = RDR_IDLE_TIMEOUT * 1000000000ll;
        }
    }
}

VOID
SMBTreeRelease(
    SMB_TREE *pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pTree->pSession->mutex);

    assert(pTree->refCount > 0);

    if (--pTree->refCount == 0)
    {
        if (pTree->state != RDR_TREE_STATE_READY)
        {
            RdrTreeUnlink(pTree);
            LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
            SMBTreeFree(pTree);
        }
        else
        {
            LWIO_LOG_VERBOSE("Tree %p is eligible for reaping", pTree);

            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pTree->pTimeout,
                    gRdrRuntime.pReaderTaskGroup,
                    RdrTreeTimeout,
                    pTree) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pTree->pTimeout);
                LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
            }
            else
            {
                RdrTreeUnlink(pTree);
                LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
                SMBTreeFree(pTree);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pTree->pSession->mutex);
    }
}

VOID
SMBTreeFree(
    PSMB_TREE pTree
    )
{
    assert(!pTree->refCount);

    pthread_cond_destroy(&pTree->event);
    pthread_mutex_destroy(&pTree->mutex);

    SMBTreeDestroyContents(pTree);

    if (pTree->pSession)
    {
        SMBSessionRelease(pTree->pSession);
    }

    SMBFreeMemory(pTree);
}

static
NTSTATUS
SMBTreeDestroyContents(
    PSMB_TREE pTree
    )
{
    LWIO_SAFE_FREE_MEMORY(pTree->pszPath);

    if (pTree->pTimeout)
    {
        LwRtlCancelTask(pTree->pTimeout);
        LwRtlReleaseTask(&pTree->pTimeout);
    }

    return 0;
}

NTSTATUS
SMBTreeWaitReady(
    PSMB_TREE pTree
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pTree->state < RDR_TREE_STATE_READY)
    {
        if (pTree->state == RDR_TREE_STATE_ERROR)
        {
            ntStatus = pTree->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pTree->event, &pTree->mutex);
    }

error:

    return ntStatus;
}
