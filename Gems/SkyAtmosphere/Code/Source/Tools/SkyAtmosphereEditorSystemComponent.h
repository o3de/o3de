/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/SkyAtmosphereSystemComponent.h>

namespace SkyAtmosphere
{
    /// System component for SkyAtmosphere editor
    class SkyAtmosphereEditorSystemComponent
        : public SkyAtmosphereSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = SkyAtmosphereSystemComponent;
    public:
        AZ_COMPONENT_DECL(SkyAtmosphereEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        SkyAtmosphereEditorSystemComponent();
        ~SkyAtmosphereEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace SkyAtmosphere
