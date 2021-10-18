/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    //! AcceptMatchRequest
    //! The container for AcceptMatch request parameters.
    struct AcceptMatchRequest
    {
        AZ_RTTI(AcceptMatchRequest, "{AD289D76-CEE2-424F-847E-E62AA83B7D79}");
        static void Reflect(AZ::ReflectContext* context);

        AcceptMatchRequest() = default;
        virtual ~AcceptMatchRequest() = default;

        // Player response to accept or reject match
        bool m_acceptMatch;
        // A list of unique identifiers for players delivering the response
        AZStd::vector<AZStd::string> m_playerIds;
        // A unique identifier for a matchmaking ticket
        AZStd::string m_ticketId;
    };

    //! StartMatchmakingRequest
    //! The container for StartMatchmaking request parameters.
    struct StartMatchmakingRequest
    {
        AZ_RTTI(StartMatchmakingRequest, "{70B47776-E8E7-4993-BEC3-5CAEC3D48E47}");
        static void Reflect(AZ::ReflectContext* context);

        StartMatchmakingRequest() = default;
        virtual ~StartMatchmakingRequest() = default;

        // A unique identifier for a matchmaking ticket
        AZStd::string m_ticketId;
    };

    //! StopMatchmakingRequest
    //! The container for StopMatchmaking request parameters.
    struct StopMatchmakingRequest
    {
        AZ_RTTI(StopMatchmakingRequest, "{6132E293-65EF-4DC2-A8A0-00269697229D}");
        static void Reflect(AZ::ReflectContext* context);

        StopMatchmakingRequest() = default;
        virtual ~StopMatchmakingRequest() = default;

        // A unique identifier for a matchmaking ticket
        AZStd::string m_ticketId;
    };
} // namespace AzFramework
