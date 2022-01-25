/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    namespace CreateSessionOnQueueActivity
    {
        static constexpr const char AWSGameLiftCreateSessionOnQueueActivityName[] = "AWSGameLiftCreateSessionOnQueueActivity";

        // Build AWS GameLift StartGameSessionPlacementRequest by using AWSGameLiftCreateSessionOnQueueRequest
        Aws::GameLift::Model::StartGameSessionPlacementRequest BuildAWSGameLiftStartGameSessionPlacementRequest(
            const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest);

        // Create StartGameSessionPlacementRequest and make a CreateGameSession call through GameLift client
        AZStd::string CreateSessionOnQueue(const AWSGameLiftCreateSessionOnQueueRequest& createSessionOnQueueRequest);

        // Validate CreateSessionOnQueueRequest and check required request parameters
        bool ValidateCreateSessionOnQueueRequest(const AzFramework::CreateSessionRequest& createSessionRequest);

    } // namespace CreateSessionOnQueueActivity
} // namespace AWSGameLift
