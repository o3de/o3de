/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Request/AWSGameLiftSearchSessionsRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    namespace SearchSessionsActivity
    {
        static constexpr const char AWSGameLiftSearchSessionsActivityName[] = "AWSGameLiftSearchSessionsActivity";
        static constexpr const char AWSGameLiftSearchSessionsRequestInvalidErrorMessage[] =
            "Invalid GameLift SearchSessions request.";

        // Build AWS GameLift SearchGameSessionsRequest by using AWSGameLiftSearchSessionsRequest
        Aws::GameLift::Model::SearchGameSessionsRequest BuildAWSGameLiftSearchGameSessionsRequest(
            const AWSGameLiftSearchSessionsRequest& searchSessionsRequest);

        // Create SearchGameSessionsRequest and make a SeachGameSessions call through GameLift client
        Multiplayer::SearchSessionsResponse SearchSessions(
            const AWSGameLiftSearchSessionsRequest& searchSessionsRequest);

        // Convert from Aws::GameLift::Model::SearchGameSessionsResult to Multiplayer::SearchSessionsResponse.
        Multiplayer::SearchSessionsResponse ParseResponse(
            const Aws::GameLift::Model::SearchGameSessionsResult& gameLiftSearchSessionsResult);

        // Validate SearchSessionsRequest and check required request parameters
        bool ValidateSearchSessionsRequest(const Multiplayer::SearchSessionsRequest& searchSessionsRequest);
    } // namespace SearchSessionsActivity
} // namespace AWSGameLift
