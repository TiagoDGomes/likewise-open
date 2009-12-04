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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __SMB2_NEGOTIATE_REQUEST_HEADER
{
    USHORT  usLength;
    USHORT  usDialectCount;
    USHORT  usSecurityMode;
    USHORT  usPad;
    ULONG   ulCapabilities;
    UCHAR   clientGUID[16];
    ULONG64 ulStartTime;

    // List of dialects follow immediately
} __attribute__((__packed__)) SMB2_NEGOTIATE_REQUEST_HEADER,
                             *PSMB2_NEGOTIATE_REQUEST_HEADER;

typedef struct __SMB2_NEGOTIATE_RESPONSE_HEADER
{
    USHORT  usLength;
    BYTE    ucFlags;
    BYTE    ucPad;
    USHORT  usDialect;
    USHORT  usPad2;
    BYTE    serverGUID[16];
    ULONG   ulCapabilities;
    ULONG   ulMaxTxSize;
    ULONG   ulMaxReadSize;
    ULONG   ulMaxWriteSize;
    ULONG64 ullCurrentTime;
    ULONG64 ullBootTime;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_NEGOTIATE_RESPONSE_HEADER,
                             *PSMB2_NEGOTIATE_RESPONSE_HEADER;

typedef struct __SMB2_SESSION_SETUP_REQUEST_HEADER
{
    USHORT  usLength;
    BYTE    ucVCNumber;
    BYTE    ucSecurityMode;
    ULONG   ulCapabilities;
    ULONG   ulChannel;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;
    ULONG64 ullPrevSessionId;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_SESSION_SETUP_REQUEST_HEADER;

typedef SMB2_SESSION_SETUP_REQUEST_HEADER *PSMB2_SESSION_SETUP_REQUEST_HEADER;

typedef struct __SMB2_SESSION_SETUP_RESPONSE_HEADER
{
    USHORT  usLength;
    USHORT  usSessionFlags;
    USHORT  usBlobOffset;
    USHORT  usBlobLength;

    /* GSS Blob follows immediately */

} __attribute__((__packed__)) SMB2_SESSION_SETUP_RESPONSE_HEADER,
                             *PSMB2_SESSION_SETUP_RESPONSE_HEADER;

typedef struct __SMB2_LOGOFF_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_LOGOFF_REQUEST_HEADER,
                             *PSMB2_LOGOFF_REQUEST_HEADER;

typedef struct __SMB2_LOGOFF_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_LOGOFF_RESPONSE_HEADER,
                             *PSMB2_LOGOFF_RESPONSE_HEADER;

typedef struct __SMB2_TREE_CONNECT_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usPad;
    USHORT usPathOffset;
    USHORT usPathLength;
} __attribute__((__packed__)) SMB2_TREE_CONNECT_REQUEST_HEADER,
                             *PSMB2_TREE_CONNECT_REQUEST_HEADER;

typedef struct __SMB2_TREE_CONNECT_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usShareType;
    ULONG  ulShareFlags;
    ULONG  ulShareCapabilities;
    ULONG  ulShareAccessMask;
} __attribute__((__packed__)) SMB2_TREE_CONNECT_RESPONSE_HEADER,
                             *PSMB2_TREE_CONNECT_RESPONSE_HEADER;

typedef struct __SMB2_TREE_DISCONNECT_REQUEST_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_TREE_DISCONNECT_REQUEST_HEADER,
                             *PSMB2_TREE_DISCONNECT_REQUEST_HEADER;

typedef struct __SMB2_TREE_DISCONNECT_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usReserved;
} __attribute__((__packed__)) SMB2_TREE_DISCONNECT_RESPONSE_HEADER,
                             *PSMB2_TREE_DISCONNECT_RESPONSE_HEADER;

typedef struct __SMB2_CREATE_CONTEXT
{
    ULONG  ulNextContextOffset;
    USHORT usNameOffset;
    USHORT usNameLength;
    USHORT usReserved;
    USHORT usDataOffset;
    ULONG  ulDataLength;

    /* ANSI Name */
    /* Optional padding to 8 byte boundary */
    /* Data */

} __attribute__((__packed__)) SMB2_CREATE_CONTEXT,
                             *PSMB2_CREATE_CONTEXT;

