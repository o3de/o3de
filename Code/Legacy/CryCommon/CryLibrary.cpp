/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <CryCommon/CryLibrary.h>

#if !defined(AZ_RESTRICTED_PLATFORM) && defined(WIN32)

HMODULE CryLoadLibrary(const char* libName)
{
    HMODULE module = ::LoadLibraryA(libName);
    if (module != NULL)
    {
        // We need to inject the environment first thing so that allocators are available immediately
        InjectEnvironmentFunction injectEnv = reinterpret_cast<InjectEnvironmentFunction>(::GetProcAddress(module, INJECT_ENVIRONMENT_FUNCTION));
        if (injectEnv)
        {
            auto env = AZ::Environment::GetInstance();
            injectEnv(env);
        }
    }

    return module;
}

// Cry code seems to have used void* as their abstraction for HMODULE across
// platforms.
bool CryFreeLibrary(void* lib)
{
    if (lib != NULL)
    {
        DetachEnvironmentFunction detachEnv = reinterpret_cast<DetachEnvironmentFunction>(::GetProcAddress((HMODULE)lib, DETACH_ENVIRONMENT_FUNCTION));
        if (detachEnv)
        {
            detachEnv();
        }
        return ::FreeLibrary((HMODULE)lib) != FALSE;
    }
    return false;
}

#endif
