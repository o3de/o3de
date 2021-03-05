/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <ScriptCanvas/Core/NodeBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

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
        void Init();
        void Activate();
        void Deactivate();
        ////

        // NodeDescriptorBus::Handler
        NodeDescriptorType GetType() const override { return m_nodeDescriptorType; }
        ////

        // GraphCanvas::NodeNotificationBus
        void OnAddedToScene(const AZ::EntityId& sceneId) override final;
        ////

    protected:
         
        virtual void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId);
        virtual AZ::EntityId FindScriptCanvasNodeId() const;
        
    private:
    
        NodeDescriptorType m_nodeDescriptorType;
    };
}