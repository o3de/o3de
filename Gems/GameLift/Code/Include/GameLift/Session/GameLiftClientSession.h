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

#if defined(BUILD_GAMELIFT_CLIENT)

#include <GridMate/Session/Session.h>
#include <GameLift/Session/GameLiftClientServiceEventsBus.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftSearch.h>

#include <AzCore/std/chrono/types.h>

namespace GridMate
{
    struct GameLiftSearchInfo;
    class GameLiftSessionReplica;

    /*!
    * GameLift client session, returned on JoinSession calls
    */
    class GameLiftClientSession
        : public GridSession
    {
        friend class GameLiftClientService;
        friend class GameLiftSessionReplica;
        friend class GameLiftMember;

    public:
        GM_CLASS_ALLOCATOR(GameLiftClientSession);

    protected:
        explicit GameLiftClientSession(GameLiftClientService* service);

        bool Initialize(const GameLiftSearchInfo& info, const JoinParams& params, const CarrierDesc& carrierDesc);
        GridMember* CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode);
        GameLiftSessionReplica* OnSessionReplicaArrived();
        bool OnStateGameLiftInit(AZ::HSM& sm, const AZ::HSM::Event& e);
        void SetGameLiftLocalParams();

        // GridSession
        bool OnStateStartup(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateHostMigrateSession(AZ::HSM&, const AZ::HSM::Event&) override;
        GridMember* CreateRemoteMember(const string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId = InvalidConnectionID) override;
        void OnSessionParamChanged(const GridSessionParam& param) override { (void)param; }
        void OnSessionParamRemoved(const string& paramId) override { (void)paramId; }
        void Update() override;
        using GridSession::AddMember;

        static void RegisterReplicaChunks();

        enum SessionStates
        {
            SS_GAMELIFT_INIT = GridSession::SS_LAST
        };

        enum StateEvents
        {
            SE_CLIENT_FAILED = GridSession::SE_LAST,
            SE_RECEIVED_GAMESESSION,
            SE_RECEIVED_PLAYERSESSION,
            SE_MATCHMAKING_JOIN
        };

        GameLiftSearchInfo m_searchInfo;
        Aws::GameLift::Model::CreatePlayerSessionOutcomeCallable m_createPlayerSessionOutcomeCallable;
        Aws::GameLift::Model::DescribeGameSessionsOutcomeCallable m_describeGameSessionsOutcomeCallable;
        Aws::GameLift::Model::PlayerSession m_playerSession;

        int m_gameSessionRetryTimeout;
        int m_numGameSessionRetryAttempts;
        AZStd::chrono::system_clock::time_point m_gameSessionRetryTimestamp;
        GameLiftClientService* m_clientService;
    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
