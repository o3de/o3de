/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <Multiplayer/Session/SessionConfig.h>

#include <AWSCoreBus.h>

#include <AWSGameLiftClientLocalTicketTracker.h>
#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftClientSystemComponent.h>

#include <Request/AWSGameLiftAcceptMatchRequest.h>
#include <Request/AWSGameLiftCreateSessionOnQueueRequest.h>
#include <Request/AWSGameLiftCreateSessionRequest.h>
#include <Request/AWSGameLiftJoinSessionRequest.h>
#include <Request/AWSGameLiftSearchSessionsRequest.h>
#include <Request/AWSGameLiftStartMatchmakingRequest.h>
#include <Request/AWSGameLiftStopMatchmakingRequest.h>

#include <aws/gamelift/GameLiftClient.h>

namespace AWSGameLift
{
    AWSGameLiftClientSystemComponent::AWSGameLiftClientSystemComponent()
    {
        m_gameliftManager = AZStd::make_unique<AWSGameLiftClientManager>();
        m_gameliftTicketTracker = AZStd::make_unique<AWSGameLiftClientLocalTicketTracker>();
    }

    void AWSGameLiftClientSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectGameLiftMatchmaking(context);
        ReflectGameLiftSession(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSGameLiftClientSystemComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext
                    ->Class<AWSGameLiftClientSystemComponent>(
                        "AWSGameLiftClient",
                        "Create the GameLift client manager that handles communication between game clients and the GameLift service.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AWSGameLiftRequestBus>("AWSGameLiftRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Event("ConfigureGameLiftClient", &AWSGameLiftRequestBus::Events::ConfigureGameLiftClient,
                    { { { "Region", "" } } })
                ->Event("CreatePlayerId", &AWSGameLiftRequestBus::Events::CreatePlayerId,
                    { { { "IncludeBrackets", "" },
                        { "IncludeDashes", "" } } });
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
        AZ::Interface<IAWSGameLiftInternalRequests>::Register(this);

        m_gameliftClient.reset();
        m_gameliftManager->ActivateManager();
        m_gameliftTicketTracker->ActivateTracker();
    }

    void AWSGameLiftClientSystemComponent::Deactivate()
    {
        m_gameliftTicketTracker->DeactivateTracker();
        m_gameliftManager->DeactivateManager();
        m_gameliftClient.reset();

        AZ::Interface<IAWSGameLiftInternalRequests>::Unregister(this);
    }
    
    void AWSGameLiftClientSystemComponent::ReflectGameLiftMatchmaking(AZ::ReflectContext* context)
    {
        AWSGameLiftAcceptMatchRequest::Reflect(context);
        AWSGameLiftStartMatchmakingRequest::Reflect(context);
        AWSGameLiftStopMatchmakingRequest::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AWSGameLiftMatchmakingAsyncRequestBus>("AWSGameLiftMatchmakingAsyncRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Matchmaking")
                ->Event("AcceptMatchAsync", &AWSGameLiftMatchmakingAsyncRequestBus::Events::AcceptMatchAsync,
                    { { { "AcceptMatchRequest", "" } } })
                ->Event("StartMatchmakingAsync", &AWSGameLiftMatchmakingAsyncRequestBus::Events::StartMatchmakingAsync,
                    { { { "StartMatchmakingRequest", "" } } })
                ->Event("StopMatchmakingAsync", &AWSGameLiftMatchmakingAsyncRequestBus::Events::StopMatchmakingAsync,
                    { { { "StopMatchmakingRequest", "" } } });

            behaviorContext->EBus<Multiplayer::MatchmakingAsyncRequestNotificationBus>("AWSGameLiftMatchmakingAsyncRequestNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Matchmaking")
                ->Handler<AWSGameLiftMatchmakingAsyncRequestNotificationBusHandler>();

            behaviorContext->EBus<AWSGameLiftMatchmakingRequestBus>("AWSGameLiftMatchmakingRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Matchmaking")
                ->Event("AcceptMatch", &AWSGameLiftMatchmakingRequestBus::Events::AcceptMatch,
                    { { { "AcceptMatchRequest", "" } } })
                ->Event("StartMatchmaking", &AWSGameLiftMatchmakingRequestBus::Events::StartMatchmaking,
                    { { { "StartMatchmakingRequest", "" } } })
                ->Event("StopMatchmaking", &AWSGameLiftMatchmakingRequestBus::Events::StopMatchmaking,
                    { { { "StopMatchmakingRequest", "" } } });

            behaviorContext->EBus<AWSGameLiftMatchmakingEventRequestBus>("AWSGameLiftMatchmakingEventRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Matchmaking")
                ->Event("StartPolling", &AWSGameLiftMatchmakingEventRequestBus::Events::StartPolling,
                    { { { "TicketId", "" },
                        { "PlayerId", "" } } })
                ->Event("StopPolling", &AWSGameLiftMatchmakingEventRequestBus::Events::StopPolling);

            behaviorContext->EBus<Multiplayer::MatchmakingNotificationBus>("AWSGameLiftMatchmakingNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Matchmaking")
                ->Handler<AWSGameLiftMatchmakingNotificationBusHandler>();
        }
    }

