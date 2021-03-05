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
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "GameLiftServerMocks.h"
#include "GameLift/Session/GameLiftServerSDKWrapper.h"
#include "GameLift/Session/GameLiftServerSession.h"
#include "GridMate/Session/Session.h"

#include <aws/gamelift/server/model/StartMatchBackfillRequest.h>

using namespace UnitTest;

namespace UnitTest
{
    const char* playerSessionId1 = "3DF3C373-7B81-43C7-841D-281C22411DBE";
    const char* playerSessionId2 = "41B5C363-8EB4-41DB-B87B-68C30C72FA14";
    const char* matchmakingTicketId = "TestMatchmakingTicketId";

    class GameLiftServerSessionMock : public GridMate::GameLiftServerSession
    {
    public:
        static void RegisterReplicaChunks()
        {
            GridMate::GameLiftServerSession::RegisterReplicaChunks();
        }

        using GameLiftServerSession::StartMatchmakingBackfill;
        using GameLiftServerSession::UpdateMatchmakerData;
        using GameLiftServerSession::StopMatchmakingBackfill;
        using GameLiftServerSession::GameSessionUpdated;
        using GameLiftServerSession::Update;
        using GameLiftServerSession::CreateRemoteMember;
        using GameLiftServerSession::m_matchmakerDataDocument;
        using GridMate::GridSession::m_myMember;

        GameLiftServerSessionMock(GridMate::GameLiftServerService* service) : GameLiftServerSession(service)
        {

        }

        bool Initialize(const GridMate::GameLiftSessionParams& params, const GridMate::CarrierDesc& carrierDesc)
        {
            return GameLiftServerSession::Initialize(params, carrierDesc);
        }

        void Update() override
        {
            GameLiftServerSession::Update();
        }

        void Shutdown() override
        {
            GameLiftServerSession::Shutdown();
        }
    };
}

class GameLiftServerSessionTest
    : public GameLiftServerAllocatorsFixture
{

public:
    GridMate::GameLiftSessionParams m_sessionParam;
    GridMate::CarrierDesc m_carrierDesc;
    Aws::GameLift::Server::Model::GameSession m_gameSession;

    const int m_maxPlayerSessionCount = 4; // Allowed max players
    const char* m_gameSessionId = "TestGameSession";
    const char* m_matchmakingConfigurationArn = "arn:aws:gamelift:us-west-2:729543576514:matchmakingconfiguration/MSTestConfig1";
    const char* m_teamName = "Players";

    // 2 players part of the current matchmaker data. 
    const char* m_matchmakerdata = "\
        {\
            \"matchId\": \"47379de7-9380-413f-9834-02299fb42dc9\",\
            \"matchmakingConfigurationArn\" : \"arn:aws:gamelift:us-west-2:729543576514:matchmakingconfiguration/MSTestConfig1\",\
            \"teams\":\
            [\
                {\
                    \"name\": \"Players\",\
                    \"players\" :\
                    [\
                        {\
                            \"playerId\": \"3DF3C373-7B81-43C7-841D-281C22411DBE\",\
                            \"attributes\" : {}\
                        },\
                        {\
                            \"playerId\": \"41B5C363-8EB4-41DB-B87B-68C30C72FA14\",\
                            \"attributes\" : {}\
                        }\
                    ]\
                }\
            ],\
            \"autoBackfillMode\": null,\
            \"autoBackfillTicketId\": null\
        }";

    GameLiftServerSessionMock *m_serverSession;

    GameLiftServerSessionTest()
    {
        m_gameSession.SetMaximumPlayerSessionCount(m_maxPlayerSessionCount);
        m_gameSession.SetGameSessionId(m_gameSessionId);
        m_gameSession.SetMatchmakerData(m_matchmakerdata);
    }

    void SetUp() override
    {
        GameLiftServerAllocatorsFixture::SetUp();

        m_sessionParam.m_gameSession = &m_gameSession;
        m_sessionParam.m_numPublicSlots = 4;
        m_serverSession = new GameLiftServerSessionMock(m_serverService);
        GameLiftServerSessionMock::RegisterReplicaChunks();
        GridMate::GameLiftServerSDKWrapper* wrapper = m_serverService->GetGameLiftServerSDKWrapper().lock().get();
        GameLiftServerSDKWrapperMock* sdkWrapperMock = (GameLiftServerSDKWrapperMock*)wrapper;
        EXPECT_CALL(*sdkWrapperMock, ActivateGameSession()).Times(1);;
        m_serverSession->Initialize(m_sessionParam, m_carrierDesc);

        AZ_Assert(m_serverSession->m_myMember != nullptr, "Expected local member to be created on initialize.");
        AZ_Assert(m_serverSession->m_myMember->IsHost(), "Expected local member to be set as host");
        AZ_Assert(m_serverSession->m_myMember->IsInvited(), "Expected local member to be set as invited");
        AZ_Assert(m_serverSession->m_myMember->GetPeerMode() == GridMate::RemotePeerMode::Mode_Peer
            , "Expected local member to be set as invited");

        m_serverSession->Update();
    }

    void TearDown() override
    {
        EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get()), TerminateGameSession()).Times(1);
        m_serverSession->Shutdown();
        delete m_serverSession;

        GameLiftServerAllocatorsFixture::TearDown();
    }

    DescribePlayerSessionsOutcome DescribePlayerSessionsMock([[maybe_unused]] const Aws::GameLift::Server::Model::DescribePlayerSessionsRequest& describePlayerSessionsRequest)
    {
        Server::Model::DescribePlayerSessionsResult result;
        Server::Model::PlayerSession playerSession1;
        playerSession1.SetPlayerId(playerSessionId1);
        result.AddPlayerSessions(playerSession1);

        Server::Model::PlayerSession playerSession2;
        playerSession2.SetPlayerId(playerSessionId2);
        result.AddPlayerSessions(playerSession2);
        DescribePlayerSessionsOutcome outcome(result);

        return outcome;
    }
};

