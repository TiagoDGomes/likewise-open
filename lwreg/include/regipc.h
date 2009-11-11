/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 *        regipc.h
 *
 * Abstract:
 *
 *        Registry Interprocess Communication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#ifndef __REGIPC_H__
#define __REGIPC_H__

#define REG_CLIENT_PATH_FORMAT "/var/tmp/.regclient_%05ld"
#define REG_SERVER_FILENAME    ".regsd"

typedef enum __REG_IPC_TAG
{
    REG_R_ERROR,
    REG_Q_ENUM_ROOT_KEYSW,
    REG_R_ENUM_ROOT_KEYSW,
    REG_Q_CREATE_KEY_EX,
    REG_R_CREATE_KEY_EX,
    REG_Q_CLOSE_KEY,
    REG_R_CLOSE_KEY,
    REG_Q_DELETE_KEY,
    REG_R_DELETE_KEY,
    REG_Q_DELETE_KEY_VALUE,
    REG_R_DELETE_KEY_VALUE,
    REG_Q_DELETE_TREE,
    REG_R_DELETE_TREE,
    REG_Q_DELETE_VALUE,
    REG_R_DELETE_VALUE,
    REG_Q_ENUM_KEYW_EX,
    REG_R_ENUM_KEYW_EX,
    REG_Q_QUERY_MULTIPLE_VALUES,
    REG_R_QUERY_MULTIPLE_VALUES,
    REG_Q_QUERY_INFO_KEYA,
    REG_R_QUERY_INFO_KEYA,
    REG_Q_QUERY_INFO_KEYW,
    REG_R_QUERY_INFO_KEYW,
    REG_Q_SET_KEY_VALUE,
    REG_R_SET_KEY_VALUE,
    REG_Q_ENUM_VALUEA,
    REG_R_ENUM_VALUEA,
    REG_Q_ENUM_VALUEW,
    REG_R_ENUM_VALUEW,
    REG_Q_GET_VALUEA,
    REG_R_GET_VALUEA,
    REG_Q_GET_VALUEW,
    REG_R_GET_VALUEW,
    REG_Q_OPEN_KEYW_EX,
    REG_R_OPEN_KEYW_EX,
    REG_Q_QUERY_VALUEA_EX,
    REG_R_QUERY_VALUEA_EX,
    REG_Q_QUERY_VALUEW_EX,
    REG_R_QUERY_VALUEW_EX,
    REG_Q_SET_VALUEW_EX,
    REG_R_SET_VALUEW_EX,
} REG_IPC_TAG;

/* Opaque type -- actual definition in state_p.h - LSA_SRV_ENUM_STATE */

/******************************************************************************/
typedef struct __REG_IPC_ERROR
{
    DWORD dwError;
} REG_IPC_ERROR, *PREG_IPC_ERROR;

/******************************************************************************/

// IN HKEY hKey,
// IN PCWSTR pSubKey,
// IN DWORD Reserved,
// IN OPTIONAL PWSTR pClass,
// IN DWORD dwOptions,
// IN REGSAM samDesired,
// IN OPTIONAL PSECURITY_ATTRIBUTES pSecurityAttributes,
// OUT PHKEY phkResult,
// OUT OPTIONAL PDWORD pdwDisposition

typedef struct __REG_IPC_CREATE_KEY_EX_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
    PWSTR pClass;
    DWORD dwOptions;
    REGSAM samDesired;
    PSECURITY_ATTRIBUTES pSecurityAttributes;
} REG_IPC_CREATE_KEY_EX_REQ, *PREG_IPC_CREATE_KEY_EX_REQ;

typedef struct __REG_IPC_CREATE_KEY_EX_RESPONSE
{
    HKEY hkResult;
    DWORD dwDisposition;
} REG_IPC_CREATE_KEY_EX_RESPONSE, *PREG_IPC_CREATE_KEY_EX_RESPONSE;

/******************************************************************************/

// IN HKEY hKey

