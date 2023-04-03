/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/GraphicsGem_AR_TestSystemComponent.h>

namespace GraphicsGem_AR_Test
{
    /// System component for GraphicsGem_AR_Test editor
    class GraphicsGem_AR_TestEditorSystemComponent
        : public GraphicsGem_AR_TestSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = GraphicsGem_AR_TestSystemComponent;
    public:
        AZ_COMPONENT_DECL(GraphicsGem_AR_TestEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        GraphicsGem_AR_TestEditorSystemComponent();
        ~GraphicsGem_AR_TestEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace GraphicsGem_AR_Test
