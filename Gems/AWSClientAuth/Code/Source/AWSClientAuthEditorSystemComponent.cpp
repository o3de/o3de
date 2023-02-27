/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSClientAuthEditorSystemComponent.h>

#include <ActionManager/Action/ActionManagerInterface.h>

#include <AWSCoreBus.h>

namespace AWSClientAuth
{
    void AWSClientAuthEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AWSClientAuthSystemComponent::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AWSClientAuthEditorSystemComponent, AWSClientAuthSystemComponent>()
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSClientAuthSystemComponent>("AWSClientAuthEditor", "Provides Client Authentication and Authorization implementations")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void AWSClientAuthEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSClientAuthEditorService"));
    }

    void AWSClientAuthEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSClientAuthEditorService"));
    }

    void AWSClientAuthEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSClientAuthEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&)
    {
    }

    void AWSClientAuthEditorSystemComponent::Init()
    {
        AWSClientAuthSystemComponent::Init();
    }

    void AWSClientAuthEditorSystemComponent::Activate()
    {
        AWSClientAuthSystemComponent::Activate();

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void AWSClientAuthEditorSystemComponent::Deactivate()
    {
        AWSClientAuthSystemComponent::Deactivate();

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    void AWSClientAuthEditorSystemComponent::OnMenuBindingHook()
    {
        constexpr const char* AWSClientAuth[] =
        {
             "Client Auth Gem" ,
             "client_auth_gem" ,
             ":/Notifications/download.svg",
             ""
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::CreateSubMenu, AWSCore::AWSMenuIdentifier, AWSClientAuth, 100);

        const auto& submenuIdentifier = AWSClientAuth[1];

        constexpr const char* AWSClientAuthGemOverview[] =
        {
             "Client Auth Gem overview" ,
             "client_auth_gem_overview" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSClientAuthGemOverview, 0);

        constexpr const char* AWSSetupClientAuthGem[] =
        {
             "Setup Client Auth Gem",
             "setup_client_auth_gem",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/setup/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSSetupClientAuthGem, 0);

        constexpr const char* AWSClientAuthCDKAndResourcesUrl[] =
        {
             "CDK application and resource mappings",
             "cdk_application_and_resource_mappings",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/setup/#3-deploy-the-cdk-application"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSClientAuthCDKAndResourcesUrl, 0);

        constexpr const char* AWSClientAuthScriptCanvasAndLua[] =
        {
             "Scripting reference",
             "scripting_reference",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/scripting/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSClientAuthScriptCanvasAndLua, 0);

        constexpr const char* AWSClientAuth3rdPartyAuthProvider[] =
        {
             "3rd Party developer authentication provider support",
             "3rd_party_developer_authentication_provider_support",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/authentication-providers/#using-a-custom-provider"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSClientAuth3rdPartyAuthProvider, 0);

        constexpr const char* AWSClientAuthCustomAuthProvider[] =
        {
             "Custom developer authentication provider support",
             "custom_developer_authentication_provider_support",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/authentication-providers/#using-a-custom-provider"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSClientAuthCustomAuthProvider, 0);

        constexpr const char* AWSClientAuthAPI[] =
        {
             "API reference",
             "api_reference",
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-client-auth/cpp-api/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSClientAuthAPI, 0);

    }

} // namespace AWSClientAuth
