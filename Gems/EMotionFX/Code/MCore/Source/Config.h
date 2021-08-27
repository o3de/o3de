/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Debug/Trace.h>

#include <wchar.h>
#include <stdint.h>

// compilers
#define MCORE_COMPILER_MSVC         1
#define MCORE_COMPILER_INTELC       2
#define MCORE_COMPILER_CODEWARRIOR  3
#define MCORE_COMPILER_GCC          4
#define MCORE_COMPILER_MINGW        5
#define MCORE_COMPILER_CLANG        6
#define MCORE_COMPILER_LLVM         7
#define MCORE_COMPILER_SNC          8


// finds the compiler type and version
#if (defined(__GNUC__) || defined(__GNUC) || defined(__gnuc))
    #define MCORE_COMPILER MCORE_COMPILER_GCC
    #if defined(__GNUC_PATCHLEVEL__)
        #define MCORE_COMPILER_VERSION_PATCH __GNUC_PATCHLEVEL__
        #define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #else
        #define MCORE_COMPILER_VERSION_PATCH 0
        #define __GNUC_VERSION__ (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)
    #endif
    #define MCORE_COMPILER_VERSION_MAJOR __GNUC__
    #define MCORE_COMPILER_VERSION_MINOR __GNUC_MINOR__
//#define MCORE_COMPILER_VERSION __VERSION__
#elif (defined(__ICL) || defined(__INTEL_COMPILER))
    #define MCORE_COMPILER MCORE_COMPILER_INTELC
    #define MCORE_COMPILER_VERSION_MAJOR __INTEL_COMPILER
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__MINGW64__)
    #define MCORE_COMPILER MCORE_COMPILER_MINGW
    #if defined(__MINGW32_MAJOR_VERSION)
        #define MCORE_COMPILER_VERSION_MAJOR __MINGW64_MAJOR_VERSION
        #define MCORE_COMPILER_VERSION_MINOR __MINGW64_MAJOR_VERSION
        #define MCORE_COMPILER_VERSION_PATCH 0
    #else
        #define MCORE_COMPILER_VERSION_MAJOR 0
        #define MCORE_COMPILER_VERSION_MINOR 0
        #define MCORE_COMPILER_VERSION_PATCH 0
    #endif
#elif defined(__SNC__)
    #define MCORE_COMPILER MCORE_COMPILER_SNC
    #define MCORE_COMPILER_VERSION_MAJOR 0
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__llvm__)
    #define MCORE_COMPILER MCORE_COMPILER_LLVM
    #define MCORE_COMPILER_VERSION_MAJOR 0
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__clang__)
    #define MCORE_COMPILER MCORE_COMPILER_CLANG
    #define MCORE_COMPILER_VERSION_MAJOR __clang_major__
    #define MCORE_COMPILER_VERSION_MINOR __clang_minor__
    #define MCORE_COMPILER_VERSION_PATCH __clang_patchlevel__
#elif (defined(__MWERKS__) || defined(__CWCC__))
    #define MCORE_COMPILER MCORE_COMPILER_CODEWARRIOR
    #if defined(__CWCC__)
        #define MCORE_COMPILER_VERSION_MAJOR __CWCC__
    #else
        #define MCORE_COMPILER_VERSION_MAJOR __MWERKS__
    #endif
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#elif defined(__MINGW32__)
    #define MCORE_COMPILER MCORE_COMPILER_MINGW
    #if defined(__MINGW32_MAJOR_VERSION)
        #define MCORE_COMPILER_VERSION_MAJOR __MINGW32_MAJOR_VERSION
        #define MCORE_COMPILER_VERSION_MINOR __MINGW32_MINOR_VERSION
        #define MCORE_COMPILER_VERSION_PATCH 0
    #else
        #define MCORE_COMPILER_VERSION_MAJOR 0
        #define MCORE_COMPILER_VERSION_MINOR 0
        #define MCORE_COMPILER_VERSION_PATCH 0
    #endif
#elif defined(_MSC_VER)
    #define MCORE_COMPILER MCORE_COMPILER_MSVC
    #define MCORE_COMPILER_VERSION_MAJOR _MSC_VER
    #define MCORE_COMPILER_VERSION_MINOR 0
    #define MCORE_COMPILER_VERSION_PATCH 0
#else
// unsupported compiler!
    #define MCORE_COMPILER MCORE_COMPILER_UNKNOWN
    #define MCORE_COMPILER_VERSION 0
#endif


// define the basic types
#include <BaseTypes.h>


//---------------------
// stringised version of line number (must be done in two steps)
#define MCORE_STRINGISE(N) #N
#define MCORE_EXPAND_THEN_STRINGISE(N) MCORE_STRINGISE(N)
#define MCORE_LINE_STR MCORE_EXPAND_THEN_STRINGISE(__LINE__)

// MSVC-suitable routines for formatting <#pragma message>
#define MCORE_LOC __FILE__ "(" MCORE_LINE_STR ")"
#define MCORE_OUTPUT_FORMAT(type) MCORE_LOC " : [" type "] "

// specific message types for <#pragma message>
#define MCORE_WARNING MCORE_OUTPUT_FORMAT("WARNING")
#define MCORE_ERROR MCORE_OUTPUT_FORMAT("ERROR")
#define MCORE_MESSAGE MCORE_OUTPUT_FORMAT("INFO")
#define MCORE_INFO MCORE_OUTPUT_FORMAT("INFO")
#define MCORE_TODO MCORE_OUTPUT_FORMAT("TODO")

// USAGE:
// #pragma message ( MCORE_MESSAGE "my message" )
//---------------------

// some special types that are missing inside Visual C++ 6
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC)
    #if (MCORE_COMPILERVERSION < 1300)
typedef unsigned long uintPointer;
    #else
        #include <stddef.h>
