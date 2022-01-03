/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Activity/AWSGameLiftStartMatchmakingActivity.h>
#include <AWSGameLiftClientFixture.h>
#include <AWSGameLiftPlayer.h>

#include <aws/gamelift/model/StartMatchmakingRequest.h>

using namespace AWSGameLift;

using AWSGameLiftStartMatchmakingActivityTest = AWSGameLiftClientFixture;

TEST_F(AWSGameLiftStartMatchmakingActivityTest, BuildAWSGameLiftStartMatchmakingRequest_Call_GetExpectedResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_configurationName = "dummyConfiguration";
    request.m_ticketId = "dummyTicketId";

    AWSGameLiftPlayer player;
    player.m_playerAttributes["dummy"] = "{\"S\": \"test\"}";
    player.m_playerId = "dummyPlayerId";
    player.m_team = "dummyTeam";
    player.m_latencyInMs["us-east-1"] = 10;
    request.m_players.emplace_back(player);
    auto awsRequest = StartMatchmakingActivity::BuildAWSGameLiftStartMatchmakingRequest(request);

    EXPECT_TRUE(strcmp(awsRequest.GetConfigurationName().c_str(), request.m_configurationName.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetTicketId().c_str(), request.m_ticketId.c_str()) == 0);

    EXPECT_TRUE(awsRequest.GetPlayers().size() == request.m_players.size());
    EXPECT_TRUE(strcmp(awsRequest.GetPlayers()[0].GetPlayerId().c_str(), request.m_players[0].m_playerId.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetPlayers()[0].GetTeam().c_str(), request.m_players[0].m_team.c_str()) == 0);

    EXPECT_TRUE(awsRequest.GetPlayers()[0].GetLatencyInMs().size() == request.m_players[0].m_latencyInMs.size());
    EXPECT_TRUE(strcmp(awsRequest.GetPlayers()[0].GetLatencyInMs().begin()->first.c_str(), request.m_players[0].m_latencyInMs.begin()->first.c_str()) == 0);
    EXPECT_TRUE(awsRequest.GetPlayers()[0].GetLatencyInMs().begin()->second == request.m_players[0].m_latencyInMs.begin()->second);

    EXPECT_TRUE(awsRequest.GetPlayers()[0].GetPlayerAttributes().size() == request.m_players[0].m_playerAttributes.size());
    EXPECT_TRUE(strcmp(awsRequest.GetPlayers()[0].GetPlayerAttributes().begin()->first.c_str(), request.m_players[0].m_playerAttributes.begin()->first.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetPlayers()[0].GetPlayerAttributes().begin()->second.GetS().c_str(), "test") == 0);
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithBaseType_GetFalseResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(AzFramework::StartMatchmakingRequest());
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithoutConfigurationName_GetFalseResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_ticketId = "dummyTicketId";

    AWSGameLiftPlayer player;
    player.m_playerAttributes["dummy"] = "{\"S\": \"test\"}";
    player.m_playerId = "dummyPlayerId";
    player.m_team = "dummyTeam";
    player.m_latencyInMs["us-east-1"] = 10;
    request.m_players.emplace_back(player);

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(request);
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithoutPlayers_GetFalseResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_configurationName = "dummyConfiguration";
    request.m_ticketId = "dummyTicketId";

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(request);
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithoutPlayerId_GetFalseResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_configurationName = "dummyConfiguration";
    request.m_ticketId = "dummyTicketId";

    AWSGameLiftPlayer player;
    player.m_playerAttributes["dummy"] = "{\"S\": \"test\"}";
    player.m_team = "dummyTeam";
    player.m_latencyInMs["us-east-1"] = 10;
    request.m_players.emplace_back(player);

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(request);
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithInvalidPlayerAttribute_GetFalseResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_configurationName = "dummyConfiguration";
    request.m_ticketId = "dummyTicketId";

    AWSGameLiftPlayer player;
    player.m_playerAttributes["dummy"] = "{\"A\": \"test\"}";
    player.m_playerId = "dummyPlayerId";
    player.m_team = "dummyTeam";
    player.m_latencyInMs["us-east-1"] = 10;
    request.m_players.emplace_back(player);

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(request);
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithoutTicketId_GetTrueResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_configurationName = "dummyConfiguration";

    AWSGameLiftPlayer player;
    player.m_playerAttributes["dummy"] = "{\"S\": \"test\"}";
    player.m_playerId = "dummyPlayerId";
    player.m_team = "dummyTeam";
    player.m_latencyInMs["us-east-1"] = 10;
    request.m_players.emplace_back(player);

    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(request);
    EXPECT_TRUE(result);
}

TEST_F(AWSGameLiftStartMatchmakingActivityTest, ValidateStartMatchmakingRequest_CallWithValidParameters_GetTrueResult)
{
    AWSGameLiftStartMatchmakingRequest request;
    request.m_ticketId = "dummyTicketId";
    request.m_configurationName = "dummyConfiguration";

    AWSGameLiftPlayer player;
    player.m_playerAttributes["dummy"] = "{\"S\": \"test\"}";
    player.m_playerId = "dummyPlayerId";
    player.m_team = "dummyTeam";
    player.m_latencyInMs["us-east-1"] = 10;
    request.m_players.emplace_back(player);

    auto result = StartMatchmakingActivity::ValidateStartMatchmakingRequest(request);
    EXPECT_TRUE(result);
}
