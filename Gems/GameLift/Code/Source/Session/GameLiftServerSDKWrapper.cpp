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

#if defined(BUILD_GAMELIFT_SERVER)

#include <GameLift/Session/GameLiftServerSDKWrapper.h>

namespace GridMate
{
    Aws::GameLift::Server::InitSDKOutcome GameLiftServerSDKWrapper::InitSDK()
    {
        return Aws::GameLift::Server::InitSDK();
    }

    Aws::GameLift::GenericOutcomeCallable GameLiftServerSDKWrapper::ProcessReadyAsync(const Aws::GameLift::Server::ProcessParameters& processParameters)
    {
        return Aws::GameLift::Server::ProcessReadyAsync(processParameters);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ProcessEnding()
    {
        return Aws::GameLift::Server::ProcessEnding();
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::Destroy()
    {
        return Aws::GameLift::Server::Destroy();
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ActivateGameSession()
    {
        return Aws::GameLift::Server::ActivateGameSession();
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::TerminateGameSession()
    {
        return Aws::GameLift::Server::TerminateGameSession();
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::AcceptPlayerSession(const std::string& playerSessionId)
    {
        return Aws::GameLift::Server::AcceptPlayerSession(playerSessionId);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::RemovePlayerSession(const char* playerSessionId)
    {
        return Aws::GameLift::Server::RemovePlayerSession(playerSessionId);
    }

    Aws::GameLift::DescribePlayerSessionsOutcome GameLiftServerSDKWrapper::DescribePlayerSessions(const Aws::GameLift::Server::Model::DescribePlayerSessionsRequest& describePlayerSessionsRequest)
    {
        return  Aws::GameLift::Server::DescribePlayerSessions(describePlayerSessionsRequest);
    }

    Aws::GameLift::StartMatchBackfillOutcome GameLiftServerSDKWrapper::StartMatchBackfill(const Aws::GameLift::Server::Model::StartMatchBackfillRequest& backfillMatchmakingRequest)
    {
        return Aws::GameLift::Server::StartMatchBackfill(backfillMatchmakingRequest);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::StopMatchBackfill(const Aws::GameLift::Server::Model::StopMatchBackfillRequest& request)
    {
        return Aws::GameLift::Server::StopMatchBackfill(request);
    }

    Aws::GameLift::AwsStringOutcome GameLiftServerSDKWrapper::GetGameSessionId()
    {
        return Aws::GameLift::Server::GetGameSessionId();
    }
}

#endif // BUILD_GAMELIFT_SERVER