AZ_PUSH_DISABLE_WARNING(4100, "-Wmissing-declarations")
MATCHER(StartMatchBackfillRequestMatchPlayerData, "Start matchbackfill request failed to match")
{    
    AZ_Assert(arg.GetPlayers().size() == 2, "Expected 2 player sessions");
    AZ_Assert(arg.GetPlayers()[0].GetPlayerId() == playerSessionId1, "Expected player session 1 id to match");
    AZ_Assert(arg.GetPlayers()[1].GetPlayerId() == playerSessionId2, "Expected player session 2 id to match");
    return true;
}

MATCHER(StartMatchBackfillRequestMatchTicketId, "Start matchbackfill retry request failed to match")
{
    AZ_Assert(arg.GetTicketId() == matchmakingTicketId, "Expected test matchmaking ticket id");    
    return true;
}
AZ_POP_DISABLE_WARNING

TEST_F(GameLiftServerSessionTest, StartMatchmakingSuccess)
{
    AZStd::string matchmakingTicket;

    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get())
        , StartMatchBackfill(StartMatchBackfillRequestMatchPlayerData())).Times(1);
    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get()), DescribePlayerSessions(_)).Times(1)
        .WillOnce(Invoke(this, &GameLiftServerSessionTest::DescribePlayerSessionsMock));
    bool result = m_serverSession->StartMatchmakingBackfill(matchmakingTicket, true);
    AZ_Assert(result, "Expected start matchmaking result to be success true");
    AZ_Assert(matchmakingTicket.c_str() != nullptr && strlen(matchmakingTicket.c_str()) != 0, "Expected matchmakingTicket to have value");
}

TEST_F(GameLiftServerSessionTest, StartMatchmaking_StartMatchBackfillError)
{
    AZStd::string matchmakingTicket = "";

    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get())
        , StartMatchBackfill(StartMatchBackfillRequestMatchPlayerData())).Times(1).WillOnce
        (
            Invoke([]([[maybe_unused]] const Server::Model::StartMatchBackfillRequest& request)
            {
                GameLiftError error(GAMELIFT_ERROR_TYPE::BAD_REQUEST_EXCEPTION, "TestError");
                StartMatchBackfillOutcome outcome(error);
                return outcome;
            })
        );
    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get()), DescribePlayerSessions(_)).Times(1)
        .WillOnce(Invoke(this, &GameLiftServerSessionTest::DescribePlayerSessionsMock));
    bool result = m_serverSession->StartMatchmakingBackfill(matchmakingTicket, true);
    AZ_Assert(!result, "Expected start matchmaking result to be false for failure");
    AZ_Assert(matchmakingTicket.size() == 0, "Expected matchmakingTicket to be empty");
}

TEST_F(GameLiftServerSessionTest, StartMatchmakingWithSetIdSuccess)
{
    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get())
        , StartMatchBackfill(StartMatchBackfillRequestMatchTicketId())).Times(1);
    AZStd::string ticketId = matchmakingTicketId;
    bool result = m_serverSession->StartMatchmakingBackfill(ticketId, true);
    AZ_Assert(result, "Expected start matchmaking retry result to be success true");
}

