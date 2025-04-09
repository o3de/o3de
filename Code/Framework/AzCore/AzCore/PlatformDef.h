/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

//////////////////////////////////////////////////////////////////////////
// Platforms

#include <AzCore/variadic.h>

#include "PlatformRestrictedFileDef.h"

#if defined(__clang__)
    #define AZ_COMPILER_CLANG   __clang_major__
#elif defined(__GNUC__)
    //  Assign AZ_COMPILER_GCC to a number that represents the major+minor (2 digits) + path level (2 digits)  i.e. 3.2.0 == 30200
    #define AZ_COMPILER_GCC     (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
    #define AZ_COMPILER_MSVC    _MSC_VER
#else
#   error This compiler is not supported
#endif

#include <AzCore/AzCore_Traits_Platform.h>

//////////////////////////////////////////////////////////////////////////

#define AZ_INLINE                       inline
#define AZ_THREAD_LOCAL                 AZ_TRAIT_COMPILER_THREAD_LOCAL
#define AZ_DYNAMIC_LIBRARY_PREFIX       AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX
#define AZ_DYNAMIC_LIBRARY_EXTENSION    AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION

#if defined(AZ_COMPILER_CLANG) || defined(AZ_COMPILER_GCC)
    #define AZ_DLL_EXPORT               AZ_TRAIT_OS_DLL_EXPORT_CLANG
    #define AZ_DLL_IMPORT               AZ_TRAIT_OS_DLL_IMPORT_CLANG
    #define AZ_DLL_EXPORT_EXTERN        AZ_TRAIT_OS_DLL_EXPORT_EXTERN_CLANG
    #define AZ_DLL_IMPORT_EXTERN        AZ_TRAIT_OS_DLL_IMPORT_EXTERN_CLANG
#elif defined(AZ_COMPILER_MSVC)
    #define AZ_DLL_EXPORT               __declspec(dllexport)
    #define AZ_DLL_IMPORT               __declspec(dllimport)
    #define AZ_DLL_EXPORT_EXTERN
    #define AZ_DLL_IMPORT_EXTERN
#endif

// These defines will be deprecated in the future with LY-99152
#if defined(AZ_PLATFORM_MAC)
    #define AZ_PLATFORM_APPLE_OSX
#endif
#if defined(AZ_PLATFORM_IOS)
    #define AZ_PLATFORM_APPLE_IOS
#endif
#if AZ_TRAIT_OS_PLATFORM_APPLE
    #define AZ_PLATFORM_APPLE
#endif

#if AZ_TRAIT_OS_HAS_DLL_SUPPORT
    #define AZ_HAS_DLL_SUPPORT
#endif

/// Deprecated macro
#define AZ_DEPRECATED(_decl, _message) [[deprecated(_message)]] _decl
#define AZ_DEPRECATED_MESSAGE(_message) [[deprecated(_message)]]

#define AZ_STRINGIZE_I(text) #text

#if defined(AZ_COMPILER_MSVC)
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_A((text))
#    define AZ_STRINGIZE_A(arg) AZ_STRINGIZE_I arg
#else
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_I(text)
#endif

#if defined(AZ_COMPILER_MSVC)

/// Disables a warning using push style. For use matched with an AZ_POP_WARNING

// Compiler specific AZ_PUSH_DISABLE_WARNING
#define AZ_PUSH_DISABLE_WARNING_MSVC(_msvcOption)       \
    __pragma(warning(push))                             \
    __pragma(warning(disable : _msvcOption))
#define AZ_PUSH_DISABLE_WARNING_CLANG(_clangOption)
#define AZ_PUSH_DISABLE_WARNING_GCC(_gccOption)

/// Compiler specific AZ_POP_DISABLE_WARNING. This needs to be matched with the compiler specific AZ_PUSH_DISABLE_WARNINGs
#define AZ_POP_DISABLE_WARNING_CLANG
#define AZ_POP_DISABLE_WARNING_MSVC                     \
    __pragma(warning(pop))
#define AZ_POP_DISABLE_WARNING_GCC


// Variadic definitions for AZ_PUSH_DISABLE_WARNING for the current compiler
#define AZ_PUSH_DISABLE_WARNING_1(_msvcOption)          \
    __pragma(warning(push))                             \
    __pragma(warning(disable : _msvcOption))

#define AZ_PUSH_DISABLE_WARNING_2(_msvcOption, _2)      \
    __pragma(warning(push))                             \
    __pragma(warning(disable : _msvcOption))

#define AZ_PUSH_DISABLE_WARNING_3(_msvcOption, _2, _3)  \
    __pragma(warning(push))                             \
    __pragma(warning(disable : _msvcOption))

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                          \
    __pragma(warning(pop))


/// Classes in Editor Sandbox and Tools which dll export there interfaces, but inherits from a base class that doesn't dll export
/// will trigger a warning
#define AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING AZ_PUSH_DISABLE_WARNING(4275, "-Wunknown-warning-option")
#define AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING AZ_POP_DISABLE_WARNING
/// Disables a warning for dll exported classes which has non dll-exported members as this can cause ABI issues if the layout of those classes differs between dlls.
/// QT classes such as QList, QString, QMap, etc... and Cry Math classes such Vec3, Quat, Color don't dllexport their interfaces
/// Therefore this macro can be used to disable the warning when caused by 3rdParty libraries
#define AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#define AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING AZ_POP_DISABLE_WARNING

#   define AZ_FORCE_INLINE  __forceinline

/// Pointer will be aliased.
#   define AZ_MAY_ALIAS
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __FUNCSIG__

//////////////////////////////////////////////////////////////////////////
#elif defined(AZ_COMPILER_CLANG) || defined(AZ_COMPILER_GCC)

#if defined(AZ_COMPILER_CLANG)

/// Disables a single warning using push style. For use matched with an AZ_POP_WARNING

// Compiler specific AZ_PUSH_DISABLE_WARNING
#define AZ_PUSH_DISABLE_WARNING_CLANG(_clangOption)  \
    _Pragma("clang diagnostic push")                 \
    _Pragma(AZ_STRINGIZE(clang diagnostic ignored _clangOption))
#define AZ_PUSH_DISABLE_WARNING_MSVC(_msvcOption)
#define AZ_PUSH_DISABLE_WARNING_GCC(_gccOption)

/// Compiler specific AZ_POP_DISABLE_WARNING. This needs to be matched with the compiler specific AZ_PUSH_DISABLE_WARNINGs
#define AZ_POP_DISABLE_WARNING_CLANG                       \
    _Pragma("clang diagnostic pop")
#define AZ_POP_DISABLE_WARNING_MSVC
#define AZ_POP_DISABLE_WARNING_GCC

// Variadic definitions for AZ_PUSH_DISABLE_WARNING for the current compiler
#define AZ_PUSH_DISABLE_WARNING_1(_1)
#define AZ_PUSH_DISABLE_WARNING_2(_1, _clangOption)     AZ_PUSH_DISABLE_WARNING_CLANG(_clangOption)
#define AZ_PUSH_DISABLE_WARNING_3(_1, _clangOption, _2) AZ_PUSH_DISABLE_WARNING_CLANG(_clangOption)

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                              \
    _Pragma("clang diagnostic pop")

#else

/// Disables a single warning using push style. For use matched with an AZ_POP_WARNING

// Compiler specific AZ_PUSH_DISABLE_WARNING
#define AZ_PUSH_DISABLE_WARNING_GCC(_gccOption)             \
    _Pragma("GCC diagnostic push")                          \
    _Pragma(AZ_STRINGIZE(GCC diagnostic ignored _gccOption))
#define AZ_PUSH_DISABLE_WARNING_CLANG(_clangOption)
#define AZ_PUSH_DISABLE_WARNING_MSVC(_msvcOption)

/// Compiler specific AZ_POP_DISABLE_WARNING. This needs to be matched with the compiler specific AZ_PUSH_DISABLE_WARNINGs
#define AZ_POP_DISABLE_WARNING_CLANG
#define AZ_POP_DISABLE_WARNING_MSVC
#define AZ_POP_DISABLE_WARNING_GCC                          \
    _Pragma("GCC diagnostic pop")

// Variadic definitions for AZ_PUSH_DISABLE_WARNING for the current compiler
#define AZ_PUSH_DISABLE_WARNING_1(_1)
#define AZ_PUSH_DISABLE_WARNING_2(_1, _2)
#define AZ_PUSH_DISABLE_WARNING_3(_1, _2, _gccOption)   AZ_PUSH_DISABLE_WARNING_GCC(_gccOption)

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING
    _Pragma("GCC diagnostic pop")

#endif // defined(AZ_COMPILER_CLANG)

#define AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
#define AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
#define AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#define AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

#   define AZ_FORCE_INLINE  inline

/// Pointer will be aliased.
#   define AZ_MAY_ALIAS __attribute__((__may_alias__))
/// Function signature macro
#   define AZ_FUNCTION_SIGNATURE    __PRETTY_FUNCTION__

