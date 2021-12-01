/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Platform dependent stuff.
//               Include this file instead of windows h

#pragma once

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define PLATFORM_H_SECTION_3 3
#define PLATFORM_H_SECTION_5 5
#define PLATFORM_H_SECTION_6 6
#define PLATFORM_H_SECTION_7 7
#define PLATFORM_H_SECTION_8 8
#define PLATFORM_H_SECTION_10 10
#define PLATFORM_H_SECTION_11 11
#define PLATFORM_H_SECTION_12 12
#define PLATFORM_H_SECTION_13 13
#define PLATFORM_H_SECTION_14 14
#define PLATFORM_H_SECTION_15 15
#endif

#if (defined(LINUX) && !defined(ANDROID)) || defined(APPLE)
    #define _FILE_OFFSET_BITS 64 // define large file support > 2GB
#endif

#include <AzCore/PlatformIncl.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_3
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(MOBILE)
    #define CONSOLE
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_5
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(LINUX) || defined(APPLE)
    #define __STDC_FORMAT_MACROS
    #include <cinttypes>
#else
    #include <cinttypes>
#endif

#if !defined(PRISIZE_T)
    #if defined(AZ_RESTRICTED_PLATFORM)
        #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_6
        #include AZ_RESTRICTED_FILE(platform_h)
    #endif
    #if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
        #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(WIN64)
        #define PRISIZE_T "I64u" //size_t defined as unsigned __int64
    #elif defined(WIN32) || defined(LINUX32)
        #define PRISIZE_T "u"
    #elif defined(MAC) || defined(LINUX64) || defined(IOS)
        #define PRISIZE_T "lu"
    #else
        #error "Please defined PRISIZE_T for this platform"
    #endif
#endif

#if !defined(PRI_THREADID)
    #if defined(AZ_RESTRICTED_PLATFORM)
        #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_7
        #include AZ_RESTRICTED_FILE(platform_h)
    #endif
    #if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
        #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(MAC) || defined(IOS) && defined(__LP64__) && defined(__LP64__)
        #define PRI_THREADID "lld"
    #elif defined(LINUX64) || defined(ANDROID)
        #define PRI_THREADID "ld"
    #else
        #define PRI_THREADID "d"
    #endif
#endif

#include "ProjectDefines.h"                         // to get some defines available in every CryEngine project

// Function attribute for printf/scanf-style parameters.
// This enables extended argument checking by GCC.
//
// Usage:
// Put this after the function or method declaration (not the definition!),
// between the final closing parenthesis and the semicolon.
// The first parameter indicates the 1-based index of the format string
// parameter, the second parameter indicates the 1-based index of the first
// variable parameter.  Example:
//   void foobar(int a, const char *fmt, ...) PRINTF_PARAMS(2, 3);
//
// For va_list based printf style functions, specfy 0 as the second parameter.
// Example:
//   void foobarv(int a, const char *fmt, va_list ap) PRINTF_PARAMS(2, 0);
//
// Note that 'this' is counted as a method argument. For non-static methods,
// add 1 to the indices.
//
// Use PRINTF_EMPTY_STRING when you want to format an empty string using
// a function defined with PRINTF_PARAMS to avoid zero length format string
// warnings when these checks are enabled.

#if defined(__GNUC__) && !defined(_RELEASE)
  #define PRINTF_PARAMS(...) __attribute__ ((format (printf, __VA_ARGS__)))
  #define SCANF_PARAMS(...) __attribute__ ((format (scanf, __VA_ARGS__)))
  #define PRINTF_EMPTY_FORMAT "%s", ""
#else
  #define PRINTF_PARAMS(...)
    #define SCANF_PARAMS(...)
  #define PRINTF_EMPTY_FORMAT ""
#endif

//default stack size for threads, currently only used on pthread platforms
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_8
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#endif

#include <AzCore/PlatformDef.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #define DLL_EXPORT
    #define DLL_IMPORT
#else // AZ_MONOLITHIC_BUILD
    #define DLL_EXPORT AZ_DLL_EXPORT
    #define DLL_IMPORT AZ_DLL_IMPORT
#endif // AZ_MONOLITHIC_BUILD


//////////////////////////////////////////////////////////////////////////
// Define BIT macro for use in enums and bit masks.
#define BIT(x) (1 << (x))
#define BIT64(x) (1ll << (x))
#define TYPED_BIT(type, x) (type(1) << (x))
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Help message, all help text in code must be wrapped in this define.
// Only include these in non RELEASE builds
#if !defined(_RELEASE)
    #define _HELP(x) x
#else
    #define _HELP(x) ""
#endif

#ifdef _PREFAST_
#   define PREFAST_ASSUME(cond) __analysis_assume(cond)
#else
#   define PREFAST_ASSUME(cond)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_10
    #include AZ_RESTRICTED_FILE(platform_h)
#else
    #if defined(WIN64)
        #include "Win64specific.h"
    #elif defined(LINUX64) && !defined(ANDROID)
        #include "Linux64Specific.h"
    #elif defined(MAC)
        #include "MacSpecific.h"
    #elif defined(ANDROID)
        #include "AndroidSpecific.h"
    #elif defined(IOS)
        #include "iOSSpecific.h"
    #endif
#endif


#if !defined(TARGET_DEFAULT_ALIGN)
# error "No default alignment specified for target architecture"
#endif

// Indicates potentially dangerous cast on 64bit machines
typedef UINT_PTR TRUNCATE_PTR;
typedef UINT_PTR EXPAND_PTR;

