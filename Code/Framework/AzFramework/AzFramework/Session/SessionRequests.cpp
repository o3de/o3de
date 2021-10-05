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
#include <AzFramework/Session/SessionRequests.h>
#include <AzFramework/Session/SessionConfig.h>

namespace AzFramework
{
    void CreateSessionRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CreateSessionRequest>()
                ->Version(0)
                ->Field("creatorId", &CreateSessionRequest::m_creatorId)
                ->Field("sessionProperties", &CreateSessionRequest::m_sessionProperties)
                ->Field("sessionName", &CreateSessionRequest::m_sessionName)
                ->Field("maxPlayer", &CreateSessionRequest::m_maxPlayer)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CreateSessionRequest>("CreateSessionRequest", "The container for CreateSession request parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_creatorId,
                        "CreatorId", "A unique identifier for a player or entity creating the session")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_sessionProperties,
                        "SessionProperties", "A collection of custom properties for a session")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_sessionName,
                        "SessionName", "A descriptive label that is associated with a session")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_maxPlayer,
                        "MaxPlayer", "The maximum number of players that can be connected simultaneously to the session")
                    ;
            }
        }
    }

    void SearchSessionsRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SearchSessionsRequest>()
                ->Version(0)
                ->Field("filterExpression", &SearchSessionsRequest::m_filterExpression)
                ->Field("sortExpression", &SearchSessionsRequest::m_sortExpression)
                ->Field("maxResult", &SearchSessionsRequest::m_maxResult)
                ->Field("nextToken", &SearchSessionsRequest::m_nextToken)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SearchSessionsRequest>("SearchSessionsRequest", "The container for SearchSessions request parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_filterExpression,
                        "FilterExpression", "String containing the search criteria for the session search")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_sortExpression,
                        "SortExpression", "Instructions on how to sort the search results")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_maxResult,
                        "MaxResult", "The maximum number of results to return")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_nextToken,
                        "NextToken", "A token that indicates the start of the next sequential page of results")
                    ;
            }
        }
    }

    void SearchSessionsResponse::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SearchSessionsResponse>()
                ->Version(0)
                ->Field("sessionConfigs", &SearchSessionsResponse::m_sessionConfigs)
                ->Field("nextToken", &SearchSessionsResponse::m_nextToken)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SearchSessionsResponse>("SearchSessionsResponse", "The container for SearchSession request results")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsResponse::m_sessionConfigs,
                        "SessionConfigs", "A collection of sessions that match the search criteria and sorted in specific order")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsResponse::m_nextToken,
                        "NextToken", "A token that indicates the start of the next sequential page of results")
                    ;
            }
        }
    }

    void JoinSessionRequest::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JoinSessionRequest>()
                ->Version(0)
                ->Field("sessionId", &JoinSessionRequest::m_sessionId)
                ->Field("playerId", &JoinSessionRequest::m_playerId)
                ->Field("playerData", &JoinSessionRequest::m_playerData)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<JoinSessionRequest>("JoinSessionRequest", "The container for JoinSession request parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JoinSessionRequest::m_sessionId,
                        "SessionId", "A unique identifier for the session")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JoinSessionRequest::m_playerId,
                        "PlayerId", "A unique identifier for a player. Player IDs are developer-defined")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JoinSessionRequest::m_playerData,
                        "PlayerData", "Developer-defined information related to a player")
                    ;
            }
        }
    }
} // namespace AzFramework
