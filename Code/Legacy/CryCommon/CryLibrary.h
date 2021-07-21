/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


/*!
    CryLibrary

    Convenience-Macros which abstract the use of DLLs/shared libraries in a platform independent way.
    A short explanation of the different macros follows:

    CrySharedLibrarySupported:
        This macro can be used to test if the current active platform supports shared library calls. The default
        value is false. This gets redefined if a certain platform (WIN32 or LINUX) is desired.

    CrySharedLibraryPrefix:
        The default prefix which will get prepended to library names in calls to CryLoadLibraryDefName
        (see below).

    CrySharedLibraryExtension:
        The default extension which will get appended to library names in calls to CryLoadLibraryDefName
        (see below).

    CryLoadLibrary(libName):
        Loads a shared library.

    CryLoadLibraryDefName(libName):
        Loads a shared library. The platform-specific default library prefix and extension are appended to the libName.
        This allows writing of somewhat platform-independent library loading code and is therefore the function
        which should be used most of the time, unless some special extensions are used (e.g. for plugins).

    CryGetProcAddress(libHandle, procName):
        Import function from the library presented by libHandle.

    CryFreeLibrary(libHandle):
        Unload the library presented by libHandle.

    HISTORY:
        03.03.2004 MarcoK
            - initial version
            - added to CryPlatform
*/

#include <stdio.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Module/Environment.h>

#define INJECT_ENVIRONMENT_FUNCTION "InjectEnvironment"
#define DETACH_ENVIRONMENT_FUNCTION "DetachEnvironment"
using InjectEnvironmentFunction = void(*)(void*);
using DetachEnvironmentFunction = void(*)();

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryLibrary_h)
#elif defined(WIN32)
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <CryWindows.h>

    HMODULE CryLoadLibrary(const char* libName);
    
    // Cry code seems to have used void* as their abstraction for HMODULE across
    // platforms.
    bool CryFreeLibrary(void* lib);

    #define CRYLIBRARY_H_TRAIT_USE_WINDOWS_DLL 1
#elif ((defined(LINUX) || AZ_TRAIT_OS_PLATFORM_APPLE))
    #include <dlfcn.h>
    #include <stdlib.h>
    #include <libgen.h>
    #include "platform.h"
    #include <AzCore/Debug/Trace.h>

// for compatibility with code written for windows
    #define CrySharedLibrarySupported true
    #define CrySharedLibraryPrefix "lib"
#if AZ_TRAIT_OS_PLATFORM_APPLE
    #include <mach-o/dyld.h>
    #define CrySharedLibraryExtension ".dylib"
#else
    #define CrySharedLibraryExtension ".so"
#endif

    #define CryGetProcAddress(libHandle, procName) ::dlsym(libHandle, procName)
    #define HMODULE void*
static const char* gEnvName("MODULE_PATH");

static const char* GetModulePath()
{
    return getenv(gEnvName);
}

static void SetModulePath(const char* pModulePath)
{
    setenv(gEnvName, pModulePath ? pModulePath : "", true);
}

// bInModulePath is only ever set to false in RC, because rc needs to load dlls from a $PATH that
// it has modified to include ..
static HMODULE CryLoadLibrary(const char* libName, bool bLazy = false, bool bInModulePath = true)
{
    const char* libPath = nullptr;
    char pathBuffer[MAX_PATH] = {0};
    
    libPath = libName;

#if !defined(AZ_PLATFORM_ANDROID)
    if (bInModulePath)
    {
        char exePath[MAX_PATH + 1] = { 0 };
        const char* modulePath = GetModulePath();
        if (!modulePath)
        {
            modulePath = ".";
            #if defined(LINUX)
                int len = readlink("/proc/self/exe", exePath, MAX_PATH);
                if (len != -1)
                {
                    exePath[len] = 0;
                    modulePath = dirname(exePath);
                }
            #elif AZ_TRAIT_OS_PLATFORM_APPLE
                uint32_t bufsize = MAX_PATH;
                if (_NSGetExecutablePath(exePath, &bufsize) == 0)
                {
                    exePath[bufsize] = 0;
                    modulePath = dirname(exePath);
                }
            #endif
        }
        sprintf_s(pathBuffer, "%s/%s", modulePath, libName);
        libPath = pathBuffer;
    }
#endif

    HMODULE module;
    #if defined(LINUX) && !defined(ANDROID)
        module = ::dlopen(libPath, (bLazy ? RTLD_LAZY : RTLD_NOW) | RTLD_DEEPBIND);
    #else
        module = ::dlopen(libPath, bLazy ? RTLD_LAZY : RTLD_NOW);
    #endif
    AZ_Warning("LMBR", module, "Can't load library [%s]: %s", libName, dlerror());

    if (module)
    {
        // We need to inject the environment first thing so that allocators are available immediately
        InjectEnvironmentFunction injectEnv = reinterpret_cast<InjectEnvironmentFunction>(CryGetProcAddress(module, INJECT_ENVIRONMENT_FUNCTION));
        if (injectEnv)
        {
            injectEnv(AZ::Environment::GetInstance());
        }
    }

    return module;
}

static bool CryFreeLibrary(void* lib)
{
    if (lib)
    {
        DetachEnvironmentFunction detachEnv = reinterpret_cast<DetachEnvironmentFunction>(CryGetProcAddress(lib, DETACH_ENVIRONMENT_FUNCTION));
        if (detachEnv)
        {
            detachEnv();
        }
        return (::dlclose(lib) == 0);
    }
    return false;
}
#endif

#if CRYLIBRARY_H_TRAIT_USE_WINDOWS_DLL
#define CrySharedLibrarySupported true
#define CrySharedLibraryPrefix ""
#define CrySharedLibraryExtension ".dll"
#define CryGetProcAddress(libHandle, procName) ::GetProcAddress((HMODULE)(libHandle), procName)
#elif !defined(CrySharedLibrarySupported)
#define CrySharedLibrarySupported false
#define CrySharedLibraryPrefix ""
#define CrySharedLibraryExtension ""
#define CryLoadLibrary(libName) NULL
#define CryGetProcAddress(libHandle, procName) NULL
#define CryFreeLibrary(libHandle)
#define GetModuleHandle(x)  0
#endif
#define CryLibraryDefName(libName) CrySharedLibraryPrefix libName CrySharedLibraryExtension
#define CryLoadLibraryDefName(libName) CryLoadLibrary(CryLibraryDefName(libName))
