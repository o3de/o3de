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
#include <AzFramework/Translation/TranslationDef.h>
#include <Multiplayer/Session/SessionRequests.h>
#include <Multiplayer/Session/SessionConfig.h>

namespace Multiplayer
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
                editContext->Class<CreateSessionRequest>(
                    QT_TRANSLATE_NOOP("Multiplayer", "CreateSessionRequest"),
                    QT_TRANSLATE_NOOP("Multiplayer", "The container for CreateSession request parameters"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_creatorId,
                        QT_TRANSLATE_NOOP("Multiplayer", "CreatorId"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A unique identifier for a player or entity creating the session"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_sessionProperties,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionProperties"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A collection of custom properties for a session"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_sessionName,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionName"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A descriptive label that is associated with a session"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CreateSessionRequest::m_maxPlayer,
                        QT_TRANSLATE_NOOP("Multiplayer", "MaxPlayer"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The maximum number of players that can be connected simultaneously to the session"))
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
                editContext->Class<SearchSessionsRequest>(
                    QT_TRANSLATE_NOOP("Multiplayer", "SearchSessionsRequest"),
                    QT_TRANSLATE_NOOP("Multiplayer", "The container for SearchSessions request parameters"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_filterExpression,
                        QT_TRANSLATE_NOOP("Multiplayer", "FilterExpression"),
                        QT_TRANSLATE_NOOP("Multiplayer", "String containing the search criteria for the session search"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_sortExpression,
                        QT_TRANSLATE_NOOP("Multiplayer", "SortExpression"),
                        QT_TRANSLATE_NOOP("Multiplayer", "Instructions on how to sort the search results"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_maxResult,
                        QT_TRANSLATE_NOOP("Multiplayer", "MaxResult"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The maximum number of results to return"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsRequest::m_nextToken,
                        QT_TRANSLATE_NOOP("Multiplayer", "NextToken"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A token that indicates the start of the next sequential page of results"))
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
                editContext->Class<SearchSessionsResponse>(
                    QT_TRANSLATE_NOOP("Multiplayer", "SearchSessionsResponse"),
                    QT_TRANSLATE_NOOP("Multiplayer", "The container for SearchSession request results"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsResponse::m_sessionConfigs,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionConfigs"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A collection of sessions that match the search criteria and sorted in specific order"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SearchSessionsResponse::m_nextToken,
                        QT_TRANSLATE_NOOP("Multiplayer", "NextToken"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A token that indicates the start of the next sequential page of results"))
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
                editContext->Class<JoinSessionRequest>(
                    QT_TRANSLATE_NOOP("Multiplayer", "JoinSessionRequest"),
                    QT_TRANSLATE_NOOP("Multiplayer", "The container for JoinSession request parameters"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JoinSessionRequest::m_sessionId,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionId"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A unique identifier for the session"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JoinSessionRequest::m_playerId,
                        QT_TRANSLATE_NOOP("Multiplayer", "PlayerId"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A unique identifier for a player. Player IDs are developer-defined"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JoinSessionRequest::m_playerData,
                        QT_TRANSLATE_NOOP("Multiplayer", "PlayerData"),
                        QT_TRANSLATE_NOOP("Multiplayer", "Developer-defined information related to a player"))
                    ;
            }
        }
    }
} // namespace Multiplayer
