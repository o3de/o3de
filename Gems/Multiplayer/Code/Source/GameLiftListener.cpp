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
#include "GameLiftListener.h"
#include "MultiplayerGem.h"
#include "Multiplayer/MultiplayerUtils.h"

#include <GridMate/NetworkGridMate.h>
#include <GameLift/Session/GameLiftServerSession.h>
#include <GameLift/Session/GameLiftServerServiceBus.h>

namespace Multiplayer
{
#if defined(BUILD_GAMELIFT_SERVER)
    GameLiftListener::GameLiftListener()
    {
        CRY_ASSERT(gEnv->pNetwork);
        CRY_ASSERT(gEnv->pNetwork->GetGridMate());

        GridMate::GameLiftServerServiceEventsBus::Handler::BusConnect(gEnv->pNetwork->GetGridMate());
    }

    GameLiftListener::~GameLiftListener()
    {
        GridMate::GameLiftServerServiceEventsBus::Handler::BusDisconnect();
    }

    void GameLiftListener::OnGameLiftGameSessionStarted([[maybe_unused]] GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        CRY_ASSERT(gEnv->pNetwork);
        CRY_ASSERT(gEnv->pNetwork->GetGridMate());

        GridMate::Network* gm = static_cast<GridMate::Network*>(gEnv->pNetwork);
        if (gm->GetCurrentSession())
        {
            CryLogAlways("New session(%s) started from gamelift while another session(%s) is still in progress.",
                gameSession.GetGameSessionId().c_str(),
                gm->GetCurrentSession()->GetId().c_str());
            return;
        }

        // server begins hosting        
        GridMate::CarrierDesc carrierDesc;

        // Get the general configurations, then override the port.
        Multiplayer::Utils::InitCarrierDesc(carrierDesc);
        Multiplayer::NetSec::ConfigureCarrierDescForHost(carrierDesc);
        
        carrierDesc.m_port = gEnv->pConsole->GetCVar("sv_port")->GetIVal();

        GridMate::GameLiftSessionParams sp;
        sp.m_topology = GridMate::ST_CLIENT_SERVER;
        sp.m_flags = 0;
        sp.m_numParams = 0;
        sp.m_numPrivateSlots = 1; // One slot for server member.
        sp.m_gameSession = &gameSession;

        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_ID_RESULT(session,gEnv->pNetwork->GetGridMate(),GridMate::GameLiftServerServiceBus, HostSession, sp, carrierDesc);

        if (session)
        {
            EBUS_EVENT(MultiplayerRequestBus, RegisterSession, session);
        }
    }

    void GameLiftListener::OnGameLiftGameSessionUpdated(GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession)
    {
        AZ_UNUSED(service);
        AZ_UNUSED(updateGameSession);
    }

    void GameLiftListener::OnGameLiftServerWillTerminate(GridMate::GameLiftServerService* service)
    {
        (void)service;
        CryLogAlways("Got terminate request from GameLift. Application will be closed!");

        gEnv->pSystem->Quit();
    }

#endif
}
