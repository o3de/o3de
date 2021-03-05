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

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/CreateGameSessionRequest.h>
#include <aws/gamelift/model/CreateGameSessionResult.h>
#include <aws/gamelift/model/StartGameSessionPlacementRequest.h>
#include <aws/gamelift/model/StartGameSessionPlacementResult.h>
#include <aws/gamelift/model/DescribeGameSessionPlacementRequest.h>
#include <aws/gamelift/model/DescribeGameSessionPlacementResult.h>
#include <aws/gamelift/model/DescribeGameSessionDetailsResult.h>
#include <aws/gamelift/model/DescribeGameSessionDetailsRequest.h>
#include <aws/gamelift/model/SearchGameSessionsRequest.h>
#include <aws/gamelift/model/SearchGameSessionsResult.h>
#include <aws/gamelift/model/DescribeGameSessionQueuesResult.h>
#include <aws/gamelift/model/DescribeGameSessionQueuesRequest.h>
#include <aws/gamelift/model/StartMatchmakingRequest.h>
#include <aws/gamelift/model/StartMatchmakingResult.h>
#include <aws/gamelift/model/DescribeMatchmakingResult.h>
#include <aws/gamelift/model/DescribeMatchmakingRequest.h>
#include <aws/gamelift/model/DescribeGameSessionsRequest.h>
#include <aws/gamelift/model/DescribeGameSessionsResult.h>
#include <aws/gamelift/model/CreatePlayerSessionRequest.h>
#include <aws/gamelift/model/CreatePlayerSessionResult.h>
#include <aws/gamelift/model/ListBuildsRequest.h>
#include <aws/gamelift/model/ListBuildsResult.h>
#include <aws/gamelift/GameLiftErrors.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
AZ_POP_DISABLE_WARNING

#include "GameLiftMocks.h"
#include "GameLift/Session/GameLiftClientService.h"

using namespace Aws::GameLift;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::NiceMock;
using testing::Eq;

namespace UnitTest
{
    class GameLiftClientServiceEventsBusMock : public GridMate::GameLiftClientServiceEventsBus::Handler
    {
    public:
        GameLiftClientServiceEventsBusMock(GridMate::IGridMate* gridMate) : m_gridMate(gridMate)
        {
            GridMate::GameLiftClientServiceEventsBus::Handler::BusConnect(m_gridMate);
        }

        ~GameLiftClientServiceEventsBusMock()
        {
            GridMate::GameLiftClientServiceEventsBus::Handler::BusDisconnect(m_gridMate);
            m_gridMate = nullptr;
        }

        MOCK_METHOD1(OnGameLiftSessionServiceReady, void(GridMate::GameLiftClientService*));
        MOCK_METHOD2(OnGameLiftSessionServiceFailed, void(GridMate::GameLiftClientService*, const AZStd::string&));

    private:
        GridMate::IGridMate* m_gridMate;
    };

    class GameLiftClientMock : public GameLiftClient
    {
    public:
        const char* m_testFleetId = "fleet-TestFleetId";
        const char* m_testFleetArn = "fleet/fleet-TestFleetArn";
        const char* m_testGameSessionId = "TestGameSessionId";
        const char* m_testPlacementId = "TestPlacementId";
        const char* m_testQueueName = "TestQueueName";
        const char* m_testTicketId = "TestTicketId";
        int m_testGameSessionPort = 33435;
        const char* m_testGameSessionIp = "127.0.0.1";
        const char* m_testPlayerId = "TestPlayerId";
        const char* m_testPlayerSessionId = "TestPlayerSessionId";
        const char* m_testError = "TestError";

