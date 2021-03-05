// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/////////////////////////////////////////////////////////////////////////
/// @file  platform.h
///
/// @brief Platform-related macros.
/////////////////////////////////////////////////////////////////////////

// clang-format off

#pragma once

#include <cstdlib>
#include <type_traits>
#include <utility>  // std::forward

// Make sure all platforms have the explicit sized integer types
#ifndef __STDC_LIMIT_MACROS
#    define __STDC_LIMIT_MACROS /* needed for some defs in stdint.h */
#endif
#include <cstdint>

#if defined(__FreeBSD__)
#    include <sys/param.h>
#endif

#ifdef __MINGW32__
#    include <malloc.h>  // for alloca
#endif

#if defined(_WIN32)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef VC_EXTRALEAN
#        define VC_EXTRALEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <windows.h>
#endif

#ifdef _MSC_VER
#    include <intrin.h>
#endif

#include <OpenImageIO/oiioversion.h>

// Detect which C++ standard we're using, and handy macros.
// See https://en.cppreference.com/w/cpp/compiler_support
//
// OIIO_CPLUSPLUS_VERSION : which C++ standard is compiling (11, 14, ...)
// OIIO_CONSTEXPR14 : constexpr for C++ >= 14, otherwise nothing (this is
//                      useful for things that can only be constexpr for 14)
// OIIO_CONSTEXPR17 : constexpr for C++ >= 17, otherwise nothing (this is
//                      useful for things that can only be constexpr for 17)
//
// Note: oiioversion.h defines OIIO_BUILD_CPP11, OIIO_BUILD_CPP14,
// OIIO_BUILD_CPP17, or OIIO_BUILD_CPP20 to be 1 if OIIO itself was *built*
// using C++11, C++14, C++17, or C++20, respectively. In contrast,
// OIIO_CPLUSPLUS_VERSION defined below will be set to the right number for
// the C++ standard being compiled RIGHT NOW. These two things may be the
// same when compiling OIIO, but they may not be the same if another
// package is compiling against OIIO and using these headers (OIIO may be
// C++11 but the client package may be newer, or vice versa -- use these two
// symbols to differentiate these cases, when important).
#if (__cplusplus >= 202001L)
#    define OIIO_CPLUSPLUS_VERSION 20
#    define OIIO_CONSTEXPR14 constexpr
#    define OIIO_CONSTEXPR17 constexpr
#    define OIIO_CONSTEXPR20 constexpr
#elif (__cplusplus >= 201703L)
#    define OIIO_CPLUSPLUS_VERSION 17
#    define OIIO_CONSTEXPR14 constexpr
#    define OIIO_CONSTEXPR17 constexpr
#    define OIIO_CONSTEXPR20 /* not constexpr before C++20 */
#elif (__cplusplus >= 201402L) || (defined(_MSC_VER) && _MSC_VER >= 1914)
#    define OIIO_CPLUSPLUS_VERSION 14
#    define OIIO_CONSTEXPR14 constexpr
#    define OIIO_CONSTEXPR17 /* not constexpr before C++17 */
#    define OIIO_CONSTEXPR20 /* not constexpr before C++20 */
#elif (__cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
#    define OIIO_CPLUSPLUS_VERSION 11
#    define OIIO_CONSTEXPR14 /* not constexpr before C++14 */
#    define OIIO_CONSTEXPR17 /* not constexpr before C++17 */
#    define OIIO_CONSTEXPR20 /* not constexpr before C++20 */
#else
#    error "This version of OIIO is meant to work only with C++11 and above"
#endif

// DEPRECATED(1.8): use C++11 constexpr
#define OIIO_CONSTEXPR constexpr
#define OIIO_CONSTEXPR_OR_CONST constexpr

// DEPRECATED(1.8): use C++11 noexcept
#define OIIO_NOEXCEPT noexcept


// In C++20 (and some compilers before that), __has_cpp_attribute can
// test for understand of [[attr]] tests.
#ifndef __has_cpp_attribute
#    define __has_cpp_attribute(x) 0
#endif