typedef struct __SRV_CREATE_CONTEXT
{
    SMB2_CONTEXT_ITEM_TYPE contextItemType;
    PCSTR                  pszName;
    USHORT                 usNameLen;
    ULONG                  ulDataLength;
    PBYTE                  pData;
} SRV_CREATE_CONTEXT, *PSRV_CREATE_CONTEXT;

typedef struct __SMB2_CREATE_REQUEST_HEADER
{
    USHORT  usLength;
    UCHAR   ucSecurityFlags;
    UCHAR   ucOplockLevel;
    ULONG   ulImpersonationLevel;
    ULONG64 ullCreateFlags;
    ULONG64 ullReserved;
    ULONG   ulDesiredAccess;
    ULONG   ulFileAttributes;
    ULONG   ulShareAccess;
    ULONG   ulCreateDisposition;
    ULONG   ulCreateOptions;
    USHORT  usNameOffset;
    USHORT  usNameLength;
    ULONG   ulCreateContextOffset;
    ULONG   ulCreateContextLength;
} __attribute__((__packed__)) SMB2_CREATE_REQUEST_HEADER,
                             *PSMB2_CREATE_REQUEST_HEADER;

typedef struct __SMB2_FID
{
    ULONG64 ullPersistentId;
    ULONG64 ullVolatileId;
} __attribute__((__packed__)) SMB2_FID, *PSMB2_FID;

typedef struct __SMB2_CREATE_RESPONSE_HEADER
{
    USHORT   usLength;
    UCHAR    ucOplockLevel;
    UCHAR    ucReserved;
    ULONG    ulCreateAction;
    ULONG64  ullCreationTime;
    ULONG64  ullLastAccessTime;
    ULONG64  ullLastWriteTime;
    ULONG64  ullLastChangeTime;
    ULONG64  ullAllocationSize;
    ULONG64  ullEndOfFile;
    ULONG    ulFileAttributes;
    ULONG    ulReserved2;
    SMB2_FID fid;
    ULONG    ulCreateContextOffset;
    ULONG    ulCreateContextLength;
} __attribute__((__packed__)) SMB2_CREATE_RESPONSE_HEADER,
                             *PSMB2_CREATE_RESPONSE_HEADER;

typedef struct __SMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT
{
    ULONG       ulQueryStatus;
    ACCESS_MASK accessMask;
} __attribute__((__packed__))  SMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT,
                             *PSMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT;

typedef struct __SMB2_CLOSE_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulReserved;
    SMB2_FID fid;
} __attribute__((__packed__)) SMB2_CLOSE_REQUEST_HEADER,
                             *PSMB2_CLOSE_REQUEST_HEADER;

typedef struct __SMB2_CLOSE_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    usReserved;
    ULONG64  ullCreationTime;
    ULONG64  ullLastAccessTime;
    ULONG64  ullLastWriteTime;
    ULONG64  ullLastChangeTime;
    ULONG64  ullAllocationSize;
    ULONG64  ullEndOfFile;
    ULONG    ulFileAttributes;
} __attribute__((__packed__)) SMB2_CLOSE_RESPONSE_HEADER,
                             *PSMB2_CLOSE_RESPONSE_HEADER;

typedef struct __SMB2_FLUSH_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usFlags;
    ULONG    ulReserved;
    SMB2_FID fid;
} __attribute__((__packed__)) SMB2_FLUSH_REQUEST_HEADER,
                             *PSMB2_FLUSH_REQUEST_HEADER;

typedef struct __SMB2_FLUSH_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
} __attribute__((__packed__)) SMB2_FLUSH_RESPONSE_HEADER,
                             *PSMB2_FLUSH_RESPONSE_HEADER;

typedef struct __SMB2_ECHO_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
} __attribute__((__packed__)) SMB2_ECHO_REQUEST_HEADER,
                             *PSMB2_ECHO_REQUEST_HEADER;

