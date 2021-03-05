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
#include "GameLift/Session/GameLiftMatchmaking.h"

using namespace UnitTest;

class GameLiftMatchmakingTest
    : public GameLiftAllocatorsFixture
{

public:
};

class GameLiftMatchmakingMock: public GridMate::GameLiftMatchmaking
{
public:
    GameLiftMatchmakingMock(GridMate::GameLiftClientService* service, const AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context, const Aws::String& matchmakingConfigName)
        : GameLiftMatchmaking(service, context, matchmakingConfigName)
    {
        // Setting negative value for delay to skip waiting for delay.
        m_pollDescribeMatchmakingDelay = -1.0F;
    }

    void Update() override
    {
        GameLiftMatchmaking::Update();
    }

};

TEST_F(GameLiftMatchmakingTest, InitializeSuccess)
{
    EXPECT_CALL(*m_gameLiftClient, StartMatchmakingCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeMatchmakingCallable(_)).Times(1);
    
    m_gameLiftRequestInterfaceContext->m_playerId = "TestPlayerId";
    GameLiftMatchmakingMock* matchmaking = new GameLiftMatchmakingMock(m_clientService, m_gameLiftRequestInterfaceContext, "TestMatchmakingConfig");
    matchmaking->Initialize();
    matchmaking->Update();

    const GridMate::SearchInfo* searchInfo = matchmaking->GetResult(0);
    const GridMate::GameLiftSearchInfo* gameLiftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo*>(searchInfo);

    AZ_Assert(gameLiftSearchInfo != nullptr, "Expected GameLiftSearchInfo type object");
    AZ_Assert(gameLiftSearchInfo->m_sessionId == m_gameLiftClient->m_testGameSessionId, "Expected test game session id values to match");
    AZ_Assert(matchmaking->IsDone(), "Search incomplete. Expected request to be done");

    matchmaking->Release();
}

TEST_F(GameLiftMatchmakingTest, InitializeFail_StartMatchmakingCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, StartMatchmakingCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::StartMatchmakingOutcome
            , Model::StartMatchmakingRequest>));
    EXPECT_CALL(*m_gameLiftClient, DescribeMatchmakingCallable(_)).Times(0);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;
    context->m_playerId = "TestPlayerId";
    GameLiftMatchmakingMock* matchmaking = new GameLiftMatchmakingMock(m_clientService, context, "TestMatchmakingConfig");
    matchmaking->Initialize();
    matchmaking->Update();

    AZ_Assert(matchmaking->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(matchmaking->IsDone(), "Search incomplete. Expected request to be done");

    matchmaking->Release();
    context.reset();
}

TEST_F(GameLiftMatchmakingTest, InitializeFail_DescribeMatchmakingCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, StartMatchmakingCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeMatchmakingCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::DescribeMatchmakingOutcome
            , Model::DescribeMatchmakingRequest>));

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;
    context->m_playerId = "TestPlayerId";
    GameLiftMatchmakingMock* matchmaking = new GameLiftMatchmakingMock(m_clientService, context, "TestMatchmakingConfig");
    matchmaking->Initialize();
    matchmaking->Update();

    AZ_Assert(matchmaking->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(matchmaking->IsDone(), "Search incomplete. Expected request to be done");

    matchmaking->Release();
    context.reset();
}
#endif
