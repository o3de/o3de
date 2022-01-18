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
    void AWSGameLiftStartMatchmakingRequest::Reflect(AZ::ReflectContext* context)
    {
        AzFramework::StartMatchmakingRequest::Reflect(context);
        AWSGameLiftPlayer::Reflect(context);

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
            behaviorContext->Class<AzFramework::StartMatchmakingRequest>("StartMatchmakingRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                // Expose base type to BehaviorContext, but hide it to be used directly
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);

            behaviorContext->Class<AWSGameLiftStartMatchmakingRequest>("AWSGameLiftStartMatchmakingRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("TicketId", BehaviorValueProperty(&AWSGameLiftStartMatchmakingRequest::m_ticketId))
                ->Property("ConfigurationName", BehaviorValueProperty(&AWSGameLiftStartMatchmakingRequest::m_configurationName))
                ->Property("Players", BehaviorValueProperty(&AWSGameLiftStartMatchmakingRequest::m_players));
        }
    }
} // namespace AWSGameLift
