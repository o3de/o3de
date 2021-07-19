/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Core/NodelingBus.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvasEditor
{
    class NodelingDescriptorComponent
        : public NodeDescriptorComponent
        , public ScriptCanvas::NodelingNotificationBus::Handler
        , private ScriptCanvas::GraphNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(NodelingDescriptorComponent, "{9EFA1DA5-2CCB-4A6D-AA84-BD121C75773A}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        NodelingDescriptorComponent(NodeDescriptorType descriptorType = NodeDescriptorType::FunctionDefinitionNode);
        ~NodelingDescriptorComponent() override = default;

        void Deactivate() override;

        // NodelingNotificationBus
        void OnNameChanged(const AZStd::string& newName) override;
        ////

        // GraphCanvas::GraphNotificationBus
        void OnConnectionComplete(const AZ::EntityId& connectionId) override;
        void OnDisonnectionComplete(const AZ::EntityId& connectionId) override;
        void OnPreConnectionRemoved(const AZ::EntityId& connectionId) override;
        ////

    protected:
        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;

        ScriptCanvas::Slot* GetSlotFromNodeling(const AZ::EntityId& connectionId);

        AZ::Entity* m_slotEntity;
        ScriptCanvas::Slot* m_removedSlot;
        AZ::EntityId m_removedSlotId;
    };
}
