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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Statistics Logging Module
 *
 *        Module Entry Points
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvInitializeStatistics(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_STATISTICS_CONFIG config = {0};

    ntStatus = SrvStatsInitConfigContents(&config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvStatsReadConfig(&config);
    if (ntStatus == STATUS_DEVICE_CONFIGURATION_ERROR)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvStatsTransferConfigContents(&config, &gSrvStatGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SrvStatsFreeConfigContents(&config);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShutdownStatistics(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gSrvStatGlobals.mutex);

    SrvStatsFreeConfigContents(&gSrvStatGlobals.config);

    LWIO_UNLOCK_MUTEX(bInLock, &gSrvStatGlobals.mutex);

    return ntStatus;
}
