/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Platform dependend stuff.
//               Include this file instead of windows h

#pragma once

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define PLATFORM_H_SECTION_1 1
#define PLATFORM_H_SECTION_2 2
#define PLATFORM_H_SECTION_3 3
#define PLATFORM_H_SECTION_4 4
#define PLATFORM_H_SECTION_5 5
#define PLATFORM_H_SECTION_6 6
#define PLATFORM_H_SECTION_7 7
#define PLATFORM_H_SECTION_8 8
#define PLATFORM_H_SECTION_9 9
#define PLATFORM_H_SECTION_10 10
#define PLATFORM_H_SECTION_11 11
#define PLATFORM_H_SECTION_12 12
#define PLATFORM_H_SECTION_13 13
#define PLATFORM_H_SECTION_14 14
#define PLATFORM_H_SECTION_15 15
#endif

// certain C++ features are not available in some compiler versions
// turn them off here:
// #define _ALLOW_KEYWORD_MACROS
// #define _DISALLOW_INITIALIZER_LISTS
// #define _DISALLOW_ENUM_CLASS

#if defined(_MSC_VER)
    #define _ALLOW_KEYWORD_MACROS

    #define alignof _alignof
    #if !defined(_HAS_EXCEPTIONS)
        #define _HAS_EXCEPTIONS 0
    #endif
#elif defined(__GNUC__)
    #define alignof __alignof__
#endif

// Alignment|InitializerList support.
#define _ALLOW_INITIALIZER_LISTS

#if (defined(LINUX) && !defined(ANDROID)) || defined(APPLE)
#define _FILE_OFFSET_BITS 64 // define large file support > 2GB
#endif

#include <AzCore/PlatformIncl.h>

#include <cstring>

#if defined(_MSC_VER) // We want the class name to be included, but __FUNCTION__ doesn't contain that on GCC/clang
    #define __FUNC__ __FUNCTION__
#else
    #define __FUNC__ __PRETTY_FUNCTION__
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_1
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(_DEBUG) && !defined(LINUX) && !defined(APPLE)
    #include <crtdbg.h>
#endif

#define RESTRICT_POINTER __restrict

// we have to use it because of VS doesn't support restrict reference variables
#if defined(APPLE) || defined(LINUX)
    #if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
        #define GCC411_OR_LATER
    #endif
    #define RESTRICT_REFERENCE __restrict
#else
    #define RESTRICT_REFERENCE
#endif


#ifndef CHECK_REFERENCE_COUNTS //define that in your StdAfx.h to override per-project
# define CHECK_REFERENCE_COUNTS 0 //default value
#endif

#if CHECK_REFERENCE_COUNTS
# define CHECK_REFCOUNT_CRASH(x) { if (!(x)) {*((int*)0) = 0; } \
}
#else
# define CHECK_REFCOUNT_CRASH(x)
#endif

#ifndef GARBAGE_MEMORY_ON_FREE //define that in your StdAfx.h to override per-project
# define GARBAGE_MEMORY_ON_FREE 0 //default value
#endif

#if GARBAGE_MEMORY_ON_FREE
# ifndef GARBAGE_MEMORY_RANDOM          //define that in your StdAfx.h to override per-project
#  define GARBAGE_MEMORY_RANDOM 1   //0 to change it to progressive pattern
# endif
#endif

//////////////////////////////////////////////////////////////////////////
// Available predefined compiler macros for Visual C++.
//      _MSC_VER                                        // Indicates MS Visual C compiler version
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_2
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
//      _WIN32, _WIN64       // Indicates target OS
#endif
//      _M_IX86, _M_PPC                         // Indicates target processor
//      _DEBUG                                          // Building in Debug mode
//      _DLL                                                // Linking with DLL runtime libs
//      _MT                                                 // Linking with multi-threaded runtime libs
//////////////////////////////////////////////////////////////////////////

//
// Translate some predefined macros.
//

// NDEBUG disables std asserts, etc.
// Define it automatically if not compiling with Debug libs, or with ADEBUG flag.
#if !defined(_DEBUG) && !defined(ADEBUG) && !defined(NDEBUG)
    #define NDEBUG
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_3
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(MOBILE)
    #define CONSOLE
#endif

//render thread settings, as this is accessed inside 3dengine and renderer and needs to be compile time defined, we need to do it here
//enable this macro to strip out the overhead for render thread
//  #define STRIP_RENDER_THREAD
#ifdef STRIP_RENDER_THREAD
    #define RT_COMMAND_BUF_COUNT 1
#else
//can be enhanced to triple buffering, FlushFrame needs to be adjusted and RenderObj would become 132 bytes
    #define RT_COMMAND_BUF_COUNT 2
#endif


// We use WIN macros without _.
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_4
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#if defined(_WIN32) && !defined(LINUX32) && !defined(LINUX64) && !defined(APPLE) && !defined(WIN32)
    #define WIN32
#endif
#if defined(_WIN64) && !defined(WIN64)
    #define WIN64
#endif
#endif

// In Win32 Release we use static linkage
#ifdef WIN32
    #if !defined(_RELEASE) || defined(RESOURCE_COMPILER) || defined(EDITOR) || defined(_FORCEDLL)
// All windows targets not in Release built as DLLs.
        #ifndef _USRDLL
            #define _USRDLL
        #endif
    #endif

#endif //WIN32

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_5
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(LINUX) || defined(APPLE)
    #define __STDC_FORMAT_MACROS
    #include <inttypes.h>
    #if defined(APPLE) || defined(LINUX64)
    // int64 is not the same type as the operating system's int64_t
        #undef PRIX64
        #undef PRIx64
        #undef PRId64
        #undef PRIu64
        #define PRIX64 "llX"
        #define PRIx64 "llx"
        #define PRId64 "lld"
        #define PRIu64 "llu"
    #endif
    #define PLATFORM_I64(x) x##ll
#else
    #include <inttypes.h>
    #define PLATFORM_I64(x) x##i64
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

#if defined(IOS)
#define USE_PTHREAD_TLS
#endif

// Storage class modifier for thread local storage.
// THEADLOCAL should NOT be defined to empty because that creates some
// really hard to find issues.
#if !defined(USE_PTHREAD_TLS)
#   define THREADLOCAL AZ_TRAIT_COMPILER_THREAD_LOCAL
#endif //!defined(USE_PTHREAD_TLS)



//////////////////////////////////////////////////////////////////////////
// define Read Write Barrier macro needed for lockless programming
//////////////////////////////////////////////////////////////////////////
#if   defined(__arm__)
/**
 * (ARMv7) Full memory barriar.
 *
 * None of GCC 4.6/4.8 or clang 3.3/3.4 have a builtin intrinsic for ARM's ldrex/strex or dmb
 * instructions.  This is a placeholder until supplied by the toolchain.
 */
inline void  __dmb()
{
    // The linux kernel uses "dmb ish" to only sync with local monitor (arch/arm/include/asm/barrier.h):
    //#define dmb(option) __asm__ __volatile__ ("dmb " #option : : : "memory")
    //#define smp_mb()        dmb(ish)
    __asm__ __volatile__ ("dmb ish" : : : "memory");
}

#define READ_WRITE_BARRIER {__dmb(); }
#else
    #define READ_WRITE_BARRIER
#endif
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// define macro to prevent memory reoderings of reads/and writes
//TODO implement for all GCC platforms, else there are potential crashes with strict aliasing
    #define MEMORY_RW_REORDERING_BARRIER do { /*not implemented*/} while (0)

//default stack size for threads, currently only used on pthread platforms
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_8
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(LINUX) || defined(APPLE)
    #if !defined(_DEBUG)
        #define SIMPLE_THREAD_STACK_SIZE_KB (256)
    #else
        #define SIMPLE_THREAD_STACK_SIZE_KB (256 * 4)
    #endif
#else
    #define SIMPLE_THREAD_STACK_SIZE_KB (32)
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
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Globally Used Defines.
//////////////////////////////////////////////////////////////////////////
// CPU Types: _CPU_X86,_CPU_AMD64,_CPU_G5
// Platform: WIN23,WIN64,LINUX32,LINUX64,MAC
// CPU supported functionality: _CPU_SSE
//////////////////////////////////////////////////////////////////////////


  #if defined(_MSC_VER)
    #define PREFAST_SUPPRESS_WARNING(W) __pragma(warning(suppress: W))
  #else
    #define PREFAST_SUPPRESS_WARNING(W)
  #endif

#ifdef _PREFAST_
#   define PREFAST_ASSUME(cond) __analysis_assume(cond)
#else
#   define PREFAST_ASSUME(cond)
#endif


#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_9
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    #if defined(WIN32) && !defined(WIN64)
        #include "Win32specific.h"
    #endif

    #if defined(WIN64)
        #include "Win64specific.h"
    #endif
#endif

#if defined(LINUX64) && !defined(ANDROID)
#include "Linux64Specific.h"
#endif

#if defined(LINUX32) && !defined(ANDROID)
#include "Linux32Specific.h"
#endif

#if defined(ANDROID)
#include "AndroidSpecific.h"
#endif


#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_10
    #include AZ_RESTRICTED_FILE(platform_h)
#endif

#if defined(MAC)
#include "MacSpecific.h"
#endif

#if defined(IOS)
#include "iOSSpecific.h"
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

#include "CryLegacyAllocator.h"

//////////////////////////////////////////////////////////////////////////
#ifndef DEPRECATED
#define DEPRECATED
#endif

//////////////////////////////////////////////////////////////////////////
// compile time error stuff
//////////////////////////////////////////////////////////////////////////
#undef STATIC_CHECK
#define STATIC_CHECK(expr, msg) static_assert(expr, #msg)

// Assert dialog box macros
#include "CryAssert.h"

// Replace standard assert calls by our custom one
// Works only ifdef USE_CRY_ASSERT && _DEBUG && WIN32
#ifndef assert
#define assert CRY_ASSERT
#endif

#include "CompileTimeAssert.h"
//////////////////////////////////////////////////////////////////////////
// Platform dependent functions that emulate Win32 API.
// Mostly used only for debugging!
//////////////////////////////////////////////////////////////////////////
void   CryDebugBreak();
void   CrySleep(unsigned int dwMilliseconds);
void   CryLowLatencySleep(unsigned int dwMilliseconds);
int    CryMessageBox(const char* lpText, const char* lpCaption, unsigned int uType);
short  CryGetAsyncKeyState(int vKey);
unsigned int CryGetFileAttributes(const char* lpFileName);

inline void CryHeapCheck()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_11
    #include AZ_RESTRICTED_FILE(platform_h)
#elif !defined(LINUX) && !defined(APPLE) // todo: this might be readded with later xdks?
#if !defined(NDEBUG)
    int Result =
#endif
        _heapchk();
    assert(Result != _HEAPBADBEGIN);
    assert(Result != _HEAPBADNODE);
    assert(Result != _HEAPBADPTR);
    assert(Result != _HEAPEMPTY);
    assert(Result == _HEAPOK);
#endif
}

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

// Convert one type to another, asserting there is no conversion loss.
// Usage: DestType dest;  check_convert(dest, src);
template<class D, class S>
inline D& check_convert(D& d, S const& s)
{
    d = D(s);
    assert(S(d) == s);
    return d;
}

// Convert one type to another, asserting there is no conversion loss.
// Usage: DestType dest;  check_convert(dest) = src;
template<class D>
struct CheckConvert
{
    CheckConvert(D& d)
        : dest(&d) {}

    template<class S>
    D& operator=(S const& s)
    {
        return check_convert(*dest, s);
    }

protected:
    D*  dest;
};

template<class D>
inline CheckConvert<D> check_convert(D& d)
{
    return d;
}

//---------------------------------------------------------------------------
// Use NoCopy as a base class to easily prevent copy init & assign for any class.
struct NoCopy
{
    NoCopy() {}
private:
    NoCopy(const NoCopy&);
    NoCopy& operator =(const NoCopy&);
};

//---------------------------------------------------------------------------
// ZeroInit: base class to zero the memory of the derived class before initialization, so local objects initialize the same as static.
// Usage:
//      class MyClass: ZeroInit<MyClass> {...}
//      class MyChild: public MyClass, ZeroInit<MyChild> {...}      // ZeroInit must be the last base class

template<class TDerived>
struct ZeroInit
{
#if defined(__clang__) || defined(__GNUC__)
    bool __dummy;                           // Dummy var to create non-zero size, ensuring proper placement in TDerived
#endif

    ZeroInit(bool bZero = true)
    {
        // Optional bool arg to selectively disable zeroing.
        if (bZero)
        {
            // Infer offset of this base class by static casting to derived class.
            // Zero only the additional memory of the derived class.
            TDerived* struct_end = static_cast<TDerived*>(this) + 1;
            size_t memory_size = (char*)struct_end - (char*)this;
            memset(this, 0, memory_size);
        }
    }
};

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

// Platform wrappers must be included before CryString.h
#   include "CryString.h"

// Include support for meta-type data.
#include "TypeInfo_decl.h"

// Include array.
#include <CryArray.h>

bool   CrySetFileAttributes(const char* lpFileName, uint32 dwFileAttributes);
threadID CryGetCurrentThreadId();

#if !defined(NOT_USE_CRY_STRING)
// Fixed-Sized (stack based string)
// put after the platform wrappers because of missing wcsicmp/wcsnicmp functions
    #include "CryFixedString.h"
#endif

// need this in a common header file and any other file would be too misleading
enum ETriState
{
    eTS_false,
    eTS_true,
    eTS_maybe
};

    #ifdef __GNUC__
        #define NO_INLINE __attribute__ ((noinline))
#   define NO_INLINE_WEAK __attribute__ ((noinline)) __attribute__((weak)) // marks a function as no_inline, but also as weak to prevent multiple-defined errors

#   define __PACKED __attribute__ ((packed))
    #else
        #define NO_INLINE _declspec(noinline)
#   define NO_INLINE_WEAK _declspec(noinline) inline

#   define __PACKED
    #endif

// Fallback for Alignment macro of GCC/CLANG (must be after the class definition)
#if !defined(_ALIGN)
        #define _ALIGN(num)
#endif

// Fallback for Alignment macro of MSVC (must be before the class definition)
#if !defined(_MS_ALIGN)
        #define _MS_ALIGN(num)
#endif

#if defined(WIN32) || defined(WIN64)
extern "C" {
__declspec(dllimport) unsigned long __stdcall TlsAlloc();
__declspec(dllimport) void* __stdcall TlsGetValue(unsigned long dwTlsIndex);
__declspec(dllimport) int __stdcall TlsSetValue(unsigned long dwTlsIndex, void* lpTlsValue);
}

    #define TLS_DECLARE(type, var) extern int var##idx;
    #define TLS_DEFINE(type, var)              \
    int var##idx;                              \
    struct Init##var {                         \
        Init##var() { var##idx = TlsAlloc(); } \
    };                                         \
    Init##var g_init##var;
    #define TLS_DEFINE_DEFAULT_VALUE(type, var, value)                                \
    int var##idx;                                                                     \
    struct Init##var {                                                                \
        Init##var() { var##idx = TlsAlloc(); TlsSetValue(var##idx, reinterpret_cast<void*>(value)); } \
    };                                                                                \
    Init##var g_init##var;
    #define TLS_GET(type, var) (type)TlsGetValue(var##idx)
    #define TLS_SET(var, val) TlsSetValue(var##idx, reinterpret_cast<void*>(val))
#elif defined(USE_PTHREAD_TLS)
    #define TLS_DECLARE(_TYPE, _VAR) extern SCryPthreadTLS<_TYPE> _VAR##TLSKey;
    #define TLS_DEFINE(_TYPE, _VAR) SCryPthreadTLS<_TYPE> _VAR##TLSKey;
    #define TLS_DEFINE_DEFAULT_VALUE(_TYPE, _VAR, _DEFAULT) SCryPthreadTLS<_TYPE> _VAR##TLSKey = _DEFAULT;
    #define TLS_GET(_TYPE, _VAR) _VAR##TLSKey.Get()
    #define TLS_SET(_VAR, _VALUE) _VAR##TLSKey.Set(_VALUE)
#elif defined(THREADLOCAL)
    #define TLS_DECLARE(type, var) extern THREADLOCAL type var;
#if defined(LINUX) || defined(MAC)
    #define TLS_DEFINE(type, var) THREADLOCAL type var = 0;
#else
    #define TLS_DEFINE(type, var) THREADLOCAL type var;
#endif // defined(LINUX) || defined(MAC)
    #define TLS_DEFINE_DEFAULT_VALUE(type, var, value) THREADLOCAL type var = value;
    #define TLS_GET(type, var) (var)
    #define TLS_SET(var, val) (var = (val))
#else // defined(THREADLOCAL)
    #error "There's no support for thread local storage"
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_13
    #include AZ_RESTRICTED_FILE(platform_h)
#elif !defined(LINUX) && !defined(APPLE)
typedef int socklen_t;
#endif


// Include MultiThreading support.
#include "CryThread.h"
#include "MultiThread.h"

// In RELEASE disable printf and fprintf
#if defined(_RELEASE) && !defined(RELEASE_LOGGING)
    #if defined(AZ_RESTRICTED_PLATFORM)
        #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_14
        #include AZ_RESTRICTED_FILE(platform_h)
    #endif
#endif

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_H_SECTION_15
    #include AZ_RESTRICTED_FILE(platform_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32) || defined(WIN64)
    #define MESSAGE(msg) message(__FILE__ "(" STRINGIFY(__LINE__) "): " msg)
#else
    #define MESSAGE(msg)
#endif

void InitRootDir(char szExeFileName[] = nullptr, uint nExeSize = 0, char szExeRootName[] = nullptr, uint nRootSize = 0);
