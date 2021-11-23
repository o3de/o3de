/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Session/SessionConfig.h>

#include <Activity/AWSGameLiftSearchSessionsActivity.h>
#include <AWSGameLiftClientFixture.h>
#include <AWSGameLiftSessionConstants.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/model/SearchGameSessionsRequest.h>

using namespace AWSGameLift;

using AWSGameLiftSearchSessionsActivityTest = AWSGameLiftClientFixture;

TEST_F(AWSGameLiftSearchSessionsActivityTest, BuildAWSGameLiftSearchGameSessionsRequest_Call_GetExpectedResult)
{
    AWSGameLiftSearchSessionsRequest request;
    request.m_aliasId = "dummyAliasId";
    request.m_fleetId = "dummyFleetId";
    request.m_location = "dummyLocation";
    request.m_filterExpression = "dummyFilterExpression";
    request.m_sortExpression = "dummySortExpression";
    request.m_maxResult = 1;
    request.m_nextToken = "dummyNextToken";

    auto awsRequest = SearchSessionsActivity::BuildAWSGameLiftSearchGameSessionsRequest(request);

    EXPECT_TRUE(strcmp(awsRequest.GetFleetId().c_str(), request.m_fleetId.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetAliasId().c_str(), request.m_aliasId.c_str()) == 0);   
    EXPECT_TRUE(strcmp(awsRequest.GetFilterExpression().c_str(), request.m_filterExpression.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetSortExpression().c_str(), request.m_sortExpression.c_str()) == 0);
    EXPECT_TRUE(awsRequest.GetLimit() == request.m_maxResult);
    EXPECT_TRUE(strcmp(awsRequest.GetNextToken().c_str(), request.m_nextToken.c_str()) == 0);
    // TODO: Update the AWS Native SDK to get the new request attributes.
    //EXPECT_TRUE(strcmp(awsRequest.GetLocation().c_str(), request.m_location.c_str()) == 0);
}

TEST_F(AWSGameLiftSearchSessionsActivityTest, ValidateSearchSessionsRequest_CallWithBaseType_GetFalseResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = SearchSessionsActivity::ValidateSearchSessionsRequest(AzFramework::SearchSessionsRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message    
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftSearchSessionsActivityTest, ValidateSearchSessionsRequest_CallWithoutAliasOrFleetId_GetFalseResult)
{
    AWSGameLiftSearchSessionsRequest request;
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = SearchSessionsActivity::ValidateSearchSessionsRequest(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message    
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftSearchSessionsActivityTest, ValidateSearchSessionsRequest_CallWithAliasId_GetTrueResult)
{
    AWSGameLiftSearchSessionsRequest request;
    request.m_aliasId = "dummyAliasId";
    auto result = SearchSessionsActivity::ValidateSearchSessionsRequest(request);
    EXPECT_TRUE(result);
}

TEST_F(AWSGameLiftSearchSessionsActivityTest, ValidateSearchSessionsRequest_CallWithFleetId_GetTrueResult)
{
    AWSGameLiftSearchSessionsRequest request;
    request.m_fleetId = "dummyFleetId";
    auto result = SearchSessionsActivity::ValidateSearchSessionsRequest(request);
    EXPECT_TRUE(result);
}

TEST_F(AWSGameLiftSearchSessionsActivityTest, ParseResponse_Call_GetExpectedResult)
{
    Aws::GameLift::Model::GameProperty gameProperty;
    gameProperty.SetKey("dummyKey");
    gameProperty.SetValue("dummyValue");
    Aws::Vector<Aws::GameLift::Model::GameProperty> gameProperties = { gameProperty };

    Aws::GameLift::Model::GameSession gameSession;
    gameSession.SetCreationTime(Aws::Utils::DateTime(0.0));
    gameSession.SetTerminationTime(Aws::Utils::DateTime(0.0));
    gameSession.SetCreatorId("dummyCreatorId");
    gameSession.SetGameProperties(gameProperties);
    gameSession.SetGameSessionId("dummyGameSessionId");
    gameSession.SetName("dummyGameSessionName");
    gameSession.SetIpAddress("dummyIpAddress");
    gameSession.SetPort(0);
    gameSession.SetMaximumPlayerSessionCount(2);
    gameSession.SetCurrentPlayerSessionCount(1);
    gameSession.SetStatus(Aws::GameLift::Model::GameSessionStatus::TERMINATED);
    gameSession.SetStatusReason(Aws::GameLift::Model::GameSessionStatusReason::INTERRUPTED);
    // TODO: Update the AWS Native SDK to set the new game session attributes.
    //gameSession.SetDnsName("dummyDnsName");
    Aws::Vector<Aws::GameLift::Model::GameSession> gameSessions = { gameSession };

    Aws::GameLift::Model::SearchGameSessionsResult result;
    result.SetNextToken("dummyNextToken");
    result.SetGameSessions(gameSessions);

    auto response = SearchSessionsActivity::ParseResponse(result);

    EXPECT_TRUE(strcmp(response.m_nextToken.c_str(), result.GetNextToken().c_str()) == 0);
    EXPECT_EQ(response.m_sessionConfigs.size(), 1);

    const auto& sessionConfig = response.m_sessionConfigs[0];
    EXPECT_EQ(gameSession.GetCreationTime().Millis(), sessionConfig.m_creationTime);
    EXPECT_EQ(gameSession.GetTerminationTime().Millis(), sessionConfig.m_terminationTime);
    EXPECT_TRUE(strcmp(gameSession.GetCreatorId().c_str(), sessionConfig.m_creatorId.c_str()) == 0);
    EXPECT_TRUE(strcmp(gameSession.GetGameSessionId().c_str(), sessionConfig.m_sessionId.c_str()) == 0);
    EXPECT_TRUE(strcmp(gameSession.GetName().c_str(), sessionConfig.m_sessionName.c_str()) == 0);
    EXPECT_TRUE(strcmp(gameSession.GetIpAddress().c_str(), sessionConfig.m_ipAddress.c_str()) == 0);
    EXPECT_EQ(gameSession.GetPort(), 0);
    EXPECT_EQ(gameSession.GetMaximumPlayerSessionCount(), 2);
    EXPECT_EQ(gameSession.GetCurrentPlayerSessionCount(), 1);
    EXPECT_TRUE(strcmp(AWSGameLiftSessionStatusNames[(int)gameSession.GetStatus()], sessionConfig.m_status.c_str()) == 0);
    EXPECT_TRUE(strcmp(AWSGameLiftSessionStatusReasons[(int)gameSession.GetStatusReason()], sessionConfig.m_statusReason.c_str()) == 0);
    // TODO: Update the AWS Native SDK to get the new game session attributes.
    // EXPECT_TRUE(strcmp(gameSession.GetDnsName().c_str(), sessionConfig.m_dnsName.c_str()) == 0);
}
