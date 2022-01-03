/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameLiftServerSDKWrapper.h>

#include <ctime>

namespace AWSGameLift
{
    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::AcceptPlayerSession(const std::string& playerSessionId)
    {
        return Aws::GameLift::Server::AcceptPlayerSession(playerSessionId);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ActivateGameSession()
    {
        return Aws::GameLift::Server::ActivateGameSession();
    }

    Aws::GameLift::DescribePlayerSessionsOutcome GameLiftServerSDKWrapper::DescribePlayerSessions(
        const Aws::GameLift::Server::Model::DescribePlayerSessionsRequest& describePlayerSessionsRequest)
    {
        return Aws::GameLift::Server::DescribePlayerSessions(describePlayerSessionsRequest);
    }

    Aws::GameLift::Server::InitSDKOutcome GameLiftServerSDKWrapper::InitSDK()
    {
        return Aws::GameLift::Server::InitSDK();
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ProcessReady(
        const Aws::GameLift::Server::ProcessParameters& processParameters)
    {
        return Aws::GameLift::Server::ProcessReady(processParameters);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ProcessEnding()
    {
        return Aws::GameLift::Server::ProcessEnding();
    }

    AZStd::string GameLiftServerSDKWrapper::GetTerminationTime()
    {
        // Timestamp format is using the UTC ISO8601 format
        std::time_t terminationTime;
        Aws::GameLift::AwsLongOutcome GetTerminationTimeOutcome = Aws::GameLift::Server::GetTerminationTime();
        if (GetTerminationTimeOutcome.IsSuccess())
        {
            terminationTime = GetTerminationTimeOutcome.GetResult();
        }
        else
        {
            // Use the current system time if the termination time is not available from GameLift.
            time(&terminationTime);
        }

        char buffer[50];
        tm time;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        gmtime_s(&time, &terminationTime);
#else
        time = *gmtime(&terminationTime);
#endif
        strftime(buffer, sizeof(buffer), "%FT%TZ", &time);

        return AZStd::string(buffer);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::RemovePlayerSession(const AZStd::string& playerSessionId)
    {
        return Aws::GameLift::Server::RemovePlayerSession(playerSessionId.c_str());
    }

    Aws::GameLift::StartMatchBackfillOutcome GameLiftServerSDKWrapper::StartMatchBackfill(
        const Aws::GameLift::Server::Model::StartMatchBackfillRequest& startMatchBackfillRequest)
    {
        return Aws::GameLift::Server::StartMatchBackfill(startMatchBackfillRequest);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::StopMatchBackfill(
        const Aws::GameLift::Server::Model::StopMatchBackfillRequest& stopMatchBackfillRequest)
    {
        return Aws::GameLift::Server::StopMatchBackfill(stopMatchBackfillRequest);
    }

} // namespace AWSGameLift
