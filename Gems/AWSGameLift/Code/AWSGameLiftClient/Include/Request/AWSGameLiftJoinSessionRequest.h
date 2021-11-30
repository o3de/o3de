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
    //! AWSGameLiftJoinSessionRequest
    //! GameLift join session request which corresponds to Amazon GameLift CreatePlayerSessionRequest.
    //! Once player session has been created successfully in game session, gamelift client manager will
    //! signal Multiplayer Gem to setup networking connection.
    struct AWSGameLiftJoinSessionRequest
        : public AzFramework::JoinSessionRequest
    {
    public:
        AZ_RTTI(AWSGameLiftJoinSessionRequest, "{6EED6D15-531A-4956-90D0-2EDA31AC9CBA}", AzFramework::JoinSessionRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftJoinSessionRequest() = default;
        virtual ~AWSGameLiftJoinSessionRequest() = default;
    };
} // namespace AWSGameLift
