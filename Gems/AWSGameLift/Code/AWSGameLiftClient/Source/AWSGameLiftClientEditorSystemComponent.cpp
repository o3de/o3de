/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AWSCoreBus.h>
#include <AWSGameLiftClientEditorSystemComponent.h>

namespace AWSGameLift
{
    AWSGameLiftClientEditorSystemComponent::AWSGameLiftClientEditorSystemComponent()
        : AWSGameLiftClientSystemComponent()
    {
    }

    void AWSGameLiftClientEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AWSGameLiftClientSystemComponent::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSGameLiftClientEditorSystemComponent, AWSGameLiftClientSystemComponent>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<AWSGameLiftClientEditorSystemComponent>("AWSGameLiftClientEditor", "Create the GameLift client manager that handles communication between game clients and the GameLift service.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AWSGameLiftClientEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSGameLiftClientServiceEditor"));
    }

    void AWSGameLiftClientEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSGameLiftClientServiceEditor"));
    }

    void AWSGameLiftClientEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSGameLiftClientEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSGameLiftClientEditorSystemComponent::Activate()
    {
        AWSGameLiftClientSystemComponent::Activate();

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void AWSGameLiftClientEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

        AWSGameLiftClientSystemComponent::Deactivate();
    }

    void AWSGameLiftClientEditorSystemComponent::OnMenuBindingHook()
    {
        constexpr const char* AWSGameLift[] =
        {
             "Game Lift" ,
             "gamelift_gem" ,
             ":/Notifications/download.svg",
             ""
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::CreateSubMenu, AWSCore::AWSMenuIdentifier, AWSGameLift, 300);

        const auto& submenuIdentifier = AWSGameLift[1];

        constexpr const char* AWSGameLiftOverview[] =
        {
             "GameLift Gem overview" ,
             "gamelift_gem_overview" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameLiftOverview, 0);

        constexpr const char* AWSSetupGamelift[] =
        {
             "Setup",
             "setup_gamelift",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/gem-setup/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSSetupGamelift, 0);

        constexpr const char* AWSGameliftScripting[] =
        {
             "Scripting Reference",
             "gamelift_scripting_reference",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/scripting/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameliftScripting, 0);

        constexpr const char* AWSGameliftAdvancedTopics[] =
        {
             "Advanced Topics",
             "gamelift_advanced_topics",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/advanced-topics/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameliftAdvancedTopics, 0);

        constexpr const char* AWSGameliftLocalTesting[] =
        {
             "Local testing",
             "gamelift_local_testing",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/local-testing/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameliftLocalTesting, 0);

        constexpr const char* AWSGameliftPackagingWindows[] =
        {
             "Build packaging (Windows)",
             "gamelift_build_packaging_windows",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/build-packaging-for-windows/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameliftPackagingWindows, 0);

        constexpr const char* AWSGameliftResourceManagement[] =
        {
             "Resource management",
             "gamelift_resource_management",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-gamelift/resource-management/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSGameliftResourceManagement, 0);
    }
} // namespace AWSGameLift
