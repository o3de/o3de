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

#if defined(BUILD_GAMELIFT_SERVER)

#include <GameLift/Session/GameLiftServerSDKWrapper.h>
#include <GameLift/Session/GameLiftServerSession.h>
#include <GameLift/Session/GameLiftServerService.h>
#include <GridMate/Online/UserServiceTypes.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>

#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Carrier/Utils.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>

#include <aws/gamelift/server/GameLiftServerAPI.h>
#include <aws/gamelift/server/model/GameSession.h>

#include <GameLift/Session/GameLiftServerServiceEventsBus.h>


namespace GridMate
{
    class GameLiftServerMember;

    //-----------------------------------------------------------------------------
    // GameLiftServerSessionReplica
    //-----------------------------------------------------------------------------
    class GameLiftServerSessionReplica
        : public Internal::GridSessionReplica
    {
    public:
        class GameLiftSessionReplicaDesc
            : public ReplicaChunkDescriptor
        {
        public:
            GameLiftSessionReplicaDesc()
                : ReplicaChunkDescriptor(GameLiftServerSessionReplica::GetChunkName(), sizeof(GameLiftServerSessionReplica))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext&) override
            {
                AZ_Assert(false, "GameLiftServerSessionReplica should never be created from stream on the server!");
                return nullptr;
            }

            void DiscardCtorStream(UnmarshalContext&) override {}
            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }
            void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override {}
        };

        GM_CLASS_ALLOCATOR(GameLiftServerSessionReplica);

        static const char* GetChunkName() { return "GridMate::GameLiftSessionReplica"; }

        GameLiftServerSessionReplica(GameLiftServerSession* session)
            : GridSessionReplica(session)
        {
        }
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftServerMemberID
    //-----------------------------------------------------------------------------
    class GameLiftServerMemberID
        : public MemberID
    {
    public:
        //-----------------------------------------------------------------------------
        // Marshaler
        //-----------------------------------------------------------------------------
        class Marshaler
        {
        public:
            void Marshal(WriteBuffer& wb, const GameLiftServerMemberID& id) const
            {
                wb.Write(id.m_id);
            }

            void Unmarshal(GameLiftServerMemberID& id, ReadBuffer& rb) const
            {
                rb.Read(id.m_id);
            }
        };
        //-----------------------------------------------------------------------------

        GameLiftServerMemberID()
            : m_id(0)
        {
        }

        GameLiftServerMemberID(const string& address, AZ::u32 memberId)
            : m_address(address)
            , m_id(memberId)
        {
            AZ_Assert(m_id != 0, "Invalid member id");
        }

        string ToString() const override { return string::format("%08X", m_id); }
        string ToAddress() const override { return m_address; }
        MemberIDCompact Compact() const override { return m_id; }
        bool IsValid() const override { return !m_address.empty() && m_id != 0; }

    private:

        AZ::u32 m_id;
        string m_address;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // GameLiftMemberInfoCtorContext
    //-----------------------------------------------------------------------------
    struct GameLiftMemberInfoCtorContext
        : public CtorContextBase
    {
        CtorDataSet<GameLiftServerMemberID, GameLiftServerMemberID::Marshaler> m_memberId;
        CtorDataSet<RemotePeerMode> m_peerMode;
        CtorDataSet<bool> m_isHost;
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftServerMemberState
    //-----------------------------------------------------------------------------
    class GameLiftServerMemberState
        : public Internal::GridMemberStateReplica
    {
    public:
        GM_CLASS_ALLOCATOR(GameLiftServerMemberState);
        static const char* GetChunkName() { return "GameLiftMemberState"; }

        explicit GameLiftServerMemberState(GridMember* member = nullptr)
            : GridMemberStateReplica(member)
        {
        }
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftServerMember
    //-----------------------------------------------------------------------------
    class GameLiftServerMember
        : public GridMember
    {
        friend class GameLiftServerSession;

    public:
        class GameLiftServerMemberDesc
            : public ReplicaChunkDescriptor
        {
        public:
            GameLiftServerMemberDesc()
                : ReplicaChunkDescriptor(GameLiftServerMember::GetChunkName(), sizeof(GameLiftServerMember))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext&) override
            {
                AZ_Assert(false, "GameLiftServerMemberDesc should never be created from stream on the server!");
                return nullptr;
            }

            void DiscardCtorStream(UnmarshalContext& mc) override
            {
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);
            }

            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override
            {
                if (!static_cast<GameLiftServerMember*>(chunkInstance)->IsLocal())
                {
                    delete chunkInstance;
                }
            }

            void MarshalCtorData(ReplicaChunkBase* chunkInstance, WriteBuffer& wb) override
            {
                GameLiftServerMember* member = static_cast<GameLiftServerMember*>(chunkInstance);
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.m_memberId.Set(member->m_memberId);
                ctorContext.m_peerMode.Set(member->m_peerMode.Get());
                ctorContext.m_isHost.Set(member->IsHost());
                ctorContext.Marshal(wb);
            }
        };

        GM_CLASS_ALLOCATOR(GameLiftServerMember);

        static const char* GetChunkName() { return "GridMate::GameLiftMember"; }

        const PlayerId* GetPlayerId() const override
        {
            return nullptr;
        }

        const MemberID& GetId() const override
        {
            return m_memberId;
        }

        void SetPlayerSessionId(const char* playerSessionId) { AZ_Assert(playerSessionId, "Invalid player session id"); m_playerSessionId = playerSessionId; }
        const char* GetPlayerSessionId() const { return m_playerSessionId.c_str(); }

        /// Remote member ctor.
        GameLiftServerMember(ConnectionID connId, const GameLiftServerMemberID& memberId, GameLiftServerSession* session)
            : GridMember(memberId.Compact())
            , m_memberId(memberId)
        {
            m_session = session;
            m_connectionId = connId;
        }

        /// Local member ctor.
        GameLiftServerMember(const GameLiftServerMemberID& memberId, GameLiftServerSession* session)
            : GridMember(memberId.Compact())
            , m_memberId(memberId)
        {
            m_session = session;

            m_clientState = CreateReplicaChunk<GameLiftServerMemberState>(this);
            m_clientState->m_name.Set(memberId.ToString());

            m_clientStateReplica = Replica::CreateReplica(memberId.ToString().c_str());
            m_clientStateReplica->AttachReplicaChunk(m_clientState);
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            if (IsMaster() && !IsLocal())
            {
                GameLiftServerSession* serverSession = static_cast<GameLiftServerSession*>(m_session);
                Aws::GameLift::GenericOutcome outcome = serverSession->GetGameLiftServerSDKWrapper().lock()->RemovePlayerSession(m_playerSessionId.c_str());
                if (!outcome.IsSuccess())
                {
                    AZ_TracePrintf("GameLift", "[SERVER SESSION] Failed to disconnect a master non-local GameLift player:'%s' with id=%s\n", outcome.GetError().GetErrorName().c_str(), GetId().ToString().c_str());
                }
                else
                {
                    AZ_TracePrintf("GameLift", "Player removed current used public slots:%d and free public slots:%d", m_session->GetNumUsedPublicSlots(), m_session->GetNumFreePublicSlots());
                    AZ_TracePrintf("GameLift", "[SERVER SESSION] Sucessfully disconnected a master non-local GameLift player with id=%s\n", GetId().ToString().c_str());
                }
            }
            else
            {
                AZ_TracePrintf("GameLift", "[SERVER SESSION] Deactivating a gridmember, memberid %d", rc.m_peer ? rc.m_peer->GetId() : 0);
            }

            GridMember::OnReplicaDeactivate(rc);
        }

        using GridMember::SetHost;

        GameLiftServerMemberID m_memberId;
        string m_playerSessionId;
    };
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftServerSession
    //-----------------------------------------------------------------------------
    GameLiftServerSession::GameLiftServerSession(GameLiftServerService* service)
        : GridSession(service)
    {
    }

    GameLiftServerSession::~GameLiftServerSession()
    {
        if(m_gameLiftSession)
        {
            delete m_gameLiftSession;
            m_gameLiftSession = nullptr;
        }
    }

    bool GameLiftServerSession::Initialize(const GameLiftSessionParams& params, const CarrierDesc& carrierDesc)
    {
        const Aws::GameLift::Server::Model::GameSession* gameSession = params.m_gameSession;
        AZ_Assert(gameSession, "No game session instance specified.");
        m_gameLiftSession = new Aws::GameLift::Server::Model::GameSession(*gameSession);

        if (!GridSession::Initialize(carrierDesc))
        {
            return false;
        }

        m_myMember = CreateLocalMember(true, true, Mode_Peer);
        m_sessionParams = params;

        UpdateMatchmakerData();

        const auto& properties = gameSession->GetGameProperties();
        GameLiftServerSessionReplica::ParamContainer sessionProperties;
        for (AZStd::size_t i = 0; i < properties.size(); ++i)
        {
            GridSessionParam param;
            param.m_id = properties[i].GetKey().c_str();
            param.m_value = properties[i].GetValue().c_str();
            sessionProperties.push_back(param);
        }

        m_sessionParams.m_numPublicSlots = gameSession->GetMaximumPlayerSessionCount();

        //////////////////////////////////////////////////////////////////////////
        // start up the session state we will bind it later
        AZ_Assert(m_sessionParams.m_numPublicSlots < 0xff && m_sessionParams.m_numPrivateSlots < 0xff, "Can't have more than 255 slots!");
        AZ_Assert(m_sessionParams.m_numPublicSlots > 0 || m_sessionParams.m_numPrivateSlots > 0, "You don't have any slots open!");

        GameLiftServerSessionReplica* state = CreateReplicaChunk<GameLiftServerSessionReplica>(this);
        state->m_numFreePrivateSlots.Set(static_cast<unsigned char>(m_sessionParams.m_numPrivateSlots));
        state->m_numFreePublicSlots.Set(static_cast<unsigned char>(m_sessionParams.m_numPublicSlots));
        state->m_peerToPeerTimeout.Set(m_sessionParams.m_peerToPeerTimeout);
        state->m_flags.Set(m_sessionParams.m_flags);
        state->m_topology.Set(m_sessionParams.m_topology);
        state->m_params.Set(sessionProperties);
        m_state = state;
        //////////////////////////////////////////////////////////////////////////

        m_sessionId = gameSession->GetGameSessionId().c_str();

        SetUpStateMachine();
        RequestEvent(SE_HOST);

        return true;
    }

    bool GameLiftServerSession::GameSessionUpdated(const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession)
    {
        // Delete previous game session before updating.
        if (m_gameLiftSession)
        {
            delete m_gameLiftSession;
            m_gameLiftSession = nullptr;
        }
        m_gameLiftSession = new Aws::GameLift::Server::Model::GameSession(updateGameSession.GetGameSession());
        Aws::GameLift::Server::Model::UpdateReason updateReason = updateGameSession.GetUpdateReason();

        switch (updateReason)
        {
        case Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED:
            return UpdateMatchmakerData();
        case Aws::GameLift::Server::Model::UpdateReason::BACKFILL_CANCELLED:
        case Aws::GameLift::Server::Model::UpdateReason::BACKFILL_FAILED:
        case Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT:
        case Aws::GameLift::Server::Model::UpdateReason::UNKNOWN:
        default:
            AZ_TracePrintf("GameLift", "GameSessionUpdate matchmaker error reasonname:%s GameSessionData:%s MatchmakerData:%s", Aws::GameLift::Server::Model::UpdateReasonMapper::GetNameForUpdateReason(updateReason).c_str()
                , updateGameSession.GetGameSession().GetGameSessionData().c_str(), updateGameSession.GetGameSession().GetMatchmakerData().c_str());
        }
        return false;
    }

    bool GameLiftServerSession::UpdateMatchmakerData()
    {
        const char* matchmakerData = m_gameLiftSession->GetMatchmakerData().c_str();
        rapidjson::ParseResult parseResult = m_matchmakerDataDocument.Parse(matchmakerData);
        if (!parseResult)
        {
            AZ_Error("GameLift", parseResult, "Error parsing matchmaker data Error:%s Offset:%u", rapidjson::GetParseError_En(parseResult.Code()), parseResult.Offset());
            return false;
        }
        return true;
    }

    bool GameLiftServerSession::GetTeamForPlayerFromMatchmakerData(const char* playerId, AZStd::string& teamName)
    {
        rapidjson::Value& teams = m_matchmakerDataDocument["teams"];
        AZ_Assert(teams.IsArray(), "Teams is not array");
        for (rapidjson::SizeType teamIndex = 0; teamIndex < teams.Size(); ++teamIndex)
        {
            rapidjson::Value& players = teams[teamIndex]["players"];
            AZ_Assert(players.IsArray(), "Players is not array");
            for (rapidjson::SizeType playerIndex = 0; playerIndex < players.Size(); ++playerIndex)
            {
                if (std::strcmp(players[playerIndex]["playerId"].GetString(), playerId) == 0)
                {
                    teamName = teams[teamIndex]["name"].GetString();
                    return true;
                }
            }
        }
        return false;
    }
    
    bool GameLiftServerSession::StartMatchmakingBackfill(AZStd::string& matchmakingTicketId, bool checkForAutoBackfill = true)
    {
        if (checkForAutoBackfill && m_matchmakerDataDocument.HasMember("autoBackfillTicketId") && m_matchmakerDataDocument["autoBackfillTicketId"].IsString())
        {
            const char* autoBackfillTicketId = m_matchmakerDataDocument["autoBackfillTicketId"].GetString();
            AZ_TracePrintf("GameLift", "Ignoring backfill request when AUTOMATIC backfill is active %s", autoBackfillTicketId);
            return false;
        }

        if (!m_matchmakerDataDocument.HasMember("matchmakingConfigurationArn"))
        {
            AZ_TracePrintf("GameLift", "Ignoring backfill request when no matchmaking config arn found");
            return false;
        }
       
        const char* gameSessionId = m_gameLiftSession->GetGameSessionId().c_str();

        rapidjson::Value& teams = m_matchmakerDataDocument["teams"];
        AZ_Assert(teams.IsArray(), "Teams is not array");
        const char* matchmakingConfigurationArn = m_matchmakerDataDocument["matchmakingConfigurationArn"].GetString();
        Aws::GameLift::Server::Model::StartMatchBackfillRequest startBackfillRequest;
        startBackfillRequest.SetMatchmakingConfigurationArn(matchmakingConfigurationArn);
        startBackfillRequest.SetGameSessionArn(gameSessionId);

        // Set matchmaking ticket id if provided.
        if (!matchmakingTicketId.empty())
        {
            startBackfillRequest.SetTicketId(matchmakingTicketId.c_str());
        }
                
        const std::vector<Aws::GameLift::Server::Model::PlayerSession> &gameLiftPlayerSessions = GetGameLiftPlayerSessions(gameSessionId, Aws::GameLift::Server::Model::PlayerSessionStatus::ACTIVE);
        for (const Aws::GameLift::Server::Model::PlayerSession& playerSession : gameLiftPlayerSessions)
        {
            Aws::GameLift::Server::Model::Player player;
            AZ_TracePrintf("GameLift", "Active member found playerId:%s", playerSession.GetPlayerId().c_str());

            player.SetPlayerId(playerSession.GetPlayerId());
            AZStd::string teamName;
            if (GetTeamForPlayerFromMatchmakerData(playerSession.GetPlayerId().c_str(), teamName))
            {
                player.SetTeam(teamName.c_str());
            }
            startBackfillRequest.AddPlayer(player);
        }
        
        Aws::GameLift::StartMatchBackfillOutcome backfillOutcome = GetGameLiftServerSDKWrapper().lock()->StartMatchBackfill(startBackfillRequest);
        if (backfillOutcome.IsSuccess())
        {
            matchmakingTicketId = backfillOutcome.GetResult().GetTicketId().c_str();
            AZ_TracePrintf("GameLift", "Matchmaking Backfill request success ticketId:%s", matchmakingTicketId.c_str());
            return true;
        }
        else
        {
            AZ_TracePrintf("GameLift", "Matchmaking Backfill request error:%s gamesession:%s config:%s", backfillOutcome.GetError().GetErrorMessage().c_str(), gameSessionId, matchmakingConfigurationArn);
            return false;
        }
        
    }

    bool GameLiftServerSession::StopMatchmakingBackfill(const AZStd::string& matchmakingTicketId)
    {
        const char* gameSessionId = m_gameLiftSession->GetGameSessionId().c_str();

        rapidjson::Value& teams = m_matchmakerDataDocument["teams"];
        AZ_Assert(teams.IsArray(), "Teams is not array");
        const char* matchmakingConfigurationArn = m_matchmakerDataDocument["matchmakingConfigurationArn"].GetString();

        Aws::GameLift::Server::Model::StopMatchBackfillRequest stopBackfillRequest;
        stopBackfillRequest.SetTicketId(matchmakingTicketId.c_str());
        stopBackfillRequest.SetMatchmakingConfigurationArn(matchmakingConfigurationArn);
        stopBackfillRequest.SetGameSessionArn(gameSessionId);
        Aws::GameLift::GenericOutcome backfillOutcome = GetGameLiftServerSDKWrapper().lock()->StopMatchBackfill(stopBackfillRequest);
        if (backfillOutcome.IsSuccess())
        {
            AZ_TracePrintf("GameLift", "Matchmaking Backfill stop success matchmakingTicketId:%s", matchmakingTicketId.c_str());
            return true;
        }
        else
        {
            AZ_TracePrintf("GameLift", "Matchmaking Backfill stop error:%s gamesession:%s config:%s matchmakingTicketId:%s", backfillOutcome.GetError().GetErrorMessage().c_str()
            , gameSessionId, matchmakingConfigurationArn, matchmakingTicketId.c_str());
            return false;
        }
    }


    GridMember* GameLiftServerSession::CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode)
    {
        AZ_Assert(isHost, "GameLiftServerSession can only run as host!");
        AZ_Assert(!m_myMember, "We already have added a local member!");

        string ip = Utils::GetMachineAddress(m_carrierDesc.m_familyType);
        string address = SocketDriverCommon::IPPortToAddressString(ip.c_str(), m_carrierDesc.m_port);

        GameLiftServerMemberID myId(address, AZ::Crc32("GameLiftServer"));

        GameLiftServerMember* member = CreateReplicaChunk<GameLiftServerMember>(myId, this);
        member->SetHost(isHost);
        member->SetInvited(isInvited);
        member->m_peerMode.Set(peerMode);
        return member;
    }

    void GameLiftServerSession::Shutdown()
    {
        Aws::GameLift::GenericOutcome outcome = GetGameLiftServerSDKWrapper().lock()->TerminateGameSession();
        if (!outcome.IsSuccess())
        {
            AZ_Warning("GridMate", outcome.IsSuccess(), "GameLift session failed to terminate:%s:%s\n",
                outcome.GetError().GetErrorName().c_str(),
                outcome.GetError().GetErrorMessage().c_str());
                return;
        }

        if (m_gameLiftSession)
        {
            delete m_gameLiftSession;
            m_gameLiftSession = nullptr;
        }

        GridSession::Shutdown();
    }

    GridMember* GameLiftServerSession::CreateRemoteMember(const string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId)
    {
        string playerSessionId;
        data.Read(playerSessionId);

        Aws::GameLift::GenericOutcome outcome = GetGameLiftServerSDKWrapper().lock()->AcceptPlayerSession(playerSessionId.c_str());
        if (!outcome.IsSuccess())
        {
            AZ_TracePrintf("GameLift", "Failed to connect GameLift player:'%s' with id=%s\n", outcome.GetError().GetErrorName().c_str(), playerSessionId.c_str());
            m_carrier->Disconnect(connId);
            return nullptr;
        }

        GameLiftServerMemberID memberId(address, AZ::Crc32(playerSessionId.c_str()));
        GameLiftServerMember* member = CreateReplicaChunk<GameLiftServerMember>(connId, memberId, this);
        member->m_peerMode.Set(peerMode);
        member->SetPlayerSessionId(playerSessionId.c_str());
        return member;
    }

    bool GameLiftServerSession::OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        bool isProcessed = GridSession::OnStateCreate(sm, e);

        switch (e.id)
        {
        case AZ::HSM::EnterEventId:
        {
            Aws::GameLift::GenericOutcome activationOutcome = GetGameLiftServerSDKWrapper().lock()->ActivateGameSession();
            if (!activationOutcome.IsSuccess())
            {
                AZ_TracePrintf("GridMate", "GameLift session activation failed: %s\n", activationOutcome.GetError().GetErrorMessage().c_str());
                RequestEvent(SE_DELETE);
            }
            else
            {
                RequestEvent(SE_CREATED);
            }
        }
        return true;
        }

        return isProcessed;
    }

    bool GameLiftServerSession::OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        bool isProcessed = GridSession::OnStateDelete(sm, e);

        switch (e.id)
        {
            case AZ::HSM::EnterEventId:
            {
                RequestEvent(SE_DELETED);
                return true;
            }
        }

        return isProcessed;
    }

    bool GameLiftServerSession::OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        (void)sm;
        (void)e;

        AZ_Assert(false, "Host migration is not supported for GameLift sessions.");
        return false;
    }

    void GameLiftServerSession::RegisterReplicaChunks()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftServerSessionReplica, GameLiftServerSessionReplica::GameLiftSessionReplicaDesc>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftServerMember, GameLiftServerMember::GameLiftServerMemberDesc>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftServerMemberState>();
    }

    AZStd::weak_ptr<GameLiftServerSDKWrapper> GameLiftServerSession::GetGameLiftServerSDKWrapper()
    {
        GameLiftServerService* gmService = static_cast<GameLiftServerService*>(m_service);
        return gmService->GetGameLiftServerSDKWrapper();
    }

    const std::vector<Aws::GameLift::Server::Model::PlayerSession> GameLiftServerSession::GetGameLiftPlayerSessions(const char* gameSessionId, Aws::GameLift::Server::Model::PlayerSessionStatus playerSessionStaus)
    {
        Aws::GameLift::Server::Model::DescribePlayerSessionsRequest request;
        request.SetPlayerSessionStatusFilter(Aws::GameLift::Server::Model::PlayerSessionStatusMapper::GetNameForPlayerSessionStatus(playerSessionStaus));
        request.SetLimit(m_gameLiftSession->GetMaximumPlayerSessionCount());
        request.SetGameSessionId(gameSessionId);
        Aws::GameLift::DescribePlayerSessionsOutcome playerSessionsOutcome = GetGameLiftServerSDKWrapper().lock()->DescribePlayerSessions(request);

        if (!playerSessionsOutcome.IsSuccess())
        {
            AZ_TracePrintf("GameLift", "describe Player Sessions failed error:%s", playerSessionsOutcome.GetError().GetErrorMessage().c_str());            
        }
        return playerSessionsOutcome.GetResult().GetPlayerSessions();
    }
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER
