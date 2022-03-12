/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Session/SessionConfig.h>

namespace AzFramework
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
                editContext->Class<SessionConfig>("SessionConfig", "Properties describing a session")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_creationTime,
                        "CreationTime", "A time stamp indicating when this session was created. Format is a number expressed in Unix time as milliseconds.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_terminationTime,
                        "TerminationTime", "A time stamp indicating when this data object was terminated. Same format as creation time.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_creatorId,
                        "CreatorId", "A unique identifier for a player or entity creating the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_sessionProperties,
                        "SessionProperties", "A collection of custom properties for a session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_matchmakingData,
                        "MatchmakingData", "The matchmaking process information that was used to create the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_sessionId,
                        "SessionId", "A unique identifier for the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_sessionName,
                        "SessionName", "A descriptive label that is associated with a session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_dnsName,
                        "DnsName", "The DNS identifier assigned to the instance that is running the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_ipAddress,
                        "IpAddress", "The IP address of the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_port,
                        "Port", "The port number for the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_maxPlayer,
                        "MaxPlayer", "The maximum number of players that can be connected simultaneously to the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_currentPlayer,
                        "CurrentPlayer", "Number of players currently in the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_status,
                        "Status", "Current status of the session.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AzFramework::SessionConfig::m_statusReason,
                        "StatusReason", "Provides additional information about session status.");
            }
        }
    }
} // namespace AzFramework