#else
    #error Compiler not supported
#endif

#define AZ_PUSH_DISABLE_WARNING(...) AZ_MACRO_SPECIALIZE(AZ_PUSH_DISABLE_WARNING_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

// We need to define AZ_DEBUG_BUILD in debug mode. We can also define it in debug optimized mode (left up to the user).
// note that _DEBUG is not in fact always defined on all platforms, and only AZ_DEBUG_BUILD should be relied on.
#if !defined(AZ_DEBUG_BUILD) && defined(_DEBUG)
#   define AZ_DEBUG_BUILD
#endif

#if !defined(AZ_PROFILE_BUILD) && defined(_PROFILE)
#   define AZ_PROFILE_BUILD
#endif

#if !defined(AZ_RELEASE_BUILD) && defined(_RELEASE)
#   define AZ_RELEASE_BUILD
#endif

// note that many include ONLY PlatformDef.h and not base.h, so flags such as below need to be here.
// AZ_ENABLE_DEBUG_TOOLS - turns on and off interaction with the debugger.
// Things like being able to check whether the current process is being debugged, to issue a "debug break" command, etc.
#if defined(AZ_DEBUG_BUILD) && !defined(AZ_ENABLE_DEBUG_TOOLS)
#   define AZ_ENABLE_DEBUG_TOOLS
#endif

// AZ_ENABLE_TRACE_ASSERTS - toggles display of native UI assert dialogs with ignore/break options
#define AZ_ENABLE_TRACE_ASSERTS 1

// AZ_ENABLE_TRACING - turns on and off the availability of AZ_TracePrintf / AZ_Assert / AZ_Error / AZ_Warning
#if (defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)) && !defined(AZ_ENABLE_TRACING)
#   define AZ_ENABLE_TRACING
#endif

#if !defined(AZ_COMMAND_LINE_LEN)
#   define AZ_COMMAND_LINE_LEN 2048
#endif

#include <type_traits>
#include <utility>
#include <memory>
#include <cstdint>
#include <cstring>

// First check if the feature if is_constant_evaluated is available via the feature test macro
// https://en.cppreference.com/w/User:D41D8CD98F/feature_testing_macros#C.2B.2B20
#if __cpp_lib_is_constant_evaluated
    #define az_builtin_is_constant_evaluated() std::is_constant_evaluated()
#endif

// Next check if there is a __builtin_is_constant_evaluated that can be used
// This works on MSVC 19.28+ toolsets when using C++17, as well as
// clang 9.0.0+ when using C++17.
// Finally it works on gcc 9.0+ when using C++17
#if !defined(az_builtin_is_constant_evaluated)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_is_constant_evaluated)
            #define az_builtin_is_constant_evaluated() __builtin_is_constant_evaluated()
        #define az_has_builtin_is_constant_evaluated true
        #endif
    #elif AZ_COMPILER_MSVC >= 1928
        #define az_builtin_is_constant_evaluated() __builtin_is_constant_evaluated()
        #define az_has_builtin_is_constant_evaluated true
    #elif AZ_COMPILER_GCC
        #define az_builtin_is_constant_evaluated() __builtin_is_constant_evaluated()
        #define az_has_builtin_is_constant_evaluated true
    #endif
#endif

// In this case no support for the determining whether an operation is occuring
// at compile time is supported so assume that evaluation is always occuring at compile time
// in order to make sure the "safe" operation is being performed
#if !defined(az_builtin_is_constant_evaluated)
    namespace AZ::Internal
    {
        constexpr bool builtin_is_constant_evaluated()
        {
            return true;
        }
    }
    #define az_builtin_is_constant_evaluated() AZ::Internal::builtin_is_constant_evaluated()
    #define az_has_builtin_is_constant_evaluated false
#endif

// define builtin functions used by char_traits class for efficient compile time and runtime
// operations
// The list of builtins can be found on the clang and GCC documentation pages at
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-functions

