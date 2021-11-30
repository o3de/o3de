/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Request/AWSGameLiftAcceptMatchRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    namespace AcceptMatchActivity
    {
        static constexpr const char AWSGameLiftAcceptMatchActivityName[] = "AWSGameLiftAcceptMatchActivity";
        static constexpr const char AWSGameLiftAcceptMatchRequestInvalidErrorMessage[] = "Invalid GameLift AcceptMatch request.";

        // Build AWS GameLift AcceptMatchRequest by using AWSGameLiftAcceptMatchRequest
        Aws::GameLift::Model::AcceptMatchRequest BuildAWSGameLiftAcceptMatchRequest(const AWSGameLiftAcceptMatchRequest& AcceptMatchRequest);

        // Create AcceptMatchRequest and make a AcceptMatch call through GameLift client
        void AcceptMatch(const AWSGameLiftAcceptMatchRequest& AcceptMatchRequest);

        // Validate AcceptMatchRequest and check required request parameters
        bool ValidateAcceptMatchRequest(const AzFramework::AcceptMatchRequest& AcceptMatchRequest);
    } // namespace AcceptMatchActivity
} // namespace AWSGameLift
