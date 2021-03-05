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

#include <GridMate/Session/LANSession.h>

#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyLANServiceWrapper.h"

#include "Multiplayer/MultiplayerUtils.h"
#include <Multiplayer_Traits_Platform.h>

namespace Multiplayer
{
    MultiplayerLobbyLANServiceWrapper::MultiplayerLobbyLANServiceWrapper(const AZ::EntityId& multiplayerLobbyEntityId)
        : MultiplayerLobbyServiceWrapper(multiplayerLobbyEntityId)
    {
    }

    MultiplayerLobbyLANServiceWrapper::~MultiplayerLobbyLANServiceWrapper()
    {
    }

    bool MultiplayerLobbyLANServiceWrapper::SanityCheck([[maybe_unused]] GridMate::IGridMate* gridMate)
    {
        // Nothing in LAN Session Service we need to sanity check
        return true;
    }

    bool MultiplayerLobbyLANServiceWrapper::StartSessionService(GridMate::IGridMate* gridMate)
    {
        Multiplayer::LAN::StartSessionService(gridMate);
        return GridMate::HasGridMateService<GridMate::LANSessionService>(gridMate);
    }

    void MultiplayerLobbyLANServiceWrapper::StopSessionService(GridMate::IGridMate* gridMate)
    {
        Multiplayer::LAN::StopSessionService(gridMate);
    }

    GridMate::GridSession* MultiplayerLobbyLANServiceWrapper::CreateServerForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc)
    {
        GridMate::GridSession* gridSession = nullptr;

        // Setup and create the LANSessionParams
        GridMate::LANSessionParams sessionParams;
        sessionParams.m_port = GetServerPort();

        // Collect the shared session params from the MultiplayerLobby
        EBUS_EVENT_ID(GetTargetEntityId(),Multiplayer::MultiplayerLobbyBus,ConfigureSessionParams,sessionParams);

        EBUS_EVENT_ID_RESULT(gridSession,gridMate,GridMate::LANSessionServiceBus,HostSession,sessionParams,carrierDesc);

        return gridSession;
    }

    GridMate::GridSearch* MultiplayerLobbyLANServiceWrapper::ListServersForService(GridMate::IGridMate* gridMate)
    {
        GridMate::GridSearch* retVal = nullptr;

        GridMate::LANSearchParams searchParams;
        searchParams.m_serverPort = GetServerPort();
        searchParams.m_listenPort = 0;

        searchParams.m_maxSessions = gEnv->pConsole->GetCVar("gm_maxSearchResults")->GetIVal();
        AZ_TracePrintf("MultiplayerModule", "Limiting search results to a maximum of %d sessions.\n", searchParams.m_maxSessions);

        searchParams.m_version = gEnv->pConsole->GetCVar("gm_version")->GetIVal();
        searchParams.m_familyType = Multiplayer::Utils::CVarToFamilyType(gEnv->pConsole->GetCVar("gm_ipversion")->GetString());

#if AZ_TRAIT_MULTIPLAYER_ASSIGN_NETWORK_FAMILY
        AZ_Error(AZ_TRAIT_MULTIPLAYER_SESSION_NAME, searchParams.m_familyType == AZ_TRAIT_MULTIPLAYER_ADDRESS_TYPE, AZ_TRAIT_MULTIPLAYER_DRIVER_MESSAGE);
        searchParams.m_familyType = AZ_TRAIT_MULTIPLAYER_ADDRESS_TYPE;
#endif

        EBUS_EVENT_ID_RESULT(retVal, gridMate, GridMate::LANSessionServiceBus, StartGridSearch, searchParams);

        return retVal;
    }

    GridMate::GridSession* MultiplayerLobbyLANServiceWrapper::JoinSessionForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo)
    {
        GridMate::GridSession* gridSession = nullptr;
        const GridMate::LANSearchInfo& lanSearchInfo = static_cast<const GridMate::LANSearchInfo&>(*searchInfo);
        GridMate::JoinParams joinParams;

        EBUS_EVENT_ID_RESULT(gridSession, gridMate, GridMate::LANSessionServiceBus, JoinSessionBySearchInfo, lanSearchInfo, joinParams, carrierDesc);

        return gridSession;
    }

    int MultiplayerLobbyLANServiceWrapper::GetServerPort() const
    {
        // GamePort is reserved for game traffic, we want to go 1 above it to manage our server duties. i.e. Responding to search requests.
        int port = 0;
        EBUS_EVENT_ID_RESULT(port,GetTargetEntityId(),MultiplayerLobbyBus,GetGamePort);

        port += 1;

        return port;
    }
}
