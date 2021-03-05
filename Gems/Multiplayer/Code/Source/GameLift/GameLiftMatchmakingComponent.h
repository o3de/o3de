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
#pragma once

#include <AzCore/Component/TickBus.h>
#include <GameLift/Session/GameLiftServerServiceEventsBus.h>
#include <GridMate/Session/Session.h>
#include <GridMate/Types.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace Multiplayer
{
#if defined(BUILD_GAMELIFT_SERVER)
    /**
    * GameLiftMatchmakingComponent responsible for handling custom matchmaking events for Multiplayer
    */
    class GameLiftMatchmakingComponent
        : public AZ::SystemTickBus::Handler
        , public GridMate::GameLiftServerServiceEventsBus::Handler
        , public GridMate::SessionEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GameLiftMatchmakingComponent, AZ::SystemAllocator, 0)

        GameLiftMatchmakingComponent(GridMate::GridSession* gridSession);
        ~GameLiftMatchmakingComponent();

    private:
        void OnSystemTick() override;
        void OnGameLiftGameSessionUpdated(GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession) override;

        void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override;
        void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member) override;

        void CallStartMatchmakingBackfill(bool checkAutoBackfill = true);

        float GetConsoleVarFloatValue(const char* param);
        bool GetConsoleVarBoolValue(const char* param);
        int GetConsoleVarIntValue(const char* param);

        // Initialized from Cvars
        float m_customMatchBackfillStartDelaySeconds;
        bool m_customMatchBackfillEnable;
        bool m_customMatchBackfillOnPlayerRemovedEnable;
        int m_minimumPlayerSessionCount;

        GridMate::TimeStamp m_startTime;
        bool m_customMatchBackfillStart = false;
        GridMate::GridSession* m_session;
        AZStd::vector<AZStd::string> m_matchmakingTicketIds;
    };
#endif
}
