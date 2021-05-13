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

#include "CrySystem_precompiled.h"
#include "System.h"
#include <AZCrySystemInitLogSink.h>

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

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_System
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        case ESYSTEM_EVENT_LEVEL_LOAD_END:
        {
            CryCleanup();
            break;
        }

        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            CryCleanup();
            break;
        }
        }
    }
};

static CSystemEventListner_System g_system_event_listener_system;

static AZ::EnvironmentVariable<IMemoryManager*> s_cryMemoryManager;


// Force the CryMemoryManager into the AZ::Environment for exposure to other DLLs
void ExportCryMemoryManager()
{
    IMemoryManager* cryMemoryManager = nullptr;
    CryGetIMemoryManagerInterface((void**)&cryMemoryManager);
    AZ_Assert(cryMemoryManager, "Unable to resolve CryMemoryManager");
    s_cryMemoryManager = AZ::Environment::CreateVariable<IMemoryManager*>("CryIMemoryManagerInterface", cryMemoryManager);
}

extern "C"
{
CRYSYSTEM_API ISystem* CreateSystemInterface(const SSystemInitParams& startupParams)
{
    CSystem* pSystem = NULL;

    // We must attach to the environment prior to allocating CSystem, as opposed to waiting
    // for ModuleInitISystem(), because the log message sink uses buses.
    // Environment should have been attached via InjectEnvironment
    AZ_Assert(AZ::Environment::IsReady(), "Environment is not attached, must be attached before CreateSystemInterface can be called");

    ExportCryMemoryManager();

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

    pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_system);

    return pSystem;
}
};

