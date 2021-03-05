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

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

#include <ScriptCanvas/Core/NodelingBus.h>

namespace ScriptCanvasEditor
{
    class NodelingDescriptorComponent
        : public NodeDescriptorComponent
        , public ScriptCanvas::NodelingNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(NodelingDescriptorComponent, "{9EFA1DA5-2CCB-4A6D-AA84-BD121C75773A}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        NodelingDescriptorComponent();
        ~NodelingDescriptorComponent() = default;
        
        // NodelingNotificationBus
        void OnNameChanged(const AZStd::string& newName) override;
        ////

    protected:
        void OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId) override;
    };
}