typedef struct __SMB2_ECHO_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
} __attribute__((__packed__)) SMB2_ECHO_RESPONSE_HEADER,
                             *PSMB2_ECHO_RESPONSE_HEADER;

typedef struct __SMB2_GET_INFO_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoType;
    UCHAR    ucInfoClass;
    ULONG    ulOutputBufferLen;
    USHORT   usInputBufferOffset;
    USHORT   usReserved;
    ULONG    ulInputBufferLen;
    ULONG    ulAdditionalInfo;
    ULONG    ulFlags;
    SMB2_FID fid;

} __attribute__((__packed__)) SMB2_GET_INFO_REQUEST_HEADER,
                             *PSMB2_GET_INFO_REQUEST_HEADER;

typedef struct __SMB2_GET_INFO_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usOutBufferOffset;
    ULONG  ulOutBufferLength;

} __attribute__((__packed__)) SMB2_GET_INFO_RESPONSE_HEADER,
                             *PSMB2_GET_INFO_RESPONSE_HEADER;

typedef struct __SMB2_SET_INFO_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoType;
    UCHAR    ucInfoClass;
    ULONG    ulInputBufferLen;
    USHORT   usInputBufferOffset;
    USHORT   usReserved;
    ULONG    ulAdditionalInfo;
    SMB2_FID fid;

} __attribute__((__packed__)) SMB2_SET_INFO_REQUEST_HEADER,
                             *PSMB2_SET_INFO_REQUEST_HEADER;

typedef struct __SMB2_SET_INFO_RESPONSE_HEADER
{
    USHORT usLength;

} __attribute__((__packed__)) SMB2_SET_INFO_RESPONSE_HEADER,
                             *PSMB2_SET_INFO_RESPONSE_HEADER;

typedef struct __SMB2_WRITE_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usDataOffset;
    ULONG    ulDataLength;
    ULONG64  ullFileOffset;
    SMB2_FID fid;
    ULONG    ulRemaining;
    ULONG    ulChannel;
    USHORT   usWriteChannelInfoOffset;
    USHORT   usWriteChannelInfoLength;
    ULONG    ulFlags;

} __attribute__((__packed__)) SMB2_WRITE_REQUEST_HEADER,
                             *PSMB2_WRITE_REQUEST_HEADER;

typedef struct __SMB2_WRITE_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulBytesWritten;
    ULONG    ulBytesRemaining;
    USHORT   usWriteChannelInfoOffset;
    USHORT   usWriteChannelInfoLength;

} __attribute__((__packed__)) SMB2_WRITE_RESPONSE_HEADER,
                             *PSMB2_WRITE_RESPONSE_HEADER;

typedef struct __SMB2_READ_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulDataLength;
    ULONG64  ullFileOffset;
    SMB2_FID fid;
    ULONG    ulMinimumCount;
    ULONG    ulChannel;
    USHORT   usReadChannelInfoOffset;
    USHORT   usReadChannelInfoLength;

} __attribute__((__packed__)) SMB2_READ_REQUEST_HEADER,
                             *PSMB2_READ_REQUEST_HEADER;

typedef struct __SMB2_READ_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usDataOffset;
    ULONG  ulDataLength;
    ULONG  ulRemaining;
    ULONG  ulReserved;

} __attribute__((__packed__)) SMB2_READ_RESPONSE_HEADER,
                             *PSMB2_READ_RESPONSE_HEADER;

typedef struct __SMB2_IOCTL_REQUEST_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulFunctionCode;
    SMB2_FID fid;
    ULONG    ulInOffset;
    ULONG    ulInLength;
    ULONG    ulMaxInLength;
    ULONG    ulOutOffset;
    ULONG    ulOutLength;
    ULONG    ulMaxOutLength;
    ULONG    ulFlags;
    ULONG    ulReserved;

} __attribute__((__packed__)) SMB2_IOCTL_REQUEST_HEADER,
                             *PSMB2_IOCTL_REQUEST_HEADER;

