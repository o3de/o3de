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

#include <Multiplayer_precompiled.h>
#include "MultiplayerGameLiftClient.h"

#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <GameLift/Session/GameLiftSessionRequest.h>
#include <CrySystemBus.h>
#include <INetwork.h>
#include <Multiplayer/MultiplayerUtils.h>


namespace Multiplayer
{
#if defined(BUILD_GAMELIFT_CLIENT)

    static const char* GameLiftSessionAlreadyConnectedErrorMessage = "Already connected to a session. Use 'mpdisconnect' to leave current session";

    MultiplayerGameLiftClient::MultiplayerGameLiftClient()
        : m_mode(Mode::None)
        , m_serviceStatus(ServiceStatus::Stopped)
        , m_console(nullptr)
        , m_gridMate(nullptr)
        , m_search(nullptr)
        , m_maxPlayers(0)
    {
        MultiplayerGameLiftClientBus::Handler::BusConnect();
    }

    MultiplayerGameLiftClient::~MultiplayerGameLiftClient()
    {
        MultiplayerGameLiftClientBus::Handler::BusDisconnect();
    }

    void MultiplayerGameLiftClient::HostGameLiftSession(const char* serverName, const char* mapName,
        const AZ::u32 maxPlayers)
    {
        GridMate::GridSession* session = nullptr;
        MultiplayerRequestBus::BroadcastResult(session, &MultiplayerRequestBus::Events::GetSession);
        if (session)
        {
            AZ_TracePrintf("MultiplayerModule", GameLiftSessionAlreadyConnectedErrorMessage);
            return;
        }

        if (m_serviceStatus == ServiceStatus::Starting)
        {
            AZ_TracePrintf("MultiplayerModule", "GameLift client service startup is already in-progress");
            return;
        }

        m_mode = Mode::Host;
        m_serverName = serverName;
        m_mapName = mapName;
        m_maxPlayers = maxPlayers;

        if (m_serviceStatus == ServiceStatus::Stopped)
        {
            StartGameLiftClientService();
        }
        else
        {
            HandleGameLiftRequestByMode();
        }
    }

    void MultiplayerGameLiftClient::JoinGameLiftSession()
    {
        GridMate::GridSession* session = nullptr;
        MultiplayerRequestBus::BroadcastResult(session, &MultiplayerRequestBus::Events::GetSession);
        if (session)
        {
            AZ_TracePrintf("MultiplayerModule", GameLiftSessionAlreadyConnectedErrorMessage);
            return;
        }

        if (m_serviceStatus == ServiceStatus::Starting)
        {
            AZ_TracePrintf("MultiplayerModule", "GameLift client service startup is already in-progress");
            return;
        }

        m_mode = Mode::Join;

        if (m_serviceStatus == ServiceStatus::Stopped)
        {
            StartGameLiftClientService();
        }
        else
        {
            HandleGameLiftRequestByMode();
        }
    }

    void MultiplayerGameLiftClient::StartGameLiftMatchmaking(const char* matchmakingConfigName)
    {
        GridMate::GridSession* session = nullptr;
        MultiplayerRequestBus::BroadcastResult(session, &MultiplayerRequestBus::Events::GetSession);
        if (session)
        {
            AZ_TracePrintf("MultiplayerModule", GameLiftSessionAlreadyConnectedErrorMessage);
            return;
        }

        if (m_serviceStatus == ServiceStatus::Starting)
        {
            AZ_TracePrintf("MultiplayerModule", "GameLift client service startup is already in-progress");
            return;
        }

        m_mode = Mode::FlexMatch;
        m_matchmakingConfigName = matchmakingConfigName;

        if (m_serviceStatus == ServiceStatus::Stopped)
        {
            StartGameLiftClientService();
        }
        else
        {
            HandleGameLiftRequestByMode();
        }
    }

    void MultiplayerGameLiftClient::StopGameLiftClientService()
    {
        GameLift::GameLiftRequestBus::Broadcast(&GameLift::GameLiftRequestBus::Events::StopClientService);
        m_serviceStatus = ServiceStatus::Stopped;
    }

    IConsole* MultiplayerGameLiftClient::GetConsole()
    {
        if (!m_console)
        {
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);
            if (system)
            {
                m_console = system->GetIConsole();
            }
        }

