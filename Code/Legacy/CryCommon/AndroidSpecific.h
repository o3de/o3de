/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Specific to Android declarations, inline functions etc.


#ifndef CRYINCLUDE_CRYCOMMON_ANDROIDSPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_ANDROIDSPECIFIC_H
#pragma once

#if defined(__arm__) || defined(__aarch64__)
#define _CPU_ARM
#endif

#if defined(__aarch64__)
#define PLATFORM_64BIT
#endif

#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#define _CPU_NEON
#endif

#ifndef MOBILE
#define MOBILE
#endif

//////////////////////////////////////////////////////////////////////////
// Standard includes.
//////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <stdint.h>
#include <fcntl.h>
#include <float.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <sys/socket.h>
#include <errno.h>
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

typedef signed long long  INT64;

typedef double real;

typedef uint32              DWORD;
typedef DWORD*              LPDWORD;
#if defined(PLATFORM_64BIT)
typedef uint64              DWORD_PTR;
#else
typedef DWORD               DWORD_PTR;
#endif
typedef intptr_t INT_PTR, *PINT_PTR;
typedef uintptr_t UINT_PTR, * PUINT_PTR;
typedef char* LPSTR, * PSTR;
typedef uint64      __uint64;
typedef int64       INT64;
typedef uint64      UINT64;

typedef long LONG_PTR, * PLONG_PTR, * PLONG;
typedef unsigned long ULONG_PTR, * PULONG_PTR;

typedef unsigned char               BYTE;
typedef unsigned short          WORD;
typedef void*                               HWND;
typedef UINT_PTR                        WPARAM;
typedef LONG_PTR                        LPARAM;
typedef LONG_PTR                        LRESULT;
#define PLARGE_INTEGER LARGE_INTEGER *
typedef const char* LPCSTR, * PCSTR;
typedef long long                       LONGLONG;
typedef ULONG_PTR                       SIZE_T;
typedef unsigned char               byte;

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
// these functions do not exist int the wchar.h header
#undef wscasecomp
#undef wscasencomp
extern int wcsicmp (const wchar_t* s1, const wchar_t* s2);
extern int wcsnicmp (const wchar_t* s1, const wchar_t* s2, size_t count);

// these are not defined in android-19 and prior
#undef wcsnlen
extern size_t wcsnlen(const wchar_t* str, size_t maxLen);

#undef stpcpy
extern char* stpcpy(char* dest, const char* str);
// end android-19


#define TARGET_DEFAULT_ALIGN (16U)

#ifdef _RELEASE
    #define __debugbreak()
#else
    #define __debugbreak() __builtin_trap()
#endif

// there is no __finite in android, only variants of isfinite
#undef __finite
#if NDK_REV_MAJOR >= 16
    #define __finite isfinite
#else
    #define __finite __isfinite
#endif

#define S_IWRITE S_IWUSR

#define ILINE __forceinline
#define _A_RDONLY (0x01)
#define _A_SUBDIR (0x10)
#define _A_HIDDEN (0x02)


#include <android/api-level.h>

#if __ANDROID_API__ == 19
    // The following were apparently introduced in API 21, however in earlier versions of the
    // platform specific headers they were defines.  In the move to unified headers, the follwoing
    // defines were removed from stat.h
    #ifndef stat64
        #define stat64 stat
    #endif

    #ifndef fstat64
        #define fstat64 fstat
    #endif

    #ifndef lstat64
        #define lstat64 lstat
    #endif
#endif // __ANDROID_API__ == 19


// std::stoull deosn't exist on android, so we need to define it
namespace std
{
    inline unsigned long long stoull(const std::string& str, size_t* idx = 0, int base = 10)
    {
        const char* start = str.c_str();
        char* end = nullptr;
        unsigned long long result = strtoull(start, &end, base);
        if (idx)
        {
            *idx = end - start;
        }
        return result;
    }
}

#endif // CRYINCLUDE_CRYCOMMON_ANDROIDSPECIFIC_H
