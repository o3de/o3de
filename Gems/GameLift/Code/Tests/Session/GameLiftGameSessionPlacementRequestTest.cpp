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
#include <AzCore/UnitTest/TestTypes.h>
#include "GameLiftClientMocks.h"
#include "GameLift/Session/GameLiftGameSessionPlacementRequest.h"

using namespace UnitTest;

class GameLiftGameSessionPlacementRequestTest
    : public GameLiftAllocatorsFixture
{
};

class GameLiftGameSessionPlacementRequestMock : public GridMate::GameLiftGameSessionPlacementRequest
{
public:

    GameLiftGameSessionPlacementRequestMock(GridMate::GameLiftClientService* service, const AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context)
        : GameLiftGameSessionPlacementRequest(service, context)
    {

    }

    void Update() override
    {
        GameLiftGameSessionPlacementRequest::Update();
    }
};

TEST_F(GameLiftGameSessionPlacementRequestTest, InitializeSuccess)
{
    EXPECT_CALL(*m_gameLiftClient, StartGameSessionPlacementCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionPlacementCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionDetails(_)).Times(1);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftGameSessionPlacementRequestMock* request = new GameLiftGameSessionPlacementRequestMock(m_clientService, context);
    request->Initialize();
    request->Update();
    request->Update();
    request->Update();
    request->Update();
    request->Update();

    const GridMate::SearchInfo* searchInfo = request->GetResult(0);
    const GridMate::GameLiftSearchInfo* gameLiftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo*>(searchInfo);

    AZ_Assert(gameLiftSearchInfo != nullptr, "Expected GameLiftSearchInfo Object");
    AZ_Assert(gameLiftSearchInfo->m_fleetId == m_gameLiftClient->m_testFleetId, "Expected test fleet id values to match");
    AZ_Assert(gameLiftSearchInfo->m_sessionId == m_gameLiftClient->m_testGameSessionId, "Expected test game session id values to match");
    AZ_Assert(request->IsDone(), "Search incomplete. Expected request to be done");

    request->Release();
    context.reset();
}

TEST_F(GameLiftGameSessionPlacementRequestTest, InitializeFail_StartGameSessionPlacementCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, StartGameSessionPlacementCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::StartGameSessionPlacementOutcome
            , Model::StartGameSessionPlacementRequest>));
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionPlacementCallable(_)).Times(0);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionDetails(_)).Times(0);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftGameSessionPlacementRequestMock* request = new GameLiftGameSessionPlacementRequestMock(m_clientService, context);
    request->Initialize();
    request->Update();
    request->Update();
    request->Update();
    request->Update();
    request->Update();

    AZ_Assert(request->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(request->IsDone(), "Search incomplete. Expected request be done");

    request->Release();
    context.reset();
}

TEST_F(GameLiftGameSessionPlacementRequestTest, InitializeFail_DescribeGameSessionPlacementCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, StartGameSessionPlacementCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionPlacementCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::DescribeGameSessionPlacementOutcome
            , Model::DescribeGameSessionPlacementRequest>));
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionDetails(_)).Times(0);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftGameSessionPlacementRequestMock* request = new GameLiftGameSessionPlacementRequestMock(m_clientService, context);
    request->Initialize();
    request->Update();
    request->Update();
    request->Update();
    request->Update();
    request->Update();

    AZ_Assert(request->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(request->IsDone(), "Search incomplete. Expected request to be done");

    request->Release();
    context.reset();
}

TEST_F(GameLiftGameSessionPlacementRequestTest, InitializeFail_DescribeGameSessionDetailsError)
{
    EXPECT_CALL(*m_gameLiftClient, StartGameSessionPlacementCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionPlacementCallable(_)).Times(1);        
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionDetails(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::ErrorMock<Model::DescribeGameSessionDetailsOutcome
            , Model::DescribeGameSessionDetailsRequest>));

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftGameSessionPlacementRequestMock* request = new GameLiftGameSessionPlacementRequestMock(m_clientService, context);
    request->Initialize();
    request->Update();
    request->Update();
    request->Update();
    request->Update();
    request->Update();

    AZ_Assert(request->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(request->IsDone(), "Search incomplete. Expected request to be done");

    request->Release();
    context.reset();
}
#endif
