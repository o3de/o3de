/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModelSystemComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

// Graph Model
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/DataType.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>
#include <GraphModel/Model/Module/InputOutputNodes.h>
#include <GraphModel/Model/Module/ModuleNode.h>
#include <GraphModel/Integration/GraphCanvasMetadata.h>
#include <GraphModel/Integration/NodePalette/InputOutputNodePaletteItem.h>
#include <GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h>
#include <GraphModel/Integration/NodePalette/ModuleNodePaletteItem.h>

namespace GraphModel
{
    void GraphModelSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect core graph classes
        DataType::Reflect(context);
        GraphContext::Reflect(context);
        GraphElement::Reflect(context);
        SlotId::Reflect(context);
        Slot::Reflect(context);
        Node::Reflect(context);
        Connection::Reflect(context);
        Graph::Reflect(context);

        // Reflect module node data types
        BaseInputOutputNode::Reflect(context);
        GraphInputNode::Reflect(context);
        GraphOutputNode::Reflect(context);
        ModuleNode::Reflect(context);

        // Reflect data types for integrating graph model with graph canvas
        GraphModelIntegration::GraphCanvasMetadata::Reflect(context);
        GraphModelIntegration::GraphCanvasSelectionData::Reflect(context);

        // Reflect MIME events for graph canvas nodes
        GraphModelIntegration::CreateGraphCanvasNodeMimeEvent::Reflect(context);
        GraphModelIntegration::CreateNodeGroupNodeMimeEvent::Reflect(context);
        GraphModelIntegration::CreateCommentNodeMimeEvent::Reflect(context);

        // Reflect MIME events for module node
        GraphModelIntegration::CreateInputOutputNodeMimeEvent<GraphInputNode>::Reflect(context);
        GraphModelIntegration::CreateInputOutputNodeMimeEvent<GraphOutputNode>::Reflect(context);
        GraphModelIntegration::CreateModuleNodeMimeEvent::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphModelSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphModelSystemComponent>("GraphModel", "A generic node graph data model")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GraphModelIntegration::GraphManagerRequestBus>("GraphManagerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Event("GetGraph", &GraphModelIntegration::GraphManagerRequests::GetGraph)
                ;

            behaviorContext->EBus<GraphModelIntegration::GraphControllerRequestBus>("GraphControllerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Event("AddNode", &GraphModelIntegration::GraphControllerRequests::AddNode)
                ->Event("RemoveNode", &GraphModelIntegration::GraphControllerRequests::RemoveNode)
                ->Event("GetPosition", &GraphModelIntegration::GraphControllerRequests::GetPosition)
                ->Event("WrapNode", &GraphModelIntegration::GraphControllerRequests::WrapNode)
                ->Event("WrapNodeOrdered", &GraphModelIntegration::GraphControllerRequests::WrapNodeOrdered)
                ->Event("UnwrapNode", &GraphModelIntegration::GraphControllerRequests::UnwrapNode)
                ->Event("IsNodeWrapped", &GraphModelIntegration::GraphControllerRequests::IsNodeWrapped)
                ->Event("SetWrapperNodeActionString", &GraphModelIntegration::GraphControllerRequests::SetWrapperNodeActionString)
                ->Event("AddConnection", &GraphModelIntegration::GraphControllerRequests::AddConnection)
                ->Event("AddConnectionBySlotId", &GraphModelIntegration::GraphControllerRequests::AddConnectionBySlotId)
                ->Event("AreSlotsConnected", &GraphModelIntegration::GraphControllerRequests::AreSlotsConnected)
                ->Event("RemoveConnection", &GraphModelIntegration::GraphControllerRequests::RemoveConnection)
                ->Event("ExtendSlot", &GraphModelIntegration::GraphControllerRequests::ExtendSlot)
                ->Event("GetNodeById", &GraphModelIntegration::GraphControllerRequests::GetNodeById)
                ->Event("GetNodesFromGraphNodeIds", &GraphModelIntegration::GraphControllerRequests::GetNodesFromGraphNodeIds)
                ->Event("GetNodeIdByNode", &GraphModelIntegration::GraphControllerRequests::GetNodeIdByNode)
                ->Event("GetSlotIdBySlot", &GraphModelIntegration::GraphControllerRequests::GetSlotIdBySlot)
                ->Event("GetNodes", &GraphModelIntegration::GraphControllerRequests::GetNodes)
                ->Event("GetSelectedNodes", &GraphModelIntegration::GraphControllerRequests::GetSelectedNodes)
                ->Event("SetSelected", &GraphModelIntegration::GraphControllerRequests::SetSelected)
                ->Event("ClearSelection", &GraphModelIntegration::GraphControllerRequests::ClearSelection)
                ->Event("EnableNode", &GraphModelIntegration::GraphControllerRequests::EnableNode)
                ->Event("DisableNode", &GraphModelIntegration::GraphControllerRequests::DisableNode)
                ->Event("CenterOnNodes", &GraphModelIntegration::GraphControllerRequests::CenterOnNodes)
                ->Event("GetMajorPitch", &GraphModelIntegration::GraphControllerRequests::GetMajorPitch)
            ;
        }
    }

    void GraphModelSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GraphModelService"));
    }

    void GraphModelSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GraphModelService"));
    }

    void GraphModelSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void GraphModelSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void GraphModelSystemComponent::Init()
    {
    }

    void GraphModelSystemComponent::Activate()
    {
        m_graphControllerManager = AZStd::make_unique<GraphModelIntegration::GraphControllerManager>();
    }

    void GraphModelSystemComponent::Deactivate()
    {
        m_graphControllerManager.reset();
    }
}
