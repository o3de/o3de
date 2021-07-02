/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <GraphCanvas/Components/SceneBus.h>


#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

namespace ScriptCanvasEditor
{
    ////////////////////////////
    // NodeDescriptorComponent
    ////////////////////////////

    void NodeDescriptorComponent::Reflect(AZ::ReflectContext* reflect)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflect))
        {
            serializeContext->Class<NodeDescriptorComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    NodeDescriptorComponent::NodeDescriptorComponent()
        : m_nodeDescriptorType(NodeDescriptorType::Unknown)
    {
    }

    NodeDescriptorComponent::NodeDescriptorComponent(NodeDescriptorType descriptorType)
        : m_nodeDescriptorType(descriptorType)
    {
    }

    void NodeDescriptorComponent::Init()
    {
    }

    void NodeDescriptorComponent::Activate()
    {
        NodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void NodeDescriptorComponent::Deactivate()
    {
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        NodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    void NodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        AZ::EntityId scriptCanvasNodeId = FindScriptCanvasNodeId();

        ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(scriptCanvasNodeId);

        OnAddedToGraphCanvasGraph(sceneId, scriptCanvasNodeId);

        bool isEnabled = true;
        ScriptCanvas::NodeRequestBus::EventResult(isEnabled, scriptCanvasNodeId, &ScriptCanvas::NodeRequests::IsNodeEnabled);

        if (!isEnabled)
        {
            GraphCanvas::GraphId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

            GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::DisableVisualState, GetEntityId());
        }
    }

    void NodeDescriptorComponent::OnAddedToGraphCanvasGraph(const GraphCanvas::GraphId& graphId, const AZ::EntityId& scriptCanvasNodeId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(scriptCanvasNodeId);
    }

    void NodeDescriptorComponent::OnNodeDisabled()
    {
        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::DisableVisualState, GetEntityId());
    }

    void NodeDescriptorComponent::OnNodeEnabled()
    {
        GraphCanvas::GraphId graphId;
        GraphCanvas::SceneMemberRequestBus::EventResult(graphId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::EnableVisualState, GetEntityId());
    }

    AZ::EntityId NodeDescriptorComponent::FindScriptCanvasNodeId() const
    {
        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);

        AZ::EntityId* scNodeId = AZStd::any_cast<AZ::EntityId>(userData);
        if (scNodeId)
        {
            return (*scNodeId);            
        }

        return AZ::EntityId();
    }
}
