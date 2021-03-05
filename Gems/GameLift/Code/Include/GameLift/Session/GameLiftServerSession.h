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

#if defined(BUILD_GAMELIFT_SERVER)

#include <GridMate/Session/Session.h>
#include <GameLift/Session/GameLiftServerServiceEventsBus.h>
#include <GameLift/Session/GameLiftSessionDefs.h>

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>

namespace GridMate
{
    class GameLiftServerSessionReplica;
    class GameLiftServerSDKWrapper;

    /*!
    * GameLift server session, returned on HostSession calls
    */
    class GameLiftServerSession
        : public GridSession
    {
        friend class GameLiftServerService;
        friend class GameLiftServerSessionReplica;
        friend class GameLiftServerMember;

    public:
        GM_CLASS_ALLOCATOR(GameLiftServerSession);

    protected:
        explicit GameLiftServerSession(GameLiftServerService* service);
        ~GameLiftServerSession();

        // Hosting a session
        bool Initialize(const GameLiftSessionParams& params, const CarrierDesc& carrierDesc);
        GridMember* CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode);

        bool GameSessionUpdated(const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession);

        // GridSession overrides
        void Shutdown() override;
        GridMember* CreateRemoteMember(const string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId = InvalidConnectionID) override;
        bool OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateHostMigrateSession(AZ::HSM&, const AZ::HSM::Event&) override;
        void OnSessionParamChanged(const GridSessionParam& param) override { (void)param; }
        void OnSessionParamRemoved(const string& paramId) override { (void)paramId; }

        static void RegisterReplicaChunks();

        GameLiftSessionParams m_sessionParams;
        Aws::GameLift::Server::Model::GameSession* m_gameLiftSession;
        bool StartMatchmakingBackfill(AZStd::string& matchmakingTicketId, bool checkForAutoBackfill);
        bool UpdateMatchmakerData();
        bool StopMatchmakingBackfill(const AZStd::string& matchmakingTicketId);
        AZStd::weak_ptr<GameLiftServerSDKWrapper> GetGameLiftServerSDKWrapper();

        rapidjson::Document m_matchmakerDataDocument;

    private:
        bool GetTeamForPlayerFromMatchmakerData(const char* playerId, AZStd::string& teamName);
        const std::vector<Aws::GameLift::Server::Model::PlayerSession> GetGameLiftPlayerSessions(const char* gameSessionId, Aws::GameLift::Server::Model::PlayerSessionStatus playerSessionStatus);
    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER
