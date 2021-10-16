/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Matchmaking/MatchmakingRequests.h>

#include <AWSGameLiftPlayer.h>

namespace AWSGameLift
{
    //! AWSGameLiftStartMatchmakingRequest
    //! GameLift start matchmaking request which corresponds to Amazon GameLift
    //! Uses FlexMatch to create a game match for a group of players based on custom matchmaking rules
    //! StartMatchmakingRequest
    struct AWSGameLiftStartMatchmakingRequest
        : public AzFramework::StartMatchmakingRequest
    {
    public:
        AZ_RTTI(AWSGameLiftStartMatchmakingRequest, "{D273DF71-9C55-48C1-95F9-8D7B66B9CF3E}", AzFramework::StartMatchmakingRequest);
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftStartMatchmakingRequest() = default;
        virtual ~AWSGameLiftStartMatchmakingRequest() = default;

        // Name of the matchmaking configuration to use for this request
        AZStd::string m_configurationName;
        // Information on each player to be matched
        AZStd::vector<AWSGameLiftPlayer> m_players;
    };
} // namespace AWSGameLift
