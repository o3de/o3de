/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Description : Specific to Linux declarations, inline functions etc.



#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <algorithm>
#include <signal.h>
#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#if defined(APPLE)
#include <dirent.h>
#endif //defined(APPLE)
#if !defined(SKIP_INET_INCLUDES)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // defined(SKIP_INET_INCLUDES)
#include <vector>
#include <string>

#include <AzCore/base.h>

#define PHYSICS_EXPORTS

#define _PACK __attribute__ ((packed))

#ifndef __cplusplus
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define TCHAR wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif
typedef AZ::u32 COLORREF;
#define RGB(r,g,b) ((COLORREF)(((AZ::u8)(r)|((AZ::u16)((AZ::u8)(g))<<8))|(((AZ::u32)(AZ::u8)(b))<<16)))

#define GetRValue(rgb)  ((AZ::u8)((AZ::u64)(rgb) & 0xff))
#define GetGValue(rgb)  ((AZ::u8)((AZ::u64)(((AZ::u16)(rgb)) >> 8) & 0xff))
#define GetBValue(rgb)  ((AZ::u8)((AZ::u64)(rgb) & 0xff))

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                \
    ((AZ::u32)(AZ::u8)(ch0) | ((AZ::u32)(AZ::u8)(ch1) << 8) | \
     ((AZ::u32)(AZ::u8)(ch2) << 16) | ((AZ::u32)(AZ::u8)(ch3) << 24))
#define FILE_ATTRIBUTE_NORMAL               0x00000080

typedef int                         BOOL;
typedef int32_t                 LONG;
typedef uint32_t            ULONG;
typedef int                         HRESULT;

#define BST_UNCHECKED   0x0000

#ifndef MAXUINT
#define MAXUINT ((uint) ~((uint)0))
#endif

#ifndef MAXINT
#define MAXINT ((int)(MAXUINT >> 1))
#endif

#if !defined(__clang__)
typedef int32 __int32;
typedef uint32 __uint32;
typedef int64 __int64;
typedef uint64 __uint64;
#endif

#define TRUE 1
#define FALSE 0

#ifndef MAX_PATH
    #define MAX_PATH 256
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#define _PTRDIFF_T_DEFINED 1

//#define __TIMESTAMP__ __DATE__" "__TIME__

// function renaming
#define _finite __finite
#define _snprintf snprintf
#define _isnan isnan
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define wcsicmp wcscasecmp
#define wcsnicmp wcsncasecmp

typedef union _LARGE_INTEGER
{
    struct
    {
        AZ::u32 LowPart;
        LONG HighPart;
    };
    struct
    {
        AZ::u32 LowPart;
        LONG HighPart;
    } u;
    long long QuadPart;
} LARGE_INTEGER;

// stdlib.h stuff
#define _MAX_DRIVE  3   // max. length of drive component
#define _MAX_DIR    256 // max. length of path component
#define _MAX_FNAME  256 // max. length of file name component
#define _MAX_EXT    256 // max. length of extension component

// fcntl.h
#define _O_RDONLY       0x0000  /* open for reading only */
#define _O_WRONLY       0x0001  /* open for writing only */
#define _O_RDWR         0x0002  /* open for reading and writing */
#define _O_APPEND       0x0008  /* writes done at eof */
#define _O_CREAT        0x0100  /* create and open file */
#define _O_TRUNC        0x0200  /* open and truncate */
#define _O_EXCL         0x0400  /* open only if file doesn't already exist */
#define _O_TEXT         0x4000  /* file mode is text (translated) */
#define _O_BINARY       0x8000  /* file mode is binary (untranslated) */
#define _O_RAW  _O_BINARY
#define _O_NOINHERIT    0x0080  /* child process doesn't inherit file */
#define _O_TEMPORARY    0x0040  /* temporary file bit */
#define _O_SHORT_LIVED  0x1000  /* temporary storage file, try not to flush */
#define _O_SEQUENTIAL   0x0020  /* file access is primarily sequential */
#define _O_RANDOM       0x0010  /* file access is primarily random */

enum
{
    IDOK        = 1,
    IDCANCEL    = 2,
    IDABORT     = 3,
    IDRETRY     = 4,
    IDIGNORE    = 5,
    IDYES       = 6,
    IDNO        = 7,
    IDTRYAGAIN  = 10,
    IDCONTINUE  = 11
};

#define MB_OK                0x00000000L
#define MB_OKCANCEL          0x00000001L
#define MB_ABORTRETRYIGNORE  0x00000002L
#define MB_YESNOCANCEL       0x00000003L
#define MB_YESNO             0x00000004L
#define MB_RETRYCANCEL       0x00000005L
#define MB_CANCELTRYCONTINUE 0x00000006L

#define MB_ICONQUESTION     0x00000020L
#define MB_ICONEXCLAMATION  0x00000030L
    
#define MB_ICONERROR        0x00000010L
#define MB_ICONWARNING      0x00000030L
#define MB_ICONINFORMATION  0x00000040L

#define MB_SETFOREGROUND    0x00010000L

#define MB_APPLMODAL    0x00000000L

#define MK_LBUTTON  0x0001
#define MK_RBUTTON  0x0002
#define MK_SHIFT    0x0004
#define MK_CONTROL  0x0008
#define MK_MBUTTON  0x0010

#define SM_MOUSEPRESENT 0x00000000L

#define SM_CMOUSEBUTTONS    43

#define VK_TAB      0x09
#define VK_SHIFT    0x10
#define VK_MENU     0x12
#define VK_ESCAPE   0x1B
#define VK_SPACE    0x20
#define VK_DELETE   0x2E

#define VK_OEM_COMMA    0xBC   // ',' any country
#define VK_OEM_PERIOD   0xBE   // '.' any country
#define VK_OEM_3        0xC0   // '`~' for US
#define VK_OEM_4        0xDB  //  '[{' for US
#define VK_OEM_6        0xDD  //  ']}' for US

#define WAIT_TIMEOUT 258L    // dderror

#define WM_MOVE 0x0003
#define WM_USER 0x0400

#define WHEEL_DELTA 120

#define WS_CHILD    0x40000000L
#define WS_VISIBLE  0x10000000L

#define CB_ERR  (-1)

#ifdef __cplusplus
extern bool QueryPerformanceCounter(LARGE_INTEGER*);
extern bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);

template<typename S, typename T>
inline S __min(const S& rS, const T& rT)
{
    return std::min(rS, rT);
}

template<typename S, typename T>
inline S __max(const S& rS, const T& rT)
{
    return std::max(rS, rT);
}

inline int64 CryGetTicks()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

#endif //__cplusplus

template <typename T, size_t N>
char (*RtlpNumberOf( T (&)[N] ))[N];

#define RTL_NUMBER_OF_V2(A) (sizeof(*RtlpNumberOf(A)))

#define ARRAYSIZE(A) RTL_NUMBER_OF_V2(A)