// NOTE: __builtin_memcpy and __builtin_memmove for GCC is not usable in a compile time context currently
// The following is a permalink to godbolt that checks which builtin intrinsics
// are available for GCC 10.1+ and clang 12.0.0+
// The non-short link is being used here to not rely on a URL shortener service
// https://godbolt.org/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1DIApACYAQuYukl9ZATwDKjdAGFUtAK4sGe1wAyeAyYAHI%2BAEaYxBIAHKQADqgKhE4MHt6%2BekkpjgJBIeEsUTFc8XaYDmlCBEzEBBk%2Bfly2mPZ5DDV1BAVhkdFxtrX1jVktCsM9wX3FA2UAlLaoXsTI7BzmAMzByN5YANQmm24TxMHAAPoAbniYAO5H2CYaAILPLwD0H/sAatF4NEwCn2ES8dEcDH2wQIZwYKWQwIICCYBH2dUw%2By8CiYEXoUMhTH2aBYCToGMcbCJskwqgI7zQcIINISxHxqKZEwuoPBwQUEHmhwA7FZXvsxVTGczWRN0CAQKdztdbnd9gplqsMUcACKHMxmAASbVoqH2AHUSLR0OYzEcRS9xRKJlLVQRZSA7gqjEr7vs7ngsEJ1WtDpsdQFrYbaMazRarXrbe93g7kMjWVgJiYAKwWTY2zNakzCwsFzZ2h13FN1C6ov1YLVAunZ3NZgtFwUlu1J8VffZsFjIBIATxBYNoEIUibM2yo%2BwuF2RCi5o4hEDn3LHwQufYHg/mXbFa%2BXm%2B3Q4g6YIpFVQcwADp0CimPzL2qVmsbykAF6Yfl70uTzauAC/4ArO85MIu64roePIMBcdwnru%2B6gZBm7wZg/anrWmD1hMl5YYGr63vetRPr6/qYARGrvngX4/gmrxbIBVCJgxordt8fYsKgVwYihcLATOc4LkuMGriJG6wZx3GYHubEHuJEJbuhXE8WeDbPted4PqRL5UZ%2B37zL%2BnZTkxAmgcJfFiXxcFSTxsn2uK0ESTZynSRAWE4ReZEBppxGPos3kUZp%2Bl0X%2BDEmQw%2BDMaxDlij2pz0JCfETuF07meBCnBFZR6wQljD2Q6TmKXlDAQLpb5%2BaFxkAZFQGpSBQkZZZRWoQiiUFY5mWwRWCiJe55GURV2mGfRbwRVFLGxfsPbbggrLJWZjUQTl2UwUp/ZzR18nWbNxBlb5w2XhoGmEdRtEjWFY01RN9WCWBy2iS13W7VtyE5S5G17fhB0kQFx2BYNt4hRd1WmTFDozehyAkiOMEpVdDX3V1q3OQhr1Pet0MJPtp2VQFEZGia5rEJa1onXpNEGUZ/5gwjd0WStGNof2JLo11H1Y/1Pm44d%2BzhnqkbRsTpN6nhA3BZTVU07V0VvHJ%2BzEJgBArJCGijcWk3Qr2TBZfZhZluKDJOqoLJsgrQJeGOIY6hyBBdXy1Pg%2BKivK8QkKKwolt0pdxYcIstCcJmvB%2BBwWikKgnBuNY1hXoRuqbDwpAEJofuLAA1iAmybDeWe53n%2BcAGz6JwkjByn4ecLwCggMdyeh37pBwLAMCICgqAkmSZAUBAxKkvQMTAFwCekFgNxrD8yoAPIJIwnCJzQY7RNXEAROXETBHUg5z7w6/MMQg6TxE2iVHXifEmwgiTwwtBb/XI%2BYKCwBuGItDV9wvBYCwhjAOId/4IrVQeJvzDjSSoXgmTb3IIINo5daB4AiMQTeHgsDlxhHgFg29FhUAMMABQE97jT1nu/GQggRBiHYFIEh8glBqHLroFoBgjAoGjpYfQ8Dq6QEWKgBIHQ34AFoZTalMJYawXBBS8GksQM46Z4CLAqFUZwEBXCjGaKQQI0wiglGyMkVIAgVHaNyGkXomi5itHaNUSY%2BjxhtBPhY7oxj%2BilCGN0Kxzj6gONmKUOR14JD%2B0DmXO%2BEcOD7FULEAufCC6SH2MAZAyB9hDxvFwfYEBcCEBIPHLg8xeB1y0IZUgGdMzHQDhwUupAMGFNICHMOQSq41yTinRYTdW7LAIAkcB5BKC907qEVg6xQnhMidE2J8Ts6J0wPgIg0i9D8FIaIcQlCZnUJUOoO%2B9DSB3EQQkTBxcOBB0qeXIJk9wFtNRKgGc/SIlRJiXEhJSSIAeA7v3DJWT6n1zyQgTATAsAxCfMU0p5TjpVIkZXWwdScmp3yZnbO%2BcYW5yLsUzYATqkgvBY0luTSIBIC6f3DpPd259wGLsb%2BXAzAaGOqPPA48p4zxDvPcES9KCrzvrvTekCWX70PsfBwkDz6MAIFfG%2B5csCP2flGN%2BidP7f1/mHf%2BtigHl1AcgcB6xE7QhgXfOBCCkEYHWGHNBGD35YJwXg6lRD56yDIfM6QizFDLLoSAMw%2Bhv7MJEawzVHCnzcN4ZwARrohEsIsGIiRPEpHkQ9XImxCi/BKMiq49RhRHEGN0ekTwTQk0dA8Vo6x5iBBdBGKmsYZjbG5smJm0xEwXEFtURW9xGjE2ZKWIRXxOy9lAorsEi5gyiVGHiWYG8Gh%2B3JNSZM552SGnpyhTnWFMKdn/JABUttNTQW1waY3dFSAWknNxdi6IPS2CcE7VE7twBe39v7bwcZaSpktEWZaih1rZC2toash16zNnbOKa2g5nAjmtPAfsM5ISwmXKJIwk9JKz0aGSQ8glrIthmBeaixYHyvkDF%2BSXXgAL9mBJBdXFdbyJ1ZyndOvOOzEXYeRRwMdBGdlmCRcCqjrzcmLBDSkZwkggA%3D%3D%3D