// On gcc & clang, __has_attribute can test for __attribute__((attr))
#ifndef __has_attribute
#    define __has_attribute(x) 0
#endif

// In C++17 (and some compilers before that), __has_include("blah.h") or
// __has_include(<blah.h>) can test for presence of an include file.
#ifndef __has_include
#    define __has_include(x) 0
#endif



// Detect which compiler and version we're using

// Notes:
//   __GNUC__ is defined for gcc and all clang varieties
//   __clang__ is defined for all clang varieties (generic and Apple)
//   __apple_build_version__ is only defined for Apple clang
//   __INTEL_COMPILER is defined only for icc
//   _MSC_VER is defined for MSVS compiler (not gcc/clang/icc even on Windows)
//   _WIN32 is defined on Windows regardless of compiler
//   __CUDACC__ is defined for nvcc and clang during Cuda compilation.


// Define OIIO_GNUC_VERSION to hold an encoded gcc version (e.g. 40802 for
// 4.8.2), or 0 if not a GCC release. N.B.: This will be 0 for clang.
#if defined(__GNUC__) && !defined(__clang__)
#  define OIIO_GNUC_VERSION (10000*__GNUC__ + 100*__GNUC_MINOR__ + __GNUC_PATCHLEVEL__)
#else
#  define OIIO_GNUC_VERSION 0
#endif

// Define OIIO_CLANG_VERSION to hold an encoded generic Clang version (e.g.
// 30402 for clang 3.4.2), or 0 if not a generic Clang release.
// N.B. This will be 0 for the clang Apple distributes (which has different
// version numbers entirely).
#if defined(__clang__) && !defined(__apple_build_version__)
#  define OIIO_CLANG_VERSION (10000*__clang_major__ + 100*__clang_minor__ + __clang_patchlevel__)
#else
#  define OIIO_CLANG_VERSION 0
#endif

// Define OIIO_APPLE_CLANG_VERSION to hold an encoded Apple Clang version
// (e.g. 70002 for clang 7.0.2), or 0 if not an Apple Clang release.
#if defined(__clang__) && defined(__apple_build_version__)
#  define OIIO_APPLE_CLANG_VERSION (10000*__clang_major__ + 100*__clang_minor__ + __clang_patchlevel__)
#else
#  define OIIO_APPLE_CLANG_VERSION 0
#endif

// Define OIIO_INTEL_COMPILER_VERSION to hold an encoded Intel compiler
// version (e.g. 1900), or 0 if not an Intel compiler.
#if defined(__INTEL_COMPILER)
#  define OIIO_INTEL_COMPILER_VERSION __INTEL_COMPILER
#else
#  define OIIO_INTEL_COMPILER_VERSION 0
#endif

// Intel's compiler on OSX may still define __clang__ and we have need to
// know when using a true clang compiler.
#if !defined(__INTEL_COMPILER) && defined(__clang__)
#  define OIIO_NON_INTEL_CLANG  __clang__
#else
#  define OIIO_NON_INTEL_CLANG  0
#endif

// Tests for MSVS versions, always 0 if not MSVS at all.
#if defined(_MSC_VER)
#  if _MSC_VER < 1900
#    error "This version of OIIO is meant to work only with Visual Studio 2015 or later"
#  endif
#  define OIIO_MSVS_AT_LEAST_2013 (_MSC_VER >= 1800)
#  define OIIO_MSVS_BEFORE_2013   (_MSC_VER <  1800)
#  define OIIO_MSVS_AT_LEAST_2015 (_MSC_VER >= 1900)
#  define OIIO_MSVS_BEFORE_2015   (_MSC_VER <  1900)
#  define OIIO_MSVS_AT_LEAST_2017 (_MSC_VER >= 1910)
#  define OIIO_MSVS_BEFORE_2017   (_MSC_VER <  1910)
#else
#  define OIIO_MSVS_AT_LEAST_2013 0
#  define OIIO_MSVS_BEFORE_2013   0
#  define OIIO_MSVS_AT_LEAST_2015 0
#  define OIIO_MSVS_BEFORE_2015   0
#  define OIIO_MSVS_AT_LEAST_2017 0
#  define OIIO_MSVS_BEFORE_2017   0
#endif