typedef struct __REG_IPC_CLOSE_KEY_REQ
{
    HKEY hKey;
} REG_IPC_CLOSE_KEY_REQ, *PREG_IPC_CLOSE_KEY_REQ;

// NO RESPONSE

/******************************************************************************/

typedef struct __REG_IPC_ENUM_ROOTKEYS_RESPONSE
{
    PWSTR* ppwszRootKeyNames;
    DWORD dwNumRootKeys;
} REG_IPC_ENUM_ROOTKEYS_RESPONSE, *PREG_IPC_ENUM_ROOTKEYS_RESPONSE;

// IN HKEY hKey,
// IN PCWSTR pSubKey

typedef struct __REG_IPC_DELETE_KEY_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
} REG_IPC_DELETE_KEY_REQ, *PREG_IPC_DELETE_KEY_REQ;

// NO RESPONSE

/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR pSubKey,
// IN OPTIONAL PCWSTR pValueName

typedef struct __REG_IPC_DELETE_KEY_VALUE_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
    PCWSTR pValueName;
} REG_IPC_DELETE_KEY_VALUE_REQ, *PREG_IPC_DELETE_KEY_VALUE_REQ;

// NO RESPONSE

/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR pSubKey

typedef struct __REG_IPC_DELETE_TREE_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
} REG_IPC_DELETE_TREE_REQ, *PREG_IPC_DELETE_TREE_REQ;

// NO RESPONSE

/******************************************************************************/

// IN HKEY hKey,
// IN PCWSTR pSubKey,
// IN PCWSTR pValueName

typedef struct __REG_IPC_DELETE_VALUE_REQ
{
    HKEY hKey;
    PCWSTR pValueName;
} REG_IPC_DELETE_VALUE_REQ, *PREG_IPC_DELETE_VALUE_REQ;

// NO RESPONSE

/******************************************************************************/

// IN HKEY hKey,
// IN DWORD dwIndex,
// OUT PWSTR pName,
// IN OUT PDWORD pcName,
// IN PDWORD pReserved,
// IN OUT PWSTR pClass,
// IN OUT OPTIONAL PDWORD pcClass,
// OUT OPTIONAL PFILETIME pftLastWriteTime

typedef struct __REG_IPC_ENUM_KEY_EX_REQ
{
    HKEY hKey;
    DWORD dwIndex;
    PWSTR pName;
    DWORD cName;
    PWSTR pClass;
    PDWORD pcClass;
} REG_IPC_ENUM_KEY_EX_REQ, *PREG_IPC_ENUM_KEY_EX_REQ;

typedef struct __REG_IPC_ENUM_KEY_EX_RESPONSE
{
    PWSTR pName;
    DWORD cName;
    //PWSTR pClass;
    //PDWORD pcClass;
    //PFILETIME pftLastWriteTime;
} REG_IPC_ENUM_KEY_EX_RESPONSE, *PREG_IPC_ENUM_KEY_EX_RESPONSE;

/******************************************************************************/


typedef struct __REG_IPC_ENUM_VALUEA_REQ
{
    HKEY hKey;
    DWORD dwIndex;
    PSTR pszName;
    DWORD cName;
    PBYTE pValue;
    DWORD cValue;
} REG_IPC_ENUM_VALUEA_REQ, *PREG_IPC_ENUM_VALUEA_REQ;

typedef struct __REG_IPC_ENUM_VALUE_REQ
{
    HKEY hKey;
    DWORD dwIndex;
    PWSTR pName;
    DWORD cName;
    PBYTE pValue;
    DWORD cValue;
} REG_IPC_ENUM_VALUE_REQ, *PREG_IPC_ENUM_VALUE_REQ;

typedef struct __REG_IPC_ENUM_VALUEA_RESPONSE
{
    PSTR pszName;
    DWORD cName;
    PBYTE pValue;
    DWORD cValue;
    REG_DATA_TYPE type;
    //PWSTR pClass;
    //PDWORD pcClass;
    //PFILETIME pftLastWriteTime;
} REG_IPC_ENUM_VALUEA_RESPONSE, *PREG_IPC_ENUM_VALUEA_RESPONSE;

