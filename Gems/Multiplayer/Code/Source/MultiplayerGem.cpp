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
#include "Multiplayer_precompiled.h"
#include "MultiplayerGem.h"
#include "GameLiftListener.h"
#include <GridMate/NetworkGridMate.h>
#include <GridMate/Carrier/Driver.h>
#include <CertificateManager/ICertificateManagerGem.h>

#include <GridMate/Carrier/DefaultSimulator.h>
#include <AzFramework/Network/NetBindingSystemBus.h>

#include <AzCore/Component/ComponentApplicationBus.h>

#include "Multiplayer/MultiplayerLobbyComponent.h"
#include "Multiplayer/MultiplayerEventsComponent.h"
#include "Multiplayer/BehaviorContext/GridSystemContext.h"

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include <AzCore/Script/ScriptSystemComponent.h>

#include "Source/GameLift/GameLiftMatchmakingComponent.h"

#ifdef NET_SUPPORT_SECURE_SOCKET_DRIVER
#   include <GridMate/Carrier/SecureSocketDriver.h>
#endif

namespace
{
    void ApplyDisconnectDetectionSettings(GridMate::CarrierDesc& carrierDesc)
    {
        carrierDesc.m_enableDisconnectDetection = !!gEnv->pConsole->GetCVar("gm_disconnectDetection")->GetIVal();

        if (gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold"))
        {
            carrierDesc.m_disconnectDetectionRttThreshold = gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold")->GetFVal();
        }

        if (gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold"))
        {
            carrierDesc.m_disconnectDetectionPacketLossThreshold = gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold")->GetFVal();
        }
    }
}

namespace Multiplayer
{

    int MultiplayerModule::s_NetsecEnabled = 0;
    int MultiplayerModule::s_NetsecVerifyClient = 0;

    MultiplayerModule::MultiplayerModule() 
        :  CryHooksModule()
        , m_session(nullptr)
        , m_secureDriver(nullptr)
        , m_simulator(nullptr)
        , m_gameLiftListener(nullptr)
        , m_matchmakingComponent(nullptr)
    {    
        m_descriptors.push_back(MultiplayerLobbyComponent::CreateDescriptor());
        m_descriptors.push_back(MultiplayerEventsComponent::CreateDescriptor());        

        // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
        // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
        AZStd::vector<AZ::Uuid> typeIds;
        typeIds.reserve(m_descriptors.size());
        for (AZ::ComponentDescriptor* descriptor : m_descriptors)
        {
            typeIds.emplace_back(descriptor->GetUuid());
        }
        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
    }

    MultiplayerModule::~MultiplayerModule() 
    {
        delete m_simulator;
    }

    void MultiplayerModule::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        CryHooksModule::OnCrySystemInitialized(system, systemInitParams);
        m_cvars.RegisterCVars();
    }

    void MultiplayerModule::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {

            case ESYSTEM_EVENT_GAME_POST_INIT:
            {
#if defined(BUILD_GAMELIFT_SERVER)
        m_gameLiftListener = aznew GameLiftListener();
#endif                
        AZ_Assert(gEnv->pNetwork->GetGridMate(), "No GridMate");
        GridMate::SessionEventBus::Handler::BusConnect(gEnv->pNetwork->GetGridMate());
        MultiplayerRequestBus::Handler::BusConnect();
        m_cvars.PostInitRegistration();
    }
            break;

            case ESYSTEM_EVENT_FULL_SHUTDOWN:
            case ESYSTEM_EVENT_FAST_SHUTDOWN:
                MultiplayerRequestBus::Handler::BusDisconnect();
                GridMate::SessionEventBus::Handler::BusDisconnect();
                m_cvars.UnregisterCVars();

#if defined(BUILD_GAMELIFT_SERVER)
                delete m_gameLiftListener;
                m_gameLiftListener = nullptr;
#endif
                break;

            default:
                (void)event;
        }
    }

    bool MultiplayerModule::IsNetSecEnabled() const
    {
        return NetSec::s_NetsecEnabled != 0;
    }

    bool MultiplayerModule::IsNetSecVerifyClient() const
    {
        return NetSec::s_NetsecVerifyClient != 0;
    }

