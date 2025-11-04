/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <ScriptCanvas/Core/NodeBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    class NodeDescriptorComponent
        : public AZ::Component
        , public GraphCanvas::NodeNotificationBus::Handler
        , public ScriptCanvas::NodeNotificationsBus::Handler
        , public NodeDescriptorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(NodeDescriptorComponent, "{C775A98E-D64E-457F-8ABA-B34CBAD10905}");
        static void Reflect(AZ::ReflectContext* reflect);

        NodeDescriptorComponent();
        NodeDescriptorComponent(NodeDescriptorType descriptorType);

        ~NodeDescriptorComponent() override = default;

        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // NodeDescriptorBus::Handler
        NodeDescriptorType GetType() const override { return m_nodeDescriptorType; }
        NodeDescriptorComponent* GetDescriptorComponent() override { return this; }
        ////

        // GraphCanvas::NodeNotificationBus
        void OnAddedToScene(const AZ::EntityId& sceneId) override final;
        void OnNodeDisabled() override;
        void OnNodeEnabled() override;
        ////

    protected:

        virtual void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId);
        virtual AZ::EntityId FindScriptCanvasNodeId() const;

    private:

        NodeDescriptorType m_nodeDescriptorType;
    };
}