// Use static branch prediction to improve the generated assembly when possible.
// This feature has an indirect effect on runtime performance, as it ensures assembly code
// which is more likely needed (the programmer has to decide this), is directly after the if
//
// For reference, as far as i am aware, all compilers use the following heuristic for static branch prediction
// if branches are always taken
// forward jumps are not taken
// backwards jumps are taken (eg. jumping back to the beginning of a loop)
#if defined(__clang__) || defined(__GNUC__)
#   define IF(condition, hint)          if (__builtin_expect(!!(condition), hint))
#   define WHILE(condition, hint)   while (__builtin_expect(!!(condition), hint))
#   define IF_UNLIKELY(condition)   if (__builtin_expect(!!(condition), 0))
#   define IF_LIKELY(condition)     if (__builtin_expect(!!(condition), 1))
#else
// Fallback for compilers which don't support static branch prediction (like MSVC)
#   define IF(condition, hint)          if ((condition))
#   define WHILE(condition, hint)   while ((condition))
#   define IF_UNLIKELY(condition)   if ((condition))
#   define IF_LIKELY(condition)     if ((condition))
#endif // !defined(__clang__) || defined(__GNUC__)

#include <stdio.h>


//////////////////////////////////////////////////////////////////////////
// Provide special cast function which mirrors C++ style casts to support aliasing correct type punning casts in gcc with strict-aliasing enabled
template<typename DestinationType, typename SourceType>
ILINE DestinationType alias_cast(SourceType pPtr)
{
    union
    {
        SourceType pSrc;
        DestinationType pDst;
    } conv_union;
    conv_union.pSrc = pPtr;
    return conv_union.pDst;
}

//////////////////////////////////////////////////////////////////////////
#ifndef DEPRECATED
#define DEPRECATED
#endif

// Assert dialog box macros
#include "CryAssert.h"

// Replace standard assert calls by our custom one
// Works only ifdef USE_CRY_ASSERT && _DEBUG && WIN32
#ifndef assert
#define assert CRY_ASSERT
#endif

//////////////////////////////////////////////////////////////////////////
// Platform dependent functions that emulate Win32 API.
// Mostly used only for debugging!
//////////////////////////////////////////////////////////////////////////
void   CrySleep(unsigned int dwMilliseconds);
void   CryMessageBox(const char* lpText, const char* lpCaption, unsigned int uType);

//---------------------------------------------------------------------------
// Useful function to clean the structure.
template <class T>
inline void ZeroStruct(T& t)
{ memset(static_cast<void*>(&t), 0, sizeof(t)); }

// Useful functions to init and destroy objects.
template<class T>
inline void Construct(T& t)
{ new(&t)T(); }

template<class T, class U>
inline void Construct(T& t, U const& u)
{   new(&t)T(u); }

template<class T>
inline void Destruct(T& t)
{ t.~T(); }

// Cast one type to another, asserting there is no conversion loss.
// Usage: DestType dest = check_cast<DestType>(src);
template<class D, class S>
inline D check_cast(S const& s)
{
    D d = D(s);
    assert(S(d) == s);
    return d;
}

//---------------------------------------------------------------------------
// Quick const-manipulation macros

// Declare a const and variable version of a function simultaneously.
#define CONST_VAR_FUNCTION(head, body) \
    inline head body                   \
    inline const head const body

template<class T>
inline
T& non_const(const T& t)
{ return const_cast<T&>(t); }

#define using_type(super, type) \
    typedef typename super::type type;

typedef unsigned char   uchar;
typedef unsigned int uint;
typedef const char* cstr;

//---------------------------------------------------------------------------
// Align function works on integer or pointer values.
// Only support power-of-two alignment.
template<typename T>
inline
T Align(T nData, size_t nAlign)
{
    assert((nAlign & (nAlign - 1)) == 0);
    size_t size = ((size_t)nData + (nAlign - 1)) & ~(nAlign - 1);
    return T(size);
}

template<typename T>
inline
bool IsAligned(T nData, size_t nAlign)
{
    assert((nAlign & (nAlign - 1)) == 0);
    return (size_t(nData) & (nAlign - 1)) == 0;
}

template<typename T, typename U>
inline
void SetFlags(T& dest, U flags, bool b)
{
    if (b)
    {
        dest |= flags;
    }
    else
    {
        dest &= ~flags;
    }
}

// Wrapper code for non-windows builds.
#if defined(LINUX) || defined(APPLE)
    #include "Linux_Win32Wrapper.h"
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_12
    #include AZ_RESTRICTED_FILE(platform_h)
#endif

bool   CrySetFileAttributes(const char* lpFileName, uint32 dwFileAttributes);
threadID CryGetCurrentThreadId();

#ifdef __GNUC__
    #define NO_INLINE __attribute__ ((noinline))
    #define NO_INLINE_WEAK __attribute__ ((noinline)) __attribute__((weak)) // marks a function as no_inline, but also as weak to prevent multiple-defined errors
    #define __PACKED __attribute__ ((packed))
#else
    #define NO_INLINE _declspec(noinline)
    #define NO_INLINE_WEAK _declspec(noinline) inline
    #define __PACKED
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_13
    #include AZ_RESTRICTED_FILE(platform_h)
#elif !defined(LINUX) && !defined(APPLE)
    typedef int socklen_t;
#endif

// In RELEASE disable printf and fprintf
#if defined(_RELEASE) && !defined(RELEASE_LOGGING)
    #if defined(AZ_RESTRICTED_PLATFORM)
        #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_14
        #include AZ_RESTRICTED_FILE(platform_h)
    #endif
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_15
    #include AZ_RESTRICTED_FILE(platform_h)
#endif

void InitRootDir(char szExeFileName[] = nullptr, uint nExeSize = 0, char szExeRootName[] = nullptr, uint nRootSize = 0);