typedef struct __REG_IPC_ENUM_VALUE_RESPONSE
{
    PWSTR pName;
    DWORD cName;
    PBYTE pValue;
    DWORD cValue;
    REG_DATA_TYPE type;
    //PWSTR pClass;
    //PDWORD pcClass;
    //PFILETIME pftLastWriteTime;
} REG_IPC_ENUM_VALUE_RESPONSE, *PREG_IPC_ENUM_VALUE_RESPONSE;


/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR pSubKey,
// IN OPTIONAL PCWSTR pValue,
// IN OPTIONAL DWORD dwFlags,
// OUT OPTIONAL PDWORD pdwType,
// OUT OPTIONAL PVOID pvData,
// IN OUT OPTIONAL PDWORD pcbData

typedef struct __REG_IPC_GET_VALUEA_REQ
{
    HKEY hKey;
    PCSTR pszSubKey;
    PCSTR pszValue;
    REG_DATA_TYPE_FLAGS Flags;
    PBYTE pData;
    DWORD cbData;
} REG_IPC_GET_VALUEA_REQ, *PREG_IPC_GET_VALUEA_REQ;

typedef struct __REG_IPC_GET_VALUE_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
    PCWSTR pValue;
    REG_DATA_TYPE_FLAGS Flags;
    PBYTE pData;
    DWORD cbData;
} REG_IPC_GET_VALUE_REQ, *PREG_IPC_GET_VALUE_REQ;

typedef struct __REG_IPC_GET_VALUE_RESPONSE
{
    DWORD dwType;
    PBYTE pvData;
    DWORD cbData;
} REG_IPC_GET_VALUE_RESPONSE, *PREG_IPC_GET_VALUE_RESPONSE;

/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR pSubKey,
// RESERVED DWORD ulOptions,
// IN REGSAM samDesired,
// OUT HKEY hkResult
typedef struct __REG_IPC_OPEN_KEY_EX_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
    REGSAM samDesired;
} REG_IPC_OPEN_KEY_EX_REQ, *PREG_IPC_OPEN_KEY_EX_REQ;

typedef struct __REG_IPC_OPEN_KEY_EX_RESPONSE
{
    HKEY hkResult;
} REG_IPC_OPEN_KEY_EX_RESPONSE, *PREG_IPC_OPEN_KEY_EX_RESPONSE;

/******************************************************************************/

// IN HKEY hKey,
// OUT PWSTR pClass,
// IN OUT OPTIONAL PDWORD pcClass,
// PDWORD pReserved,
// OUT OPTIONAL PDWORD pcSubKeys,
// OUT OPTIONAL PDWORD pcMaxSubKeyLen,
// OUT OPTIONAL PDWORD pcMaxClassLen,
// OUT OPTIONAL PDWORD pcValues,
// OUT OPTIONAL PDWORD pcMaxValueNameLen,
// OUT OPTIONAL PDWORD pcMaxValueLen,
// OUT OPTIONAL PDWORD pcbSecurityDescriptor,
// OUT OPTIONAL PFILETIME pftLastWriteTime

typedef struct __REG_IPC_QUERY_INFO_KEY_REQ
{
    HKEY hKey;
    PDWORD pcClass;
} REG_IPC_QUERY_INFO_KEY_REQ, *PREG_IPC_QUERY_INFO_KEY_REQ;

typedef struct __REG_IPC_QUERY_INFO_KEY_RESPONSE
{
    PWSTR pClass;
    //PDWORD pcClass;
    DWORD cSubKeys;
    DWORD cMaxSubKeyLen;

    DWORD cValues;
    DWORD cMaxValueNameLen;
    DWORD cMaxValueLen;
  //  PDWORD pcMaxClassLen;
  //  PDWORD pcValues;
  //  PDWORD pcMaxValueNameLen;
  //  PDWORD pcMaxValueLen;
  //  PDWORD pcbSecurityDescriptor;
  //  PFILETIME pftLastWriteTime;
} REG_IPC_QUERY_INFO_KEY_RESPONSE, *PREG_IPC_QUERY_INFO_KEY_RESPONSE;

