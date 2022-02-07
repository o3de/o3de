/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Request/AWSGameLiftStopMatchmakingRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    namespace StopMatchmakingActivity
    {
        static constexpr const char AWSGameLiftStopMatchmakingActivityName[] = "AWSGameLiftStopMatchmakingActivity";
        static constexpr const char AWSGameLiftStopMatchmakingRequestInvalidErrorMessage[] = "Invalid GameLift StopMatchmaking request.";

        // Build AWS GameLift StopMatchmakingRequest by using AWSGameLiftStopMatchmakingRequest
        Aws::GameLift::Model::StopMatchmakingRequest BuildAWSGameLiftStopMatchmakingRequest(const AWSGameLiftStopMatchmakingRequest& stopMatchmakingRequest);

        // Create StopMatchmakingRequest and make a StopMatchmaking call through GameLift client
        void StopMatchmaking(const Aws::GameLift::GameLiftClient& gameliftClient, const AWSGameLiftStopMatchmakingRequest& stopMatchmakingRequest);

        // Validate StopMatchmakingRequest and check required request parameters
        bool ValidateStopMatchmakingRequest(const AzFramework::StopMatchmakingRequest& stopMatchmakingRequest);
    } // namespace StopMatchmakingActivity
} // namespace AWSGameLift
