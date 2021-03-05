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
#include "Source/GameLift/GameLiftMatchmakingComponent.h"
#include <GridMate/NetworkGridMate.h>
#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftServerServiceBus.h>

namespace Multiplayer
{
#if defined(BUILD_GAMELIFT_SERVER)
    GameLiftMatchmakingComponent::GameLiftMatchmakingComponent(GridMate::GridSession* gridSession)
    {
        m_session = gridSession;
        m_startTime = AZStd::chrono::system_clock::now();

        m_customMatchBackfillEnable = GetConsoleVarBoolValue("gamelift_flexmatch_enable");
        m_customMatchBackfillOnPlayerRemovedEnable = GetConsoleVarBoolValue("gamelift_flexmatch_onplayerremoved_enable");
        m_customMatchBackfillStartDelaySeconds = GetConsoleVarFloatValue("gamlift_flexmatch_start_delay");
        m_minimumPlayerSessionCount = GetConsoleVarIntValue("gamelift_flexmatch_minimumplayersessioncount");

        AZ::SystemTickBus::Handler::BusConnect();
        GridMate::GameLiftServerServiceEventsBus::Handler::BusConnect(m_session->GetGridMate());
        GridMate::SessionEventBus::Handler::BusConnect(m_session->GetGridMate());
    }

    GameLiftMatchmakingComponent::~GameLiftMatchmakingComponent()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        GridMate::GameLiftServerServiceEventsBus::Handler::BusDisconnect(m_session->GetGridMate());
        GridMate::SessionEventBus::Handler::BusDisconnect(m_session->GetGridMate());
        m_matchmakingTicketIds.clear();
        m_session = nullptr;
    }

    void GameLiftMatchmakingComponent::OnSystemTick()
    {
        if (m_customMatchBackfillEnable && m_customMatchBackfillStart)
        {
            GridMate::TimeStamp now = AZStd::chrono::system_clock::now();
            float timeElapsed = AZStd::chrono::duration<float>(now - m_startTime).count();
            if (timeElapsed > m_customMatchBackfillStartDelaySeconds)
            {
                CallStartMatchmakingBackfill();
                m_customMatchBackfillStart = false;
            }
        }
    }

    void GameLiftMatchmakingComponent::OnMemberJoined([[maybe_unused]] GridMate::GridSession* session, [[maybe_unused]] GridMate::GridMember* member)
    {
        AZ_TracePrintf("Multiplayer", "calling OnMemberJoined FreeSlots:%d UsedSLots:%d", m_session->GetNumFreePublicSlots(), m_session->GetNumUsedPublicSlots());

        // Start delayed matchbackfill after initial minimum players have joined.
        if (m_session->GetNumUsedPublicSlots() == m_minimumPlayerSessionCount)
        {
            m_customMatchBackfillStart = true;
            return;
        }
        
        if (m_customMatchBackfillEnable && m_session->GetNumFreePublicSlots() > 0 // Need at least 1 free slot
            && (m_session->GetNumUsedPublicSlots() > m_minimumPlayerSessionCount // Minimum number of players already exists for the one time delayed back fill
            || (m_session->GetNumUsedPublicSlots() == m_minimumPlayerSessionCount && !m_customMatchBackfillStart))) // After initial delayed backfill rest of the times
        { 
            CallStartMatchmakingBackfill();
        }
    }

    void GameLiftMatchmakingComponent::OnGameLiftGameSessionUpdated([[maybe_unused]] GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession)
    {
        if (updateGameSession.GetUpdateReason() == Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT)
        {
            auto it = AZStd::find(m_matchmakingTicketIds.begin(), m_matchmakingTicketIds.end(), AZStd::string(updateGameSession.GetBackfillTicketId().c_str()));
            if (it != m_matchmakingTicketIds.end())
            {
                EBUS_EVENT_ID(gEnv->pNetwork->GetGridMate(), GridMate::GameLiftServerServiceBus, StartMatchmakingBackfill, m_session, *it, false);
            }
        }
    }

    void GameLiftMatchmakingComponent::OnMemberLeaving([[maybe_unused]] GridMate::GridSession* session, [[maybe_unused]] GridMate::GridMember* member)
    {
        AZ_TracePrintf("Multiplayer", "calling OnMemberLeaving FreeSlots:%d UsedSLots:%d", m_session->GetNumFreePublicSlots(), m_session->GetNumUsedPublicSlots());
        if (m_session->GetNumUsedPublicSlots() < m_minimumPlayerSessionCount)
        {
            for (auto ticketId : m_matchmakingTicketIds)
            {
                EBUS_EVENT_ID(gEnv->pNetwork->GetGridMate(), GridMate::GameLiftServerServiceBus, StopMatchmakingBackfill, m_session, ticketId);
            }
            m_matchmakingTicketIds.clear();
            EBUS_EVENT(GridMate::GameLiftServerServiceBus, ShutdownSession, m_session);
            return;
        }

        if (m_customMatchBackfillOnPlayerRemovedEnable)
        {
            CallStartMatchmakingBackfill(false);
        }
    }

    void GameLiftMatchmakingComponent::CallStartMatchmakingBackfill(bool checkAutoBackfill)
    {
        bool matchmakingBackfillTicketCreated = false;
        AZStd::string matchmakingBackfillTicketId = "";
        EBUS_EVENT_ID_RESULT(matchmakingBackfillTicketCreated, gEnv->pNetwork->GetGridMate(), GridMate::GameLiftServerServiceBus
            , StartMatchmakingBackfill, m_session, matchmakingBackfillTicketId, checkAutoBackfill);
        if (matchmakingBackfillTicketCreated)
        {
            m_matchmakingTicketIds.push_back(matchmakingBackfillTicketId);
        }
    }

    float GameLiftMatchmakingComponent::GetConsoleVarFloatValue(const char* param)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);
        return cvar ? cvar->GetFVal() : 0.0F;
    }

    int GameLiftMatchmakingComponent::GetConsoleVarIntValue(const char* param)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);
        return cvar ? cvar->GetIVal() : 0;
    }

    bool GameLiftMatchmakingComponent::GetConsoleVarBoolValue(const char* param)
    {
        ICVar* cvar = gEnv->pConsole->GetCVar(param);
        return cvar ? cvar->GetI64Val() : false;
    }
#endif
}
