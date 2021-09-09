/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Specific to Linux declarations, inline functions etc.


#ifndef CRYINCLUDE_CRYCOMMON_LINUX32SPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_LINUX32SPECIFIC_H
#pragma once


#define _CPU_X86
//#define _CPU_SSE

//////////////////////////////////////////////////////////////////////////
// Standard includes.
//////////////////////////////////////////////////////////////////////////
#include <malloc.h>
//#include <winbase.h>
#include <stdint.h>
#include <sys/dir.h>
#include <sys/io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

typedef signed long long  INT64;

typedef double real;

typedef unsigned long       DWORD;
typedef unsigned long*      LPDWORD;
typedef DWORD               DWORD_PTR;
typedef int INT_PTR, * PINT_PTR;
typedef unsigned int UINT_PTR, * PUINT_PTR;
typedef char* LPSTR, * PSTR;

typedef long LONG_PTR, * PLONG_PTR, * PLONG;
typedef unsigned long ULONG_PTR, * PULONG_PTR;

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef void*               HWND;
typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;
#define PLARGE_INTEGER LARGE_INTEGER *
typedef const char* LPCSTR, * PCSTR;
typedef long long           LONGLONG;
typedef ULONG_PTR           SIZE_T;
typedef unsigned char       byte;

#define __int64     long long

#define ILINE __forceinline

#define _A_RDONLY (0x01)
#define _A_SUBDIR (0x10)

//////////////////////////////////////////////////////////////////////////
// Win32 FileAttributes.
//////////////////////////////////////////////////////////////////////////
#define FILE_ATTRIBUTE_READONLY             0x00000001
#define FILE_ATTRIBUTE_HIDDEN               0x00000002
#define FILE_ATTRIBUTE_SYSTEM               0x00000004
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
#define FILE_ATTRIBUTE_DEVICE               0x00000040
#define FILE_ATTRIBUTE_NORMAL               0x00000080
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
#define FILE_ATTRIBUTE_OFFLINE              0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000

#define INVALID_FILE_ATTRIBUTES (-1)

#include "LinuxSpecific.h"

#define TARGET_DEFAULT_ALIGN (0x4U)

#endif // CRYINCLUDE_CRYCOMMON_LINUX32SPECIFIC_H
