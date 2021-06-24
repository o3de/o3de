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

#pragma once

#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/StartGameSessionPlacementRequest.h>

namespace AWSGameLift
{
    namespace CreateSessionOnQueueActivity
    {
        static constexpr const char AWSGameLiftCreateSessionOnQueueActivityName[] = "AWSGameLiftCreateSessionOnQueueActivity";

        // Build AWS GameLift StartGameSessionPlacementRequest by using AWSGameLiftCreateSessionOnQueueRequest
        Aws::GameLift::Model::StartGameSessionPlacementRequest BuildAWSGameLiftStartGameSessionPlacementRequest(
            const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest);

        // Create StartGameSessionPlacementRequest and make a CreateGameSession call through GameLift client
        AZStd::string CreateSessionOnQueue(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest);

        // Validate CreateSessionOnQueueRequest and check required request parameters
        bool ValidateCreateSessionOnQueueRequest(const AzFramework::CreateSessionRequest& createSessionRequest);

    } // namespace CreateSessionOnQueueActivity
} // namespace AWSGameLift
