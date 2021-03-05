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
#include "GameLift/Session/GameLiftClientSession.h"
#include "GridMate/Session/Session.h"

using namespace UnitTest;

class GameLiftClientSessionTest
    : public GameLiftAllocatorsFixture
{

public:
};


class GameLiftClientSessionMock : public GridMate::GameLiftClientSession
{
public:
    static void RegisterReplicaChunks()
    {
        GridMate::GameLiftClientSession::RegisterReplicaChunks();
    }

    GameLiftClientSessionMock(GridMate::GameLiftClientService* service) : GameLiftClientSession(service)
    {

    }

    bool Initialize(const GridMate::GameLiftSearchInfo& info, const GridMate::JoinParams& params, const GridMate::CarrierDesc& carrierDesc)
    {
        return GameLiftClientSession::Initialize(info, params, carrierDesc);
    }

    void Update() override
    {
        GameLiftClientSession::Update();
    }

    void Shutdown() override
    {
        GameLiftClientSession::Shutdown();
    }

    void OnSessionReplicaArrived()
    {
        RequestEvent(GridSession::SE_JOINED);
    }
};
#if AZ_TRAIT_DISABLE_FAILED_GAMELIFT_CLIENT_SESSION_TEST
TEST_F(GameLiftClientSessionTest, DISABLED_InitializeSuccess)
#else
TEST_F(GameLiftClientSessionTest, InitializeSuccess)
#endif // AZ_TRAIT_DISABLE_FAILED_GAMELIFT_CLIENT_SESSION_TEST
{
    EXPECT_CALL((*(GameLiftClientMock*)m_clientService->GetClient().get()), DescribeGameSessionsCallable(_)).Times(1);
    EXPECT_CALL((*(GameLiftClientMock*)m_clientService->GetClient().get()), CreatePlayerSessionCallable(_)).Times(1);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionCreated(_)).Times(1);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionHosted(_)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionJoined(_)).Times(1);

    GameLiftClientSessionMock *session = new GameLiftClientSessionMock(m_clientService);

    GridMate::GameLiftSearchInfo info;
    GridMate::JoinParams params;
    GridMate::CarrierDesc carrierDesc;
    GameLiftClientSessionMock::RegisterReplicaChunks();
    session->Initialize(info, params, carrierDesc);
    session->Update();
    session->Update();
    session->OnSessionReplicaArrived();

    session->Shutdown();
    delete session;
}

TEST_F(GameLiftClientSessionTest, InitializeFail_DescribeGameSessionsCallableError)
{
    EXPECT_CALL((*(GameLiftClientMock*)m_clientService->GetClient().get()), DescribeGameSessionsCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::DescribeGameSessionsOutcome
            , Model::DescribeGameSessionsRequest>));
    EXPECT_CALL((*(GameLiftClientMock*)m_clientService->GetClient().get()), CreatePlayerSessionCallable(_)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionCreated(_)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionHosted(_)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionJoined(_)).Times(0);

    GameLiftClientSessionMock *session = new GameLiftClientSessionMock(m_clientService);

    GridMate::GameLiftSearchInfo info;
    GridMate::JoinParams params;
    GridMate::CarrierDesc carrierDesc;
    GameLiftClientSessionMock::RegisterReplicaChunks();
    session->Initialize(info, params, carrierDesc);
    // Update calls shutdown and deletes the grid session
    session->Update();
}


TEST_F(GameLiftClientSessionTest, InitializeFail_CreatePlayerSessionCallableError)
{
    EXPECT_CALL((*(GameLiftClientMock*)m_clientService->GetClient().get()), DescribeGameSessionsCallable(_)).Times(1);
    EXPECT_CALL((*(GameLiftClientMock*)m_clientService->GetClient().get()), CreatePlayerSessionCallable(_)).Times(1)
        .WillOnce(Invoke(m_gameLiftClient.get(), &GameLiftClientMock::CallableErrorMock<Model::CreatePlayerSessionOutcome
            , Model::CreatePlayerSessionRequest>));
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionCreated(_)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionHosted(_)).Times(0);
    EXPECT_CALL(*m_sessionEventBusMock, OnSessionJoined(_)).Times(0);

    GameLiftClientSessionMock *session = new GameLiftClientSessionMock(m_clientService);

    GridMate::GameLiftSearchInfo info;
    GridMate::JoinParams params;
    GridMate::CarrierDesc carrierDesc;
    GameLiftClientSessionMock::RegisterReplicaChunks();
    session->Initialize(info, params, carrierDesc);
    // Update calls shutdown and deletes the grid session
    session->Update();
}
#endif
