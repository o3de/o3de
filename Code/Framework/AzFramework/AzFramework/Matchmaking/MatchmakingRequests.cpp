/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Matchmaking/MatchmakingRequests.h>

namespace AzFramework
{
    void AcceptMatchRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AcceptMatchRequest>()
                ->Version(0)
                ->Field("acceptMatch", &AcceptMatchRequest::m_acceptMatch)
                ->Field("playerIds", &AcceptMatchRequest::m_playerIds)
                ->Field("ticketId", &AcceptMatchRequest::m_ticketId);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AcceptMatchRequest>("AcceptMatchRequest", "The container for AcceptMatch request parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AcceptMatchRequest::m_acceptMatch, "AcceptMatch",
                        "Player response to accept or reject match")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AcceptMatchRequest::m_playerIds, "PlayerIds",
                        "A list of unique identifiers for players delivering the response")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AcceptMatchRequest::m_ticketId, "TicketId",
                        "A unique identifier for a matchmaking ticket");
            }
        }
    }

    void StartMatchmakingRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StartMatchmakingRequest>()
                ->Version(0)
                ->Field("ticketId", &StartMatchmakingRequest::m_ticketId);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StartMatchmakingRequest>("StartMatchmakingRequest", "The container for StartMatchmaking request parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StartMatchmakingRequest::m_ticketId, "TicketId",
                        "A unique identifier for a matchmaking ticket");
            }
        }
    }

    void StopMatchmakingRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<StopMatchmakingRequest>()
                ->Version(0)
                ->Field("ticketId", &StopMatchmakingRequest::m_ticketId);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StopMatchmakingRequest>("StopMatchmakingRequest", "The container for StopMatchmaking request parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StopMatchmakingRequest::m_ticketId, "TicketId",
                        "A unique identifier for a matchmaking ticket");
            }
        }
    }
} // namespace AzFramework
