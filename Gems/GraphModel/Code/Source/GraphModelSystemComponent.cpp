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
        GraphModel::Graph::Reflect(context);
        GraphModel::DataType::Reflect(context);
        GraphModelIntegration::GraphCanvasMetadata::Reflect(context);

        // Mime Events for Graph Canvas nodes
        GraphModelIntegration::CreateGraphCanvasNodeMimeEvent::Reflect(context);
        GraphModelIntegration::CreateNodeGroupNodeMimeEvent::Reflect(context);
        GraphModelIntegration::CreateCommentNodeMimeEvent::Reflect(context);

        // Reflect all the nodes needed to support ModuleNode
        GraphModel::BaseInputOutputNode::Reflect(context);
        GraphModel::GraphInputNode::Reflect(context);
        GraphModel::GraphOutputNode::Reflect(context);
        GraphModel::ModuleNode::Reflect(context);
        GraphModelIntegration::CreateInputOutputNodeMimeEvent<GraphModel::GraphInputNode>::Reflect(context);
        GraphModelIntegration::CreateInputOutputNodeMimeEvent<GraphModel::GraphOutputNode>::Reflect(context);
        GraphModelIntegration::CreateModuleNodeMimeEvent::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GraphModelSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            serialize->RegisterGenericType<GraphModel::NodePtrList>();
            serialize->RegisterGenericType<GraphModel::SlotPtrList>();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<GraphModelSystemComponent>("GraphModel", "A generic node graph data model")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<GraphModelIntegration::GraphManagerRequestBus>("GraphManagerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, GraphCanvas::EditorGraphModuleName)
                ->Event("GetGraph", &GraphModelIntegration::GraphManagerRequests::GetGraph)
                ;

            behaviorContext->EBus<GraphModelIntegration::GraphControllerRequestBus>("GraphControllerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, GraphCanvas::EditorGraphModuleName)
                ->Event("AddNode", &GraphModelIntegration::GraphControllerRequests::AddNode)
                ->Event("RemoveNode", &GraphModelIntegration::GraphControllerRequests::RemoveNode)
                ->Event("WrapNode", &GraphModelIntegration::GraphControllerRequests::WrapNode)
                ->Event("AddConnection", &GraphModelIntegration::GraphControllerRequests::AddConnection)
                ->Event("AddConnectionBySlotId", &GraphModelIntegration::GraphControllerRequests::AddConnectionBySlotId)
                ->Event("AreSlotsConnected", &GraphModelIntegration::GraphControllerRequests::AreSlotsConnected)
                ->Event("RemoveConnection", &GraphModelIntegration::GraphControllerRequests::RemoveConnection)
                ->Event("ExtendSlot", &GraphModelIntegration::GraphControllerRequests::ExtendSlot)
                ;

            behaviorContext->Class<GraphModel::SlotId>("GraphModelSlotId")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, GraphCanvas::EditorGraphModuleName)
                ->Constructor<const GraphModel::SlotName&>()
                ->Constructor<const GraphModel::SlotName&, GraphModel::SlotSubId>()
                ->Property("name", BehaviorValueProperty(&GraphModel::SlotId::m_name))
                ->Property("subId", BehaviorValueProperty(&GraphModel::SlotId::m_subId))
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