typedef struct __SMB2_IOCTL_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usReserved;
    ULONG    ulFunctionCode;
    SMB2_FID fid;
    ULONG    ulInOffset;
    ULONG    ulInLength;
    ULONG    ulOutOffset;
    ULONG    ulOutLength;
    ULONG    ulFlags;
    ULONG    ulReserved;

} __attribute__((__packed__)) SMB2_IOCTL_RESPONSE_HEADER,
                             *PSMB2_IOCTL_RESPONSE_HEADER;

typedef struct __SMB2_LOCK
{
    ULONG64 ullFileOffset;
    ULONG64 ullByteRange;
    ULONG   ulFlags;
    ULONG   ulReserved;
} __attribute__((__packed__)) SMB2_LOCK, *PSMB2_LOCK;

typedef struct __SMB2_LOCK_REQUEST_HEADER
{
    USHORT    usLength;
    USHORT    usLockCount;
    ULONG     ulReserved;
    SMB2_FID  fid;
    SMB2_LOCK locks[1];
} __attribute__((__packed__)) SMB2_LOCK_REQUEST_HEADER,
                             *PSMB2_LOCK_REQUEST_HEADER;

typedef struct __SMB2_LOCK_RESPONSE_HEADER
{
    USHORT    usLength;
    USHORT    usReserved;
} __attribute__((__packed__)) SMB2_LOCK_RESPONSE_HEADER,
                             *PSMB2_LOCK_RESPONSE_HEADER;;

typedef struct _SRV_SMB2_LOCK_REQUEST* PSRV_SMB2_LOCK_REQUEST;

typedef struct _SRV_SMB2_LOCK_CONTEXT
{
    SMB2_LOCK               lockInfo;

    ULONG                   ulKey;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    IO_STATUS_BLOCK         ioStatusBlock;

    PSRV_SMB2_LOCK_REQUEST  pLockRequest;
    PSRV_EXEC_CONTEXT       pExecContext;

} SRV_SMB2_LOCK_CONTEXT, *PSRV_SMB2_LOCK_CONTEXT;

typedef struct _SRV_SMB2_LOCK_REQUEST
{
    LONG                   refCount;

    pthread_mutex_t        mutex;
    pthread_mutex_t*       pMutex;

    ULONG                  ulNumContexts;

    PSRV_SMB2_LOCK_CONTEXT pLockContexts; /* unlocks and locks */

    LONG                   lPendingContexts;

    IO_STATUS_BLOCK        ioStatusBlock;

    BOOLEAN                bComplete;

} SRV_SMB2_LOCK_REQUEST;

typedef struct __SMB2_FIND_REQUEST_HEADER
{
    USHORT   usLength;
    UCHAR    ucInfoClass;
    UCHAR    ucSearchFlags;
    ULONG    ulFileIndex;
    SMB2_FID fid;
    USHORT   usFilenameOffset;
    USHORT   usFilenameLength;
    ULONG    ulOutBufferLength;

    /* File name/Search pattern follows */

} __attribute__((__packed__)) SMB2_FIND_REQUEST_HEADER,
                             *PSMB2_FIND_REQUEST_HEADER;

typedef struct __SMB2_FIND_RESPONSE_HEADER
{
    USHORT   usLength;
    USHORT   usOutBufferOffset;
    ULONG    ulOutBufferLength;

    /* File name/Search results follow */

} __attribute__((__packed__)) SMB2_FIND_RESPONSE_HEADER,
                             *PSMB2_FIND_RESPONSE_HEADER;

typedef struct __SMB2_OPLOCK_BREAK_HEADER
{
    USHORT   usLength;
    UCHAR    ucOplockLevel;
    UCHAR    ucReserved;
    ULONG    ulReserved;
    SMB2_FID fid;
} __attribute__((__packed__)) SMB2_OPLOCK_BREAK_HEADER,
                             *PSMB2_OPLOCK_BREAK_HEADER;

typedef struct __SMB2_ERROR_RESPONSE_HEADER
{
    USHORT usLength;
    USHORT usReserved;
    ULONG  ulByteCount;
    // Error message follows
} __attribute__((__packed__)) SMB2_ERROR_RESPONSE_HEADER,
                             *PSMB2_ERROR_RESPONSE_HEADER;

