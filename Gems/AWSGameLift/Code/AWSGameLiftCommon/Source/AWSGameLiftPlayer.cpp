/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AWSGameLiftPlayer.h>

namespace AWSGameLift
{
    void AWSGameLiftPlayer::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSGameLiftPlayer>()
                ->Version(0)
                ->Field("latencyInMs", &AWSGameLiftPlayer::m_latencyInMs)
                ->Field("playerAttributes", &AWSGameLiftPlayer::m_playerAttributes)
                ->Field("playerId", &AWSGameLiftPlayer::m_playerId)
                ->Field("team", &AWSGameLiftPlayer::m_team);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AWSGameLiftPlayer>("AWSGameLiftPlayer", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayer::m_latencyInMs, "LatencyInMs",
                        "A set of values, expressed in milliseconds, that indicates the amount of latency that"
                        "a player experiences when connected to AWS Regions")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayer::m_playerAttributes, "PlayerAttributes",
                        "A collection of key:value pairs containing player information for use in matchmaking")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayer::m_playerId, "PlayerId",
                        "A unique identifier for a player")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &AWSGameLiftPlayer::m_team, "Team",
                        "Name of the team that the player is assigned to in a match");
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSGameLiftPlayer>("AWSGameLiftPlayer")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("LatencyInMs", BehaviorValueProperty(&AWSGameLiftPlayer::m_latencyInMs))
                ->Property("PlayerAttributes", BehaviorValueProperty(&AWSGameLiftPlayer::m_playerAttributes))
                ->Property("PlayerId", BehaviorValueProperty(&AWSGameLiftPlayer::m_playerId))
                ->Property("Team", BehaviorValueProperty(&AWSGameLiftPlayer::m_team));
        }
    }
} // namespace AWSGameLift
