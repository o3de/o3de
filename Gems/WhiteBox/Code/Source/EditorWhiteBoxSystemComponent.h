/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxSystemComponent.h"
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace WhiteBox
{
    //! System component for the White Box Editor/Tool application.
    class EditorWhiteBoxSystemComponent
        : public WhiteBoxSystemComponent
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorWhiteBoxSystemComponent, "{42D40E84-A8C4-474B-A4D6-B665CCEA8A83}", WhiteBoxSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionContextModeRegistrationHook() override;
        void OnActionUpdaterRegistrationHook() override;
        void OnActionRegistrationHook() override;
        void OnActionContextModeBindingHook() override;
        void OnMenuBindingHook() override;

    private:
        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    };
} // namespace WhiteBox