typedef struct _SMB2_FILE_ID_BOTH_DIR_INFORMATION
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    UCHAR           ucShortNameLength;
    UCHAR           ucReserved1;
    WCHAR           wszShortName[12];
    USHORT          usReserved2;
    LONG64          llFileId;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_ID_BOTH_DIR_INFO_HEADER,
                             *PSMB2_FILE_ID_BOTH_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_ID_FULL_DIR_INFORMATION
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    ULONG           ulReserved;
    LONG64          llFileId;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_ID_FULL_DIR_INFO_HEADER,
                             *PSMB2_FILE_ID_FULL_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_BOTH_DIR_INFORMATION
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    USHORT          usShortNameLength;
    WCHAR           wszShortName[12];
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_BOTH_DIR_INFO_HEADER,
                             *PSMB2_FILE_BOTH_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_FULL_DIR_INFORMATION
{
    ULONG           ulNextEntryOffset;
    ULONG           ulFileIndex;
    LONG64          ullCreationTime;
    LONG64          ullLastAccessTime;
    LONG64          ullLastWriteTime;
    LONG64          ullChangeTime;
    LONG64          ullEndOfFile;
    LONG64          ullAllocationSize;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulFileNameLength;
    ULONG           ulEaSize;
    // WCHAR           wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_FULL_DIR_INFO_HEADER,
                             *PSMB2_FILE_FULL_DIR_INFO_HEADER;

typedef struct _SMB2_FILE_ALL_INFORMATION_HEADER
{
    LONG64          llCreationTime;       /* FileBasicInformation     */
    LONG64          llLastAccessTime;
    LONG64          llLastWriteTime;
    LONG64          llChangeTime;
    FILE_ATTRIBUTES ulFileAttributes;
    ULONG           ulReserved;
    LONG64          ullAllocationSize;    /* FileStandardInformation  */
    LONG64          ullEndOfFile;
    ULONG           ulNumberOfLinks;
    UCHAR           ucDeletePending;
    UCHAR           ucIsDirectory;
    USHORT          usReserved;
    ULONG64         ullIndexNumber;       /* FileInternalInformation  */
    ULONG           ulEaSize;             /* FileEAInformation        */
    ULONG           ulAccessMask;         /* FileAccessInformation    */
    ULONG64         ullCurrentByteOffset; /* FilePositionInformation  */
    ULONG           ulMode;               /* FileModeInformation      */
    ULONG           ulAlignment;          /* FileAlignmentInformation */
    ULONG           ulFilenameLength;     /* FileNameInformation      */
    // WCHAR           wszFilename[1];

} __attribute__((__packed__)) SMB2_FILE_ALL_INFORMATION_HEADER,
                             *PSMB2_FILE_ALL_INFORMATION_HEADER;

typedef struct _SMB2_FILE_RENAME_INFORMATION
{
    UCHAR     ucReplaceIfExists;
    UCHAR     ucReserved[7];
    ULONG64   ullRootDir;
    ULONG     ulFileNameLength;
    WCHAR     wszFileName[1];

} __attribute__((__packed__)) SMB2_FILE_RENAME_INFO_HEADER,
                             *PSMB2_FILE_RENAME_INFO_HEADER;

typedef struct _SMB2_FILE_STREAM_INFORMATION_HEADER
{
    ULONG   ulNextEntryOffset;
    ULONG   ulStreamNameLength;
    ULONG64 ullStreamSize;
    LONG64  llStreamAllocationSize;
} __attribute__((__packed__)) SMB2_FILE_STREAM_INFORMATION_HEADER,
                             *PSMB2_FILE_STREAM_INFORMATION_HEADER;

typedef struct _SMB2_FILE_FULL_EA_INFORMATION_HEADER
{
    ULONG  ulNextEntryOffset;
    UCHAR  ucFlags;
    UCHAR  ucEaNameLength;
    USHORT usEaValueLength;
    // CHAR   szEaName[1];
    // PBYTE  pEaValue;
} __attribute__((__packed__)) SMB2_FILE_FULL_EA_INFORMATION_HEADER,
                             *PSMB2_FILE_FULL_EA_INFORMATION_HEADER;

typedef struct _SMB2_FILE_COMPRESSION_INFORMATION_HEADER
{
    LONG64 llCompressedFileSize;
    USHORT usCompressionFormat;
    UCHAR  ucCompressionUnitShift;
    UCHAR  ucChunkShift;
    UCHAR  ucClusterShift;
    UCHAR  ucReserved[3];
} __attribute__((__packed__)) SMB2_FILE_COMPRESSION_INFORMATION_HEADER,
                             *PSMB2_FILE_COMPRESSION_INFORMATION_HEADER;

typedef VOID (*PFN_SRV_MESSAGE_STATE_RELEASE_SMB_V2)(HANDLE hState);

typedef struct _SRV_OPLOCK_INFO
{
    UCHAR oplockRequest;
    UCHAR oplockLevel;
} SRV_OPLOCK_INFO, *PSRV_OPLOCK_INFO;

typedef struct _SRV_OPLOCK_STATE_SMB_V2
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    PLWIO_SRV_CONNECTION    pConnection;

    ULONG64                 ullUid;
    ULONG64                 ulTid;

    ULONG64                 ullFid;
    BOOLEAN                 bBreakRequestSent;

    PSRV_TIMER_REQUEST      pTimerRequest;

    IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER   oplockBuffer_in;
    IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER  oplockBuffer_out;
    IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER oplockBuffer_ack;

} SRV_OPLOCK_STATE_SMB_V2, *PSRV_OPLOCK_STATE_SMB_V2;

