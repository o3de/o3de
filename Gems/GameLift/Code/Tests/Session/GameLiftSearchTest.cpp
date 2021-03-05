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
#include "GameLift/Session/GameLiftSearch.h"

using namespace UnitTest;

class GameLiftSearchTest
    : public GameLiftAllocatorsFixture
{

public:
};

class GameLiftSearchMock : public GridMate::GameLiftSearch
{
public:

    GameLiftSearchMock(GridMate::GameLiftClientService* service, const AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context)
        : GameLiftSearch(service, context)
    {

    }

    void Update() override
    {
        GameLiftSearch::Update();
    }

    bool Initialize() override
    {
        return GameLiftSearch::Initialize();
    }
    
};

TEST_F(GameLiftSearchTest, InitializeSuccess)
{
    EXPECT_CALL(*m_gameLiftClient, SearchGameSessionsCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionQueuesCallable(_)).Times(0);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftSearchMock* search = new GameLiftSearchMock(m_clientService, context);
    search->Initialize();
    search->Update();

    const GridMate::SearchInfo* searchInfo = search->GetResult(0);
    const GridMate::GameLiftSearchInfo* gameLiftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo*>(searchInfo);

    AZ_Assert(gameLiftSearchInfo != nullptr, "Expected GameLiftSearchInfo type object");
    AZ_Assert(gameLiftSearchInfo->m_fleetId == m_gameLiftClient->m_testFleetId, "Expected test fleet id values to match");
    AZ_Assert(gameLiftSearchInfo->m_sessionId == m_gameLiftClient->m_testGameSessionId, "Expected test game session id values to match");
    AZ_Assert(search->IsDone(), "Search incomplete. Expected request to be done");

    search->Release();
    context.reset();
}

TEST_F(GameLiftSearchTest, InitializeFail_SearchGameSessionCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, SearchGameSessionsCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::SearchGameSessionsOutcome
            , Model::SearchGameSessionsRequest>));
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionQueuesCallable(_)).Times(0);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftSearchMock* search = new GameLiftSearchMock(m_clientService, context);
    search->Initialize();
    search->Update();

    AZ_Assert(search->GetNumResults() == 0, "Result count should be 0 in case of error.  Expected 0 results");
    AZ_Assert(search->IsDone(), "Search incomplete.  Expected request to be done");

    search->Release();
    context.reset();
}

TEST_F(GameLiftSearchTest, InitializeSuccess_WithQueueName)
{
    EXPECT_CALL(*m_gameLiftClient, SearchGameSessionsCallable(_)).Times(1);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionQueuesCallable(_)).Times(1);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;
    context->m_searchParams.m_queueName = m_gameLiftClient->m_testQueueName;

    GameLiftSearchMock* search = new GameLiftSearchMock(m_clientService, context);
    search->Initialize();
    search->Update();

    const GridMate::SearchInfo* searchInfo = search->GetResult(0);
    const GridMate::GameLiftSearchInfo* gameLiftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo*>(searchInfo);

    AZ_Assert(gameLiftSearchInfo != nullptr, "Expected GameLiftSearchInfo type object");
    AZ_Assert(gameLiftSearchInfo->m_fleetId == m_gameLiftClient->m_testFleetId, "No matching fleet id found");
    AZ_Assert(gameLiftSearchInfo->m_sessionId == m_gameLiftClient->m_testGameSessionId, "No matching game session id found");
    AZ_Assert(search->IsDone(), "Search incomplete. Expected request to be done");

    search->Release();
    context.reset();
}

TEST_F(GameLiftSearchTest, InitializeFail_DescribeGameSessionQueuesCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, SearchGameSessionsCallable(_)).Times(0);
    EXPECT_CALL(*m_gameLiftClient, DescribeGameSessionQueuesCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::DescribeGameSessionQueuesOutcome
            , Model::DescribeGameSessionQueuesRequest>));

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;
    context->m_searchParams.m_queueName = m_gameLiftClient->m_testQueueName;

    GameLiftSearchMock* search = new GameLiftSearchMock(m_clientService, context);
    search->Initialize();
    search->Update();

    AZ_Assert(search->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(search->IsDone(), "Search incomplete. Expected request to be done");

    search->Release();
    context.reset();
}
#endif

