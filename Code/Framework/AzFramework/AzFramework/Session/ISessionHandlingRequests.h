/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    //! SessionConnectionConfig
    //! The properties for handling join session request.
    struct SessionConnectionConfig
    {
        //! A unique identifier for registered player in session.
        AZStd::string m_playerSessionId;

        //! The DNS identifier assigned to the instance that is running the session.
        AZStd::string m_dnsName;

        //! The IP address of the session.
        AZStd::string m_ipAddress;

        //! The port number for the session.
        uint16_t m_port = 0;
    };

    //! SessionConnectionConfig
    //! The properties for handling player connect/disconnect
    struct PlayerConnectionConfig
    {
        //! A unique identifier for player connection.
        uint32_t m_playerConnectionId = 0;

        //! A unique identifier for registered player in session.
        AZStd::string m_playerSessionId;
    };

    //! ISessionHandlingClientRequests
    //! Requests made to the client to manage their membership in a session
    class ISessionHandlingClientRequests
    {
    public:
        AZ_RTTI(ISessionHandlingClientRequests, "{41DE6BD3-72BC-4443-BFF9-5B1B9396657A}");
        ISessionHandlingClientRequests() = default;
        virtual ~ISessionHandlingClientRequests() = default;

        //! Request the player join session
        //! @param  sessionConnectionConfig The required properties to handle the player join session process
        //! @return The result of player join session process
        virtual bool RequestPlayerJoinSession(const SessionConnectionConfig& sessionConnectionConfig) = 0;

        //! Request the connected player leave session
        virtual void RequestPlayerLeaveSession() = 0;
    };

    //! ISessionProviderRequests
    //! Requests made to the service providing server/fleet management by the server
    class ISessionHandlingProviderRequests
    {
    public:
        AZ_RTTI(ISessionHandlingProviderRequests, "{4F0C17BA-F470-4242-A8CB-EC7EA805257C}");
        ISessionHandlingProviderRequests() = default;
        virtual ~ISessionHandlingProviderRequests() = default;

        //! Handle the destroy session process
        virtual void HandleDestroySession() = 0;

        //! Validate the player join session process
        //! @param  playerConnectionConfig The required properties to validate the player join session process
        //! @return The result of player join session validation
        virtual bool ValidatePlayerJoinSession(const PlayerConnectionConfig& playerConnectionConfig) = 0;

        //! Handle the player leave session process
        //! @param  playerConnectionConfig The required properties to handle the player leave session process
        virtual void HandlePlayerLeaveSession(const PlayerConnectionConfig& playerConnectionConfig) = 0;

        //! Retrieves the file location of a pem-encoded TLS certificate for Client to Server communication
        //! @return If successful, returns the file location of TLS certificate file; if not successful, returns
        //!         empty string.
        virtual AZ::IO::Path GetExternalSessionCertificate() = 0;

        //! Retrieves the file location of a pem-encoded TLS certificate for Server to Server communication
        //! @return If successful, returns the file location of TLS certificate file; if not successful, returns
        //!         empty string.
        virtual AZ::IO::Path GetInternalSessionCertificate() = 0;
    };
} // namespace AzFramework
