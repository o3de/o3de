/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Activity/AWSGameLiftStopMatchmakingActivity.h>
#include <AWSGameLiftClientFixture.h>

#include <aws/gamelift/model/StopMatchmakingRequest.h>

using namespace AWSGameLift;

using AWSGameLiftStopMatchmakingActivityTest = AWSGameLiftClientFixture;

TEST_F(AWSGameLiftStopMatchmakingActivityTest, BuildAWSGameLiftStopMatchmakingRequest_Call_GetExpectedResult)
{
    AWSGameLiftStopMatchmakingRequest request;
    request.m_ticketId = "dummyTicketId";
    auto awsRequest = StopMatchmakingActivity::BuildAWSGameLiftStopMatchmakingRequest(request);
    EXPECT_TRUE(strcmp(awsRequest.GetTicketId().c_str(), request.m_ticketId.c_str()) == 0);  
}

TEST_F(AWSGameLiftStopMatchmakingActivityTest, ValidateStopMatchmakingRequest_CallWithoutTicketId_GetFalseResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = StopMatchmakingActivity::ValidateStopMatchmakingRequest(AzFramework::StopMatchmakingRequest());
    EXPECT_FALSE(result);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftStopMatchmakingActivityTest, ValidateStopMatchmakingRequest_CallWithTicketId_GetTrueResult)
{
    AWSGameLiftStopMatchmakingRequest request;
    request.m_ticketId = "dummyTicketId";
    auto result = StopMatchmakingActivity::ValidateStopMatchmakingRequest(request);
    EXPECT_TRUE(result);
}
