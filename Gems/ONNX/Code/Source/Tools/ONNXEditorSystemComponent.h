/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Clients/ONNXSystemComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace ONNX
{
    /// System component for ONNX editor
    class ONNXEditorSystemComponent
        : public ONNXSystemComponent
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = ONNXSystemComponent;
    public:
        AZ_COMPONENT(ONNXEditorSystemComponent, "{761BD9F8-5707-4104-B182-CF3A5C0C412E}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        ONNXEditorSystemComponent();
        ~ONNXEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ONNX