// Pragma control
//
// OIIO_PRAGMA(x)  make a pragma for *unquoted* x
// OIIO_PRAGMA_WARNING_PUSH/POP -- push/pop warning state
// OIIO_VISIBILITY_PUSH/POP -- push/pop symbol visibility state
// OIIO_GCC_PRAGMA(x) -- pragma on gcc/clang/icc only
// OIIO_CLANG_PRAGMA(x) -- pragma on clang only (not gcc or icc)
// OIIO_MSVS_PRAGMA(x) -- pragma on MSVS only

// Generic pragma definition
#if defined(_MSC_VER)
    // Of couse MS does it in a quirky way
    #define OIIO_PRAGMA(UnQuotedPragma) __pragma(UnQuotedPragma)
#else
    // All other compilers seem to support C99 _Pragma
    #define OIIO_PRAGMA(UnQuotedPragma) _Pragma(#UnQuotedPragma)
#endif

#if defined(__GNUC__) /* gcc, clang, icc */
#    define OIIO_PRAGMA_WARNING_PUSH    OIIO_PRAGMA(GCC diagnostic push)
#    define OIIO_PRAGMA_WARNING_POP     OIIO_PRAGMA(GCC diagnostic pop)
#    define OIIO_PRAGMA_VISIBILITY_PUSH OIIO_PRAGMA(GCC visibility push(default))
#    define OIIO_PRAGMA_VISIBILITY_POP  OIIO_PRAGMA(GCC visibility pop)
#    define OIIO_GCC_PRAGMA(UnQuotedPragma) OIIO_PRAGMA(UnQuotedPragma)
#    if defined(__clang__)
#        define OIIO_CLANG_PRAGMA(UnQuotedPragma) OIIO_PRAGMA(UnQuotedPragma)
#    else
#        define OIIO_CLANG_PRAGMA(UnQuotedPragma)
#    endif
#    define OIIO_MSVS_PRAGMA(UnQuotedPragma)
#elif defined(_MSC_VER)
#    define OIIO_PRAGMA_WARNING_PUSH __pragma(warning(push))
#    define OIIO_PRAGMA_WARNING_POP  __pragma(warning(pop))
#    define OIIO_PRAGMA_VISIBILITY_PUSH /* N/A on MSVS */
#    define OIIO_PRAGMA_VISIBILITY_POP  /* N/A on MSVS */
#    define OIIO_GCC_PRAGMA(UnQuotedPragma)
#    define OIIO_CLANG_PRAGMA(UnQuotedPragma)
#    define OIIO_MSVS_PRAGMA(UnQuotedPragma) OIIO_PRAGMA(UnQuotedPragma)
#else
#    define OIIO_PRAGMA_WARNING_PUSH
#    define OIIO_PRAGMA_WARNING_POP
#    define OIIO_PRAGMA_VISIBILITY_PUSH
#    define OIIO_PRAGMA_VISIBILITY_POP
#    define OIIO_GCC_PRAGMA(UnQuotedPragma)
#    define OIIO_CLANG_PRAGMA(UnQuotedPragma)
#    define OIIO_MSVS_PRAGMA(UnQuotedPragma)
#endif



/// allocates smallish stack memory, equivalent of C99 type var_name[size]
#if defined(__GNUC__)
#    define OIIO_ALLOCA(type, size) ((size) != 0 ? ((type*)__builtin_alloca((size) * sizeof(type))) : nullptr)
#else
#    define OIIO_ALLOCA(type, size) ((size) != 0 ? ((type*)alloca((size) * sizeof(type))) : nullptr)
#endif

/// Deprecated (for namespace pollution reasons)
#define ALLOCA(type, size) OIIO_ALLOCA(type, size)