#if defined(__has_builtin)
    #if __has_builtin(__builtin_memcpy) && (!defined(AZ_COMPILER_GCC))
        #define az_has_builtin_memcpy true
    #endif
    #if __has_builtin(__builtin_wmemcpy)
        #define az_has_builtin_wmemcpy true
    #endif
    #if __has_builtin(__builtin_memmove) && (!defined(AZ_COMPILER_GCC))
        #define az_has_builtin_memmove true
    #endif
    #if __has_builtin(__builtin_wmemmove)
        #define az_has_builtin_wmemmove true
    #endif
    #if (__has_builtin(__builtin_strlen)) || defined(AZ_COMPILER_MSVC)
        #define az_has_builtin_strlen true
    #endif
    #if (__has_builtin(__builtin_wcslen)) || defined(AZ_COMPILER_MSVC)
        #define az_has_builtin_wcslen true
    #endif
    #if (__has_builtin(__builtin_char_memchr)) || defined(AZ_COMPILER_MSVC)
        #define az_has_builtin_char_memchr true
    #endif
    #if (__has_builtin(__builtin_wmemchr)) || defined(AZ_COMPILER_MSVC)
        #define az_has_builtin_wmemchr true
    #endif
    #if (__has_builtin(__builtin_memcmp) && (!defined(AZ_COMPILER_GCC))) || defined(AZ_COMPILER_MSVC)
        #define az_has_builtin_memcmp true
    #endif
    #if (__has_builtin(__builtin_wmemcmp)) || defined(AZ_COMPILER_MSVC)
        #define az_has_builtin_wmemcmp true
    #endif
#elif defined(AZ_COMPILER_MSVC)
//  MSVC doesn't support the __has_builtin macro, so instead hardcode the list of known compile time builtins
    #define az_has_builtin_strlen true
    #define az_has_builtin_wcslen true
    #define az_has_builtin_char_memchr true
    #define az_has_builtin_wmemchr true
    #define az_has_builtin_memcmp true
    #define az_has_builtin_wmemcmp true
#endif

#if !defined(az_has_builtin_memcpy)
    #define az_has_builtin_memcpy false
#endif
#if !defined(az_has_builtin_wmemcpy)
    #define az_has_builtin_wmemcpy false
#endif
#if !defined(az_has_builtin_memmove)
    #define az_has_builtin_memmove false
#endif
#if !defined(az_has_builtin_wmemmove)
    #define az_has_builtin_wmemmove false
#endif
#if !defined(az_has_builtin_strlen)
    #define az_has_builtin_strlen false
#endif
#if !defined(az_has_builtin_wcslen)
    #define az_has_builtin_wcslen false
#endif
#if !defined(az_has_builtin_char_memchr)
    #define az_has_builtin_char_memchr false
#endif
#if !defined(az_has_builtin_wmemchr)
    #define az_has_builtin_wmemchr false
#endif
#if !defined(az_has_builtin_memcmp)
    #define az_has_builtin_memcmp false
#endif
#if !defined(az_has_builtin_wmemcmp)
    #define az_has_builtin_wmemcmp false
#endif

// no unique address attribute support in C++17
#if __has_cpp_attribute(no_unique_address)
    #define AZ_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
    #define AZ_NO_UNIQUE_ADDRESS
#endif
