/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Session/SessionRequests.h>

namespace AWSGameLift
{
    //! AWSGameLiftSearchSessionsRequest
    //! GameLift search sessions request which corresponds to Amazon GameLift
    //! SearchSessionsRequest
    struct AWSGameLiftSearchSessionsRequest
        : public AzFramework::SearchSessionsRequest
    {
    public:
        AZ_RTTI(AWSGameLiftSearchSessionsRequest, "{864C91C0-CA53-4585-BF07-066C0DF3E198}", AzFramework::SearchSessionsRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftSearchSessionsRequest() = default;
        virtual ~AWSGameLiftSearchSessionsRequest() = default;

        // A unique identifier for the alias associated with the fleet to search for active game sessions.
        AZStd::string m_aliasId;

        // A unique identifier for the fleet to search for active game sessions.
        AZStd::string m_fleetId;

        // A fleet location to search for game sessions.
        AZStd::string m_location;
    };
} // namespace AWSGameLift

