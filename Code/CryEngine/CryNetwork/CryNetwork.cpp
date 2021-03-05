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

// Description : dll entry point


#include "CryNetwork_precompiled.h"

#include <INetwork.h>
#include <GridMate/NetworkGridMate.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>

#include <AzCore/Module/Environment.h>

//////////////////////////////////////////////////////////////////////////
struct CSystemEventListner_Network
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            STLALLOCATOR_CLEANUP;
            break;
        }
        }
    }
};

static CSystemEventListner_Network g_system_event_listener_network;

//////////////////////////////////////////////////////////////////////////
class CEngineModule_CryNetwork
    : public IEngineModule
{
    CRYINTERFACE_SIMPLE(IEngineModule)
    CRYGENERATE_SINGLETONCLASS(CEngineModule_CryNetwork, "EngineModule_CryNetwork", 0x7dc5c3b8bb374063, 0xa29ac2d6dd718e0f)

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName() const {
        return "CryNetwork";
    };
    virtual const char* GetCategory() const { return "CryEngine"; };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SSystemGlobalEnvironment& env, [[maybe_unused]] const SSystemInitParams& initParams)
    {
        ISystem* pSystem = env.pSystem;

        AZ::AllocatorInstance<GridMate::GridMateAllocator>::Create();
        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

        CNetwork* pNetwork = new CNetwork;

        int ncpu = env.pi.numCoresAvailableToProcess;
        if (!pNetwork->Init(ncpu))
        {
            pNetwork->Release();
            return false;
        }
        pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_network);
        env.pNetwork = pNetwork;

        CryLogAlways("[Network Version]: "
#if defined(_RELEASE)
            "RELEASE "
#elif defined(_PROFILE)
            "PROFILE "
#else
            "DEBUG "
#endif
            );

        return true;
    }
};

CRYREGISTER_SINGLETON_CLASS(CEngineModule_CryNetwork)

CEngineModule_CryNetwork::CEngineModule_CryNetwork()
{
};

CEngineModule_CryNetwork::~CEngineModule_CryNetwork()
{
    AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
    AZ::AllocatorInstance<GridMate::GridMateAllocator>::Destroy();
};

#if !defined(AZ_MONOLITHIC_BUILD)
    #include <CrtDebugStats.h>
#endif

