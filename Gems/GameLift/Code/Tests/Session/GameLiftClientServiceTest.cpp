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
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "GameLiftClientMocks.h"
#include "GameLift/Session/GameLiftClientService.h"
#include "GameLift/Session/GameLiftGameSessionPlacementRequest.h"
#include "GameLift/Session/GameLiftSessionRequest.h"
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/EBus.h>

using namespace UnitTest;

class GameLiftClientServiceTest
    : public GameLiftAllocatorsFixture
{

public:
};

TEST_F(GameLiftClientServiceTest, StartGameLiftClientSuccess)
{
    GameLiftClientMock* gameLiftClientMock = (GameLiftClientMock*)m_clientService->GetClient().get();
    EXPECT_CALL(*gameLiftClientMock, ListBuildsCallable(_)).Times(1);
    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceReady(_)).Times(1);
    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceFailed(_, _)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionServiceReady()).Times(1);


    m_clientService->StartGameLiftClient();
    m_clientService->Update();
}

TEST_F(GameLiftClientServiceTest, StartGameLiftClientFail_ListBuildsCallableError)
{
    GameLiftClientMock* gameLiftClientMock = (GameLiftClientMock*)m_clientService->GetClient().get();
    EXPECT_CALL(*gameLiftClientMock, ListBuildsCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::ListBuildsOutcome
            , Model::ListBuildsRequest>));
    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceReady(_)).Times(0);
    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceFailed(_, _)).Times(1);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionServiceReady()).Times(0);

    m_clientService->StartGameLiftClient();
    m_clientService->Update();
    AZ::TickBus::ExecuteQueuedEvents();
}

TEST_F(GameLiftClientServiceTest, RequestSession_UsingQueueName)
{
    GameLiftClientMock* gameLiftClientMock = (GameLiftClientMock*)m_clientService->GetClient().get();
    
    EXPECT_CALL(*gameLiftClientMock, ListBuildsCallable(_)).Times(1);
    EXPECT_CALL(*gameLiftClientMock, StartGameSessionPlacementCallable(_)).Times(1);

    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceReady(_)).Times(1);
    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceFailed(_, _)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionServiceReady()).Times(1);

    EXPECT_CALL(*m_sessionEventBusMock, OnGridSearchStart(_)).Times(1);
    EXPECT_CALL(*m_sessionEventBusMock, OnGridSearchRelease(_)).Times(1);

    m_clientService->StartGameLiftClient();
    m_clientService->Update();

    GridMate::GameLiftSessionRequestParams params;
    params.m_queueName = "TestQueueName";
    GridMate::GridSearch* search = m_clientService->RequestSession(params);
    GridMate::GameLiftGameSessionPlacementRequest* request = (GridMate::GameLiftGameSessionPlacementRequest*)search;
    AZ_Assert(request != nullptr, "Incorrect object type returned for request session. Expected GameLiftGameSessionPlacementRequest object type");

    request->Release();
    request = nullptr;
}

TEST_F(GameLiftClientServiceTest, RequestSession_WithoutUsingQueueName)
{
    GameLiftClientMock* gameLiftClientMock = (GameLiftClientMock*)m_clientService->GetClient().get();

    EXPECT_CALL(*gameLiftClientMock, ListBuildsCallable(_)).Times(1);
    EXPECT_CALL(*gameLiftClientMock, CreateGameSessionCallable(_)).Times(1);

    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceReady(_)).Times(1);
    EXPECT_CALL(*m_glClientServiceEventBusMock, OnGameLiftSessionServiceFailed(_, _)).Times(0);

    EXPECT_CALL(*m_sessionEventBusMock, OnSessionServiceReady()).Times(1);
    EXPECT_CALL(*m_sessionEventBusMock, OnGridSearchStart(_)).Times(1);
    EXPECT_CALL(*m_sessionEventBusMock, OnGridSearchRelease(_)).Times(1);
    m_clientService->StartGameLiftClient();
    m_clientService->Update();

    GridMate::GameLiftSessionRequestParams params;
    GridMate::GridSearch* search = m_clientService->RequestSession(params);
    GridMate::GameLiftSessionRequest* request = (GridMate::GameLiftSessionRequest*)search;
    AZ_Assert(request != nullptr, "Incorrect object type returned for request session. Expected GameLiftSessionRequest object type");

    request->Release();
    request = nullptr;
}
#endif
