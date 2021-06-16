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

#include <AzFramework/Session/ISessionHandlingRequests.h>

#include <Request/AWSGameLiftJoinSessionRequest.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/CreatePlayerSessionRequest.h>

namespace AWSGameLift
{
    namespace JoinSessionActivity
    {
        static constexpr const char AWSGameLiftJoinSessionActivityName[] = "AWSGameLiftJoinSessionActivity";
        static constexpr const char AWSGameLiftJoinSessionRequestInvalidErrorMessage[] =
            "Invalid GameLift JoinSession request.";
        static constexpr const char AWSGameLiftJoinSessionMissingRequestHandlerErrorMessage[] =
            "Missing GameLift JoinSession request handler, please make sure Multiplayer Gem is enabled and registered as handler.";

        // Build AWS GameLift CreatePlayerSessionRequest by using AWSGameLiftJoinSessionRequest
        Aws::GameLift::Model::CreatePlayerSessionRequest BuildAWSGameLiftCreatePlayerSessionRequest(
            const AWSGameLiftJoinSessionRequest& joinSessionRequest);

        // Build session connection config by using CreatePlayerSessionOutcome
        AzFramework::SessionConnectionConfig BuildSessionConnectionConfig(
            const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome);

        // Create CreatePlayerSessionRequest and make a CreatePlayerSession call through GameLift client
        Aws::GameLift::Model::CreatePlayerSessionOutcome CreatePlayerSession(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftJoinSessionRequest& joinSessionRequest);

        // Request to setup networking connection for player
        bool RequestPlayerJoinSession(
            const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome);

        // Validate JoinSessionRequest and check required request parameters
        bool ValidateJoinSessionRequest(const AzFramework::JoinSessionRequest& joinSessionRequest);

    } // namespace JoinSessionActivity
} // namespace AWSGameLift
