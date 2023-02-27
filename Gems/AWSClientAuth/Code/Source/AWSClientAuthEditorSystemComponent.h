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

#include <AWSClientAuthSystemComponent.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace AWSClientAuth
{
    //! Gem System Component. Responsible for instantiating and managing Authentication and Authorization Controller
    class AWSClientAuthEditorSystemComponent
        : public AWSClientAuthSystemComponent
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        ~AWSClientAuthEditorSystemComponent() override = default;

        AZ_COMPONENT(AWSClientAuthEditorSystemComponent, "{4483B6C0-6D9C-425A-B6D8-21AA54561937}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&);

    protected:
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // ActionManagerRegistrationNotificationBus implementation
        void OnMenuBindingHook() override;

    };

} // namespace AWSClientAuth