        return m_console;
    }

    GridMate::IGridMate* MultiplayerGameLiftClient::GetGridMate()
    {
        if (!m_gridMate)
        {
            ISystem* system = nullptr;
            CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);
            if (system && system->GetINetwork())
            {
                m_gridMate = system->GetINetwork()->GetGridMate();
            }
        }

        return m_gridMate;
    }

    const char* MultiplayerGameLiftClient::GetConsoleParam(const char* paramName)
    {
        const char* value = "";
        IConsole* console = GetConsole();
        if (console)
        {
            ICVar* cvar = console->GetCVar(paramName);
            if (cvar)
            {
                value = cvar->GetString();
            }
        }
        else
        {
            AZ_TracePrintf("MultiplayerModule", "Console has not been initialized.");
        }

        return value;
    }

    const bool MultiplayerGameLiftClient::GetConsoleBoolParam(const char* paramName)
    {
        bool value = false;
        IConsole* console = GetConsole();
        if (console)
        {
            ICVar* cvar = GetConsole()->GetCVar(paramName);
            value = cvar && cvar->GetI64Val();
        }
        else
        {
            AZ_TracePrintf("MultiplayerModule", "Console has not been initialized.");
        }

        return value;
    }

    void MultiplayerGameLiftClient::AddRequestParameter(GridMate::GameLiftSessionRequestParams& params,
        const char* name, const char* value)
    {
        if (params.m_numParams < params.k_maxNumParams)
        {
            params.m_params[params.m_numParams].m_id = name;
            params.m_params[params.m_numParams].m_value = value;
            params.m_numParams++;
        }
        else
        {
            AZ_TracePrintf("MultiplayerModule", "Failed to add parameter to request; request contains maximum number of parameters.");
        }
    }

    void MultiplayerGameLiftClient::StartGameLiftClientService()
    {
        GridMate::IGridMate* gridMate = GetGridMate();

        if (!gridMate)
        {
            AZ_TracePrintf("MultiplayerModule", "GridMate has not been initialized.");
            return;
        }

        bool netSecEnabled = false;
        MultiplayerRequestBus::BroadcastResult(netSecEnabled, &MultiplayerRequestBus::Events::IsNetSecEnabled);

        if (netSecEnabled)
        {
            if (!Multiplayer::NetSec::CanCreateSecureSocketForJoining())
            {
                m_serviceStatus = ServiceStatus::Stopped;
                AZ_TracePrintf("MultiplayerModule", "Invalid Secure Socket Configuration.");
                return;
            }
        }

        GridMate::GameLiftClientServiceEventsBus::Handler::BusConnect(gridMate);

        GridMate::GameLiftClientServiceDesc serviceDesc;
        serviceDesc.m_accessKey = GetConsoleParam("gamelift_aws_access_key");
        serviceDesc.m_secretKey = GetConsoleParam("gamelift_aws_secret_key");
        serviceDesc.m_endpoint = GetConsoleParam("gamelift_endpoint");
        serviceDesc.m_region = GetConsoleParam("gamelift_aws_region");
        serviceDesc.m_playerId = GetConsoleParam("gamelift_player_id");
        serviceDesc.m_useGameLiftLocalServer = GetConsoleBoolParam("gamelift_uselocalserver");

        m_serviceStatus = ServiceStatus::Starting;
        GameLift::GameLiftRequestBus::Broadcast(
            &GameLift::GameLiftRequestBus::Events::StartClientService, serviceDesc);
    }

    void MultiplayerGameLiftClient::HostGameLiftSessionInternal()
    {
        GridMate::GameLiftSessionRequestParams params;

        params.m_instanceName = m_serverName;
        params.m_numPublicSlots = m_maxPlayers;
        params.m_numParams = 0;
        AddRequestParameter(params, "sv_name", m_serverName.c_str());
        AddRequestParameter(params, "sv_map", m_mapName.c_str());

        params.m_fleetId = GetConsoleParam("gamelift_fleet_id");
        params.m_aliasId = GetConsoleParam("gamelift_alias_id");
        params.m_queueName = GetConsoleParam("gamelift_queue_name");
        params.m_useFleetId = !params.m_fleetId.empty();

        GridMate::GameLiftClientServiceBus::BroadcastResult(m_search,
            &GridMate::GameLiftClientServiceBus::Events::RequestSession, params);
    }

    void MultiplayerGameLiftClient::StartGameLiftMatchmakingInternal()
    {
        GridMate::GameLiftClientServiceBus::BroadcastResult(m_search,
            &GridMate::GameLiftClientServiceBus::Events::StartMatchmaking, m_matchmakingConfigName);
    }

    void MultiplayerGameLiftClient::JoinGameLiftSessionInternal(const GridMate::GameLiftSearchInfo& searchInfo)
    {
        GridMate::GridSession* session = nullptr;
        MultiplayerRequestBus::BroadcastResult(session, &MultiplayerRequestBus::Events::GetSession);
        if (session)
        {
            AZ_TracePrintf("MultiplayerModule", GameLiftSessionAlreadyConnectedErrorMessage);
        }

        GridMate::CarrierDesc carrierDesc;
        Multiplayer::Utils::InitCarrierDesc(carrierDesc);
        Multiplayer::NetSec::ConfigureCarrierDescForJoin(carrierDesc);

        GridMate::GameLiftClientServiceBus::BroadcastResult(session,
            &GridMate::GameLiftClientServiceBus::Events::JoinSessionBySearchInfo, searchInfo, carrierDesc);

        if (session)
        {
            MultiplayerRequestBus::Broadcast(&MultiplayerRequestBus::Events::RegisterSession, session);
        }
        else
        {
            AZ_TracePrintf("MultiplayerModule", "Failed to create GameLift session.");
            Multiplayer::NetSec::OnSessionFailedToCreate(carrierDesc);
        }
    }

    void MultiplayerGameLiftClient::QueryGameLiftServers()
    {
        m_search = nullptr;
        GridMate::GameLiftSearchParams searchParams;
        searchParams.m_fleetId = GetConsoleParam("gamelift_fleet_id");
        searchParams.m_aliasId = GetConsoleParam("gamelift_alias_id");
        searchParams.m_queueName = GetConsoleParam("gamelift_queue_name");
        searchParams.m_useFleetId = !searchParams.m_fleetId.empty();
        GridMate::GameLiftClientServiceBus::BroadcastResult(m_search,
            &GridMate::GameLiftClientServiceBus::Events::StartSearch, searchParams);

        if (m_search == nullptr)
        {
            AZ_TracePrintf("MultiplayerModule", "Failed to start a GridSearch");
        }
    }

    void MultiplayerGameLiftClient::HandleGameLiftRequestByMode()
    {
        GridMate::SessionEventBus::Handler::BusConnect(GetGridMate());

        switch (m_mode)
        {
        case Mode::Join:
            QueryGameLiftServers();
            break;
        case Mode::Host:
            HostGameLiftSessionInternal();
            break;
        case Mode::FlexMatch:
            StartGameLiftMatchmakingInternal();
            break;
        default:
            break;
        }
    }

    void MultiplayerGameLiftClient::OnGameLiftSessionServiceReady(GridMate::GameLiftClientService*)
    {
        GridMate::GameLiftClientServiceEventsBus::Handler::BusDisconnect();
        m_serviceStatus = ServiceStatus::Started;
        HandleGameLiftRequestByMode();
    }

    void MultiplayerGameLiftClient::OnGameLiftSessionServiceFailed(GridMate::GameLiftClientService*, [[maybe_unused]] const AZStd::string& message)
    {
        GridMate::GameLiftClientServiceEventsBus::Handler::BusDisconnect();
        AZ_TracePrintf("MultiplayerModule", "GameLift Error: %s", message.c_str());
        StopGameLiftClientService();
    }

    void MultiplayerGameLiftClient::OnGridSearchComplete(GridMate::GridSearch* gridSearch)
    {
        // When connecting the SessionEventBus, we will be notified when any grid search completes.
        // This check ensures that we are only handling grid searches that we initiated (which populate m_search).
        if (gridSearch != m_search)
        {
            return;
        }

        if (m_search->GetNumResults() == 0)
        {
            AZ_TracePrintf("MultiplayerModule", "GridSearch returned no results.");
            return;
        }

        const GridMate::GameLiftSearchInfo& gameliftSearchInfo =
            static_cast<const GridMate::GameLiftSearchInfo&>(*m_search->GetResult(0));
        JoinGameLiftSessionInternal(gameliftSearchInfo);

        m_search->Release();
        m_search = nullptr;

        GridMate::SessionEventBus::Handler::BusDisconnect(GetGridMate());
    }
#endif
}
