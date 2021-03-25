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

#include <AWSClientAuthSystemComponent.h>
#include <Authentication/AuthenticationProviderTypes.h>
#include <Authentication/AuthenticationNotificationBusBehaviorHandler.h>
#include <UserManagement/UserManagementNotificationBusBehaviorHandler.h>
#include <Authorization/AWSCognitoAuthorizationNotificationBusBehaviorHandler.h>
#include <Authorization/AWSCognitoAuthorizationController.h>
#include <Authorization/AWSCognitoAuthorizationTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>

namespace AWSClientAuth
{
    constexpr char SERIALIZE_COMPONENT_NAME[] = "AWSClientAuth";

    void AWSClientAuthSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AWSClientAuthSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSClientAuthSystemComponent>("AWSClientAuth", "Provides Client Authentication and Authorization implementations")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
            AWSClientAuth::AWSCognitoProviderSetting::Reflect(*serialize);
            AWSClientAuth::LWAProviderSetting::Reflect(*serialize);
            AWSClientAuth::GoogleProviderSetting::Reflect(*serialize);
            AWSClientAuth::CognitoAuthorizationSettings::Reflect(*serialize);
            AWSClientAuth::AWSCognitoUserManagementSetting::Reflect(*serialize);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AuthenticationProviderRequestBus>("AuthenticationProviderRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, SERIALIZE_COMPONENT_NAME)
                ->Event("Initialize", &AuthenticationProviderRequestBus::Events::Initialize)
                ->Event("IsSignedIn", &AuthenticationProviderRequestBus::Events::IsSignedIn)
                ->Event("GetAuthenticationTokens", &AuthenticationProviderRequestBus::Events::GetAuthenticationTokens)
                ->Event(
                    "PasswordGrantSingleFactorSignInAsync", &AuthenticationProviderRequestBus::Events::PasswordGrantSingleFactorSignInAsync)
                ->Event("DeviceCodeGrantSignInAsync", &AuthenticationProviderRequestBus::Events::DeviceCodeGrantSignInAsync)
                ->Event("DeviceCodeGrantConfirmSignInAsync", &AuthenticationProviderRequestBus::Events::DeviceCodeGrantConfirmSignInAsync)
                ->Event("RefreshTokensAsync", &AuthenticationProviderRequestBus::Events::RefreshTokensAsync)
                ->Event("GetTokensWithRefreshAsync", &AuthenticationProviderRequestBus::Events::GetTokensWithRefreshAsync)
                ->Event("SignOut", &AuthenticationProviderRequestBus::Events::SignOut);

            behaviorContext->EBus<AWSCognitoAuthorizationRequestBus>("AWSCognitoAuthorizationRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, SERIALIZE_COMPONENT_NAME)
                ->Event("Initialize", &AWSCognitoAuthorizationRequestBus::Events::Initialize)
                ->Event("Reset", &AWSCognitoAuthorizationRequestBus::Events::Reset)
                ->Event("GetIdentityId", &AWSCognitoAuthorizationRequestBus::Events::GetIdentityId)
                ->Event("HasPersistedLogins", &AWSCognitoAuthorizationRequestBus::Events::HasPersistedLogins)
                ->Event("RequestAWSCredentialsAsync", &AWSCognitoAuthorizationRequestBus::Events::RequestAWSCredentialsAsync);

            behaviorContext->EBus<AWSCognitoUserManagementRequestBus>("AWSCognitoUserManagementRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, SERIALIZE_COMPONENT_NAME)
                ->Event("Initialize", &AWSCognitoUserManagementRequestBus::Events::Initialize)
                ->Event("EmailSignUpAsync", &AWSCognitoUserManagementRequestBus::Events::EmailSignUpAsync)
                ->Event("PhoneSignUpAsync", &AWSCognitoUserManagementRequestBus::Events::PhoneSignUpAsync)
                ->Event("ConfirmSignUpAsync", &AWSCognitoUserManagementRequestBus::Events::ConfirmSignUpAsync)
                ->Event("ForgotPasswordAsync", &AWSCognitoUserManagementRequestBus::Events::ForgotPasswordAsync)
                ->Event("ConfirmForgotPasswordAsync", &AWSCognitoUserManagementRequestBus::Events::ConfirmForgotPasswordAsync)
                ->Event("EnableMFAAsync", &AWSCognitoUserManagementRequestBus::Events::EnableMFAAsync);

            behaviorContext->EBus<AuthenticationProviderNotificationBus>("AuthenticationProviderNotificationBus")
                ->Handler<AuthenticationNotificationBusBehaviorHandler>();
            behaviorContext->EBus<AWSCognitoUserManagementNotificationBus>("AWSCognitoUserManagementNotificationBus")
                ->Handler<UserManagementNotificationBusBehaviorHandler>();
            behaviorContext->EBus<AWSCognitoAuthorizationNotificationBus>("AWSCognitoAuthorizationNotificationBus")
                ->Handler<AWSCognitoAuthorizationNotificationBusBehaviorHandler>();
        }
    }

    void AWSClientAuthSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSClientAuthService"));
    }

    void AWSClientAuthSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSClientAuthService"));
    }

    void AWSClientAuthSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSClientAuthSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSClientAuthSystemComponent::Init()
    {
        m_enabledProviderNames.push_back(ProviderNameEnum::AWSCognitoIDP);

        // As this Gem depends on AWSCore, AWSCoreSystemComponent gets activated before AWSClientAuth and will miss the OnSDKInitialized
        // notification if BusConnect is not in Init.
        AWSCore::AWSCoreNotificationsBus::Handler::BusConnect();
    }

    void AWSClientAuthSystemComponent::Activate()
    {
        AZ::Interface<IAWSClientAuthRequests>::Register(this);
        AWSClientAuthRequestBus::Handler::BusConnect();

        // Objects below depend on bus above.
        m_authenticationProviderManager = AZStd::make_unique<AuthenticationProviderManager>();
        m_awsCognitoUserManagementController = AZStd::make_unique<AWSCognitoUserManagementController>();
        m_awsCognitoAuthorizationController = AZStd::make_unique<AWSCognitoAuthorizationController>();

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::SetAWSClientAuthEnabled);
    }

    void AWSClientAuthSystemComponent::Deactivate()
    {
        m_authenticationProviderManager.reset();
        m_awsCognitoUserManagementController.reset();
        m_awsCognitoAuthorizationController.reset();

        AWSClientAuthRequestBus::Handler::BusDisconnect();
        AWSCore::AWSCoreNotificationsBus::Handler::BusDisconnect();
        AZ::Interface<IAWSClientAuthRequests>::Unregister(this);

        m_cognitoIdentityProviderClient.reset();
        m_cognitoIdentityClient.reset();
    }

    void AWSClientAuthSystemComponent::OnSDKInitialized()
    {
        Aws::Client::ClientConfiguration clientConfiguration;
        AZStd::string region;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(region, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);

        clientConfiguration.region = "us-west-2";
        if (!region.empty())
        {
            clientConfiguration.region = region.c_str();
        }

        m_cognitoIdentityProviderClient =
            std::make_shared<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient>(clientConfiguration);
        m_cognitoIdentityClient = std::make_shared<Aws::CognitoIdentity::CognitoIdentityClient>(clientConfiguration);
    }

    std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> AWSClientAuthSystemComponent::GetCognitoIDPClient()
    {
        return m_cognitoIdentityProviderClient;
    }

    std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> AWSClientAuthSystemComponent::GetCognitoIdentityClient()
    {
        return m_cognitoIdentityClient;
    }

} // namespace AWSClientAuth
