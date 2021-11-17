/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            const AWSGameLiftJoinSessionRequest& joinSessionRequest);

        // Request to setup networking connection for player
        bool RequestPlayerJoinSession(
            const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome);

        // Validate JoinSessionRequest and check required request parameters
        bool ValidateJoinSessionRequest(const AzFramework::JoinSessionRequest& joinSessionRequest);

    } // namespace JoinSessionActivity
} // namespace AWSGameLift