typedef uintptr_t uintPointer;
    #endif
#else
    #include <stddef.h>
    #include <stdint.h>
typedef uintptr_t uintPointer;
#endif


// set the inline macro
// we want to use __inline for Visual Studio, so that we can still enable these inline functions in debug mode
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC)
    #define MCORE_INLINE __inline
    #define MCORE_FORCEINLINE __forceinline
#elif (MCORE_COMPILER == MCORE_COMPILER_GCC)
    #define MCORE_INLINE __inline
    #define MCORE_FORCEINLINE __attribute__((always_inline))
#else
    #define MCORE_INLINE inline
    #define MCORE_FORCEINLINE inline
#endif


// debug macro
#ifdef _DEBUG
    #define MCORE_DEBUG
#endif

// date define
#define MCORE_WIDEN2(x) L ## x
#define MCORE_WIDEN(x) MCORE_WIDEN2(x)
#define __WDATE__ MCORE_WIDEN(__DATE__)
#define MCORE_WDATE __WDATE__
#define MCORE_DATE __DATE__

// time define
#define __WTIME__ MCORE_WIDEN(__TIME__)
#define MCORE_WTIME __WTIME__
#define MCORE_TIME __TIME__


#ifdef _DEBUG
    #define __WFILE__ MCORE_WIDEN(__FILE__)
    #define MCORE_WFILE __WFILE__
    #define MCORE_WLINE __LINE__
    #define MCORE_FILE __FILE__
    #define MCORE_LINE __LINE__
#else
    #define MCORE_FILE nullptr
    #define MCORE_LINE 0xFFFFFFFF
    #define MCORE_WFILE nullptr
    #define MCORE_WLINE 0xFFFFFFFF
#endif


// define the __cdecl
#if (MCORE_COMPILER == MCORE_COMPILER_MSVC || MCORE_COMPILER == MCORE_COMPILER_INTELC || MCORE_COMPILER == MCORE_COMPILER_CLANG)
    #define MCORE_CDECL __cdecl
#else
    #define MCORE_CDECL
#endif


// endian conversion, defaults to big endian
#if defined(AZ_BIG_ENDIAN)
    #define MCORE_FROM_LITTLE_ENDIAN16(buffer, count)   MCore::Endian::ConvertEndian16(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN32(buffer, count)   MCore::Endian::ConvertEndian32(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN64(buffer, count)   MCore::Endian::ConvertEndian64(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN16(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN32(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN64(buffer, count)
#else   // little endian
    #define MCORE_FROM_LITTLE_ENDIAN16(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN32(buffer, count)
    #define MCORE_FROM_LITTLE_ENDIAN64(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN16(buffer, count)  MCore::Endian::ConvertEndian16(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN32(buffer, count)  MCore::Endian::ConvertEndian32(buffer, count)
    #define MCORE_FROM_BIG_ENDIAN64(buffer, count)  MCore::Endian::ConvertEndian64(buffer, count)
#endif

#ifndef NULL
    #define NULL 0
#endif

// detect and enable OpenMP support
#if defined(_OPENMP)
    #define MCORE_OPENMP_ENABLED
#endif

// shared lib import/export
#define MCORE_EXPORT AZ_DLL_EXPORT
#define MCORE_IMPORT AZ_DLL_IMPORT

// shared lib importing/exporting
#if defined(MCORE_DLL_EXPORT)
    #define MCORE_API       MCORE_EXPORT
#else
    #if defined(MCORE_DLL)
        #define MCORE_API   MCORE_IMPORT
    #else
        #define MCORE_API
    #endif
#endif


// define a custom assert macro
#ifndef MCORE_NO_ASSERT
    #define MCORE_ASSERT(x) AZ_Assert(x, "MCore Asserted")
#else
    #define MCORE_ASSERT(x)
#endif


// check wchar_t size
#if (WCHAR_MAX > 0xFFFFu)
    #define MCORE_UTF32
#elif (WCHAR_MAX > 0xFFu)
    #define MCORE_UTF16
#else
    #define MCORE_UTF8
// ERROR: unsupported
    #error "MCore: sizeof(wchar_t)==8, while MCore does not support this!"
#endif

// mark as unused to prevent compiler warnings
#define MCORE_UNUSED(x) static_cast<void>(x)

namespace MCore
{
    template<class IndexType>
    inline static constexpr IndexType InvalidIndexT = static_cast<IndexType>(-1);

    inline static constexpr const size_t InvalidIndex = InvalidIndexT<size_t>;
    inline static constexpr const AZ::u64 InvalidIndex64 = InvalidIndexT<AZ::u64>;
    inline static constexpr const AZ::u32 InvalidIndex32 = InvalidIndexT<AZ::u32>;
    inline static constexpr const AZ::u16 InvalidIndex16 = InvalidIndexT<AZ::u16>;
    inline static constexpr const AZ::u8 InvalidIndex8 = InvalidIndexT<AZ::u8>;
} // namespace MCore

/**
 * Often there are functions that allow you to search for objects. Such functions return some index value that points
 * inside for example the array of objects. However, in case the object we are searching for cannot be found, some
 * value has to be returned that identifies that the object cannot be found. The MCORE_INVALIDINDEX32 value is used
 * used as this value. The real value is 0xFFFFFFFF.
 */
#define MCORE_INVALIDINDEX32 MCore::InvalidIndex32

/**
 * The 16 bit index variant of MCORE_INVALIDINDEX32.
 * The real value is 0xFFFF.
 */
#define MCORE_INVALIDINDEX16 MCore::InvalidIndex16

/**
 * The 8 bit index variant of MCORE_INVALIDINDEX32.
 * The real value is 0xFF.
 */
#define MCORE_INVALIDINDEX8 MCore::InvalidIndex8

