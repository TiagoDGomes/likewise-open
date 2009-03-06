#include "npfs.h"


NTSTATUS
NpfsCreateFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;

    ntStatus = NpfsAllocateMemory(
                        sizeof(*pFCB),
                        OUT_PPVOID(&pFCB)
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlUnicodeStringDuplicate(
                    &pFCB->PipeName,
                    pUnicodeString
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_rwlock_init(&pFCB->PipeListRWLock, NULL);

    pFCB->MaxNumberOfInstances = 0xFF;
    // Number of currently available instances
    pFCB->CurrentNumberOfInstances = 0xFF;
    // TODO: This should be the default type
    pFCB->NamedPipeType = FILE_PIPE_MESSAGE_TYPE;

    pFCB->pNext = gpFCB;
    gpFCB = pFCB;

    *ppFCB = pFCB;

    return(ntStatus);

error:

    if (pFCB) {
        NpfsFreeMemory(pFCB);
    }

    *ppFCB = NULL;

    return(ntStatus);
}

NTSTATUS
NpfsFindFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;
    BOOLEAN bEqual = FALSE;

    pFCB = gpFCB;
    while (pFCB) {
         bEqual = RtlUnicodeStringIsEqual(
                            pUnicodeString,
                            &pFCB->PipeName,
                            FALSE
                            );
        if (bEqual) {
            NpfsAddRefFCB(pFCB);
            *ppFCB = pFCB;
            return (ntStatus);
        }
        pFCB = pFCB->pNext;
    }
    ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
    *ppFCB = NULL;
    return(ntStatus);
}

VOID
NpfsReleaseFCB(
    PNPFS_FCB pFCB
    )
{
    NpfsInterlockedDecrement(&pFCB->cRef);
    if (!NpfsInterlockedCounter(&pFCB->cRef)) {

        NpfsFreeFCB(pFCB);
    }
    return;
}

VOID
NpfsAddRefFCB(
    PNPFS_FCB pFCB
    )
{
    NpfsInterlockedIncrement(&pFCB->cRef);
    return;
}


NTSTATUS
NpfsFreeFCB(
    PNPFS_FCB pFCB
    )
{
    NTSTATUS ntStatus = 0;


    NpfsFreeMemory(pFCB);


    return(ntStatus);
}




