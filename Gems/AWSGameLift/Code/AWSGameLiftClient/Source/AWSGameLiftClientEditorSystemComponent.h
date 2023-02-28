/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AWSGameLiftClientSystemComponent.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace AWSGameLift
{
    //! Gem client system component. Responsible for creating the gamelift client manager.
    class AWSGameLiftClientEditorSystemComponent
        : public AWSGameLiftClientSystemComponent
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(AWSGameLiftClientEditorSystemComponent, "{AE1388B1-542A-4B49-8B4F-48988D78AD67}", AWSGameLiftClientSystemComponent);

        AWSGameLiftClientEditorSystemComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnMenuBindingHook() override;
    };

} // namespace AWSGameLift
