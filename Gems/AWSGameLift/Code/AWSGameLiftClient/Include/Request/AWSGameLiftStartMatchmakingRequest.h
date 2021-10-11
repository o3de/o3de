/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Matchmaking/MatchmakingRequests.h>

namespace AWSGameLift
{
    //! AWSGameLiftPlayerInformation
    //! Information on each player to be matched
    //! This information must include a player ID, and may contain player attributes and latency data to be used in the matchmaking process
    //! After a successful match, Player objects contain the name of the team the player is assigned to
    struct AWSGameLiftPlayerInformation
    {
        AZ_RTTI(AWSGameLiftPlayerInformation, "{B62C118E-C55D-4903-8ECB-E58E8CA613C4}");
        static void Reflect(AZ::ReflectContext* context);

        AWSGameLiftPlayerInformation() = default;
        virtual ~AWSGameLiftPlayerInformation() = default;

        // A map of region names to latencies in millseconds, that indicates
        // the amount of latency that a player experiences when connected to AWS Regions
        AZStd::unordered_map<AZStd::string, int> m_latencyInMs;
        // A collection of key:value pairs containing player information for use in matchmaking
        // Player attribute keys must match the playerAttributes used in a matchmaking rule set
        // Example: {"skill": "{\"N\": \"23\"}", "gameMode": "{\"S\": \"deathmatch\"}"}
        AZStd::unordered_map<AZStd::string, AZStd::string> m_playerAttributes;
        // A unique identifier for a player
        AZStd::string m_playerId;
        // Name of the team that the player is assigned to in a match
        AZStd::string m_team;
    };

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
        AZStd::vector<AWSGameLiftPlayerInformation> m_players;
    };
} // namespace AWSGameLift
