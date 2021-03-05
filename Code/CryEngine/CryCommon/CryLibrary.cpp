/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