    void AWSGameLiftClientSystemComponent::ReflectGameLiftSession(AZ::ReflectContext* context)
    {
        ReflectCreateSessionRequest(context);
        AWSGameLiftCreateSessionOnQueueRequest::Reflect(context);
        AWSGameLiftCreateSessionRequest::Reflect(context);
        AWSGameLiftJoinSessionRequest::Reflect(context);
        AWSGameLiftSearchSessionsRequest::Reflect(context);
        ReflectSearchSessionsResponse(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AWSGameLiftSessionAsyncRequestBus>("AWSGameLiftSessionAsyncRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Session")
                ->Event("CreateSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::CreateSessionAsync,
                    { { { "CreateSessionRequest", "" } } })
                ->Event("JoinSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::JoinSessionAsync, { { { "JoinSessionRequest", "" } } })
                ->Event("SearchSessionsAsync", &AWSGameLiftSessionAsyncRequestBus::Events::SearchSessionsAsync,
                    { { { "SearchSessionsRequest", "" } } })
                ->Event("LeaveSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::LeaveSessionAsync);

            behaviorContext->EBus<Multiplayer::SessionAsyncRequestNotificationBus>("AWSGameLiftSessionAsyncRequestNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Session")
                ->Handler<AWSGameLiftSessionAsyncRequestNotificationBusHandler>();

            behaviorContext->EBus<AWSGameLiftSessionRequestBus>("AWSGameLiftSessionRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift/Session")
                ->Event("CreateSession", &AWSGameLiftSessionRequestBus::Events::CreateSession, { { { "CreateSessionRequest", "" } } })
                ->Event("JoinSession", &AWSGameLiftSessionRequestBus::Events::JoinSession, { { { "JoinSessionRequest", "" } } })
                ->Event("SearchSessions", &AWSGameLiftSessionRequestBus::Events::SearchSessions, { { { "SearchSessionsRequest", "" } } })
                ->Event("LeaveSession", &AWSGameLiftSessionRequestBus::Events::LeaveSession);
        }
    }

    void AWSGameLiftClientSystemComponent::ReflectCreateSessionRequest(AZ::ReflectContext* context)
    {
        Multiplayer::CreateSessionRequest::Reflect(context);
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Multiplayer::CreateSessionRequest>("CreateSessionRequest")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                // Expose base type to BehaviorContext, but hide it to be used directly
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;
        }
    }

    void AWSGameLiftClientSystemComponent::ReflectSearchSessionsResponse(AZ::ReflectContext* context)
    {
        // As it is a common response type, reflection could be moved to AzFramework to avoid duplication
        Multiplayer::SessionConfig::Reflect(context);
        Multiplayer::SearchSessionsResponse::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Multiplayer::SessionConfig>("SessionConfig")
                ->Attribute(AZ::Script::Attributes::Category, "Session")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("CreationTime", BehaviorValueProperty(&Multiplayer::SessionConfig::m_creationTime))
                ->Property("CreatorId", BehaviorValueProperty(&Multiplayer::SessionConfig::m_creatorId))
                ->Property("CurrentPlayer", BehaviorValueProperty(&Multiplayer::SessionConfig::m_currentPlayer))
                ->Property("DnsName", BehaviorValueProperty(&Multiplayer::SessionConfig::m_dnsName))
                ->Property("IpAddress", BehaviorValueProperty(&Multiplayer::SessionConfig::m_ipAddress))
                ->Property("MaxPlayer", BehaviorValueProperty(&Multiplayer::SessionConfig::m_maxPlayer))
                ->Property("Port", BehaviorValueProperty(&Multiplayer::SessionConfig::m_port))
                ->Property("SessionId", BehaviorValueProperty(&Multiplayer::SessionConfig::m_sessionId))
                ->Property("SessionName", BehaviorValueProperty(&Multiplayer::SessionConfig::m_sessionName))
                ->Property("SessionProperties", BehaviorValueProperty(&Multiplayer::SessionConfig::m_sessionProperties))
                ->Property("MatchmakingData", BehaviorValueProperty(&Multiplayer::SessionConfig::m_matchmakingData))
                ->Property("Status", BehaviorValueProperty(&Multiplayer::SessionConfig::m_status))
                ->Property("StatusReason", BehaviorValueProperty(&Multiplayer::SessionConfig::m_statusReason))
                ->Property("TerminationTime", BehaviorValueProperty(&Multiplayer::SessionConfig::m_terminationTime))
                ;
            behaviorContext->Class<Multiplayer::SearchSessionsResponse>("SearchSessionsResponse")
                ->Attribute(AZ::Script::Attributes::Category, "Session")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("NextToken", BehaviorValueProperty(&Multiplayer::SearchSessionsResponse::m_nextToken))
                ->Property("SessionConfigs", BehaviorValueProperty(&Multiplayer::SearchSessionsResponse::m_sessionConfigs))
                ;
        }
    }

    AZStd::shared_ptr<Aws::GameLift::GameLiftClient> AWSGameLiftClientSystemComponent::GetGameLiftClient() const
    {
        return m_gameliftClient;
    }

    void AWSGameLiftClientSystemComponent::SetGameLiftClient(AZStd::shared_ptr<Aws::GameLift::GameLiftClient> gameliftClient)
    {
        m_gameliftClient.swap(gameliftClient);
    }

    void AWSGameLiftClientSystemComponent::SetGameLiftClientManager(AZStd::unique_ptr<AWSGameLiftClientManager> gameliftManager)
    {
        m_gameliftManager.reset();
        m_gameliftManager = AZStd::move(gameliftManager);
    }

    void AWSGameLiftClientSystemComponent::SetGameLiftClientTicketTracker(AZStd::unique_ptr<AWSGameLiftClientLocalTicketTracker> gameliftTicketTracker)
    {
        m_gameliftTicketTracker.reset();
        m_gameliftTicketTracker = AZStd::move(gameliftTicketTracker);
    }
} // namespace AWSGameLift
