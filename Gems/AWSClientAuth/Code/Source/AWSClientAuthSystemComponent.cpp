/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSClientAuthSystemComponent.h>
#include <Authentication/AuthenticationProviderTypes.h>
#include <Authentication/AuthenticationNotificationBusBehaviorHandler.h>
#include <UserManagement/UserManagementNotificationBusBehaviorHandler.h>
#include <Authorization/AWSCognitoAuthorizationNotificationBusBehaviorHandler.h>
#include <Authorization/AWSCognitoAuthorizationController.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <Framework/AWSApiJobConfig.h>

#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AWSClientAuth::ProviderNameEnum, "{FB34B23A-B249-47A2-B1F1-C05284B50CCC}");
}

namespace AWSClientAuth
{
    constexpr char SerializeComponentName[] = "AWSClientAuth";

    void AWSClientAuthSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AWSClientAuthSystemComponent, AZ::Component>()->Version(2);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSClientAuthSystemComponent>("AWSClientAuth", "Provides Client Authentication and Authorization implementations")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
            AWSClientAuth::LWAProviderSetting::Reflect(*serialize);
            AWSClientAuth::GoogleProviderSetting::Reflect(*serialize);
        }

        AWSClientAuth::AuthenticationTokens::Reflect(context);
        AWSClientAuth::ClientAuthAWSCredentials::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Enum<(int)ProviderNameEnum::None>("ProviderNameEnum_None")
                ->Enum<(int)ProviderNameEnum::AWSCognitoIDP>("ProviderNameEnum_AWSCognitoIDP")
                ->Enum<(int)ProviderNameEnum::LoginWithAmazon>("ProviderNameEnum_LoginWithAmazon")
                ->Enum<(int)ProviderNameEnum::Google>("ProviderNameEnum_Google");
            behaviorContext->EBus<AuthenticationProviderScriptCanvasRequestBus>("AuthenticationProviderRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, SerializeComponentName)
                ->Event("Initialize", &AuthenticationProviderScriptCanvasRequestBus::Events::Initialize)
                ->Event("IsSignedIn", &AuthenticationProviderScriptCanvasRequestBus::Events::IsSignedIn,
                    { { { "Provider name", "The identity provider name" } } })
                ->Event("GetAuthenticationTokens", &AuthenticationProviderScriptCanvasRequestBus::Events::GetAuthenticationTokens,
                    { { { "Provider name", "The identity provider name" } } })
                ->Event(
                    "PasswordGrantSingleFactorSignInAsync",
                    &AuthenticationProviderScriptCanvasRequestBus::Events::PasswordGrantSingleFactorSignInAsync,
                    { { { "Provider name", "The identity provider" }, { "Username", "The client's username" }, { "Password", "The client's password" } } })
                ->Event(
                    "PasswordGrantMultiFactorSignInAsync",
                    &AuthenticationProviderScriptCanvasRequestBus::Events::PasswordGrantMultiFactorSignInAsync,
                    { { { "Provider name", "The identity provider name" },
                        { "Username", "The client's username" },
                        { "Password", "The client's password" } } })
                ->Event(
                    "PasswordGrantMultiFactorConfirmSignInAsync",
                    &AuthenticationProviderScriptCanvasRequestBus::Events::PasswordGrantMultiFactorConfirmSignInAsync,
                    { { { "Provider name", "The identity provider name" },
                        { "Username", "The client's username" },
                        { "Confirmation code", "The client's confirmation code" } } })
                ->Event(
                    "DeviceCodeGrantSignInAsync", &AuthenticationProviderScriptCanvasRequestBus::Events::DeviceCodeGrantSignInAsync,
                    { { { "Provider name", "The identity provider name" } } })
                ->Event(
                    "DeviceCodeGrantConfirmSignInAsync",
                    &AuthenticationProviderScriptCanvasRequestBus::Events::DeviceCodeGrantConfirmSignInAsync,
                    { { { "Provider name", "The identity provider name" } } })
                ->Event(
                    "RefreshTokensAsync", &AuthenticationProviderScriptCanvasRequestBus::Events::RefreshTokensAsync,
                    { { { "Provider name", "The identity provider name" } } })
                ->Event("GetTokensWithRefreshAsync", &AuthenticationProviderScriptCanvasRequestBus::Events::GetTokensWithRefreshAsync,
                    { { { "Provider name", "The identity provider name" } } })
                ->Event(
                    "SignOut", &AuthenticationProviderScriptCanvasRequestBus::Events::SignOut,
                    { { { "Provider name", "The identity provider name" } } });

            behaviorContext->EBus<AWSCognitoAuthorizationRequestBus>("AWSCognitoAuthorizationRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, SerializeComponentName)
                ->Event("Initialize", &AWSCognitoAuthorizationRequestBus::Events::Initialize)
                ->Event("Reset", &AWSCognitoAuthorizationRequestBus::Events::Reset)
                ->Event("GetIdentityId", &AWSCognitoAuthorizationRequestBus::Events::GetIdentityId)
                ->Event("HasPersistedLogins", &AWSCognitoAuthorizationRequestBus::Events::HasPersistedLogins)
                ->Event("RequestAWSCredentialsAsync", &AWSCognitoAuthorizationRequestBus::Events::RequestAWSCredentialsAsync);

            behaviorContext->EBus<AWSCognitoUserManagementRequestBus>("AWSCognitoUserManagementRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, SerializeComponentName)
                ->Event("Initialize", &AWSCognitoUserManagementRequestBus::Events::Initialize)
                ->Event(
                    "EmailSignUpAsync", &AWSCognitoUserManagementRequestBus::Events::EmailSignUpAsync,
                    { { { "Username", "The client's username" }, { "Password", "The client's password" }, { "Email", "The email address used to sign up" } } })
                ->Event(
                    "PhoneSignUpAsync", &AWSCognitoUserManagementRequestBus::Events::PhoneSignUpAsync,
                    { { { "Username", "The client's username" }, { "Password", "The client's password" }, { "Phone number", "The phone number used to sign up" } } })
                ->Event(
                    "ConfirmSignUpAsync", &AWSCognitoUserManagementRequestBus::Events::ConfirmSignUpAsync,
                    { { { "Username", "The client's username" }, { "Confirmation code", "The client's confirmation code" } } })
                ->Event(
                    "ForgotPasswordAsync", &AWSCognitoUserManagementRequestBus::Events::ForgotPasswordAsync,
                    { { { "Username", "The client's username" } } })
                ->Event(
                    "ConfirmForgotPasswordAsync", &AWSCognitoUserManagementRequestBus::Events::ConfirmForgotPasswordAsync,
                    { { { "Username", "The client's username" }, { "Confirmation code", "The client's confirmation code" }, { "New password", "The new password for the client" } } })
                ->Event("EnableMFAAsync", &AWSCognitoUserManagementRequestBus::Events::EnableMFAAsync, { { { "Access token", "The MFA access token" } } });


            behaviorContext->EBus<AuthenticationProviderNotificationBus>("AuthenticationProviderNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, SerializeComponentName)
                ->Handler<AuthenticationNotificationBusBehaviorHandler>();
            behaviorContext->EBus<AWSCognitoUserManagementNotificationBus>("AWSCognitoUserManagementNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, SerializeComponentName)
                ->Handler<UserManagementNotificationBusBehaviorHandler>();
            behaviorContext->EBus<AWSCognitoAuthorizationNotificationBus>("AWSCognitoAuthorizationNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, SerializeComponentName)
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
        AWSCore::AwsApiJobConfig* defaultConfig;
        AWSCore::AWSCoreRequestBus::BroadcastResult(defaultConfig, &AWSCore::AWSCoreRequests::GetDefaultConfig);
        Aws::Client::ClientConfiguration clientConfiguration =
            defaultConfig ? defaultConfig->GetClientConfiguration() : Aws::Client::ClientConfiguration();

        AZStd::string region;
        AWSCore::AWSResourceMappingRequestBus::BroadcastResult(region, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);

        clientConfiguration.region = "us-west-2";
        if (!region.empty())
        {
            clientConfiguration.region = region.c_str();
        }

        m_cognitoIdentityProviderClient =
            std::make_shared<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient>(Aws::Auth::AWSCredentials(), clientConfiguration);
        m_cognitoIdentityClient = std::make_shared<Aws::CognitoIdentity::CognitoIdentityClient>(Aws::Auth::AWSCredentials(), clientConfiguration);
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
