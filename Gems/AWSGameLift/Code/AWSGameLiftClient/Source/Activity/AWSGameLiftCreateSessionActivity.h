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

#include <Request/AWSGameLiftCreateSessionRequest.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/CreateGameSessionRequest.h>

namespace AWSGameLift
{
    namespace CreateSessionActivity
    {
        static constexpr const char AWSGameLiftCreateSessionActivityName[] = "AWSGameLiftCreateSessionActivity";

        // Build AWS GameLift CreateGameSessionRequest by using AWSGameLiftCreateSessionRequest
        Aws::GameLift::Model::CreateGameSessionRequest BuildAWSGameLiftCreateGameSessionRequest(const AWSGameLiftCreateSessionRequest& createSessionRequest);

        // Create CreateGameSessionRequest and make a CreateGameSession call through GameLift client
        AZStd::string CreateSession(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftCreateSessionRequest& createSessionRequest);

        // Validate CreateSessionRequest and check required request parameters
        bool ValidateCreateSessionRequest(const AzFramework::CreateSessionRequest& createSessionRequest);

    } // namespace CreateSessionActivity
} // namespace AWSGameLift
