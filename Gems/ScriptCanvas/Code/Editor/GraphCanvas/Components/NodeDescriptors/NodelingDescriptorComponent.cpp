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
#include "precompiled.h"

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include "NodelingDescriptorComponent.h"

#include <ScriptCanvas/Bus/RequestBus.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////////
    // NodelingDescriptorComponent
    ////////////////////////////////

    void NodelingDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<NodelingDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }
    
    NodelingDescriptorComponent::NodelingDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::ExecutionNodeling)
    {
    }
    
    void NodelingDescriptorComponent::OnNameChanged(const AZStd::string& displayName)
    {
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, displayName);
    }
    
    void NodelingDescriptorComponent::OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);
        
        ScriptCanvas::GraphScopedNodeId scopedNodeId(scriptCanvasId, scriptCanvasNodeId);

        ScriptCanvas::NodelingNotificationBus::Handler::BusConnect(scopedNodeId);
        
        AZStd::string displayName;
        ScriptCanvas::NodelingRequestBus::EventResult(displayName, scopedNodeId, &ScriptCanvas::NodelingRequests::GetDisplayName);
        
        OnNameChanged(displayName);
    }
}