typedef enum
{
    SRV_CREATE_STAGE_SMB_V2_INITIAL = 0,
    SRV_CREATE_STAGE_SMB_V2_CREATE_FILE_COMPLETED,
    SRV_CREATE_STAGE_SMB_V2_ATTEMPT_QUERY_INFO,
    SRV_CREATE_STAGE_SMB_V2_QUERY_CREATE_CONTEXTS,
    SRV_CREATE_STAGE_SMB_V2_QUERY_INFO_COMPLETED,
    SRV_CREATE_STAGE_SMB_V2_REQUEST_OPLOCK,
    SRV_CREATE_STAGE_SMB_V2_DONE
} SRV_CREATE_STAGE_SMB_V2;

typedef struct _SRV_CREATE_STATE_SMB_V2
{
    LONG                         refCount;

    pthread_mutex_t              mutex;
    pthread_mutex_t*             pMutex;

    SRV_CREATE_STAGE_SMB_V2      stage;

    PSMB2_CREATE_REQUEST_HEADER  pRequestHeader; // Do not free

    PWSTR                        pwszFilename;

    IO_STATUS_BLOCK              ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK       acb;
    PIO_ASYNC_CONTROL_BLOCK      pAcb;

    PVOID                        pSecurityDescriptor;
    PVOID                        pSecurityQOS;
    PIO_FILE_NAME                pFilename;
    PIO_ECP_LIST                 pEcpList;
    IO_FILE_HANDLE               hFile;

    PSRV_CREATE_CONTEXT          pCreateContexts;
    ULONG                        iContext;
    ULONG                        ulNumContexts;

    FILE_BASIC_INFORMATION       fileBasicInfo;
    PFILE_BASIC_INFORMATION      pFileBasicInfo;

    FILE_STANDARD_INFORMATION    fileStdInfo;
    PFILE_STANDARD_INFORMATION   pFileStdInfo;

    FILE_PIPE_INFORMATION        filePipeInfo;
    PFILE_PIPE_INFORMATION       pFilePipeInfo;

    FILE_PIPE_LOCAL_INFORMATION  filePipeLocalInfo;
    PFILE_PIPE_LOCAL_INFORMATION pFilePipeLocalInfo;

    ACCESS_MASK                  ulMaximalAccessMask;

    FILE_CREATE_RESULT           ulCreateAction;

    UCHAR                        ucOplockLevel;

    PLWIO_SRV_TREE_2             pTree;
    PLWIO_SRV_FILE_2             pFile;
    BOOLEAN                      bRemoveFileFromTree;

} SRV_CREATE_STATE_SMB_V2, *PSRV_CREATE_STATE_SMB_V2;

typedef enum
{
    SRV_FLUSH_STAGE_SMB_V2_INITIAL = 0,
    SRV_FLUSH_STAGE_SMB_V2_FLUSH_COMPLETED,
    SRV_FLUSH_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_FLUSH_STAGE_SMB_V2_DONE
} SRV_FLUSH_STAGE_SMB_V2;

typedef struct _SRV_FLUSH_STATE_SMB_V2
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_FLUSH_STAGE_SMB_V2    stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PSMB2_FID                 pFid; // Do not free

    PLWIO_SRV_FILE_2          pFile;

} SRV_FLUSH_STATE_SMB_V2, *PSRV_FLUSH_STATE_SMB_V2;

typedef enum
{
    SRV_READ_STAGE_SMB_V2_INITIAL = 0,
    SRV_READ_STAGE_SMB_V2_ATTEMPT_READ,
    SRV_READ_STAGE_SMB_V2_ATTEMPT_READ_COMPLETED,
    SRV_READ_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_READ_STAGE_SMB_V2_DONE
} SRV_READ_STAGE_SMB_V2;

typedef struct _SRV_READ_STATE_SMB_V2
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_READ_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PLWIO_SRV_FILE_2          pFile;

    LONG64                    llByteOffset;
    ULONG                     ulBytesRead;
    ULONG                     ulRemaining;

    PSMB2_READ_REQUEST_HEADER pRequestHeader; // Do not free

    PBYTE                     pData;
    ULONG                     ulKey;

} SRV_READ_STATE_SMB_V2, *PSRV_READ_STATE_SMB_V2;

typedef enum
{
    SRV_WRITE_STAGE_SMB_V2_INITIAL = 0,
    SRV_WRITE_STAGE_SMB_V2_ATTEMPT_WRITE,
    SRV_WRITE_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_WRITE_STAGE_SMB_V2_DONE
} SRV_WRITE_STAGE_SMB_V2;

typedef struct _SRV_WRITE_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_WRITE_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PLWIO_SRV_FILE_2           pFile;

    PSMB2_WRITE_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                      pData;          // Do not free

    ULONG                      ulBytesWritten;
    LONG64                     llDataOffset;
    ULONG                      ulKey;

} SRV_WRITE_STATE_SMB_V2, *PSRV_WRITE_STATE_SMB_V2;

typedef enum
{
    SRV_GET_INFO_STAGE_SMB_V2_INITIAL = 0,
    SRV_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO,
    SRV_GET_INFO_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_GET_INFO_STAGE_SMB_V2_DONE
} SRV_GET_INFO_STAGE_SMB_V2;

typedef struct _SRV_GET_INFO_STATE_SMB_V2
{
    LONG                          refCount;

    pthread_mutex_t               mutex;
    pthread_mutex_t*              pMutex;

    SRV_GET_INFO_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK               ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK        acb;
    PIO_ASYNC_CONTROL_BLOCK       pAcb;

    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader; // Do not free

    PLWIO_SRV_CONNECTION          pConnection;
    PLWIO_SRV_FILE_2              pFile;

    PBYTE                         pData2;
    ULONG                         ulDataLength;
    ULONG                         ulActualDataLength;

    PBYTE                         pResponseBuffer;
    size_t                        sAllocatedSize;
    ULONG                         ulResponseBufferLen;

} SRV_GET_INFO_STATE_SMB_V2, *PSRV_GET_INFO_STATE_SMB_V2;

typedef enum
{
    SRV_SET_INFO_STAGE_SMB_V2_INITIAL = 0,
    SRV_SET_INFO_STAGE_SMB_V2_ATTEMPT_IO,
    SRV_SET_INFO_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_SET_INFO_STAGE_SMB_V2_DONE
} SRV_SET_INFO_STAGE_SMB_V2;

