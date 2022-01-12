/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Request/AWSGameLiftCreateSessionRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    namespace CreateSessionActivity
    {
        static constexpr const char AWSGameLiftCreateSessionActivityName[] = "AWSGameLiftCreateSessionActivity";

        // Build AWS GameLift CreateGameSessionRequest by using AWSGameLiftCreateSessionRequest
        Aws::GameLift::Model::CreateGameSessionRequest BuildAWSGameLiftCreateGameSessionRequest(const AWSGameLiftCreateSessionRequest& createSessionRequest);

        // Create CreateGameSessionRequest and make a CreateGameSession call through GameLift client
        AZStd::string CreateSession(const AWSGameLiftCreateSessionRequest& createSessionRequest);

        // Validate CreateSessionRequest and check required request parameters
        bool ValidateCreateSessionRequest(const AzFramework::CreateSessionRequest& createSessionRequest);

    } // namespace CreateSessionActivity
} // namespace AWSGameLift
