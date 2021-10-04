/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Request/AWSGameLiftStartMatchmakingRequest.h>

namespace AWSGameLift
{
    void AWSGameLiftPlayerInformation::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftPlayerInformation>()
                ->Version(0)
                ->Field("latencyInMs", &AWSGameLiftPlayerInformation::m_latencyInMs)
                ->Field("playerAttributes", &AWSGameLiftPlayerInformation::m_playerAttributes)
                ->Field("playerId", &AWSGameLiftPlayerInformation::m_playerId)
                ->Field("team", &AWSGameLiftPlayerInformation::m_team);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftPlayerInformation>("AWSGameLiftPlayerInformation", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayerInformation::m_latencyInMs, "LatencyInMs",
                        "A set of values, expressed in milliseconds, that indicates the amount of latency that"
                        "a player experiences when connected to AWS Regions")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayerInformation::m_playerAttributes, "PlayerAttributes",
                        "A collection of key:value pairs containing player information for use in matchmaking")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayerInformation::m_playerId, "PlayerId",
                        "A unique identifier for a player")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayerInformation::m_team, "Team",
                        "Name of the team that the player is assigned to in a match");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSGameLiftPlayerInformation>("AWSGameLiftPlayerInformation")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("LatencyInMs", BehaviorValueProperty(&AWSGameLiftPlayerInformation::m_latencyInMs))
                ->Property("PlayerAttributes", BehaviorValueProperty(&AWSGameLiftPlayerInformation::m_playerAttributes))
                ->Property("PlayerId", BehaviorValueProperty(&AWSGameLiftPlayerInformation::m_playerId))
                ->Property("Team", BehaviorValueProperty(&AWSGameLiftPlayerInformation::m_team));
        }
    }

    void AWSGameLiftStartMatchmakingRequest::Reflect(AZ::ReflectContext* context)
    {
        AzFramework::StartMatchmakingRequest::Reflect(context);
        AWSGameLiftPlayerInformation::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftStartMatchmakingRequest, AzFramework::StartMatchmakingRequest>()
                ->Version(0)
                ->Field("configurationName", &AWSGameLiftStartMatchmakingRequest::m_configurationName)
                ->Field("players", &AWSGameLiftStartMatchmakingRequest::m_players);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftStartMatchmakingRequest>("AWSGameLiftStartMatchmakingRequest", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AWSGameLiftStartMatchmakingRequest::m_configurationName, "ConfigurationName",
                        "Name of the matchmaking configuration to use for this request")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AWSGameLiftStartMatchmakingRequest::m_players, "Players",
                        "Information on each player to be matched.");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSGameLiftStartMatchmakingRequest>("AWSGameLiftStartMatchmakingRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("TicketId", BehaviorValueProperty(&AWSGameLiftStartMatchmakingRequest::m_ticketId))
                ->Property("ConfigurationName", BehaviorValueProperty(&AWSGameLiftStartMatchmakingRequest::m_configurationName))
                ->Property("Players", BehaviorValueProperty(&AWSGameLiftStartMatchmakingRequest::m_players));
        }
    }
} // namespace AWSGameLift
