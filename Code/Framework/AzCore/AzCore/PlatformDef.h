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

#include "PlatformRestrictedFileDef.h"

#if defined(__clang__)
    #define AZ_COMPILER_CLANG   __clang_major__
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

#if defined(AZ_COMPILER_CLANG)
    #define AZ_DLL_EXPORT               AZ_TRAIT_OS_DLL_EXPORT_CLANG
    #define AZ_DLL_IMPORT               AZ_TRAIT_OS_DLL_IMPORT_CLANG
#elif defined(AZ_COMPILER_MSVC)
    #define AZ_DLL_EXPORT               __declspec(dllexport)
    #define AZ_DLL_IMPORT               __declspec(dllimport)
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

#define AZ_STRINGIZE_I(text) #text

#if defined(AZ_COMPILER_MSVC)
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_A((text))
#    define AZ_STRINGIZE_A(arg) AZ_STRINGIZE_I arg
#else
#    define AZ_STRINGIZE(text) AZ_STRINGIZE_I(text)
#endif

#if defined(AZ_COMPILER_MSVC)

/// Disables a warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(_msvcOption, __)    \
    __pragma(warning(push))                         \
    __pragma(warning(disable : _msvcOption))

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                      \
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
#elif defined(AZ_COMPILER_CLANG)

/// Disables a single warning using push style. For use matched with an AZ_POP_WARNING
#define AZ_PUSH_DISABLE_WARNING(__, _clangOption)           \
    _Pragma("clang diagnostic push")                        \
    _Pragma(AZ_STRINGIZE(clang diagnostic ignored _clangOption))

/// Pops the warning stack. For use matched with an AZ_PUSH_DISABLE_WARNING
#define AZ_POP_DISABLE_WARNING                              \
    _Pragma("clang diagnostic pop")

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

// We need to define AZ_DEBUG_BUILD in debug mode. We can also define it in debug optimized mode (left up to the user).
// note that _DEBUG is not in fact always defined on all platforms, and only AZ_DEBUG_BUILD should be relied on.
#if !defined(AZ_DEBUG_BUILD) && defined(_DEBUG)
#   define AZ_DEBUG_BUILD
#endif

#if !defined(AZ_PROFILE_BUILD) && defined(_PROFILE)
#   define AZ_PROFILE_BUILD
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
