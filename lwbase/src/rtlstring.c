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

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>

// XXX - HACK!!!
#define RTLP_STRING_LOG_COUNT 1000

typedef struct _RTLP_STRING_LOG_DATA {
    ULONG NextIndex;
    PSTR ppszString[RTLP_STRING_LOG_COUNT];
} RTLP_STRING_LOG_DATA, *PRTLP_STRING_LOG_DATA;

RTLP_STRING_LOG_DATA gpLwRtlStringLogData = { 0 };
#define RTLP_STRING_NULL_TEXT "<null>"

static
BOOLEAN
LwRtlpUnicodeStringIsNullTerminated(
    IN PUNICODE_STRING pString
    )
{
    BOOLEAN bIsNullTermianted = FALSE;

    if (pString &&
        pString->Buffer &&
        (pString->MaximumLength > pString->Length) &&
        !pString->Buffer[pString->Length])
    {
        bIsNullTermianted = TRUE;
    }

    return bIsNullTermianted;
}

static
BOOLEAN
LwRtlpAnsiStringIsNullTerminated(
    IN PANSI_STRING pString
    )
{
    BOOLEAN bIsNullTermianted = FALSE;

    if (pString &&
        pString->Buffer &&
        (pString->MaximumLength > pString->Length) &&
        !pString->Buffer[pString->Length])
    {
        bIsNullTermianted = TRUE;
    }

    return bIsNullTermianted;
}

static
PCSTR
LwRtlpStringLogRotate(
    IN OPTIONAL PSTR pszString
    )
{
    // TODO-Locking or interlocked increment.
    if (gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex])
    {
       free(gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex]);
    }
    gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex] = pszString;
    gpLwRtlStringLogData.NextIndex++;
    if (gpLwRtlStringLogData.NextIndex >= RTLP_STRING_LOG_COUNT)
    {
        gpLwRtlStringLogData.NextIndex = 0;
    }
    return pszString ? pszString : RTLP_STRING_NULL_TEXT;
}

PCSTR
LwRtlUnicodeStringToLog(
    IN PUNICODE_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (LwRtlpUnicodeStringIsNullTerminated(pString))
    {
        pszOutput = RtlWC16StringToLog(pString->Buffer);
    }
    else
    {
        UNICODE_STRING tempString = { 0 };
        RtlUnicodeStringDuplicate(&tempString, pString);
        pszOutput = RtlWC16StringToLog(tempString.Buffer);
        RtlUnicodeStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
LwRtlAnsiStringToLog(
    IN PANSI_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (LwRtlpAnsiStringIsNullTerminated(pString))
    {
        pszOutput = pString->Buffer;
    }
    else
    {
        ANSI_STRING tempString = { 0 };
        RtlAnsiStringDuplicate(&tempString, pString);
        pszOutput = LwRtlpStringLogRotate(tempString.Buffer ? strdup(tempString.Buffer) : NULL);
        RtlAnsiStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
LwRtlWC16StringToLog(
    IN PCWSTR pszString
    )
{
    return LwRtlpStringLogRotate(pszString ? awc16stombs(pszString) : NULL);
}

