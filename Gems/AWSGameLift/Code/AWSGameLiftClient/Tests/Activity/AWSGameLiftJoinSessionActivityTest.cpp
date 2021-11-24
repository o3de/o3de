/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Session/ISessionHandlingRequests.h>

#include <AWSGameLiftClientFixture.h>
#include <Activity/AWSGameLiftJoinSessionActivity.h>

using namespace AWSGameLift;

using AWSGameLiftJoinSessionActivityTest = AWSGameLiftClientFixture;

TEST_F(AWSGameLiftJoinSessionActivityTest, BuildAWSGameLiftCreatePlayerSessionRequest_Call_GetExpectedResult)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_playerData = "dummyPlayerData";
    request.m_playerId = "dummyPlayerId";
    request.m_sessionId = "dummySessionId";
    auto awsRequest = JoinSessionActivity::BuildAWSGameLiftCreatePlayerSessionRequest(request);

    EXPECT_TRUE(strcmp(awsRequest.GetPlayerData().c_str(), request.m_playerData.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetPlayerId().c_str(), request.m_playerId.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetGameSessionId().c_str(), request.m_sessionId.c_str()) == 0);
}

TEST_F(AWSGameLiftJoinSessionActivityTest, BuildSessionConnectionConfig_Call_GetExpectedResult)
{
    Aws::GameLift::Model::PlayerSession playerSession;
    playerSession.SetIpAddress("dummyIpAddress");
    playerSession.SetPlayerSessionId("dummyPlayerSessionId");
    playerSession.SetPort(123);
    Aws::GameLift::Model::CreatePlayerSessionResult createPlayerSessionResult;
    createPlayerSessionResult.SetPlayerSession(playerSession);
    Aws::GameLift::Model::CreatePlayerSessionOutcome createPlayerSessionOutcome(createPlayerSessionResult);
    auto connectionConfig = JoinSessionActivity::BuildSessionConnectionConfig(createPlayerSessionOutcome);

    EXPECT_TRUE(strcmp(connectionConfig.m_ipAddress.c_str(), playerSession.GetIpAddress().c_str()) == 0);
    EXPECT_TRUE(strcmp(connectionConfig.m_playerSessionId.c_str(), playerSession.GetPlayerSessionId().c_str()) == 0);
    EXPECT_TRUE(connectionConfig.m_port == playerSession.GetPort());
}

TEST_F(AWSGameLiftJoinSessionActivityTest, ValidateJoinSessionRequest_CallWithBaseType_GetFalseResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = JoinSessionActivity::ValidateJoinSessionRequest(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message 
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftJoinSessionActivityTest, ValidateJoinSessionRequest_CallWithEmptyPlayerId_GetFalseResult)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = JoinSessionActivity::ValidateJoinSessionRequest(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message   
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftJoinSessionActivityTest, ValidateJoinSessionRequest_CallWithEmptySessionId_GetFalseResult)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_playerId = "dummyPlayerId";
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = JoinSessionActivity::ValidateJoinSessionRequest(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message  
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftJoinSessionActivityTest, ValidateJoinSessionRequest_CallWithPlayerAndSessionId_GetTrueResult)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_playerId = "dummyPlayerId";
    request.m_sessionId = "dummySessionId";
    auto result = JoinSessionActivity::ValidateJoinSessionRequest(request);
    EXPECT_TRUE(result);
}
