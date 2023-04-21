/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <aws/gamelift/server/GameLiftServerAPI.h>

#include <AzCore/std/string/string.h>

namespace AWSGameLift
{
    /* Wrapper to use to GameLift Server SDK.
     */
    class GameLiftServerSDKWrapper
    {
    public:
        GameLiftServerSDKWrapper() = default;
        virtual ~GameLiftServerSDKWrapper() = default;

        //! Processes and validates a player session connection.
        //! This method should be called when a client requests a connection to the server.
        //! @param playerSessionId the ID of the joining player's session.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome AcceptPlayerSession(const std::string& playerSessionId);

        //! Reports to GameLift that the server process is now ready to receive player sessions.
        //! Should be called once all GameSession initialization has finished.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome ActivateGameSession();

        //! Retrieves player session data, including settings, session metadata, and player data.
        //! Use this action to get information for a single player session,
        //! for all player sessions in a game session, or for all player sessions associated with a single player ID.
        //! @param  describePlayerSessionsRequest The request object describing which player sessions to retrieve.
        //! @return If successful, returns a DescribePlayerSessionsOutcome object containing a set of player session objects
        //!         that fit the request parameters.
        virtual Aws::GameLift::DescribePlayerSessionsOutcome DescribePlayerSessions(
            const Aws::GameLift::Server::Model::DescribePlayerSessionsRequest& describePlayerSessionsRequest);

        //! Retrieves the path to the TLS certificate used to encrypt the network connection between your Amazon GameLift Anywhere
        //! compute resource and Amazon GameLift. You can use the certificate path when you register your compute device to a
        //! Amazon GameLift Anywhere fleet.
        //! @return If successful, returns a GetComputeCertificateOutcome object containing the location of the fleet's TLS certificate
        //!         file, which is stored on the instance. If not successful, returns an error message.
        virtual Aws::GameLift::GetComputeCertificateOutcome GetComputeCertificate();

        //! Retrieves the service role credentials that you create to extend permissions to your other AWS services to Amazon GameLift.
        //! These credentials allow your game server to use your AWS resources.
        //! @return If successful, returns a GetFleetRoleCredentialsOutcome object containing the service role credentials.
        //!         If not successful, returns an error message.
        virtual Aws::GameLift::GetFleetRoleCredentialsOutcome GetFleetRoleCredentials(
            Aws::GameLift::Server::Model::GetFleetRoleCredentialsRequest request);

        //! Initializes the GameLift SDK.
        //! Should be called when the server starts, before any GameLift-dependent initialization happens.
        //! @return If successful, returns an InitSdkOutcome object indicating that the server process is ready to call ProcessReady().
        virtual Aws::GameLift::Server::InitSDKOutcome InitSDK(Aws::GameLift::Server::Model::ServerParameters serverParameters);

        //! Notifies the GameLift service that the server process is ready to host game sessions.
        //! @param processParameters A ProcessParameters object communicating the names of callback methods, port number and game
        //! session-specific log files about the server process.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome ProcessReady(const Aws::GameLift::Server::ProcessParameters& processParameters);

        //! Notifies the GameLift service that the server process is shutting down.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome ProcessEnding();

        //! Returns the time that a server process is scheduled to be shut down.
        //! @return Timestamp using the UTC ISO8601 format.
        virtual AZStd::string GetTerminationTime();

        //! Notifies the GameLift service that a player with the specified player session ID has disconnected from the server process.
        //! @param playerSessionId Unique ID issued by the Amazon GameLift service in response to a call to the AWS SDK Amazon GameLift API action CreatePlayerSession.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome RemovePlayerSession(const AZStd::string& playerSessionId);

        //! Sends a request to find new players for open slots in a game session created with FlexMatch.
        //! When the match has been successfully, backfilled updated matchmaker data will be sent to the OnUpdateGameSession callback.
        //! @param  startMatchBackfillRequest This data type is used to send a matchmaking backfill request.
        //! @return Returns a StartMatchBackfillOutcome object with the match backfill ticket or failure with an error message.
        virtual Aws::GameLift::StartMatchBackfillOutcome StartMatchBackfill(const Aws::GameLift::Server::Model::StartMatchBackfillRequest& startMatchBackfillRequest);

        //! Cancels an active match backfill request that was created with StartMatchBackfill
        //! @param  stopMatchBackfillRequest This data type is used to cancel a matchmaking backfill request.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome StopMatchBackfill(const Aws::GameLift::Server::Model::StopMatchBackfillRequest& stopMatchBackfillRequest);
    };
} // namespace AWSGameLift
