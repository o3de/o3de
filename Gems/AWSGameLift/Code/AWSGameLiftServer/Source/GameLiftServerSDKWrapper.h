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

        //! Initializes the GameLift SDK.
        //! Should be called when the server starts, before any GameLift-dependent initialization happens.
        //! @return If successful, returns an InitSdkOutcome object indicating that the server process is ready to call ProcessReady().
        virtual Aws::GameLift::Server::InitSDKOutcome InitSDK();

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
    };
} // namespace AWSGameLift
