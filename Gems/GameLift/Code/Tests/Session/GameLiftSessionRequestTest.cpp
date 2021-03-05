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
#include "GameLift/Session/GameLiftSessionRequest.h"

using namespace UnitTest;

class GameLiftSessionRequestTest
    : public GameLiftAllocatorsFixture
{

public:
};

class GameLiftSessionRequestMock : public GridMate::GameLiftSessionRequest
{
public:

    GameLiftSessionRequestMock(GridMate::GameLiftClientService* service, const AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context)
        : GameLiftSessionRequest(service, context)
    {

    }

    void Update() override
    {
        GameLiftSessionRequest::Update();
    }
};

TEST_F(GameLiftSessionRequestTest, InitializeSuccess)
{
    EXPECT_CALL(*m_gameLiftClient, CreateGameSessionCallable(_)).Times(1);

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();

    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftSessionRequestMock* gameLiftSessionRequest = new GameLiftSessionRequestMock(m_clientService, context);
    gameLiftSessionRequest->Initialize();
    gameLiftSessionRequest->Update();

    const GridMate::SearchInfo* searchInfo = gameLiftSessionRequest->GetResult(0);
    const GridMate::GameLiftSearchInfo* gameLiftSearchInfo = static_cast<const GridMate::GameLiftSearchInfo*>(searchInfo);

    AZ_Assert(gameLiftSearchInfo != nullptr, "Expected GameLiftSearchInfo type object");
    AZ_Assert(gameLiftSearchInfo->m_fleetId == m_gameLiftClient->m_testFleetId, "No matching fleet id found");
    AZ_Assert(gameLiftSearchInfo->m_sessionId == m_gameLiftClient->m_testGameSessionId, "No matching game session id found");
    AZ_Assert(gameLiftSessionRequest->IsDone(), "Search incomplete. Expected request to be done");

    gameLiftSessionRequest->Release();
    context.reset();
}     

TEST_F(GameLiftSessionRequestTest, InitializeFail_CreateGameSessionCallableError)
{
    EXPECT_CALL(*m_gameLiftClient, CreateGameSessionCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::CreateGameSessionOutcome
            , Model::CreateGameSessionRequest>));

    AZStd::shared_ptr<GridMate::GameLiftRequestInterfaceContext> context = AZStd::make_shared<GridMate::GameLiftRequestInterfaceContext>();
    context->m_gameLiftClient = m_gameLiftClient;

    GameLiftSessionRequestMock* gameLiftSessionRequest = new GameLiftSessionRequestMock(m_clientService, context);
    gameLiftSessionRequest->Initialize();
    gameLiftSessionRequest->Update();

    AZ_Assert(gameLiftSessionRequest->GetNumResults() == 0, "Result count should be 0 in case of error. Expected 0 results");
    AZ_Assert(gameLiftSessionRequest->IsDone(), "Search incomplete. Expected request to be done");

    gameLiftSessionRequest->Release();
    context.reset();
}
#endif