        GameLiftClientMock() : GameLiftClient(Aws::Auth::AWSCredentials())
        {
            ON_CALL(*this, CreateGameSessionCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::CreateGameSessionCallableMock));
            ON_CALL(*this, StartGameSessionPlacementCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::StartGameSessionPlacementCallableMock));
            ON_CALL(*this, DescribeGameSessionPlacementCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::DescribeGameSessionPlacementCallableMock));
            ON_CALL(*this, DescribeGameSessionDetails(_)).WillByDefault(Invoke(this, &GameLiftClientMock::DescribeGameSessionDetailsMock));
            ON_CALL(*this, SearchGameSessionsCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::SearchGameSessionsCallableMock));
            ON_CALL(*this, DescribeGameSessionQueuesCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::DescribeGameSessionQueuesCallableMock));
            ON_CALL(*this, StartMatchmakingCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::StartMatchmakingCallableMock));
            ON_CALL(*this, DescribeMatchmakingCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::DescribeMatchmakingCallableMock));
            ON_CALL(*this, DescribeGameSessionsCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::DescribeGameSessionsCallableMock));
            ON_CALL(*this, CreatePlayerSessionCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::CreatePlayerSessionCallableMock));
            ON_CALL(*this, ListBuildsCallable(_)).WillByDefault(Invoke(this, &GameLiftClientMock::ListBuildsCallableMock));
        }

        MOCK_CONST_METHOD1(CreateGameSessionCallable
                , Model::CreateGameSessionOutcomeCallable(const Model::CreateGameSessionRequest& request));

        MOCK_CONST_METHOD1(StartGameSessionPlacementCallable
            , Model::StartGameSessionPlacementOutcomeCallable(const Model::StartGameSessionPlacementRequest& request));

       MOCK_CONST_METHOD1(DescribeGameSessionPlacementCallable
            , Model::DescribeGameSessionPlacementOutcomeCallable(const Model::DescribeGameSessionPlacementRequest& request));

       MOCK_CONST_METHOD1(DescribeGameSessionDetails
           , Model::DescribeGameSessionDetailsOutcome(const Model::DescribeGameSessionDetailsRequest& request));

       MOCK_CONST_METHOD1(SearchGameSessionsCallable
           , Model::SearchGameSessionsOutcomeCallable(const Model::SearchGameSessionsRequest& request));

       MOCK_CONST_METHOD1(DescribeGameSessionQueuesCallable
           , Model::DescribeGameSessionQueuesOutcomeCallable(const Model::DescribeGameSessionQueuesRequest& request));

       MOCK_CONST_METHOD1(StartMatchmakingCallable
           , Model::StartMatchmakingOutcomeCallable(const Model::StartMatchmakingRequest& request));

       MOCK_CONST_METHOD1(DescribeMatchmakingCallable
           , Model::DescribeMatchmakingOutcomeCallable(const Model::DescribeMatchmakingRequest& request));

       MOCK_CONST_METHOD1(DescribeGameSessionsCallable
           , Model::DescribeGameSessionsOutcomeCallable(const Model::DescribeGameSessionsRequest& request));

       MOCK_CONST_METHOD1(CreatePlayerSessionCallable
           , Model::CreatePlayerSessionOutcomeCallable(const Model::CreatePlayerSessionRequest& request));

       MOCK_CONST_METHOD1(ListBuildsCallable
           , Model::ListBuildsOutcomeCallable(const Model::ListBuildsRequest& request));

        Model::CreateGameSessionOutcomeCallable CreateGameSessionCallableMock([[maybe_unused]] const Model::CreateGameSessionRequest& request) const
        {
            Model::GameSession gameSession = GetGameSessionMock();
            Model::CreateGameSessionResult result;
            result.SetGameSession(gameSession);
            Model::CreateGameSessionOutcome outcome(result);
            return GetFuture<Model::CreateGameSessionOutcome>(outcome);
        }

        Model::StartGameSessionPlacementOutcomeCallable StartGameSessionPlacementCallableMock([[maybe_unused]] const Model::StartGameSessionPlacementRequest& request) const
        {
            Model::GameSession gameSession = GetGameSessionMock();
            Model::GameSessionPlacement placement = Model::GameSessionPlacement();
            placement.SetPlacementId(m_testPlacementId);
            Model::StartGameSessionPlacementResult result;
            result.SetGameSessionPlacement(placement);
            Model::StartGameSessionPlacementOutcome outcome(result);
            return GetFuture<Model::StartGameSessionPlacementOutcome>(outcome);
        }

        Model::DescribeGameSessionPlacementOutcomeCallable DescribeGameSessionPlacementCallableMock([[maybe_unused]] const Model::DescribeGameSessionPlacementRequest& request) const
        {
            Model::GameSessionPlacement placement = Model::GameSessionPlacement();
            placement.SetPlacementId(m_testPlacementId);
            placement.SetStatus(Model::GameSessionPlacementState::FULFILLED);
            placement.SetGameSessionId(m_testGameSessionId);
            Model::DescribeGameSessionPlacementResult result;
            result.SetGameSessionPlacement(placement);
            Model::DescribeGameSessionPlacementOutcome outcome(result);
            return GetFuture<Model::DescribeGameSessionPlacementOutcome>(outcome);
        }

        Model::DescribeGameSessionDetailsOutcome DescribeGameSessionDetailsMock([[maybe_unused]] const Model::DescribeGameSessionDetailsRequest& request) const
        {
            AZStd::vector<Model::GameSessionDetail> gameSessionDetails;
            Model::GameSessionDetail detail;
            detail.SetGameSession(GetGameSessionMock());
            gameSessionDetails.push_back(detail);
            Model::DescribeGameSessionDetailsResult result;
            result.AddGameSessionDetails(detail);
            Model::DescribeGameSessionDetailsOutcome outcome(result);
            return outcome;
        }

        Model::SearchGameSessionsOutcomeCallable SearchGameSessionsCallableMock([[maybe_unused]] const Model::SearchGameSessionsRequest& request) const
        {
            Model::SearchGameSessionsResult result;
            result.AddGameSessions(GetGameSessionMock());;
            Model::SearchGameSessionsOutcome outcome(result);
            std::promise<Model::SearchGameSessionsOutcome> outcomePromise;
            outcomePromise.set_value(outcome);
            return outcomePromise.get_future();
        }

        Model::DescribeGameSessionQueuesOutcomeCallable DescribeGameSessionQueuesCallableMock([[maybe_unused]] const Model::DescribeGameSessionQueuesRequest& request) const
        {
            Model::DescribeGameSessionQueuesResult result;
            Model::GameSessionQueue queue;
            queue.SetName(m_testQueueName);
            Model::GameSessionQueueDestination destination;
            destination.SetDestinationArn(m_testFleetArn);
            queue.AddDestinations(destination);
            result.AddGameSessionQueues(queue);
            Model::DescribeGameSessionQueuesOutcome outcome(result);
            return GetFuture<Model::DescribeGameSessionQueuesOutcome>(outcome);
        }

        Model::StartMatchmakingOutcomeCallable StartMatchmakingCallableMock([[maybe_unused]] const Model::StartMatchmakingRequest& request) const
        {
            Model::StartMatchmakingResult result;
            Model::MatchmakingTicket ticket;
            ticket.SetTicketId(m_testTicketId);
            result.SetMatchmakingTicket(ticket);
            Model::StartMatchmakingOutcome outcome(result);
            return GetFuture<Model::StartMatchmakingOutcome>(outcome);
        }

        Model::DescribeMatchmakingOutcomeCallable DescribeMatchmakingCallableMock([[maybe_unused]] const Model::DescribeMatchmakingRequest& request) const
        {
            Model::DescribeMatchmakingResult result;
            Model::MatchmakingTicket ticket;
            ticket.SetTicketId(m_testTicketId);
            ticket.SetStatus(Model::MatchmakingConfigurationStatus::COMPLETED);
            Model::GameSessionConnectionInfo connectionInfo;
            connectionInfo.SetGameSessionArn(m_testGameSessionId);
            connectionInfo.SetPort(m_testGameSessionPort);
            connectionInfo.SetIpAddress(m_testGameSessionIp);
            Model::MatchedPlayerSession playerSession;
            playerSession.SetPlayerId(m_testPlayerId);
            connectionInfo.AddMatchedPlayerSessions(playerSession);
            ticket.SetGameSessionConnectionInfo(connectionInfo);
            result.AddTicketList(ticket);
            Model::DescribeMatchmakingOutcome outcome(result);
            return GetFuture<Model::DescribeMatchmakingOutcome>(outcome);
        }

        Model::DescribeGameSessionsOutcomeCallable DescribeGameSessionsCallableMock([[maybe_unused]] const Model::DescribeGameSessionsRequest& request) const
        {
            Model::DescribeGameSessionsResult result;
            result.AddGameSessions(GetGameSessionMock());
            Model::DescribeGameSessionsOutcome outcome(result);
            return GetFuture<Model::DescribeGameSessionsOutcome>(outcome);
        }

        Model::CreatePlayerSessionOutcomeCallable CreatePlayerSessionCallableMock([[maybe_unused]] const Model::CreatePlayerSessionRequest& request) const
        {
            Model::CreatePlayerSessionResult result;
            result.SetPlayerSession(GetPlayerSessionMock());
            Model::CreatePlayerSessionOutcome outcome(result);
            return GetFuture<Model::CreatePlayerSessionOutcome>(outcome);
        }

        Model::ListBuildsOutcomeCallable ListBuildsCallableMock([[maybe_unused]] const Model::ListBuildsRequest& request) const
        {
            Model::ListBuildsResult result;
            Model::Build build;
            result.AddBuilds(build);
            Model::ListBuildsOutcome outcome(result);
            return GetFuture<Model::ListBuildsOutcome>(outcome);
        }

        template <typename T, typename U>
        std::future<T> CallableErrorMock([[maybe_unused]] const U& request) const
        {
            Aws::Client::AWSError<GameLiftErrors> error;
            error.SetExceptionName(m_testError);
            T outcome(error);
            std::promise<T> outcomePromise;
            outcomePromise.set_value(outcome);
            return outcomePromise.get_future();
        }

        template <typename T, typename U>
        T ErrorMock([[maybe_unused]] const U& request) const
        {
            Aws::Client::AWSError<GameLiftErrors> error;
            error.SetExceptionName(m_testError);
            T outcome(error);
            return outcome;   
        }

    private:

        Model::GameSession GetGameSessionMock() const
        {
            Model::GameSession gameSession;
            gameSession.SetFleetId(m_testFleetId);
            gameSession.SetGameSessionId(m_testGameSessionId);
            gameSession.SetMaximumPlayerSessionCount(2);
            gameSession.SetCurrentPlayerSessionCount(1);
            gameSession.SetStatus(Model::GameSessionStatus::ACTIVE);
            return gameSession;
        }

        Model::PlayerSession GetPlayerSessionMock() const
        {
            Model::PlayerSession playerSession;
            playerSession.SetGameSessionId(m_testGameSessionId);
            playerSession.SetPlayerSessionId(m_testPlayerSessionId);
            playerSession.SetIpAddress(m_testGameSessionIp);
            playerSession.SetPort(m_testGameSessionPort);
            return playerSession;
        }
    };

    class GameLiftClientServiceMock : public GridMate::GameLiftClientService
    {

    public:
        GameLiftClientServiceMock(const GridMate::GameLiftClientServiceDesc desc): GameLiftClientService(desc)
        {
            m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
            m_clientSharedPtr = AZStd::make_shared<GameLiftClientMock>();
        }
        ~GameLiftClientServiceMock()
        {
            m_clientSharedPtr.reset();
            GridMate::GridMateDestroy(m_gridMate);
            m_gridMate = nullptr;
        }

        bool StartGameLiftClient()
        {
            return GameLiftClientService::StartGameLiftClient();
        }

        void CreateSharedAWSGameLiftClient() override
        {
        }
    };

    class GameLiftAllocatorsFixture
        : public AllocatorsTestFixture
    {
        Aws::SDKOptions m_sdkOptions;

    public:

        const char* m_testAccessKey = "TestAccessKey";
        const char* m_testSecretKey = "TestSecretKey";

        GameLiftAllocatorsFixture()
            : AllocatorsTestFixture()
        {
        }

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            Aws::InitAPI(m_sdkOptions);

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

            m_gameLiftClient = AZStd::make_shared<GameLiftClientMock>();

            m_gameLiftRequestInterfaceContext = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();
            m_gameLiftRequestInterfaceContext->m_gameLiftClient = m_gameLiftClient;

            GridMate::GameLiftClientServiceDesc desc;
            desc.m_accessKey = m_testAccessKey;
            desc.m_secretKey = m_testSecretKey;
            m_clientService = new NiceMock<GameLiftClientServiceMock>(desc);

            m_sessionEventBusMock = new NiceMock<SessionEventBusMock>(m_clientService->GetGridMate());
            m_glClientServiceEventBusMock = new NiceMock<GameLiftClientServiceEventsBusMock>(m_clientService->GetGridMate());
        }

        void TearDown() override
        {
            m_gameLiftRequestInterfaceContext.reset();

            m_gameLiftClient.reset();

            delete m_sessionEventBusMock;
            m_sessionEventBusMock = nullptr;

            delete m_glClientServiceEventBusMock;
            m_glClientServiceEventBusMock = nullptr;

            delete m_clientService;
            m_clientService = nullptr;

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
            Aws::ShutdownAPI(m_sdkOptions);

            AllocatorsTestFixture::TearDown();
        }

        AZStd::shared_ptr<GameLiftClientMock> m_gameLiftClient;
        NiceMock<GameLiftClientServiceMock> *m_clientService;
        NiceMock<SessionEventBusMock> *m_sessionEventBusMock;
        NiceMock<GameLiftClientServiceEventsBusMock> *m_glClientServiceEventBusMock;
        AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> m_gameLiftRequestInterfaceContext;
    };
};
#endif
