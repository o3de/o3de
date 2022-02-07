/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

namespace AWSGameLift
{
    //! AWSGameLiftPlayer
    //! Information on each player to be matched
    //! This information must include a player ID, and may contain player attributes and latency data to be used in the matchmaking process
    //! After a successful match, Player objects contain the name of the team the player is assigned to
    struct AWSGameLiftPlayer
    {
        AZ_RTTI(AWSGameLiftPlayer, "{B62C118E-C55D-4903-8ECB-E58E8CA613C4}");
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftPlayer() = default;
        virtual ~AWSGameLiftPlayer() = default;

        // A map of region names to latencies in millseconds, that indicates
        // the amount of latency that a player experiences when connected to AWS Regions
        AZStd::unordered_map<AZStd::string, int> m_latencyInMs;

        // A collection of key:value pairs containing player information for use in matchmaking
        // Player attribute keys must match the playerAttributes used in a matchmaking rule set
        // Example: {"skill": "{\"N\": 23}", "gameMode": "{\"S\": \"deathmatch\"}"}
        AZStd::unordered_map<AZStd::string, AZStd::string> m_playerAttributes;

        // A unique identifier for a player
        AZStd::string m_playerId;

        // Name of the team that the player is assigned to in a match
        AZStd::string m_team;
    };
} // namespace AWSGameLift
