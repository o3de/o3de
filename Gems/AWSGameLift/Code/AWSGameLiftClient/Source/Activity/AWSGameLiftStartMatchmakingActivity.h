/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Request/AWSGameLiftStartMatchmakingRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    namespace StartMatchmakingActivity
    {
        static constexpr const char AWSGameLiftStartMatchmakingActivityName[] = "AWSGameLiftStartMatchmakingActivity";
        static constexpr const char AWSGameLiftStartMatchmakingRequestInvalidErrorMessage[] = "Invalid GameLift StartMatchmaking request.";

        // Build AWS GameLift StartMatchmakingRequest by using AWSGameLiftStartMatchmakingRequest
        Aws::GameLift::Model::StartMatchmakingRequest BuildAWSGameLiftStartMatchmakingRequest(const AWSGameLiftStartMatchmakingRequest& startMatchmakingRequest);

        // Create StartMatchmakingRequest and make a StartMatchmaking call through GameLift client
        AZStd::string StartMatchmaking(const AWSGameLiftStartMatchmakingRequest& startMatchmakingRequest);

        // Validate StartMatchmakingRequest and check required request parameters
        bool ValidateStartMatchmakingRequest(const AzFramework::StartMatchmakingRequest& startMatchmakingRequest);
    } // namespace StartMatchmakingActivity
} // namespace AWSGameLift
