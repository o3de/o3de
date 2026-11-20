/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>
#include <Multiplayer/Session/SessionConfig.h>

namespace Multiplayer
{
    void SessionConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SessionConfig>()
                ->Version(0)
                ->Field("creationTime", &SessionConfig::m_creationTime)
                ->Field("terminationTime", &SessionConfig::m_terminationTime)
                ->Field("creatorId", &SessionConfig::m_creatorId)
                ->Field("sessionProperties", &SessionConfig::m_sessionProperties)
                ->Field("matchmakingData", &SessionConfig::m_matchmakingData)
                ->Field("sessionId", &SessionConfig::m_sessionId)
                ->Field("sessionName", &SessionConfig::m_sessionName)
                ->Field("dnsName", &SessionConfig::m_dnsName)
                ->Field("ipAddress", &SessionConfig::m_ipAddress)
                ->Field("port", &SessionConfig::m_port)
                ->Field("maxPlayer", &SessionConfig::m_maxPlayer)
                ->Field("currentPlayer", &SessionConfig::m_currentPlayer)
                ->Field("status", &SessionConfig::m_status)
                ->Field("statusReason", &SessionConfig::m_statusReason)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SessionConfig>(
                    QT_TRANSLATE_NOOP("Multiplayer", "SessionConfig"),
                    QT_TRANSLATE_NOOP("Multiplayer", "Properties describing a session"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_creationTime,
                        QT_TRANSLATE_NOOP("Multiplayer", "CreationTime"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A time stamp indicating when this session was created. Format is a number expressed in Unix time as milliseconds."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_terminationTime,
                        QT_TRANSLATE_NOOP("Multiplayer", "TerminationTime"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A time stamp indicating when this data object was terminated. Same format as creation time."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_creatorId,
                        QT_TRANSLATE_NOOP("Multiplayer", "CreatorId"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A unique identifier for a player or entity creating the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_sessionProperties,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionProperties"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A collection of custom properties for a session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_matchmakingData,
                        QT_TRANSLATE_NOOP("Multiplayer", "MatchmakingData"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The matchmaking process information that was used to create the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_sessionId,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionId"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A unique identifier for the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_sessionName,
                        QT_TRANSLATE_NOOP("Multiplayer", "SessionName"),
                        QT_TRANSLATE_NOOP("Multiplayer", "A descriptive label that is associated with a session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_dnsName,
                        QT_TRANSLATE_NOOP("Multiplayer", "DnsName"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The DNS identifier assigned to the instance that is running the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_ipAddress,
                        QT_TRANSLATE_NOOP("Multiplayer", "IpAddress"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The IP address of the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_port,
                        QT_TRANSLATE_NOOP("Multiplayer", "Port"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The port number for the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_maxPlayer,
                        QT_TRANSLATE_NOOP("Multiplayer", "MaxPlayer"),
                        QT_TRANSLATE_NOOP("Multiplayer", "The maximum number of players that can be connected simultaneously to the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_currentPlayer,
                        QT_TRANSLATE_NOOP("Multiplayer", "CurrentPlayer"),
                        QT_TRANSLATE_NOOP("Multiplayer", "Number of players currently in the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_status,
                        QT_TRANSLATE_NOOP("Multiplayer", "Status"),
                        QT_TRANSLATE_NOOP("Multiplayer", "Current status of the session."))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Multiplayer::SessionConfig::m_statusReason,
                        QT_TRANSLATE_NOOP("Multiplayer", "StatusReason"),
                        QT_TRANSLATE_NOOP("Multiplayer", "Provides additional information about session status."));
            }
        }
    }
} // namespace Multiplayer
