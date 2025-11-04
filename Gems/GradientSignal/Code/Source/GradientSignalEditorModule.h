/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignalModule.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace GradientSignal
{
    class GradientSignalEditorModule
        : public GradientSignalModule
    {
    public:
        AZ_RTTI(GradientSignalEditorModule, "{F8AB732B-3563-4727-9326-3DF2AC42A6D8}", GradientSignalModule);
        AZ_CLASS_ALLOCATOR(GradientSignalEditorModule, AZ::SystemAllocator);

        GradientSignalEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };

    class GradientSignalEditorSystemComponent
        : public AZ::Component
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(GradientSignalEditorSystemComponent, "{A3F1E796-7C69-441C-8FA1-3A4001EF2DE3}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionContextModeBindingHook() override;

    private:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
}
