/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/CompressionLZ4SystemComponent.h>

namespace CompressionLZ4
{
    /// System component for CompressionLZ4 editor
    class CompressionLZ4EditorSystemComponent
        : public CompressionLZ4SystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = CompressionLZ4SystemComponent;
    public:
        AZ_COMPONENT_DECL(CompressionLZ4EditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        CompressionLZ4EditorSystemComponent();
        ~CompressionLZ4EditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace CompressionLZ4
