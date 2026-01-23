/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <dirent.h>
#include <vector>
#include <AzCore/std/string/string.h>

#if defined(PLATFORM_64BIT)
#   define MEMORY_ALLOCATION_ALIGNMENT 16
#else
#   define MEMORY_ALLOCATION_ALIGNMENT 8
#endif

#if !defined(_CPU_SSE)
typedef int64 __m128;
#endif

//////////////////////////////////////////////////////////////////////////
// io.h stuff
#if !defined(ANDROID)
extern int errno;
#endif
typedef unsigned int _fsize_t;


// Defined in the launcher.
// AZ_DLL_IMPORT void OutputDebugString(const char*);
// AZ_DLL_IMPORT void DebugBreak();

//#ifdef __cplusplus
//#define IGNORE              0       // Ignore signal
//#define INFINITE            0xFFFFFFFF  // Infinite timeout
//#endif

//////////////////////////////////////////////////////////////////////////
extern threadID GetCurrentThreadId();

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus

//helper function
extern void adaptFilenameToLinux(char* rAdjustedFilename);
extern void replaceDoublePathFilename(char* szFileName);//removes "\.\" to "\" and "/./" to "/"

//////////////////////////////////////////////////////////////////////////
extern void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext);

template <size_t size>
void _makepath_s(char(&path)[size], const char *drive, const char *dir, const char *fname, const char *ext)
{
    _makepath(path, drive, dir, fname, ext);
}

extern void _splitpath(const char* inpath, char* drv, char* dir, char* fname, char* ext);

template <size_t drivesize, size_t dirsize, size_t fnamesize, size_t extsize>
void _splitpath_s(const char *path, char(&drive)[drivesize], char(&dir)[dirsize], char(&fname)[fnamesize], char(&ext)[extsize])
{
    _splitpath(path, drive, dir, fname, ext);
}

extern char* _ui64toa(unsigned long long value,    char* str, int radix);
extern long long _atoi64(const char* str);



//////////////////////////////////////////////////////////////////////////

#ifndef __TRLTOA__
#define __TRLTOA__
extern char* ltoa (long i, char* a, int radix);
#endif
#define itoa ltoa

//////////////////////////////////////////////////////////////////////////
#include <cmath>
using std::abs;
using std::sqrt;
using std::fabs;

extern char* _strtime(char* date);
extern char* _strdate(char* date);

#if !defined(_CPU_SSE)
#define _MM_HINT_T0     (1)
#define _MM_HINT_T1     (2)
#define _MM_HINT_T2     (3)
#define _MM_HINT_NTA    (0)
inline void _mm_prefetch(const char*, int) { }
#endif // !_CPU_SSE

#endif //__cplusplus
//////////////////////////////////////////////////////////////////////////
// Byte Swapping functions

inline unsigned short _byteswap_ushort(unsigned short input)
{
    return ((input & 0xff) << 8) | ((input & 0xff00) >> 8);
}

inline LONG _byteswap_ulong(LONG input)
{
    return (input & 0x000000ff) << 24 |
           (input & 0x0000ff00) << 8 |
           (input & 0x00ff0000) >> 8 |
           (input & 0xff000000) >> 24;
}

inline unsigned long long   _byteswap_uint64(unsigned long long input)
{
    return (((input & 0xff00000000000000ull) >> 56) |
            ((input & 0x00ff000000000000ull) >> 40) |
            ((input & 0x0000ff0000000000ull) >> 24) |
            ((input & 0x000000ff00000000ull) >> 8) |
            ((input & 0x00000000ff000000ull) << 8) |
            ((input & 0x0000000000ff0000ull) << 24) |
            ((input & 0x000000000000ff00ull) << 40) |
            ((input & 0x00000000000000ffull) << 56));
}
