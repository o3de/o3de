/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "System.h"
#include <AZCrySystemInitLogSink.h>
#include "DebugCallStack.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define DLLMAIN_CPP_SECTION_1 1
#define DLLMAIN_CPP_SECTION_2 2
#define DLLMAIN_CPP_SECTION_3 3
#define DLLMAIN_CPP_SECTION_4 4
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION DLLMAIN_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(DllMain_cpp)
#endif

// For lua debugger
//#include <malloc.h>

HMODULE gDLLHandle = NULL;

#if !defined(AZ_MONOLITHIC_BUILD) && defined(AZ_HAS_DLL_SUPPORT) && AZ_TRAIT_LEGACY_CRYSYSTEM_DEFINE_DLLMAIN
AZ_PUSH_DISABLE_WARNING(4447, "-Wunknown-warning-option")
BOOL APIENTRY DllMain(HANDLE hModule,
    DWORD  ul_reason_for_call,
    [[maybe_unused]] LPVOID lpReserved
    )
AZ_POP_DISABLE_WARNING
{
    PREVENT_MODULE_AND_ENVIRONMENT_SYMBOL_STRIPPING

    gDLLHandle = (HMODULE)hModule;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:


        break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    //  int sbh = _set_sbh_threshold(1016);

    return TRUE;
}
#endif

extern "C"
{
CRYSYSTEM_API ISystem* CreateSystemInterface(const SSystemInitParams& startupParams)
{
    CSystem* pSystem = NULL;

    // We must attach to the environment prior to allocating CSystem, as opposed to waiting
    // for ModuleInitISystem(), because the log message sink uses buses.
    // Environment should have been attached via InjectEnvironment
    AZ_Assert(AZ::Environment::IsReady(), "Environment is not attached, must be attached before CreateSystemInterface can be called");

    pSystem = new CSystem(startupParams.pSharedEnvironment);
    ModuleInitISystem(pSystem, "CrySystem");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION DLLMAIN_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(DllMain_cpp)
#endif

       // the earliest point the system exists - w2e tell the callback
    if (startupParams.pUserCallback)
    {
        startupParams.pUserCallback->OnSystemConnect(pSystem);
    }

#if defined(WIN32)
    // Environment Variable to signal we don't want to override our exception handler - our crash report system will set this
    auto envVar = AZ::Environment::FindVariable<bool>("ExceptionHandlerIsSet");
    const bool handlerIsSet = (envVar && *envVar);
    if (!handlerIsSet)
    {
        ((DebugCallStack*)IDebugCallStack::instance())->installErrorHandler(pSystem);
    }
#endif

    bool retVal = false;
    {
        AZ::Debug::StartupLogSinkReporter<AZ::Debug::CrySystemInitLogSink> initLogSink;
        retVal = pSystem->Init(startupParams);
        if (!retVal)
        {
            initLogSink.GetContainedLogSink().SetFatalMessageBox();
        }
    }
    if (!retVal)
    {
        delete pSystem;
        gEnv = nullptr;

        return 0;
    }

    return pSystem;
}
};

