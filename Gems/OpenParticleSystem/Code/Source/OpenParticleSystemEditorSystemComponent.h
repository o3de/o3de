/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <OpenParticleSystemSystemComponent.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <QAction>


namespace OpenParticleSystem
{
    /// System component for OpenParticleSystem editor
    class OpenParticleSystemEditorSystemComponent
        : public OpenParticleSystemSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , public AzToolsFramework::EditorMenuNotificationBus::Handler
    {
        using BaseSystemComponent = OpenParticleSystemSystemComponent;
    public:
        AZ_COMPONENT(OpenParticleSystemEditorSystemComponent, "{2EB53DE4-8A9A-4438-A773-B0F7DF0618AA}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        OpenParticleSystemEditorSystemComponent();
        ~OpenParticleSystemEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        void OnResetToolMenuItems() override;

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler...
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////
    };
} // namespace OpenParticleSystem