TEST_F(GameLiftServerSessionTest, StopMatchmakingSuccess)
{
    AZStd::string matchmakingTicket = "TestMatchmakingTicket";

    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get()), StopMatchBackfill(_)).Times(1);
    bool result = m_serverSession->StopMatchmakingBackfill(matchmakingTicket);

    AZ_Assert(result, "Expected stop matchmaking result to be success true");
}

TEST_F(GameLiftServerSessionTest, StopMatchmaking_StopMatchBackfillError)
{
    AZStd::string matchmakingTicket = "TestMatchmakingTicket";

    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get())
        , StopMatchBackfill(_)).Times(1).WillOnce(
            Invoke([]([[maybe_unused]] const Server::Model::StopMatchBackfillRequest& request)
            {
                GameLiftError error(GAMELIFT_ERROR_TYPE::BAD_REQUEST_EXCEPTION, "TestError");
                GenericOutcome outcome(error);
                return outcome;
            })
        );
    bool result = m_serverSession->StopMatchmakingBackfill(matchmakingTicket);

    AZ_Assert(!result, "Expected stop matchmaking to return false for failure");
}

TEST_F(GameLiftServerSessionTest, GameSessionUpdateSuccess)
{
    Aws::GameLift::Server::Model::UpdateGameSession updateGameSession(m_gameSession
        , Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED, "TestBackfillTicketId");

    bool result = m_serverSession->GameSessionUpdated(updateGameSession);

    AZ_Assert(result, "Expected GameSessionUpdated to return true");
    AZ_Assert(!m_serverSession->m_matchmakerDataDocument.HasParseError(), "Expected matchmaker data to parse correctly");
    AZ_Assert(m_serverSession->m_matchmakerDataDocument.HasMember("matchId"), "Expected matchId in matchmaker");
    AZ_Assert(m_serverSession->m_matchmakerDataDocument.HasMember("matchmakingConfigurationArn"), "Expected matchmakingConfigurationArn in matchmaker");
}

TEST_F(GameLiftServerSessionTest, GameSessionUpdate_TimedOutError)
{
    Aws::GameLift::Server::Model::UpdateGameSession updateGameSession(m_gameSession
        , Aws::GameLift::Server::Model::UpdateReason::BACKFILL_TIMED_OUT, "TestBackfillTicketId");
    bool result = m_serverSession->GameSessionUpdated(updateGameSession);

    AZ_Assert(!result, "Expected GameSessionUpdated to return false");
}

TEST_F(GameLiftServerSessionTest, CreateRemoteMemberSuccess)
{
    const char* playerId = "TestPlayerId";
    GridMate::ReadBuffer readBuffer(GridMate::kSessionEndian, playerId, sizeof(playerId));
    GridMate::ConnectionID connId = new GridMate::ConnectionCommon();

    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get()), AcceptPlayerSession(_)).Times(1);
    GridMate::GridMember* gridMember = m_serverSession->CreateRemoteMember("TestAddress", readBuffer, GridMate::RemotePeerMode::Mode_Peer, connId);

    AZ_Assert(gridMember != nullptr, "Expected a valid grid member");

    delete gridMember;
    delete connId;
}

TEST_F(GameLiftServerSessionTest, CreateRemoteMember_AcceptPlayerSessionError)
{
    const char* playerId = "TestPlayerId";
    GridMate::ReadBuffer readBuffer(GridMate::kSessionEndian, playerId, sizeof(playerId));
    GridMate::ConnectionID connId = new GridMate::ConnectionCommon();

    EXPECT_CALL(*((GameLiftServerSDKWrapperMock*)m_serverService->GetGameLiftServerSDKWrapper().lock().get())
        , AcceptPlayerSession(_)).Times(1).WillOnce(
            Invoke([]([[maybe_unused]] const std::string& playerSessionId)
            {
                GameLiftError error(GAMELIFT_ERROR_TYPE::BAD_REQUEST_EXCEPTION, "TestError");
                GenericOutcome outcome(error);
                return outcome;
            })
        );
    GridMate::GridMember* gridMember = m_serverSession->CreateRemoteMember("TestAddress", readBuffer, GridMate::RemotePeerMode::Mode_Peer, connId);

    AZ_Assert(gridMember == nullptr, "Expected grid member to be null");

    delete connId;
}
#endif
