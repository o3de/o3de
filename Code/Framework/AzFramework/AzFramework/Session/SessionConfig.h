/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    //! SessionConfig
    //! Properties describing a session.
    struct SessionConfig
    {
        AZ_RTTI(SessionConfig, "{992DD4BE-8BA5-4071-8818-B99FD2952086}");
        static void Reflect(AZ::ReflectContext* context);

        SessionConfig() = default;
        virtual ~SessionConfig() = default;

        // A time stamp indicating when this session was created. Format is a number expressed in Unix time as milliseconds.
        uint64_t m_creationTime = 0;
        
        // A time stamp indicating when this data object was terminated. Same format as creation time.
        uint64_t m_terminationTime = 0;
        
        // A unique identifier for a player or entity creating the session.
        AZStd::string m_creatorId;
        
        // A collection of custom properties for a session.
        AZStd::unordered_map<AZStd::string, AZStd::string> m_sessionProperties;

        // The matchmaking process information that was used to create the session.
        AZStd::string m_matchmakingData;
        
        // A unique identifier for the session.
        AZStd::string m_sessionId;
        
        // A descriptive label that is associated with a session.
        AZStd::string m_sessionName;

        // The DNS identifier assigned to the instance that is running the session.
        AZStd::string m_dnsName;

        // The IP address of the session.
        AZStd::string m_ipAddress;
        
        // The port number for the session.
        uint16_t m_port = 0;
        
        // The maximum number of players that can be connected simultaneously to the session.
        uint64_t m_maxPlayer = 0;

        // Number of players currently in the session.
        uint64_t m_currentPlayer = 0;

        // Current status of the session.
        AZStd::string m_status;
        
        // Provides additional information about session status.
        AZStd::string m_statusReason;
    };
} // namespace AzFramework