// Define a macro that can be used for memory alignment.
// This macro is mostly obsolete and C++11 alignas() should be preferred
// for new code.
#if defined(__GNUC__) || __has_attribute(aligned)
#    define OIIO_ALIGN(size) __attribute__((aligned(size)))
#elif defined(_MSC_VER)
#    define OIIO_ALIGN(size) __declspec(align(size))
#elif defined(__INTEL_COMPILER)
#    define OIIO_ALIGN(size) __declspec(align((size)))
#else
#    define OIIO_ALIGN(size) alignas(size)
#endif

// Cache line size is 64 on all modern x86 CPUs. If this changes or we
// anticipate ports to other architectures, we'll need to change this.
#define OIIO_CACHE_LINE_SIZE 64

// Align the next declaration to be on its own cache line
#define OIIO_CACHE_ALIGN OIIO_ALIGN(OIIO_CACHE_LINE_SIZE)



// gcc defines a special intrinsic to use in conditionals that can speed
// up extremely performance-critical spots if the conditional usually
// (or rarely) is true.  You use it by replacing
//     if (x) ...
// with
//     if (OIIO_LIKELY(x)) ...     // if you think x will usually be true
// or
//     if (OIIO_UNLIKELY(x)) ...   // if you think x will rarely be true
// Caveat: Programmers are notoriously bad at guessing this, so it
// should be used only with thorough benchmarking.
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#    define OIIO_LIKELY(x) (__builtin_expect(bool(x), true))
#    define OIIO_UNLIKELY(x) (__builtin_expect(bool(x), false))
#else
#    define OIIO_LIKELY(x) (x)
#    define OIIO_UNLIKELY(x) (x)
#endif


// OIIO_FORCELINE is a function attribute that attempts to make the function
// always inline. On many compilers regular 'inline' is only advisory. Put
// this attribute before the function return type, just like you would use
// 'inline'.
#if defined(__CUDACC__)
#    define OIIO_FORCEINLINE __inline__
#elif defined(__GNUC__) || defined(__clang__) || __has_attribute(always_inline)
#    define OIIO_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
#    define OIIO_FORCEINLINE __forceinline
#else
#    define OIIO_FORCEINLINE inline
#endif

// OIIO_PURE_FUNC is a function attribute that assures the compiler that the
// function does not write to any non-local memory other than its return
// value and has no side effects. This can ebable additional compiler
// optimizations by knowing that calling the function cannot possibly alter
// any other memory. This declaration goes after the function declaration:
//   int blah (int arg) OIIO_PURE_FUNC;
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER) || __has_attribute(pure)
#    define OIIO_PURE_FUNC __attribute__((pure))
#elif defined(_MSC_VER)
#    define OIIO_PURE_FUNC /* seems not supported by MSVS */
#else
#    define OIIO_PURE_FUNC
#endif

// OIIO_CONST_FUNC is a function attribute that assures the compiler that
// the function does not examine (read) any values except for its arguments,
// does not write any non-local memory other than its return value, and has
// no side effects. This is even more strict than 'pure', and allows even
// more optimizations (such as eliminating multiple calls to the function
// that have the exact same argument values).
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER) || __has_attribute(const)
#    define OIIO_CONST_FUNC __attribute__((const))
#elif defined(_MSC_VER)
#    define OIIO_CONST_FUNC /* seems not supported by MSVS */
#else
#    define OIIO_CONST_FUNC
#endif

// OIIO_MAYBE_UNUSED is a function or variable attribute that assures the
// compiler that it's fine for the item to appear to be unused.
#if OIIO_CPLUSPLUS_VERSION >= 17 || __has_cpp_attribute(maybe_unused)
#    define OIIO_MAYBE_UNUSED [[maybe_unused]]
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER) || __has_attribute(unused)
#    define OIIO_MAYBE_UNUSED __attribute__((unused))
#else
#    define OIIO_MAYBE_UNUSED
#endif

// DEPRECATED(1.9) name:
#define OIIO_UNUSED_OK OIIO_MAYBE_UNUSED

// OIIO_RESTRICT is a parameter attribute that indicates a promise that the
// parameter definitely will not alias any other parameters in such a way
// that creates a data dependency. Use with caution!
#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER) || defined(__INTEL_COMPILER)
#  define OIIO_RESTRICT __restrict
#else
#  define OIIO_RESTRICT
#endif


// OIIO_DEPRECATED before a function declaration marks it as deprecated in
// a way that will generate compile warnings if it is called.
#if OIIO_CPLUSPLUS_VERSION >= 14 || __has_cpp_attribute(deprecated)
#    define OIIO_DEPRECATED(msg) [[deprecated(msg)]]
#elif defined(__GNUC__) || defined(__clang__) || __has_attribute(deprecated)
#    define OIIO_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#    define OIIO_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#    define OIIO_DEPRECATED(msg)
#endif


// OIIO_FALLTHROUGH at the end of a `case` label's statements documents that
// he switch statement case is intentionally falling through to the code for
// the next case.
#if OIIO_CPLUSPLUS_VERSION >= 17 || __has_cpp_attribute(fallthrough)
#    define OIIO_FALLTHROUGH [[fallthrough]]
#else
#    define OIIO_FALLTHROUGH
#endif


// OIIO_NODISCARD following a function declaration documents that the
// function's return value should never be ignored.
#if OIIO_CPLUSPLUS_VERSION >= 17 || __has_cpp_attribute(nodiscard)
#    define OIIO_NODISCARD [[nodiscard]]
#else
#    define OIIO_NODISCARD
#endif


// OIIO_NO_SANITIZE_ADDRESS can be used to mark a function that you don't
// want address sanitizer to catch. Only use this if you know there are
// false positives that you can't easily get rid of.
// This should work for any clang >= 3.3 and gcc >= 4.8, which are
// guaranteed by our minimum requirements.
#if defined(__clang__) || defined(__GNUC__) || __has_attribute(no_sanitize_address)
#    define OIIO_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#    define OIIO_NO_SANITIZE_ADDRESS
#endif


// OIIO_RETURNS_NONNULL following a function declaration of a function
// indicates that the pointer returned by the function is guaranteed to
// never be nullptr.
#if defined(__clang__) || defined(__GNUC__) || __has_attribute(returns_nonnull)
#    define OIIO_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#    define OIIO_RETURNS_NONNULL
#endif


// OIIO_HOSTDEVICE is used before a function declaration to supply the
// function decorators needed when compiling for CUDA devices.
#ifdef __CUDACC__
#    define OIIO_HOSTDEVICE __host__ __device__
#else
#    define OIIO_HOSTDEVICE
#endif



// OIIO_PRETTY_FUNCTION gives a text string of the current function
// declaration.
#if defined(__PRETTY_FUNCTION__)
#    define OIIO_PRETTY_FUNCTION __PRETTY_FUNCTION__ /* gcc, clang */
#elif defined(__FUNCSIG__)
#    define OIIO_PRETTY_FUNCTION __FUNCSIG__ /* MS gotta be different */
#else
#    define OIIO_PRETTY_FUNCTION __FUNCTION__
#endif



OIIO_NAMESPACE_BEGIN

/// Class for describing endianness. Test for endianness as
/// `if (endian::native == endian::little)` or
/// `if (endian::native == endian::big)`.
/// This uses the same semantics as C++20's std::endian.
enum class endian {
#ifdef _WIN32 /* All Windows platforms are little endian */
    little = 0,
    big    = 1,
    native = little
#else /* gcc, clang, icc all define these macros */
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};


/// Return true if the architecture we are running on is little endian.
OIIO_FORCEINLINE constexpr bool
littleendian(void) noexcept
{
    return endian::native == endian::little;
}


/// Return true if the architecture we are running on is big endian.
OIIO_FORCEINLINE constexpr bool
bigendian(void) noexcept
{
    return endian::native == endian::big;
}



