/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <aws/gamelift/model/AcceptMatchRequest.h>

#include <AWSGameLiftClientFixture.h>
#include <Activity/AWSGameLiftAcceptMatchActivity.h>

#include <aws/gamelift/model/AcceptMatchRequest.h>

using namespace AWSGameLift;

using AWSGameLiftAcceptMatchActivityTest = AWSGameLiftClientFixture;

TEST_F(AWSGameLiftAcceptMatchActivityTest, BuildAWSGameLiftAcceptMatchRequest_Call_GetExpectedResult)
{
    AWSGameLiftAcceptMatchRequest request;
    request.m_acceptMatch = true;
    request.m_ticketId = "dummyTicketId";
    request.m_playerIds = { "dummyPlayerId" };

    auto awsRequest = AcceptMatchActivity::BuildAWSGameLiftAcceptMatchRequest(request);

    EXPECT_EQ(awsRequest.GetAcceptanceType(), Aws::GameLift::Model::AcceptanceType::ACCEPT);
    EXPECT_TRUE(strcmp(awsRequest.GetTicketId().c_str(), request.m_ticketId.c_str()) == 0);
    EXPECT_EQ(awsRequest.GetPlayerIds().size(), request.m_playerIds.size());
    EXPECT_TRUE(strcmp(awsRequest.GetPlayerIds().begin()->c_str(), request.m_playerIds.begin()->c_str()) == 0);
}

TEST_F(AWSGameLiftAcceptMatchActivityTest, ValidateAcceptMatchRequest_CallWithBaseType_GetFalseResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = AcceptMatchActivity::ValidateAcceptMatchRequest(AzFramework::AcceptMatchRequest());
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftAcceptMatchActivityTest, ValidateAcceptMatchRequest_CallWithoutTicketId_GetFalseResult)
{
    AWSGameLiftAcceptMatchRequest request;
    request.m_acceptMatch = true;
    request.m_playerIds = { "dummyPlayerId" };

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = AcceptMatchActivity::ValidateAcceptMatchRequest(request);
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftAcceptMatchActivityTest, ValidateAcceptMatchRequest_CallWithoutPlayerIds_GetFalseResult)
{
    AWSGameLiftAcceptMatchRequest request;
    request.m_acceptMatch = true;
    request.m_playerIds = { "dummyPlayerId" };

    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = AcceptMatchActivity::ValidateAcceptMatchRequest(request);
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftAcceptMatchActivityTest, ValidateAcceptMatchRequest_CallWithValidAttributes_GetTrueResult)
{

    AWSGameLiftAcceptMatchRequest request;
    request.m_acceptMatch = true;
    request.m_ticketId = "dummyTicketId";
    request.m_playerIds = { "dummyPlayerId" };

    auto result = AcceptMatchActivity::ValidateAcceptMatchRequest(request);
    EXPECT_TRUE(result);
}
