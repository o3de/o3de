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

#include <AzFramework/Session/ISessionRequests.h>

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
