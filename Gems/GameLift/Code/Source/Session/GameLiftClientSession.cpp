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

#if defined(BUILD_GAMELIFT_CLIENT)

#include <AzCore/PlatformIncl.h>

#include <GameLift/Session/GameLiftClientSession.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftSearch.h>
#include <GridMate/Online/UserServiceTypes.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaFunctions.h>

#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Carrier/Utils.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>

#include <aws/core/utils/Outcome.h>

// To avoid the warning below
// Semaphore.h(50): warning C4251: 'Aws::Utils::Threading::Semaphore::m_mutex': class 'std::mutex' needs to have dll-interface to be used by clients of class 'Aws::Utils::Threading::Semaphore'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <aws/gamelift/model/DescribeGameSessionsRequest.h>
#include <aws/gamelift/model/CreatePlayerSessionRequest.h>
#include <aws/gamelift/model/CreatePlayerSessionResult.h>
AZ_POP_DISABLE_WARNING

namespace
{
    const int k_minGameSessionRetryInterval = 100; //< 100 msec minimum interval for retrying gamesession status
    const int k_maxGameSessionRetries = 8; //< max 8 retries -> ~50 sec
    const int k_gameSessionRetryBase = 200; //< base for exponential retries (200, 400, 800, 1600, 3200 msec, etc...)
}

namespace GridMate
{
    class GameLiftMember;

    //-----------------------------------------------------------------------------
    // GameLiftSessionReplica
    //-----------------------------------------------------------------------------
    class GameLiftSessionReplica
        : public Internal::GridSessionReplica
    {
    public:
        class GameLiftSessionReplicaDesc
            : public ReplicaChunkDescriptor
        {
        public:
            GameLiftSessionReplicaDesc()
                : ReplicaChunkDescriptor(GameLiftSessionReplica::GetChunkName(), sizeof(GameLiftSessionReplica))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
            {
                GameLiftClientSession* session = static_cast<GameLiftClientSession*>(static_cast<GridSession*>(mc.m_rm->GetUserContext(AZ_CRC("GridSession", 0x099df4e6))));
                AZ_Assert(session, "We need to have a valid session!");
                return session->OnSessionReplicaArrived();
            }

            void DiscardCtorStream(UnmarshalContext&) override {}
            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }
            void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override {}
        };

        GM_CLASS_ALLOCATOR(GameLiftSessionReplica);

        static const char* GetChunkName()
        {
            return "GridMate::GameLiftSessionReplica";
        }