/// Retrieve cpuid flags into 'info'.
inline void cpuid (int info[4], int infoType, int extra)
{
    // Implementation cribbed from Halide (http://halide-lang.org), which
    // cribbed it from ISPC (https://github.com/ispc/ispc).
#if (defined(_WIN32) || defined(__i386__) || defined(__x86_64__))
# ifdef _MSC_VER
    __cpuidex(info, infoType, extra);
# elif defined(__x86_64__)
    __asm__ __volatile__ (
        "cpuid                 \n\t"
        : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
        : "0" (infoType), "2" (extra));
# else
    __asm__ __volatile__ (
        "mov{l}\t{%%}ebx, %1  \n\t"
        "cpuid                 \n\t"
        "xchg{l}\t{%%}ebx, %1  \n\t"
        : "=a" (info[0]), "=r" (info[1]), "=c" (info[2]), "=d" (info[3])
        : "0" (infoType), "2" (extra));
# endif
#else
    info[0] = 0; info[1] = 0; info[2] = 0; info[3] = 0;
#endif
}


inline bool cpu_has_sse2  () {int i[4]; cpuid(i,1,0); return (i[3] & (1<<26)) != 0; }
inline bool cpu_has_sse3  () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<0)) != 0; }
inline bool cpu_has_ssse3 () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<9)) != 0; }
inline bool cpu_has_fma   () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<12)) != 0; }
inline bool cpu_has_sse41 () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<19)) != 0; }
inline bool cpu_has_sse42 () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<20)) != 0; }
inline bool cpu_has_popcnt() {int i[4]; cpuid(i,1,0); return (i[2] & (1<<23)) != 0; }
inline bool cpu_has_avx   () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<28)) != 0; }
inline bool cpu_has_f16c  () {int i[4]; cpuid(i,1,0); return (i[2] & (1<<29)) != 0; }
inline bool cpu_has_rdrand() {int i[4]; cpuid(i,1,0); return (i[2] & (1<<30)) != 0; }
inline bool cpu_has_avx2  () {int i[4]; cpuid(i,7,0); return (i[1] & (1<<5)) != 0; }
inline bool cpu_has_avx512f() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<16)) != 0; }
inline bool cpu_has_avx512dq() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<17)) != 0; }
inline bool cpu_has_avx512ifma() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<21)) != 0; }
inline bool cpu_has_avx512pf() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<26)) != 0; }
inline bool cpu_has_avx512er() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<27)) != 0; }
inline bool cpu_has_avx512cd() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<28)) != 0; }
inline bool cpu_has_avx512bw() {int i[4]; cpuid(i,7,0); return (i[1] & (1<<30)) != 0; }
inline bool cpu_has_avx512vl() {int i[4]; cpuid(i,7,0); return (i[1] & (0x80000000 /*1<<31*/)) != 0; }

// portable aligned malloc
void* aligned_malloc(std::size_t size, std::size_t align);
void  aligned_free(void* ptr);

// basic wrappers to new/delete over-aligned types since this isn't guaranteed to be supported until C++17
template <typename T, class... Args>
inline T* aligned_new(Args&&... args) {
    static_assert(alignof(T) > alignof(void*), "Type doesn't seem to be over-aligned, aligned_new is not required");
    void* ptr = aligned_malloc(sizeof(T), alignof(T));
    return ptr ? new (ptr) T(std::forward<Args>(args)...) : nullptr;
}

template <typename T>
inline void aligned_delete(T* t) {
    if (t) {
        t->~T();
        aligned_free(t);
    }
}



#if OIIO_CPLUSPLUS_VERSION >= 14
    using std::enable_if_t;    // Use C++14 std::enable_if_t
#else
    // Define enable_if_t for C++11
    template <bool B, class T = void>
    using enable_if_t = typename std::enable_if<B, T>::type;
#endif

// An enable_if helper to be used in template parameters which results in
// much shorter symbols: https://godbolt.org/z/sWw4vP
// Borrowed from fmtlib.
#define OIIO_ENABLE_IF(...) OIIO::enable_if_t<(__VA_ARGS__), int> = 0


OIIO_NAMESPACE_END
