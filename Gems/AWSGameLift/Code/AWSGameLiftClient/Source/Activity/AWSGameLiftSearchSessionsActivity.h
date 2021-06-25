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

#include <Request/AWSGameLiftSearchSessionsRequest.h>

#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/model/SearchGameSessionsRequest.h>

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
        AzFramework::SearchSessionsResponse SearchSessions(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftSearchSessionsRequest& searchSessionsRequest);

        // Convert from Aws::GameLift::Model::SearchGameSessionsResult to AzFramework::SearchSessionsResponse.
        AzFramework::SearchSessionsResponse ParseResponse(
            const Aws::GameLift::Model::SearchGameSessionsResult& gameLiftSearchSessionsResult);

        // Validate SearchSessionsRequest and check required request parameters
        bool ValidateSearchSessionsRequest(const AzFramework::SearchSessionsRequest& searchSessionsRequest);
    } // namespace SearchSessionsActivity
} // namespace AWSGameLift
