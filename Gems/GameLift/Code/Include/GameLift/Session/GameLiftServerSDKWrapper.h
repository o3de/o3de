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

#if defined(BUILD_GAMELIFT_SERVER)

#include <aws/gamelift/server/GameLiftServerAPI.h>
namespace GridMate
{
    /* Wrapper to use to GameLift Server SDK.
    */
    class GameLiftServerSDKWrapper
    {
    public:
        GameLiftServerSDKWrapper() {}
        virtual ~GameLiftServerSDKWrapper() {}

        virtual Aws::GameLift::Server::InitSDKOutcome InitSDK();
        virtual Aws::GameLift::GenericOutcomeCallable ProcessReadyAsync(const Aws::GameLift::Server::ProcessParameters& processParameters);
        virtual Aws::GameLift::GenericOutcome ProcessEnding();
        virtual Aws::GameLift::GenericOutcome Destroy();

        virtual Aws::GameLift::GenericOutcome ActivateGameSession();
        virtual Aws::GameLift::GenericOutcome TerminateGameSession();

        virtual Aws::GameLift::GenericOutcome AcceptPlayerSession(const std::string& playerSessionId);
        virtual Aws::GameLift::GenericOutcome RemovePlayerSession(const char* playerSessionId);
        virtual Aws::GameLift::DescribePlayerSessionsOutcome DescribePlayerSessions(const Aws::GameLift::Server::Model::DescribePlayerSessionsRequest& describePlayerSessionsRequest);

        virtual Aws::GameLift::StartMatchBackfillOutcome StartMatchBackfill(const Aws::GameLift::Server::Model::StartMatchBackfillRequest& backfillMatchmakingRequest);
        virtual Aws::GameLift::GenericOutcome StopMatchBackfill(const Aws::GameLift::Server::Model::StopMatchBackfillRequest& request);

        virtual Aws::GameLift::AwsStringOutcome GetGameSessionId();
    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER
