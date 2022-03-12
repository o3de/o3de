/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSGameLiftClientFixture.h>
#include <Activity/AWSGameLiftCreateSessionOnQueueActivity.h>

#include <aws/gamelift/model/StartGameSessionPlacementRequest.h>

using namespace AWSGameLift;

using AWSGameLiftCreateSessionOnQueueActivityTest = AWSGameLiftClientFixture;

TEST_F(AWSGameLiftCreateSessionOnQueueActivityTest, BuildAWSGameLiftCreateGameSessionRequest_Call_GetExpectedResult)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_sessionName = "dummySessionName";
    request.m_maxPlayer = 1;
    request.m_sessionProperties.emplace("dummyKey", "dummyValue");
    request.m_queueName = "dummyQueueName";
    request.m_placementId = "dummyPlacementId";
    auto awsRequest = CreateSessionOnQueueActivity::BuildAWSGameLiftStartGameSessionPlacementRequest(request);

    EXPECT_TRUE(strcmp(awsRequest.GetGameSessionName().c_str(), request.m_sessionName.c_str()) == 0);
    EXPECT_TRUE(awsRequest.GetMaximumPlayerSessionCount() == request.m_maxPlayer);
    EXPECT_TRUE(strcmp(awsRequest.GetGameProperties()[0].GetKey().c_str(), request.m_sessionProperties.begin()->first.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetGameProperties()[0].GetValue().c_str(), request.m_sessionProperties.begin()->second.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetGameSessionQueueName().c_str(), request.m_queueName.c_str()) == 0);
    EXPECT_TRUE(strcmp(awsRequest.GetPlacementId().c_str(), request.m_placementId.c_str()) == 0);
}

TEST_F(AWSGameLiftCreateSessionOnQueueActivityTest, ValidateCreateSessionOnQueueRequest_CallWithBaseType_GetFalseResult)
{
    auto result = CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(AzFramework::CreateSessionRequest());
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftCreateSessionOnQueueActivityTest, ValidateCreateSessionOnQueueRequest_CallWithNegativeMaxPlayer_GetFalseResult)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_maxPlayer = std::numeric_limits<uint64_t>::max();

    auto result = CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(request);
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftCreateSessionOnQueueActivityTest, ValidateCreateSessionOnQueueRequest_CallWithoutQueueName_GetFalseResult)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_maxPlayer = 1;
    request.m_placementId = "dummyPlacementId";
    auto result = CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(request);
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftCreateSessionOnQueueActivityTest, ValidateCreateSessionOnQueueRequest_CallWithoutPlacementId_GetFalseResult)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_maxPlayer = 1;
    request.m_queueName = "dummyQueueName";
    auto result = CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(request);
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftCreateSessionOnQueueActivityTest, ValidateCreateSessionOnQueueRequest_CallWithValidRequest_GetTrueResult)
{
    AWSGameLiftCreateSessionOnQueueRequest request;
    request.m_maxPlayer = 1;
    request.m_queueName = "dummyQueueName";
    request.m_placementId = "dummyPlacementId";
    auto result = CreateSessionOnQueueActivity::ValidateCreateSessionOnQueueRequest(request);
    EXPECT_TRUE(result);
}
