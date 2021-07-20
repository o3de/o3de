/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/Session/SessionConfig.h>

#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftClientSystemComponent.h>
#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>
#include <Request/AWSGameLiftCreateSessionRequest.h>
#include <Request/AWSGameLiftJoinSessionRequest.h>
#include <Request/AWSGameLiftSearchSessionsRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    AWSGameLiftClientSystemComponent::AWSGameLiftClientSystemComponent()
    {
        m_gameliftClientManager = AZStd::make_unique<AWSGameLiftClientManager>();
    }

    void AWSGameLiftClientSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectCreateSessionRequest(context);
        AWSGameLiftCreateSessionOnQueueRequest::Reflect(context);
        AWSGameLiftCreateSessionRequest::Reflect(context);
        AWSGameLiftJoinSessionRequest::Reflect(context);
        AWSGameLiftSearchSessionsRequest::Reflect(context);
        ReflectSearchSessionsResponse(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSGameLiftClientSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext
                    ->Class<AWSGameLiftClientSystemComponent>(
                        "AWSGameLiftClient",
                        "Create the GameLift client manager that handles communication between game clients and the GameLift service.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AWSGameLiftRequestBus>("AWSGameLiftRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Event("ConfigureGameLiftClient", &AWSGameLiftRequestBus::Events::ConfigureGameLiftClient,
                    {{{"Region", ""}}})
                ->Event("CreatePlayerId", &AWSGameLiftRequestBus::Events::CreatePlayerId,
                    {{{"IncludeBrackets", ""},
                      {"IncludeDashes", ""}}})
                ;
            behaviorContext->EBus<AWSGameLiftSessionAsyncRequestBus>("AWSGameLiftSessionAsyncRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Event("CreateSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::CreateSessionAsync,
                    {{{"CreateSessionRequest", ""}}})
                ->Event("JoinSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::JoinSessionAsync,
                    {{{"JoinSessionRequest", ""}}})
                ->Event("SearchSessionsAsync", &AWSGameLiftSessionAsyncRequestBus::Events::SearchSessionsAsync,
                    {{{"SearchSessionsRequest", ""}}})
                ->Event("LeaveSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::LeaveSessionAsync)
                ;
            behaviorContext
                ->EBus<AzFramework::SessionAsyncRequestNotificationBus>("AWSGameLiftSessionAsyncRequestNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Handler<AWSGameLiftSessionAsyncRequestNotificationBusHandler>()
                ;
            behaviorContext->EBus<AWSGameLiftSessionRequestBus>("AWSGameLiftSessionRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Event("CreateSession", &AWSGameLiftSessionRequestBus::Events::CreateSession,
                    {{{"CreateSessionRequest", ""}}})
                ->Event("JoinSession", &AWSGameLiftSessionRequestBus::Events::JoinSession,
                    {{{"JoinSessionRequest", ""}}})
                ->Event("SearchSessions", &AWSGameLiftSessionRequestBus::Events::SearchSessions,
                    {{{"SearchSessionsRequest", ""}}})
                ->Event("LeaveSession", &AWSGameLiftSessionRequestBus::Events::LeaveSession)
                ;
        }
    }

    void AWSGameLiftClientSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSGameLiftClientService"));
    }

    void AWSGameLiftClientSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSGameLiftClientService"));
    }

    void AWSGameLiftClientSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSGameLiftClientSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSGameLiftClientSystemComponent::Init()
    {
    }

    void AWSGameLiftClientSystemComponent::Activate()
    {
        m_gameliftClientManager->ActivateManager();
    }

    void AWSGameLiftClientSystemComponent::Deactivate()
    {
        m_gameliftClientManager->DeactivateManager();
    }

    void AWSGameLiftClientSystemComponent::ReflectCreateSessionRequest(AZ::ReflectContext* context)
    {
        AzFramework::CreateSessionRequest::Reflect(context);
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::CreateSessionRequest>("CreateSessionRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                // Expose base type to BehaviorContext, but hide it to be used directly
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;
        }
    }

    void AWSGameLiftClientSystemComponent::ReflectSearchSessionsResponse(AZ::ReflectContext* context)
    {
        // As it is a common response type, reflection could be moved to AzFramework to avoid duplication
        AzFramework::SessionConfig::Reflect(context);
        AzFramework::SearchSessionsResponse::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AzFramework::SessionConfig>("SessionConfig")
                ->Attribute(AZ::Script::Attributes::Category, "Session")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("CreationTime", BehaviorValueProperty(&AzFramework::SessionConfig::m_creationTime))
                ->Property("CreatorId", BehaviorValueProperty(&AzFramework::SessionConfig::m_creatorId))
                ->Property("CurrentPlayer", BehaviorValueProperty(&AzFramework::SessionConfig::m_currentPlayer))
                ->Property("DnsName", BehaviorValueProperty(&AzFramework::SessionConfig::m_dnsName))
                ->Property("IpAddress", BehaviorValueProperty(&AzFramework::SessionConfig::m_ipAddress))
                ->Property("MaxPlayer", BehaviorValueProperty(&AzFramework::SessionConfig::m_maxPlayer))
                ->Property("Port", BehaviorValueProperty(&AzFramework::SessionConfig::m_port))
                ->Property("SessionId", BehaviorValueProperty(&AzFramework::SessionConfig::m_sessionId))
                ->Property("SessionName", BehaviorValueProperty(&AzFramework::SessionConfig::m_sessionName))
                ->Property("SessionProperties", BehaviorValueProperty(&AzFramework::SessionConfig::m_sessionProperties))
                ->Property("Status", BehaviorValueProperty(&AzFramework::SessionConfig::m_status))
                ->Property("StatusReason", BehaviorValueProperty(&AzFramework::SessionConfig::m_statusReason))
                ->Property("TerminationTime", BehaviorValueProperty(&AzFramework::SessionConfig::m_terminationTime))
                ;
            behaviorContext->Class<AzFramework::SearchSessionsResponse>("SearchSessionsResponse")
                ->Attribute(AZ::Script::Attributes::Category, "Session")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("NextToken", BehaviorValueProperty(&AzFramework::SearchSessionsResponse::m_nextToken))
                ->Property("SessionConfigs", BehaviorValueProperty(&AzFramework::SearchSessionsResponse::m_sessionConfigs))
                ;
        }
    }

    void AWSGameLiftClientSystemComponent::SetGameLiftClientManager(AZStd::unique_ptr<AWSGameLiftClientManager> gameliftClientManager)
    {
        m_gameliftClientManager.reset();
        m_gameliftClientManager = AZStd::move(gameliftClientManager);
    }
} // namespace AWSGameLift
