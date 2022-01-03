/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Variable/VariableBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace ScriptCanvasEditor
{
    class VariableNodeDescriptorComponent
        : public NodeDescriptorComponent
        , public GraphCanvas::SceneMemberNotificationBus::Handler
        , public ScriptCanvas::VariableNotificationBus::Handler
        , public ScriptCanvas::VariableNodeNotificationBus::Handler
        , public VariableNodeDescriptorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(VariableNodeDescriptorComponent, "{80CB9400-E40D-4DC7-B185-412F766C8565}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        VariableNodeDescriptorComponent() = default;
        VariableNodeDescriptorComponent(NodeDescriptorType descriptorType);
        ~VariableNodeDescriptorComponent() = default;

        // Component
        void Activate() override;
        void Deactivate() override;
        ////

        // VariableNotificationBus
        void OnVariableRenamed(AZStd::string_view variableName) override;
        void OnVariableRemoved() override;
        ////

        // VariableNodeNotificationBus
        void OnVariableIdChanged(const ScriptCanvas::VariableId& oldVariableId, const ScriptCanvas::VariableId& newVariableId) override;
        ////        

        // SceneMemberNotifications
        void OnSceneMemberAboutToSerialize(GraphCanvas::GraphSerialization& graphSerialization) override;
        void OnSceneMemberDeserialized(const AZ::EntityId& graphId, const GraphCanvas::GraphSerialization& graphSerialization) override;
        ////

        // VariableNodeDescriptorBus
        ScriptCanvas::VariableId GetVariableId() const override;
        ////
        
    protected:

        void OnAddedToGraphCanvasGraph(const AZ::EntityId& sceneId, const AZ::EntityId& scriptCanvasEntityId) override;
    
        virtual void UpdateTitle(AZStd::string_view variableName);

    private:
        
        void SetVariableId(const ScriptCanvas::VariableId& variableId);        
    };
}
