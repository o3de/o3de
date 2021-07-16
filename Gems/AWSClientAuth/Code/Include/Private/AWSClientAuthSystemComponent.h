/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Authorization/AWSCognitoAuthorizationController.h>
#include <Authentication/AuthenticationProviderManager.h>
#include <UserManagement/AWSCognitoUserManagementController.h>
#include <AWSClientAuthBus.h>
#include <AWSCoreBus.h>

namespace AWSClientAuth
{
    //! Gem System Component. Responsible for instantiating and managing Authentication and Authroziation Controller
    class AWSClientAuthSystemComponent
        : public AZ::Component
        , public AWSCore::AWSCoreNotificationsBus::Handler
        , public AWSClientAuthRequestBus::Handler
    {
    public:

        virtual ~AWSClientAuthSystemComponent() = default;

        AZ_COMPONENT(AWSClientAuthSystemComponent, "{0C2660C8-1B4A-4474-BE65-B487E2DE8649}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AWSCoreNotification interface
        void OnSDKInitialized() override;
        void OnSDKShutdownStarted() override {}

        // AWSClientAuthRequests interface
        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> GetCognitoIDPClient() override;
        std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> GetCognitoIdentityClient() override;

        AZStd::vector<ProviderNameEnum> m_enabledProviderNames;
        AZStd::unique_ptr<AuthenticationProviderManager> m_authenticationProviderManager;
        AZStd::unique_ptr<AWSCognitoUserManagementController> m_awsCognitoUserManagementController;
        AZStd::unique_ptr<AWSCognitoAuthorizationController> m_awsCognitoAuthorizationController;

        std::shared_ptr<Aws::CognitoIdentityProvider::CognitoIdentityProviderClient> m_cognitoIdentityProviderClient;
        std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient> m_cognitoIdentityClient;
    };

} // namespace AWSClientAuth