typedef struct _SRV_SET_INFO_STATE_SMB_V2
{
    LONG                          refCount;

    pthread_mutex_t               mutex;
    pthread_mutex_t*              pMutex;

    SRV_SET_INFO_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK               ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK        acb;
    PIO_ASYNC_CONTROL_BLOCK       pAcb;

    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                         pData;          // Do not free

    PBYTE                         pSecurityDescriptor;
    PBYTE                         pSecurityQOS;
    IO_FILE_HANDLE                hDir;
    IO_FILE_NAME                  dirPath;

    PBYTE                         pData2;
    ULONG                         ulData2Length;

    PLWIO_SRV_FILE_2              pFile;
    PLWIO_SRV_FILE_2              pRootDir;

} SRV_SET_INFO_STATE_SMB_V2, *PSRV_SET_INFO_STATE_SMB_V2;

typedef enum
{
    SRV_IOCTL_STAGE_SMB_V2_INITIAL = 0,
    SRV_IOCTL_STAGE_SMB_V2_ATTEMPT_IO,
    SRV_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_IOCTL_STAGE_SMB_V2_DONE
} SRV_IOCTL_STAGE_SMB_V2;

typedef struct _SRV_IOCTL_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_IOCTL_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                      pData;          // Do not free

    PLWIO_SRV_CONNECTION       pConnection;
    PLWIO_SRV_FILE_2           pFile;

    PBYTE                      pResponseBuffer;
    size_t                     sResponseBufferLen;
    ULONG                      ulResponseBufferLen;

} SRV_IOCTL_STATE_SMB_V2, *PSRV_IOCTL_STATE_SMB_V2;

typedef enum
{
    SRV_CLOSE_STAGE_SMB_V2_INITIAL = 0,
    SRV_CLOSE_STAGE_SMB_V2_ATTEMPT_IO,
    SRV_CLOSE_STAGE_SMB_V2_BUILD_RESPONSE,
    SRV_CLOSE_STAGE_SMB_V2_DONE
} SRV_CLOSE_STAGE_SMB_V2;

typedef struct _SRV_CLOSE_STATE_SMB_V2
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_CLOSE_STAGE_SMB_V2     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB2_FID                  pFid; // Do not free

    FILE_BASIC_INFORMATION     fileBasicInfo;
    PFILE_BASIC_INFORMATION    pFileBasicInfo;

    FILE_STANDARD_INFORMATION  fileStdInfo;
    PFILE_STANDARD_INFORMATION pFileStdInfo;

    PLWIO_SRV_TREE_2           pTree;
    PLWIO_SRV_FILE_2           pFile;

} SRV_CLOSE_STATE_SMB_V2, *PSRV_CLOSE_STATE_SMB_V2;

typedef struct __SRV_MESSAGE_SMB_V2
{
    PBYTE        pBuffer;
    PSMB2_HEADER pHeader;
    ULONG        ulHeaderSize;
    ULONG        ulMessageSize;

    ULONG        ulBytesAvailable;

} SRV_MESSAGE_SMB_V2, *PSRV_MESSAGE_SMB_V2;

typedef struct _SRV_EXEC_CONTEXT_SMB_V2
{
    PSRV_MESSAGE_SMB_V2                  pRequests;
    ULONG                                ulNumRequests;
    ULONG                                iMsg;

    PLWIO_SRV_SESSION_2                  pSession;
    PLWIO_SRV_TREE_2                     pTree;
    PLWIO_SRV_FILE_2                     pFile;

    HANDLE                               hState;
    PFN_SRV_MESSAGE_STATE_RELEASE_SMB_V2 pfnStateRelease;

    ULONG                                ulNumResponses;
    PSRV_MESSAGE_SMB_V2                  pResponses;

    PBYTE                                pErrorMessage;
    ULONG                                ulErrorMessageLength;

} SRV_EXEC_CONTEXT_SMB_V2;

typedef struct _SRV_RUNTIME_GLOBALS_SMB_V2
{
    pthread_mutex_t      mutex;

    PSMB_PROD_CONS_QUEUE pWorkQueue;

    ULONG                ulOplockTimeout;

} SRV_RUNTIME_GLOBALS_SMB_V2, *PSRV_RUNTIME_GLOBALS_SMB_V2;

#endif /* __STRUCTS_H__ */