    void MultiplayerModule::RegisterSecureDriver(GridMate::SecureSocketDriver* driver)
    {
#if defined(NET_SUPPORT_SECURE_SOCKET_DRIVER)
        AZ_Assert(driver != nullptr || m_session == nullptr, "Trying to Unregister secure driver with an active session. Once a session is active, MultiplayerGem will clean up the driver once the session terminates.");
        AZ_Assert(m_secureDriver == nullptr || driver == nullptr,"Trying to Register two secure driver's at once. Unsupported behavior");

        m_secureDriver = driver;
#else
        (void)driver;
        AZ_Error("MultiplayerModule", false, "Attempt to set SecureSocketDriver for unsupported platform\n");
#endif
    }

    GridMate::GridSession* MultiplayerModule::GetSession()
    {
        return m_session;
    }

    void MultiplayerModule::RegisterSession(GridMate::GridSession* session)
    {
        if (m_session != nullptr && session != nullptr)
        {
            CryLog("Already participating in the session '%s'. Leave existing session first!", m_session->GetId().c_str());
            return;
        }    

        m_session = session;

    #if defined(BUILD_GAMELIFT_SERVER)
        m_matchmakingComponent = aznew GameLiftMatchmakingComponent(m_session);
    #endif
    }

    void MultiplayerModule::OnSessionCreated(GridMate::GridSession* session)
    {
        AZ_TracePrintf("MultiplayerModule", "Session %s has been created.\n", session->GetId().c_str());

        if (session == m_session)
        {
            EBUS_EVENT(AzFramework::NetBindingSystemEventsBus, OnNetworkSessionCreated, session);
        }
    }

    void MultiplayerModule::OnSessionHosted(GridMate::GridSession* session)
    {
        AZ_TracePrintf("MultiplayerModule", "Session %s has been hosted.\n", session->GetId().c_str());

        if (session == m_session)
        {
            ActivateNetworkSession(session);
        }
    }

    void MultiplayerModule::OnSessionJoined(GridMate::GridSession* session)
    {
        AZ_TracePrintf("MultiplayerModule", "Session %s has been joined.\n", session->GetId().c_str());

        if (session == m_session)
        {
            ActivateNetworkSession(session);
        }
    }

    void MultiplayerModule::ActivateNetworkSession(GridMate::GridSession* session)
    {
        AZ_Assert(session, "Invalid session");

        session->GetReplicaMgr()->SetSendTimeInterval(gEnv->pConsole->GetCVar("gm_replicasSendTime")->GetIVal());

        if (gEnv->pConsole->GetCVar("gm_replicasSendLimit"))
        {
            session->GetReplicaMgr()->SetSendLimit(gEnv->pConsole->GetCVar("gm_replicasSendLimit")->GetIVal());
        }

        if (gEnv->pConsole->GetCVar("gm_burstTimeLimit"))
        {
            session->GetReplicaMgr()->SetSendLimitBurstRange(gEnv->pConsole->GetCVar("gm_burstTimeLimit")->GetFVal());
        }

        EBUS_EVENT(AzFramework::NetBindingSystemEventsBus, OnNetworkSessionActivated, session);
    }

    //-----------------------------------------------------------------------------
    void MultiplayerModule::OnSessionDelete(GridMate::GridSession* session)
    {
        CryLog("Session %s has been deleted.", session->GetId().c_str());

        if (session == m_session)
        {
            EBUS_EVENT(AzFramework::NetBindingSystemEventsBus, OnNetworkSessionDeactivated, session);
            m_session = nullptr;

#if defined(BUILD_GAMELIFT_SERVER)
            delete m_matchmakingComponent;
            m_matchmakingComponent = nullptr;
#endif

#ifdef NET_SUPPORT_SECURE_SOCKET_DRIVER
            delete m_secureDriver;
            m_secureDriver = nullptr;
#endif
        }
    }

    //-----------------------------------------------------------------------------
    void MultiplayerModule::OnCrySystemPostShutdown()
    {
#ifdef NET_SUPPORT_SECURE_SOCKET_DRIVER
        delete m_secureDriver;
        m_secureDriver = nullptr;
#endif

        CryHooksModule::OnCrySystemPostShutdown();
    }

    //-----------------------------------------------------------------------------
    GridMate::Simulator* MultiplayerModule::GetSimulator()
    {
        return m_simulator;
    }

    //-----------------------------------------------------------------------------
    void MultiplayerModule::EnableSimulator()
    {
        if (!m_simulator)
        {
            m_simulator = aznew GridMate::DefaultSimulator();
        }

        m_simulator->Enable();
    }

    //-----------------------------------------------------------------------------
    void MultiplayerModule::DisableSimulator()
    {
        if (m_simulator)
        {
            m_simulator->Disable();
        }
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer, Multiplayer::MultiplayerModule)