        GameLiftSessionReplica(GameLiftClientSession* session)
            : GridSessionReplica(session)
        {
        }
    };
    //-----------------------------------------------------------------------------

    // Maintain size and alignment same for corresponding classes in GameLiftServerSession
    // GameLiftMemberId -> GameLiftServerMemeberId
    // GameLiftMemeber -> GameLiftServerMember

    //-----------------------------------------------------------------------------
    // GameLiftMemberID
    //-----------------------------------------------------------------------------
    class GameLiftMemberID
        : public MemberID
    {
    public:
        //-----------------------------------------------------------------------------
        // Marshaler
        //-----------------------------------------------------------------------------
        class Marshaler
        {
        public:
            void Marshal(WriteBuffer& wb, const GameLiftMemberID& id) const
            {
                wb.Write(id.m_id);
            }

            void Unmarshal(GameLiftMemberID& id, ReadBuffer& rb) const
            {
                rb.Read(id.m_id);
            }
        };
        //-----------------------------------------------------------------------------

        GameLiftMemberID()
            : m_id(0)
        {
        }

        explicit GameLiftMemberID(AZ::u32 memberId)
            : m_id(memberId)
        {
            AZ_Assert(m_id != 0, "Invalid member id");
        }

        string ToString() const override { return string::format("%08X", m_id); }
        string ToAddress() const override { return ToString(); }
        MemberIDCompact Compact() const override { return m_id; }
        bool IsValid() const override { return m_id != 0; }

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
        CtorDataSet<GameLiftMemberID, GameLiftMemberID::Marshaler> m_memberId;
        CtorDataSet<RemotePeerMode> m_peerMode;
        CtorDataSet<bool> m_isHost;
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftMemberState
    //-----------------------------------------------------------------------------
    class GameLiftMemberState
        : public Internal::GridMemberStateReplica
    {
    public:
        GM_CLASS_ALLOCATOR(GameLiftMemberState);
        static const char* GetChunkName() { return "GameLiftMemberState"; }

        explicit GameLiftMemberState(GridMember* member = nullptr)
            : GridMemberStateReplica(member)
        {
        }
    };
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftMember
    //-----------------------------------------------------------------------------
    class GameLiftMember
        : public GridMember
    {
        friend class GameLiftClientSession;

    public:
        class GameLiftMemberDesc
            : public ReplicaChunkDescriptor
        {
        public:
            GameLiftMemberDesc()
                : ReplicaChunkDescriptor(GameLiftMember::GetChunkName(), sizeof(GameLiftMember))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
            {
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);

                GameLiftClientSession* session = static_cast<GameLiftClientSession*>(static_cast<GridSession*>(mc.m_rm->GetUserContext(AZ_CRC("GridSession"))));
                AZ_Assert(session, "Invalid session");

                GameLiftMemberID memberId = ctorContext.m_memberId.Get();
                RemotePeerMode remotePeerMode = ctorContext.m_peerMode.Get();
                bool isMemberHost = ctorContext.m_isHost.Get();

                GameLiftMember* member = nullptr;
                if (memberId != session->GetMyMember()->GetId())
                {
                    // Put the appscale id back into a buffer so it can be passed to CreateRemoteMember
                    WriteBufferDynamic memberIdBuf(EndianType::IgnoreEndian);
                    memberIdBuf.Write(memberId.Compact());
                    ReadBuffer rb(memberIdBuf.GetEndianType(), memberIdBuf.Get(), memberIdBuf.Size());
                    member = static_cast<GameLiftMember*>(session->CreateRemoteMember(memberId.ToAddress(), rb, remotePeerMode, isMemberHost ? mc.m_peer->GetConnectionId() : InvalidConnectionID));
                }
                else
                {
                    member = static_cast<GameLiftMember*>(session->GetMyMember());
                }

                bool isAdded = session->AddMember(member);
                AZ_Assert(isAdded, "Failed to add a member, there is something wrong with the member replicas!");
                if (!isAdded)
                {
                    member = nullptr;
                    AZ_TracePrintf("GameLift", "[CLIENT SESSION] Failed to add a member, there is something wrong with the member replicas, peerid %d", mc.m_peer ? mc.m_peer->GetId() : 0);
                }
                else
                {
                    AZ_TracePrintf("GameLift", "[CLIENT SESSION] Added a member, peerid %d", mc.m_peer ? mc.m_peer->GetId() : 0);
                }

                return member;
            }

            void DiscardCtorStream(UnmarshalContext& mc) override
            {
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);
            }

            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override
            {
                if (!static_cast<GameLiftMember*>(chunkInstance)->IsLocal())
                {
                    delete chunkInstance;
                }
            }

            void MarshalCtorData(ReplicaChunkBase* chunkInstance, WriteBuffer& wb) override
            {
                GameLiftMember* member = static_cast<GameLiftMember*>(chunkInstance);
                GameLiftMemberInfoCtorContext ctorContext;
                ctorContext.m_memberId.Set(member->m_memberId);
                ctorContext.m_peerMode.Set(member->m_peerMode.Get());
                ctorContext.m_isHost.Set(member->IsHost());
                ctorContext.Marshal(wb);
            }
        };

        GM_CLASS_ALLOCATOR(GameLiftMember);

        static const char* GetChunkName() { return "GridMate::GameLiftMember"; }

        const PlayerId* GetPlayerId() const override
        {
            return nullptr;
        }

        const MemberID& GetId() const override
        {
            return m_memberId;
        }

        /// Remote member ctor.
        GameLiftMember(ConnectionID connId, const GameLiftMemberID& memberId, GameLiftClientSession* session)
            : GridMember(memberId.Compact())
            , m_memberId(memberId)
        {
            m_session = session;
            m_connectionId = connId;
        }

        /// Local member ctor.
        GameLiftMember(const GameLiftMemberID& memberId, GameLiftClientSession* session)
            : GridMember(memberId.Compact())
            , m_memberId(memberId)
        {
            m_session = session;

            m_clientState = CreateReplicaChunk<GameLiftMemberState>(this);
            m_clientState->m_name.Set(memberId.ToString());

            m_clientStateReplica = Replica::CreateReplica(memberId.ToString().c_str());
            m_clientStateReplica->AttachReplicaChunk(m_clientState);
        }

        void OnReplicaDeactivate(const ReplicaContext& rc) override
        {
            GridMember::OnReplicaDeactivate(rc);

            AZ_TracePrintf("GameLift", "[CLIENT SESSION] Deactivating a replica, peerid %d", rc.m_peer ? rc.m_peer->GetId() : 0);
        }

        using GridMember::SetHost;

        GameLiftMemberID m_memberId;
        string m_playerSessionId;
    };
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------


    //-----------------------------------------------------------------------------
    // GameLiftClientSession
    //-----------------------------------------------------------------------------
    GameLiftClientSession::GameLiftClientSession(GameLiftClientService* service)
        : GridSession(service)
        , m_gameSessionRetryTimeout(0)
        , m_numGameSessionRetryAttempts(0)
        , m_clientService(service)
    {
    }

    bool GameLiftClientSession::Initialize(const GameLiftSearchInfo& info, const JoinParams& params, const CarrierDesc& carrierDesc)
    {
        (void)params;

        if (!GridSession::Initialize(carrierDesc))
        {
            return false;
        }

        m_state = CreateReplicaChunk<GameLiftSessionReplica>(this);

        m_searchInfo = info;

        m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_GAMELIFT_INIT), AZ::HSM::StateHandler(this, &GameLiftClientSession::OnStateGameLiftInit), SS_NO_SESSION);
        SetUpStateMachine();

        // If player session id is already set then join existing player session.
        if (!info.m_playerSessionId.empty())
        {
            m_playerSession.SetGameSessionId(m_searchInfo.m_sessionId.c_str());
            m_playerSession.SetPlayerSessionId(m_searchInfo.m_playerSessionId.c_str());
            m_playerSession.SetPort(m_searchInfo.m_port);
            m_playerSession.SetIpAddress(m_searchInfo.m_ipAddress.c_str());

            SetGameLiftLocalParams();

            m_sessionId = m_searchInfo.m_sessionId;
            RequestEvent(SE_MATCHMAKING_JOIN);
        }
        else
        {
            RequestEvent(SE_JOIN);
        }

        return true;
    }

    GridMember* GameLiftClientSession::CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode)
    {
        (void)isInvited;
        AZ_Assert(!isHost, "GameLiftClientSession can never run as host!");
        AZ_Assert(!m_myMember, "We already have added a local member!");

        string ip = Utils::GetMachineAddress(m_carrierDesc.m_familyType);
        string address = SocketDriverCommon::IPPortToAddressString(ip.c_str(), m_carrier->GetPort());
        string playerSessionId = m_playerSession.GetPlayerSessionId().c_str();

        AZ_Assert(!playerSessionId.empty(), "GameLift clients must have a valid playerSessionId to connect to the server!");
        GameLiftMemberID myId(AZ::Crc32(playerSessionId.c_str()));

        GameLiftMember* member = CreateReplicaChunk<GameLiftMember>(myId, this);
        member->SetHost(isHost);
        member->m_peerMode.Set(peerMode);
        return member;
    }

    GameLiftSessionReplica* GameLiftClientSession::OnSessionReplicaArrived()
    {
        AZ_TracePrintf("GameLift", "(%s - %s) has joined session: %s\n",
            m_myMember->GetId().ToString().c_str(),
            m_myMember->GetId().ToAddress().c_str(),
            m_sessionId.c_str());

        RequestEvent(GridSession::SE_JOINED);
        return static_cast<GameLiftSessionReplica*>(m_state.get());
    }

    bool GameLiftClientSession::OnStateGameLiftInit(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        switch (e.id)
        {
            case SE_RECEIVED_GAMESESSION:
            {
                const Aws::GameLift::Model::DescribeGameSessionsResult* result = reinterpret_cast<const Aws::GameLift::Model::DescribeGameSessionsResult*>(e.userData);

                if (result->GetGameSessions().size() != 1)
                {
                    AZ_TracePrintf("GridMate", "Game session does not exist %s\n", m_searchInfo.m_sessionId.c_str());
                    RequestEvent(SE_DELETE);
                    return true;
                }

                const Aws::GameLift::Model::GameSession& gameSession = result->GetGameSessions().front();
                if (gameSession.GetStatus() == Aws::GameLift::Model::GameSessionStatus::ACTIVE)
                {
                    Aws::GameLift::Model::CreatePlayerSessionRequest request;
                    request.WithGameSessionId(m_searchInfo.m_sessionId.c_str()).WithPlayerId(m_clientService->GetPlayerId());
                    m_createPlayerSessionOutcomeCallable = m_clientService->GetClient()->CreatePlayerSessionCallable(request);
                }
                else if (gameSession.GetStatus() == Aws::GameLift::Model::GameSessionStatus::ACTIVATING && m_numGameSessionRetryAttempts < k_maxGameSessionRetries)
                {
                    m_gameSessionRetryTimeout = k_minGameSessionRetryInterval + k_gameSessionRetryBase * (1 << m_numGameSessionRetryAttempts);
                    m_gameSessionRetryTimestamp = AZStd::chrono::system_clock::now();
                    ++m_numGameSessionRetryAttempts;
                }
                else
                {
                    AZ_TracePrintf("GridMate", "Failed to activate session %s\n", gameSession.GetGameSessionId().c_str());
                    sm.Transition(SS_NO_SESSION);
                }
                return true;
            }

            case SE_RECEIVED_PLAYERSESSION:
            {
                m_playerSession = *reinterpret_cast<const Aws::GameLift::Model::PlayerSession*>(e.userData);
                SetGameLiftLocalParams();
                m_sessionId = m_playerSession.GetGameSessionId().c_str();
                sm.Transition(SS_CREATE);
                return true;
            }

            case SE_CLIENT_FAILED:
            {
                sm.Transition(SS_NO_SESSION);
                return true;
            }
        }
        return false;
    }

    void GameLiftClientSession::SetGameLiftLocalParams()
    {
        const auto& clientEndpoint = m_clientService->GetEndpoint();
        //To support GameLiftLocal on a remote server, convert the reported 127.0.0.1
                // address to the configured GameLiftLocal endpoint address
        if (m_clientService->UseGameLiftLocal() &&
            m_playerSession.GetIpAddress().compare("127.0.0.1") == 0 &&
            //Ignore actual loopback connections
            clientEndpoint.find("localhost") == -1 &&
            clientEndpoint.find("127.") != 0)
        {
            const auto portLocation = clientEndpoint.find(":");
            if (portLocation != -1)
            {
                //Copy only the host name/address
                m_playerSession.SetIpAddress(clientEndpoint.substr(0, portLocation).c_str());
            }
            else
            {
                m_playerSession.SetIpAddress(clientEndpoint.c_str());
            }
        }
    }

    bool GameLiftClientSession::OnStateStartup(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        switch (e.id)
        {
            case SE_JOIN:
            {
                sm.Transition(SS_GAMELIFT_INIT);
                return true;
            }
            case SE_MATCHMAKING_JOIN:
            {
                sm.Transition(SS_CREATE);
                return true;
            }
        }
        return false;
    }

    bool GameLiftClientSession::OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        bool isProcessed = GridSession::OnStateCreate(sm, e);

        switch (e.id)
        {
            case AZ::HSM::EnterEventId:
            {
                AZ_Assert(m_carrier, "Carrier must be created!");
                m_myMember = CreateLocalMember(false, false, Mode_Peer);

                // setting player's session id for handshake
                WriteBufferStatic<> wb(kSessionEndian);
                string playerSessionId = m_playerSession.GetPlayerSessionId().c_str();
                wb.Write(playerSessionId);
                SetHandshakeUserData(wb.Get(), wb.Size());

                Aws::String resolvedIp = m_playerSession.GetIpAddress();
                if (!resolvedIp.empty())
                {
                    m_hostAddress = SocketDriverCommon::IPPortToAddressString(resolvedIp.c_str(), m_playerSession.GetPort());
                    RequestEvent(SE_CREATED);
                }
                else
                {
                    AZ_TracePrintf("GameLift", "Error retrieving ipAddress for player session.\n");
                    sm.Transition(SS_DELETE);
                    return true;
                }
                return true;
            }
        }

        return isProcessed;
    }

    bool GameLiftClientSession::OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        bool isProcessed = GridSession::OnStateDelete(sm, e);

        switch (e.id)
        {
        case AZ::HSM::EnterEventId:
        {
            RequestEvent(SE_DELETED);
        }
        return true;
        }

        return isProcessed;
    }

    bool GameLiftClientSession::OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e)
    {
        (void)sm;
        (void)e;

        AZ_Assert(false, "Host migration is not supported for GameLift sessions.");
        return false;
    }

    GridMember* GameLiftClientSession::CreateRemoteMember(const string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId)
    {
        (void)address;

        AZ::u32 remoteId = 0;
        if (data.Read(remoteId))
        {
            GameLiftMemberID memberId(remoteId);
            GameLiftMember* member = CreateReplicaChunk<GameLiftMember>(connId, memberId, this);
            member->m_peerMode.Set(peerMode);
            return member;
        }
        else
        {
            return nullptr;
        }
    }

    void GameLiftClientSession::Update()
    {
        if (m_gameSessionRetryTimeout >= 0)
        {
            if (AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - m_gameSessionRetryTimestamp).count() >= m_gameSessionRetryTimeout)
            {
                Aws::GameLift::Model::DescribeGameSessionsRequest request;
                request.SetGameSessionId(m_searchInfo.m_sessionId.c_str());
                m_describeGameSessionsOutcomeCallable = m_clientService->GetClient()->DescribeGameSessionsCallable(request);

                m_gameSessionRetryTimeout = -1;
            }
        }

        if (m_describeGameSessionsOutcomeCallable.valid()
            && m_describeGameSessionsOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_describeGameSessionsOutcomeCallable.get();
            if (result.IsSuccess())
            {
                RequestEventData(SE_RECEIVED_GAMESESSION, result.GetResult());
            }
            else
            {
                AZ_TracePrintf("GameLift", "Failed to get game session: %s\n", result.GetError().GetMessage().c_str());
                RequestEvent(SE_CLIENT_FAILED);
            }
        }

        if (m_createPlayerSessionOutcomeCallable.valid()
            && m_createPlayerSessionOutcomeCallable.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            auto result = m_createPlayerSessionOutcomeCallable.get();
            if (result.IsSuccess())
            {
                RequestEventData(SE_RECEIVED_PLAYERSESSION, result.GetResult());
            }
            else
            {
                AZ_TracePrintf("GameLift", "Failed to entitle session: %s\n", result.GetError().GetMessage().c_str());
                RequestEvent(SE_CLIENT_FAILED);
            }
        }

        GridSession::Update();
    }

    void GameLiftClientSession::RegisterReplicaChunks()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftSessionReplica, GameLiftSessionReplica::GameLiftSessionReplicaDesc>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftMember, GameLiftMember::GameLiftMemberDesc>();
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<GameLiftMemberState>();
    }
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