/******************************************************************************/

// IN HKEY hKey,
// OUT PVALENT val_list,
// IN DWORD num_vals,
// OUT OPTIONAL PWSTR pValueBuf,
// IN OUT OPTIONAL PDWORD dwTotsize

typedef struct __REG_IPC_QUERY_MULTIPLE_VALUES_REQ
{
    HKEY hKey;
    DWORD num_vals;
    PVALENT val_list;
    DWORD dwTotalsize;
    PWSTR pValue;
} REG_IPC_QUERY_MULTIPLE_VALUES_REQ, *PREG_IPC_QUERY_MULTIPLE_VALUES_REQ;

typedef struct __REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE
{
    DWORD num_vals;
    PVALENT val_list;
    DWORD dwTotalsize;
    PWSTR pValue;
} REG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE, *PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE;


/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR pValueName,
// RESERVED PDWORD pReserved,
// OUT OPTIONAL PDWORD pType,
// OUT OPTIONAL PBYTE pData,
// IN OUT OPTIONAL PDWORD pcbData

typedef struct __REG_IPC_QUERY_VALUE_EX_REQ
{
    HKEY hKey;
    PCWSTR pValueName;
    PDWORD pcbData;
} REG_IPC_QUERY_VALUE_EX_REQ, *PREG_IPC_QUERY_VALUE_EX_REQ;

typedef struct __REG_IPC_QUERY_VALUE_EX_RESPONSE
{
    PDWORD pType;
    PBYTE pData;
    PDWORD pcbData;
} REG_IPC_QUERY_VALUE_EX_RESPONSE, *PREG_IPC_QUERY_VALUE_EX_RESPONSE;

/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR lpSubKey,
// IN OPTIONAL PCWSTR lpValueName,
// IN DWORD dwType,
// IN OPTIONAL PCVOID lpData,
// IN DWORD cbData

typedef struct __REG_IPC_SET_KEY_VALUE_REQ
{
    HKEY hKey;
    PCWSTR pSubKey;
    PCWSTR pValueName;
    DWORD dwType;
    PCVOID pData;
    DWORD cbData;
} REG_IPC_SET_KEY_VALUE_REQ, *PREG_IPC_SET_KEY_VALUE_REQ;

// NO RESPONSE

/******************************************************************************/

// IN HKEY hKey,
// IN OPTIONAL PCWSTR pValueName,
// IN DWORD Reserved,
// IN DWORD dwType,
// IN OPTIONAL const BYTE *pData,
// IN DWORD cbData
typedef struct __REG_IPC_SET_VALUE_EX_REQ
{
    HKEY hKey;
    PCWSTR pValueName;
    DWORD dwType;
    const BYTE *pData;
    DWORD cbData;
} REG_IPC_SET_VALUE_EX_REQ, *PREG_IPC_SET_VALUE_EX_REQ;

// NO RESPONSE

/******************************************************************************/

#define MAP_LWMSG_ERROR(_e_) (RegMapLwmsgStatus(_e_))
#define MAP_REG_ERROR_IPC(_e_) ((_e_) ? LWMSG_STATUS_ERROR : LWMSG_STATUS_SUCCESS)

LWMsgProtocolSpec*
RegIPCGetProtocolSpec(
    void
    );

DWORD
RegOpenServer(
    PHANDLE phConnection
    );

VOID
RegCloseServer(
    HANDLE hConnection
    );

DWORD
RegIpcAcquireCall(
    HANDLE hServer,
    LWMsgCall** ppCall
    );

DWORD
RegWriteData(
    DWORD dwFd,
    PSTR  pszBuf,
    DWORD dwLen);

DWORD
RegReadData(
    DWORD  dwFd,
    PSTR   pszBuf,
    DWORD  dwBytesToRead,
    PDWORD pdwBytesRead);

DWORD
RegMapLwmsgStatus(
    LWMsgStatus status
    );

#endif /*__REGIPC_H__*/


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
