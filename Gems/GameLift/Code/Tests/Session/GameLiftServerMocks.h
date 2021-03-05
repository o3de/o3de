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
#include "GameLift/Session/GameLiftServerService.h"
#include "GameLift/Session/GameLiftServerSDKWrapper.h"
#include "GameLift/Session/GameLiftServerServiceBus.h"

#include "GameLiftMocks.h"

using namespace Aws::GameLift;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::NiceMock;
using testing::Eq;

namespace UnitTest
{
    class GameLiftServerSDKWrapperMock : public GridMate::GameLiftServerSDKWrapper
    {
    public:
        GameLiftServerSDKWrapperMock()
        {
            GenericOutcome successOutcome(nullptr);
            Server::InitSDKOutcome sdkOutcome(nullptr);
            Server::Model::StartMatchBackfillResult startMatchBackfillResult;
            startMatchBackfillResult.SetTicketId("TestTicketId");
            StartMatchBackfillOutcome startMatchBackfillOutcome(startMatchBackfillResult);

            Server::Model::DescribePlayerSessionsResult describePlayerSessionsResult;
            Server::Model::PlayerSession playerSession;
            playerSession.SetPlayerId("TestPlayer1");
            Server::Model::PlayerSession playerSession1;
            playerSession.SetPlayerId("TestPlayer2");
            describePlayerSessionsResult.AddPlayerSessions(playerSession);
            describePlayerSessionsResult.AddPlayerSessions(playerSession1);

            DescribePlayerSessionsOutcome describePlayerSessionsOutcome(describePlayerSessionsResult);

            ON_CALL(*this, InitSDK()).WillByDefault(Return(sdkOutcome));
            ON_CALL(*this, ProcessReadyAsync(_)).WillByDefault(Invoke(this, &GameLiftServerSDKWrapperMock::ProcessReadyAsyncMock));
            ON_CALL(*this, ProcessEnding()).WillByDefault(Return(successOutcome));
            ON_CALL(*this, ActivateGameSession()).WillByDefault(Return(successOutcome));
            ON_CALL(*this, TerminateGameSession()).WillByDefault(Return(successOutcome));
            ON_CALL(*this, AcceptPlayerSession(_)).WillByDefault(Return(successOutcome));
            ON_CALL(*this,RemovePlayerSession(_)).WillByDefault(Return(successOutcome));
            ON_CALL(*this,DescribePlayerSessions(_)).WillByDefault(Return(describePlayerSessionsOutcome));
            ON_CALL(*this, StartMatchBackfill(_)).WillByDefault(Return(startMatchBackfillOutcome));
            ON_CALL(*this, StopMatchBackfill(_)).WillByDefault(Return(successOutcome));
        }
        MOCK_METHOD0(InitSDK, Server::InitSDKOutcome());
        MOCK_METHOD1(ProcessReadyAsync, GenericOutcomeCallable(const Server::ProcessParameters &processParameters));
        MOCK_METHOD0(ProcessEnding, GenericOutcome());
        MOCK_METHOD0(ActivateGameSession, GenericOutcome());
        MOCK_METHOD0(TerminateGameSession, GenericOutcome());
        MOCK_METHOD1(AcceptPlayerSession, GenericOutcome(const std::string& playerSessionId));
        MOCK_METHOD1(RemovePlayerSession, GenericOutcome(const char* playerSessionId));
        MOCK_METHOD1(DescribePlayerSessions, DescribePlayerSessionsOutcome(const Server::Model::DescribePlayerSessionsRequest &describePlayerSessionsRequest));
        MOCK_METHOD1(StartMatchBackfill, StartMatchBackfillOutcome(const Server::Model::StartMatchBackfillRequest &backfillMatchmakingRequest));
        MOCK_METHOD1(StopMatchBackfill, GenericOutcome(const Server::Model::StopMatchBackfillRequest &request));

        GenericOutcomeCallable ProcessReadyAsyncMock([[maybe_unused]] const Server::ProcessParameters &processParameters)
        {
            GenericOutcome successOutcome(nullptr);
            return GetFuture<GenericOutcome>(successOutcome);
        }
    };

    class GameLiftServerServiceBusMock : public GridMate::GameLiftServerServiceBus::Handler
    {
    public:
        GameLiftServerServiceBusMock(GridMate::IGridMate* gridMate) : m_gridMate(gridMate)
        {
            GridMate::GameLiftServerServiceBus::Handler::BusConnect(m_gridMate);
        }

        ~GameLiftServerServiceBusMock()
        {
            GridMate::GameLiftServerServiceBus::Handler::BusDisconnect(m_gridMate);
            m_gridMate = nullptr;
        }

        MOCK_METHOD2(HostSession, GridMate::GridSession*(const GridMate::GameLiftSessionParams& params, const GridMate::CarrierDesc& carrierDesc));
        MOCK_METHOD1(ShutdownSession, void(const GridMate::GridSession* gridSession));
        MOCK_METHOD1(QueryGameLiftSession, GridMate::GameLiftServerSession*(const GridMate::GridSession* gridSession));
        MOCK_METHOD3(StartMatchmakingBackfill, bool(const GridMate::GridSession* gameSession, AZStd::string& matchmakingTicketId, bool checkForAutoBackfill));
        MOCK_METHOD2(StopMatchmakingBackfill, bool(const GridMate::GridSession* gameSession, const AZStd::string& matchmakingTicketId));

    private:
        GridMate::IGridMate* m_gridMate = nullptr;
    };

    class GameLiftServerServiceMock : public GridMate::GameLiftServerService
    {

    public:
        GameLiftServerServiceMock(const GridMate::GameLiftServerServiceDesc desc) : GameLiftServerService(desc)
        {
            m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
            m_gameLiftServerSDKWrapperMock = AZStd::make_shared<NiceMock<GameLiftServerSDKWrapperMock>>();
            ON_CALL(*this, GetGameLiftServerSDKWrapper()).WillByDefault(Return(m_gameLiftServerSDKWrapperMock));
        }
        ~GameLiftServerServiceMock()
        {
            m_gameLiftServerSDKWrapperMock.reset();

            GridMate::GridMateDestroy(m_gridMate);
            m_gridMate = nullptr;
        }

        MOCK_METHOD0(GetGameLiftServerSDKWrapper, AZStd::weak_ptr<GridMate::GameLiftServerSDKWrapper>());

    protected:
        AZStd::shared_ptr<NiceMock<GameLiftServerSDKWrapperMock>> m_gameLiftServerSDKWrapperMock;
    };

    class GameLiftServerAllocatorsFixture
        : public AllocatorsTestFixture
    {
    public:
        GameLiftServerAllocatorsFixture()
            : AllocatorsTestFixture()
        {
        }

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

            GridMate::GameLiftServerServiceDesc serverDesc;
            m_serverService = new NiceMock<GameLiftServerServiceMock>(serverDesc);

            m_sessionEventBusMock = new NiceMock<SessionEventBusMock>(m_serverService->GetGridMate());
        }

        void TearDown() override
        {
            delete m_sessionEventBusMock;
            m_sessionEventBusMock = nullptr;

            delete m_serverService;
            m_serverService = nullptr;

            AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();;

            AllocatorsTestFixture::TearDown();
        }

        NiceMock<GameLiftServerServiceMock> *m_serverService;
        NiceMock<SessionEventBusMock> *m_sessionEventBusMock;
    };
};
#endif
