/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftClientSystemComponent.h>
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
        AWSGameLiftCreateSessionRequest::Reflect(context);
        AWSGameLiftJoinSessionRequest::Reflect(context);
        AWSGameLiftSearchSessionsRequest::Reflect(context);

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
                ->Event("ConfigureGameLiftClient", &AWSGameLiftRequestBus::Events::ConfigureGameLiftClient)
                ->Event("CreatePlayerId", &AWSGameLiftRequestBus::Events::CreatePlayerId)
                ;
            behaviorContext->EBus<AWSGameLiftSessionAsyncRequestBus>("AWSGameLiftSessionAsyncRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Event("CreateSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::CreateSessionAsync)
                ->Event("JoinSessionAsync", &AWSGameLiftSessionAsyncRequestBus::Events::JoinSessionAsync)
                ->Event("SearchSessionsAsync", &AWSGameLiftSessionAsyncRequestBus::Events::SearchSessionsAsync)
                ;
            behaviorContext
                ->EBus<AzFramework::SessionAsyncRequestNotificationBus>("AWSGameLiftSessionAsyncRequestNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Handler<AWSGameLiftSessionAsyncRequestNotificationBusHandler>()
                ;
            behaviorContext->EBus<AWSGameLiftSessionRequestBus>("AWSGameLiftSessionRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSGameLift")
                ->Event("CreateSession", &AWSGameLiftSessionRequestBus::Events::CreateSession)
                ->Event("JoinSession", &AWSGameLiftSessionRequestBus::Events::JoinSession)
                ->Event("SearchSessions", &AWSGameLiftSessionRequestBus::Events::SearchSessions)
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

    void AWSGameLiftClientSystemComponent::SetGameLiftClientManager(AZStd::unique_ptr<AWSGameLiftClientManager> gameliftClientManager)
    {
        m_gameliftClientManager.reset();
        m_gameliftClientManager = AZStd::move(gameliftClientManager);
    }
} // namespace AWSGameLift
