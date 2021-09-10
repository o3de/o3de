/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContextUtilities.h>

#include <Editor/Components/IconComponent.h>
#include <Editor/GraphCanvas/PropertySlotIds.h>
#include <Editor/Nodes/NodeDisplayUtils.h>

#include <GraphCanvas/Components/DynamicOrderingDynamicSlotComponent.h>
#include <GraphCanvas/Components/MappingComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/AzEventHandlerNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/ClassMethodNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/EBusHandlerEventNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/EBusHandlerNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/EBusSenderNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/FunctionNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/GetVariableNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/NodelingDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/SetVariableNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverEventNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/ScriptEventReceiverNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/ScriptEventSenderNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/UserDefinedNodeDescriptorComponent.h>
#include <GraphCanvas/Components/NodeDescriptors/FunctionDefinitionNodeDescriptorComponent.h>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>

namespace ScriptCanvasEditor::Nodes::SlotDisplayHelper
{
    AZ::EntityId DisplayPropertySlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& propertyConfiguration);
    AZ::EntityId DisplayExtendableSlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& extenderConfiguration);
    AZ::EntityId DisplayVisualExtensionSlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& extensionConfiguration);
} 

namespace ScriptCanvasEditor::Nodes
{
    // Handles the creation of a node through the node configurations for most nodes.
    AZ::EntityId DisplayGeneralScriptCanvasNode(AZ::EntityId, const ScriptCanvas::Node* node, const NodeConfiguration& nodeConfiguration)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        AZ::Entity* graphCanvasEntity = nullptr;

        if (nodeConfiguration.m_nodeType == NodeType::GeneralNode)
        {
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, nodeConfiguration.m_nodeSubStyle.c_str());
        }
        else if (nodeConfiguration.m_nodeType == NodeType::WrapperNode)
        {
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, nodeConfiguration.m_nodeSubStyle.c_str());
        }

        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

        if (graphCanvasEntity == nullptr)
        {
            return AZ::EntityId();
        }

        for (const AZ::Uuid& componentId : nodeConfiguration.m_customComponents)
        {
            graphCanvasEntity->CreateComponent(componentId);
        }

        // Apply SceneMember remapping if ScriptCanvasId is valid.
        if (nodeConfiguration.m_scriptCanvasId.IsValid())
        {
            graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(nodeConfiguration.m_scriptCanvasId);
            graphCanvasEntity->CreateComponent<SlotMappingComponent>(nodeConfiguration.m_scriptCanvasId);
        }

        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasEntity->GetId(), &GraphCanvas::NodeRequests::GetUserData);

        if (graphCanvasUserData)
        {
            *graphCanvasUserData = node->GetEntityId();
        }

        GraphCanvas::TranslationKeyedString nodeKeyedString(nodeConfiguration.m_titleFallback, nodeConfiguration.m_translationContext);
        nodeKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, TranslationItemType::Node, TranslationKeyId::Name);

        AZStd::string nodeName = nodeKeyedString.GetDisplayString();

        int paramIndex = 0;
        int outputIndex = 0;

        // Create the GraphCanvas slots
        for (const auto& slot : node->GetSlots())
        {
            if (slot.IsVisible())
            {
                AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasEntity->GetId(), slot);

                GraphCanvas::TranslationKeyedString slotNameKeyedString(slot.GetName(), nodeKeyedString.m_context);
                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(slot.GetToolTip(), nodeKeyedString.m_context);

                TranslationItemType itemType = TranslationHelper::GetItemType(slot.GetDescriptor());

                if (itemType == TranslationItemType::ParamDataSlot || itemType == TranslationItemType::ReturnDataSlot)
                {
                    int& index = (itemType == TranslationItemType::ParamDataSlot) ? paramIndex : outputIndex;

                    slotNameKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, itemType, TranslationKeyId::Name, index);
                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, itemType, TranslationKeyId::Tooltip, index);
                    index++;
                }

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);
            }
        }

        const auto& visualExtensions = node->GetVisualExtensions();

        for (const auto& extensionConfiguration : visualExtensions)
        {
            SlotDisplayHelper::DisplayVisualExtensionSlot(graphCanvasEntity->GetId(), extensionConfiguration);
        }

        GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeConfiguration.m_subtitleFallback, nodeConfiguration.m_translationContext);
        subtitleKeyedString.m_key = TranslationHelper::GetKey(nodeConfiguration.m_translationGroup, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, TranslationItemType::Node, TranslationKeyId::Category);

        graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", nodeKeyedString.GetDisplayString().c_str()));

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, subtitleKeyedString);

        if (!nodeConfiguration.m_titlePalette.empty())
        {
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeTitleRequests::SetPaletteOverride, nodeConfiguration.m_titlePalette);
        }

        // Set the name
        GraphCanvas::TranslationKeyedString tooltipKeyedString(nodeConfiguration.m_tooltipFallback, nodeConfiguration.m_translationContext);
        tooltipKeyedString.m_key = TranslationHelper::GetKey(TranslationContextGroup::ClassMethod, nodeConfiguration.m_translationKeyContext, nodeConfiguration.m_translationKeyName, TranslationItemType::Node, TranslationKeyId::Tooltip);

        GraphCanvas::NodeRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

        EditorNodeNotificationBus::Event(node->GetEntityId(), &EditorNodeNotifications::OnGraphCanvasNodeDisplayed, graphCanvasEntity->GetId());

        return graphCanvasEntity->GetId();
    }

    AZ::EntityId DisplayNode(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Node* node, StyleConfiguration styleConfiguration = StyleConfiguration())
    {
        NodeConfiguration nodeConfiguration;

        nodeConfiguration.PopulateComponentDescriptors<IconComponent, UserDefinedNodeDescriptorComponent>();

        if (node->RequiresDynamicSlotOrdering())
        {
            nodeConfiguration.PopulateComponentDescriptors<DynamicOrderingDynamicSlotComponent>();
        }
        else
        {
            nodeConfiguration.PopulateComponentDescriptors<DynamicSlotComponent>();
        }

        nodeConfiguration.m_nodeSubStyle = styleConfiguration.m_nodeSubStyle;
        nodeConfiguration.m_titlePalette = styleConfiguration.m_titlePalette;
        nodeConfiguration.m_scriptCanvasId = node->GetEntityId();

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(azrtti_typeid(node));

        if (classData)
        {
            AZStd::string nodeContext = GetContextName(*classData);
            nodeConfiguration.m_translationContext = TranslationHelper::GetUserDefinedContext(nodeContext);

            nodeConfiguration.m_titleFallback = (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name;
            nodeConfiguration.m_tooltipFallback = (classData->m_editData && classData->m_editData->m_description) ? classData->m_editData->m_description : "";

            GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeContext, nodeConfiguration.m_translationContext);
            subtitleKeyedString.m_key = TranslationHelper::GetUserDefinedNodeKey(nodeContext, nodeConfiguration.m_titleFallback, ScriptCanvasEditor::TranslationKeyId::Category);

            nodeConfiguration.m_subtitleFallback = subtitleKeyedString.GetDisplayString();

            nodeConfiguration.m_translationKeyName = nodeConfiguration.m_titleFallback;
            nodeConfiguration.m_translationKeyContext = nodeContext;

            nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;

            if (classData->m_editData)
            {
                const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);

                if (elementData)
                {
                    if (auto nodeTypeAttribute = elementData->FindAttribute(ScriptCanvas::Attributes::Node::NodeType))
                    {
                        if (auto nodeTypeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<NodeType>*>(nodeTypeAttribute))
                        {
                            nodeConfiguration.m_nodeType = nodeTypeAttributeData->Get(nullptr);
                        }
                    }
                }
            }
        }

        return DisplayGeneralScriptCanvasNode(graphCanvasGraphId, node, nodeConfiguration);
    }

    static void ConfigureGeneralScriptCanvasEntity(const ScriptCanvas::Node* node, AZ::Entity* graphCanvasEntity, const GraphCanvas::SlotGroup& slotGroup = GraphCanvas::SlotGroups::Invalid)
    {
        if (node->RequiresDynamicSlotOrdering())
        {
            graphCanvasEntity->CreateComponent<DynamicOrderingDynamicSlotComponent>(slotGroup);
        }
        else
        {
            graphCanvasEntity->CreateComponent<DynamicSlotComponent>(slotGroup);
        }
    }

    AZ::EntityId DisplayMethodNode(AZ::EntityId, const ScriptCanvas::Nodes::Core::Method* methodNode)
    {
        AZ::EntityId graphCanvasNodeId;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".method");
        AZ_Error("GraphCanvas", graphCanvasEntity, "Unable to create GraphCanvas Node");

        if (graphCanvasEntity)
        {
            graphCanvasNodeId = graphCanvasEntity->GetId();
        }

        // Add the icon component
        ConfigureGeneralScriptCanvasEntity(methodNode, graphCanvasEntity);
        graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::Method::RTTI_Type());
        graphCanvasEntity->CreateComponent<SlotMappingComponent>(methodNode->GetEntityId());
        graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(methodNode->GetEntityId());

        TranslationContextGroup contextGroup = TranslationContextGroup::Invalid;

        switch (methodNode->GetMethodType())
        {
        case ScriptCanvas::MethodType::Event:
            graphCanvasEntity->CreateComponent<EBusSenderNodeDescriptorComponent>();
            contextGroup = TranslationContextGroup::EbusSender;
            break;
        case ScriptCanvas::MethodType::Member:
        case ScriptCanvas::MethodType::Getter:
        case ScriptCanvas::MethodType::Setter:
        case ScriptCanvas::MethodType::Free:
            graphCanvasEntity->CreateComponent<ClassMethodNodeDescriptorComponent>();
            contextGroup = TranslationContextGroup::ClassMethod;
            break;
        default:
            AZ_Error("ScriptCanvas", false, "Invalid method node type, node creation failed. This node needs to be deleted.");
            break;
        }

        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

        if (graphCanvasUserData)
        {
            *graphCanvasUserData = methodNode->GetEntityId();
        }

        const AZStd::string& className = methodNode->GetMethodClassName();
        const AZStd::string& methodName = methodNode->GetName();

        AZStd::string translationContext = TranslationHelper::GetContextName(contextGroup, className);

        GraphCanvas::TranslationKeyedString nodeKeyedString(methodName, translationContext);
        nodeKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, TranslationItemType::Node, TranslationKeyId::Name);

        GraphCanvas::TranslationKeyedString classKeyedString(className, translationContext);
        classKeyedString.m_key = TranslationHelper::GetClassKey(contextGroup, className, TranslationKeyId::Name);

        GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), translationContext);
        tooltipKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, TranslationItemType::Node, TranslationKeyId::Tooltip);

        int paramIndex = 0;
        int outputIndex = 0;

        auto busId = methodNode->GetBusSlotId();
        for (const auto& slot : methodNode->GetSlots())
        {
            if (slot.IsVisible())
            {
                AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, slot);

                GraphCanvas::TranslationKeyedString slotNameKeyedString(slot.GetName(), translationContext);
                GraphCanvas::TranslationKeyedString slotTooltipKeyedString(slot.GetToolTip(), translationContext);

                if (methodNode->HasBusID() && busId == slot.GetId() && slot.GetDescriptor() == ScriptCanvas::SlotDescriptors::DataIn())
                {
                    slotNameKeyedString = TranslationHelper::GetEBusSenderBusIdNameKey();
                    slotTooltipKeyedString = TranslationHelper::GetEBusSenderBusIdTooltipKey();
                }
                else
                {
                    TranslationItemType itemType = TranslationHelper::GetItemType(slot.GetDescriptor());

                    int& index = (itemType == TranslationItemType::ParamDataSlot) ? paramIndex : outputIndex;

                    slotNameKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, itemType, TranslationKeyId::Name, index);
                    slotTooltipKeyedString.m_key = TranslationHelper::GetKey(contextGroup, className, methodName, itemType, TranslationKeyId::Tooltip, index);

                    if ((itemType == TranslationItemType::ParamDataSlot) || (itemType == TranslationItemType::ReturnDataSlot))
                    {
                        index++;
                    }
                }

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotNameKeyedString);
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTooltipKeyedString);

                CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), graphCanvasSlotId);
            }
        }

        // Set the name
        AZStd::string displayName = methodNode->GetName();
        graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", displayName.c_str()));

        GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, classKeyedString);
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "MethodNodeTitlePalette");

        // Override the title if it has the Setter or Getter suffixes
        AZStd::string title;
        GraphCanvas::NodeTitleRequestBus::EventResult(title, graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::GetTitle);
        if (!title.empty())
        {
            AZ::RemovePropertyNameArtifacts(title);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTitle, title);

        }

        return graphCanvasNodeId;
    }
        
    AZ::EntityId DisplayEbusWrapperNode(AZ::EntityId, const ScriptCanvas::Nodes::Core::EBusEventHandler* busNode)
    {
        AZ::Entity* graphCanvasEntity = nullptr;

        AZStd::string busName = busNode->GetEBusName();

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, "");
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

        AZ::EntityId graphCanvasNodeId = graphCanvasEntity->GetId();

        // Add the icon component
        graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type());
        graphCanvasEntity->CreateComponent<EBusHandlerNodeDescriptorComponent>(busName);
        graphCanvasEntity->CreateComponent<SlotMappingComponent>(busNode->GetEntityId());
        graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(busNode->GetEntityId());
        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
        if (graphCanvasUserData)
        {
            *graphCanvasUserData = busNode->GetEntityId();
        }

        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::EBusConnectionSlotGroup, GraphCanvas::SlotGroupConfiguration(0));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(1));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(2));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::SetDividersEnabled, false);

        AZStd::vector< ScriptCanvas::SlotId > scriptCanvasSlots = busNode->GetNonEventSlotIds();

        for (const auto& slotId : scriptCanvasSlots)
        {
            ScriptCanvas::Slot* slot = busNode->GetSlot(slotId);

            GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid;

            if (slot->GetDescriptor().IsExecution())
            {
                group = SlotGroups::EBusConnectionSlotGroup;
            }

            if (slot->IsVisible())
            {
                AZ::EntityId gcSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, (*slot), group);

                if (busNode->IsIDRequired() && slot->GetDescriptor() == ScriptCanvas::SlotDescriptors::DataIn())
                {
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerBusIdNameKey());
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerBusIdTooltipKey());
                }
            }
        }

        GraphCanvas::TranslationKeyedString nodeKeyedString(busName);
        nodeKeyedString.m_context = TranslationHelper::GetEbusHandlerContext(busName);
        nodeKeyedString.m_key = TranslationHelper::GetEbusHandlerKey(busName, TranslationKeyId::Name);

        GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
        tooltipKeyedString.m_key = TranslationHelper::GetEbusHandlerKey(busName, TranslationKeyId::Tooltip);

        // Set the name
        graphCanvasEntity->SetName(AZStd::string::format("GC-BusNode: %s", busName.data()));

        GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetDefaultPalette, "HandlerWrapperNodeTitlePalette");

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplayEbusEventNode(AZ::EntityId, const AZStd::string& busName, const AZStd::string& eventName, const ScriptCanvas::EBusEventId& eventId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        AZ::EntityId graphCanvasNodeId;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".handler");
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
        graphCanvasNodeId = graphCanvasEntity->GetId();

        graphCanvasEntity->CreateComponent<EBusHandlerEventNodeDescriptorComponent>(busName, eventName, eventId);
        graphCanvasEntity->CreateComponent<SlotMappingComponent>();

        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        AZStd::string decoratedName = AZStd::string::format("%s::%s", busName.c_str(), eventName.c_str());

        GraphCanvas::TranslationKeyedString nodeKeyedString(eventName);
        nodeKeyedString.m_context = TranslationHelper::GetEbusHandlerContext(busName);
        nodeKeyedString.m_key = TranslationHelper::GetEbusHandlerEventKey(busName, eventName, TranslationKeyId::Name);

        GraphCanvas::TranslationKeyedString tooltipKeyedString(AZStd::string(), nodeKeyedString.m_context);
        tooltipKeyedString.m_key = TranslationHelper::GetEbusHandlerEventKey(busName, eventName, TranslationKeyId::Tooltip);

        // Set the name
        graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", decoratedName.c_str()));

        GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, tooltipKeyedString);

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeKeyedString);
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "HandlerNodeTitlePalette");

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplayAzEventHandlerNode(AZ::EntityId, const ScriptCanvas::Nodes::Core::AzEventHandler* azEventNode)
    {
        AZ::Entity* graphCanvasEntity{};
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".azeventhandler");
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

        AZ::EntityId graphCanvasNodeId = graphCanvasEntity->GetId();

        const ScriptCanvas::Nodes::Core::AzEventEntry& azEventEntry{ azEventNode->GetEventEntry() };
        // Add the icon component
        graphCanvasEntity->CreateComponent<IconComponent>(azrtti_typeid<ScriptCanvas::Nodes::Core::AzEventHandler>());
        graphCanvasEntity->CreateComponent<AzEventHandlerNodeDescriptorComponent>(azEventEntry.m_eventName);
        graphCanvasEntity->CreateComponent<SlotMappingComponent>(azEventNode->GetEntityId());
        graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(azEventNode->GetEntityId());
        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = {};
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
        if (graphCanvasUserData)
        {
            *graphCanvasUserData = azEventNode->GetEntityId();
        }

        for (const ScriptCanvas::Slot& slot: azEventNode->GetSlots())
        {
            GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid;

            if (slot.IsVisible())
            {
                AZ::EntityId gcSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, slot, group);
                if (slot.GetId() == azEventEntry.m_azEventInputSlotId)
                {
                    GraphCanvas::TranslationKeyedString slotTranslationEntry(azEventEntry.m_eventName);
                    slotTranslationEntry.m_context = TranslationHelper::GetAzEventHandlerContextKey();
                    // The translation key in this case acts like a json pointer referencing a particular
                    // json string within a hypothetical json document
                    AZ::StackedString azEventHandlerNodeKey = TranslationHelper::GetAzEventHandlerRootPointer(azEventEntry.m_eventName);

                    azEventHandlerNodeKey.Push("Name");
                    slotTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotTranslationEntry);
                    azEventHandlerNodeKey.Pop();
                    azEventHandlerNodeKey.Push("Tooltip");
                    slotTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTranslationEntry);
                }
                else
                {
                    GraphCanvas::TranslationKeyedString slotTranslationEntry(slot.GetName());
                    slotTranslationEntry.m_context = TranslationHelper::GetAzEventHandlerContextKey();
                    // The translation key in this case acts like a json pointer referencing a particular
                    // json string within a hypothetical json document
                    // translation key is rooted at /AzEventHandler/${EventName}/Slots/${SlotName}/{In,Out,Param,Return}
                    AZ::StackedString azEventHandlerNodeKey = TranslationHelper::GetAzEventHandlerRootPointer(azEventEntry.m_eventName);
                    azEventHandlerNodeKey.Push("Slots");
                    azEventHandlerNodeKey.Push(slot.GetName());
                    switch(TranslationHelper::GetItemType(slot.GetDescriptor()))
                    {
                    case TranslationItemType::ExecutionInSlot:
                        azEventHandlerNodeKey.Push("In");
                        break;
                    case TranslationItemType::ExecutionOutSlot:
                        azEventHandlerNodeKey.Push("Out");
                        break;
                    case TranslationItemType::ParamDataSlot:
                        azEventHandlerNodeKey.Push("Param");
                        break;
                    case TranslationItemType::ReturnDataSlot:
                        azEventHandlerNodeKey.Push("Return");
                        break;
                    default:
                        // Slot is not an execution or data slot, do nothing
                        break;
                    }

                    azEventHandlerNodeKey.Push("Name");
                    slotTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, slotTranslationEntry);
                    azEventHandlerNodeKey.Pop();
                    azEventHandlerNodeKey.Push("Tooltip");
                    slotTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, slotTranslationEntry);
                }
            }
        }

        GraphCanvas::TranslationKeyedString nodeTranslationEntry(azEventEntry.m_eventName);
        nodeTranslationEntry.m_context = TranslationHelper::GetAzEventHandlerContextKey();
        // The translation key in this case acts like a json pointer referencing a particular
        // json string within a hypothetical json document
        AZ::StackedString azEventHandlerNodeKey = TranslationHelper::GetAzEventHandlerRootPointer(azEventEntry.m_eventName);
        azEventHandlerNodeKey.Push("Name");
        nodeTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, nodeTranslationEntry);
        azEventHandlerNodeKey.Pop();
        azEventHandlerNodeKey.Push("Tooltip");
        nodeTranslationEntry.m_key = AZStd::string_view{ azEventHandlerNodeKey };
        GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::SetTranslationKeyedTooltip, nodeTranslationEntry);

        // Set the name
        graphCanvasEntity->SetName(AZStd::string::format("GC-EventNode: %s", azEventEntry.m_eventName.c_str()));

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "HandlerNodeTitlePalette");

        return graphCanvasNodeId;
    }
        
    AZ::EntityId DisplayScriptEventWrapperNode(AZ::EntityId, const ScriptCanvas::Nodes::Core::ReceiveScriptEvent* busNode)
    {
        AZ::Entity* graphCanvasEntity = nullptr;

        const AZ::Data::AssetId assetId = busNode->GetAssetId();

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateWrapperNode, "");
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");

        AZ::EntityId graphCanvasNodeId = graphCanvasEntity->GetId();

        // Add the icon component
        ConfigureGeneralScriptCanvasEntity(busNode, graphCanvasEntity);
        graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::ReceiveScriptEvent::RTTI_Type());
        graphCanvasEntity->CreateComponent<ScriptEventReceiverNodeDescriptorComponent>(assetId);
        graphCanvasEntity->CreateComponent<SlotMappingComponent>(busNode->GetEntityId());
        graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(busNode->GetEntityId());
        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);
        if (graphCanvasUserData)
        {
            *graphCanvasUserData = busNode->GetEntityId();
        }

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();
        if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
        {
            return graphCanvasNodeId;
        }

        const ScriptEvents::ScriptEvent& definition = asset.Get()->m_definition;

        AZStd::string busName = definition.GetName();

        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, SlotGroups::EBusConnectionSlotGroup, GraphCanvas::SlotGroupConfiguration(0));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(1));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(2));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::SetDividersEnabled, false);

        AZStd::vector< ScriptCanvas::SlotId > scriptCanvasSlots = busNode->GetNonEventSlotIds();

        for (const auto& slotId : scriptCanvasSlots)
        {
            ScriptCanvas::Slot* slot = busNode->GetSlot(slotId);

            GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid;

            if (slot->GetDescriptor().IsExecution())
            {
                group = SlotGroups::EBusConnectionSlotGroup;
            }

            if (slot->IsVisible())
            {
                AZ::EntityId gcSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, (*slot), group);

                if (busNode->IsIDRequired() && slot->GetDescriptor() == ScriptCanvas::SlotDescriptors::DataIn())
                {
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedName, TranslationHelper::GetEBusHandlerBusIdNameKey());
                    GraphCanvas::SlotRequestBus::Event(gcSlotId, &GraphCanvas::SlotRequests::SetTranslationKeyedTooltip, TranslationHelper::GetEBusHandlerBusIdTooltipKey());
                }
            }
        }

        // Set the name
        graphCanvasEntity->SetName(AZStd::string::format("GC-BusNode: %s", busName.data()));

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetDefaultPalette, "HandlerWrapperNodeTitlePalette");

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplayScriptEventNode(AZ::EntityId, const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        AZ::EntityId graphCanvasNodeId;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".handler");
        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Node");
        graphCanvasNodeId = graphCanvasEntity->GetId();

        graphCanvasEntity->CreateComponent<ScriptEventReceiverEventNodeDescriptorComponent>(assetId, methodDefinition);
        graphCanvasEntity->CreateComponent<SlotMappingComponent>();
        graphCanvasEntity->CreateComponent<DynamicSlotComponent>();

        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        AZStd::string eventName = methodDefinition.GetName();

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();

        const AZStd::string& busName = asset.Get()->m_definition.GetName();
        AZStd::string decoratedName = AZStd::string::format("%s::%s", busName.data(), eventName.c_str());

        // Set the name
        graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", decoratedName.c_str()));

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "HandlerNodeTitlePalette");

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplayScriptEventSenderNode(AZ::EntityId, const ScriptCanvas::Nodes::Core::SendScriptEvent* senderNode)
    {
        AZ::EntityId graphCanvasNodeId;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".method");
        AZ_Error("GraphCanvas", graphCanvasEntity, "Unable to create GraphCanvas Node");

        if (graphCanvasEntity)
        {
            graphCanvasNodeId = graphCanvasEntity->GetId();
        }

        // Add the icon component
        ConfigureGeneralScriptCanvasEntity(senderNode, graphCanvasEntity);
        graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::Method::RTTI_Type());
        graphCanvasEntity->CreateComponent<SlotMappingComponent>(senderNode->GetEntityId());
        graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(senderNode->GetEntityId());

        TranslationContextGroup contextGroup = TranslationContextGroup::Invalid;

        graphCanvasEntity->CreateComponent<ScriptEventSenderNodeDescriptorComponent>(senderNode->GetAssetId(), senderNode->GetEventId());
        contextGroup = TranslationContextGroup::EbusSender;

        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

        if (graphCanvasUserData)
        {
            *graphCanvasUserData = senderNode->GetEntityId();
        }

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(senderNode->GetAssetId(), AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();

        if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
        {
            return graphCanvasNodeId;
        }

        for (const auto& slot : senderNode->GetSlots())
        {
            if (slot.IsVisible())
            {
                AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, slot);

                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetName, slot.GetName());
                GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTooltip, slot.GetToolTip());

                CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), graphCanvasSlotId);
            }
        }

        // Set the name
        AZStd::string displayName = senderNode->GetEventName();
        graphCanvasEntity->SetName(AZStd::string::format("GC-Node(%s)", displayName.c_str()));

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "MethodNodeTitlePalette");

        return graphCanvasNodeId;
    }

// Function Nodes
    AZ::EntityId DisplayFunctionNode(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Nodes::Core::FunctionCallNode* functionNode)
    {
        return DisplayFunctionNode(graphCanvasGraphId, const_cast<ScriptCanvas::Nodes::Core::FunctionCallNode*>(functionNode));
    }

    AZ::EntityId DisplayFunctionNode(AZ::EntityId, ScriptCanvas::Nodes::Core::FunctionCallNode* functionNode)
    {
        AZ::EntityId graphCanvasNodeId;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateGeneralNode, ".method");
        AZ_Error("GraphCanvas", graphCanvasEntity, "Unable to create GraphCanvas Node");

        if (graphCanvasEntity)
        {
            graphCanvasNodeId = graphCanvasEntity->GetId();
        }

        auto asset = functionNode->GetAsset();

        // Add the icon component
        ConfigureGeneralScriptCanvasEntity(functionNode, graphCanvasEntity);

        graphCanvasEntity->CreateComponent<IconComponent>(ScriptCanvas::Nodes::Core::Method::RTTI_Type());
        graphCanvasEntity->CreateComponent<SlotMappingComponent>(functionNode->GetEntityId());
        graphCanvasEntity->CreateComponent<SceneMemberMappingComponent>(functionNode->GetEntityId());
        graphCanvasEntity->CreateComponent<FunctionNodeDescriptorComponent>(functionNode->GetAssetId(), functionNode->GetName());

        graphCanvasEntity->Init();
        graphCanvasEntity->Activate();

        // Set the user data on the GraphCanvas node to be the EntityId of the ScriptCanvas node
        AZStd::any* graphCanvasUserData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(graphCanvasUserData, graphCanvasNodeId, &GraphCanvas::NodeRequests::GetUserData);

        if (graphCanvasUserData)
        {
            *graphCanvasUserData = functionNode->GetEntityId();
        }

        if (asset->GetStatus() == AZ::Data::AssetData::AssetStatus::Error)
        {
            AZ_Error("Script Canvas", false, "Script Canvas Function asset (%s) is not loaded, unable to display the node.", functionNode->GetAssetId().ToString<AZStd::string>().c_str());

            GraphCanvas::TranslationKeyedString errorTitle("ERROR!");
            GraphCanvas::TranslationKeyedString errorSubstring("Missing Script Canvas Function Asset!");

            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedTitle, errorTitle);
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTranslationKeyedSubTitle, errorSubstring);

            return graphCanvasNodeId;
        }

        for (const auto& slot : functionNode->GetSlots())
        {
            AZ::EntityId graphCanvasSlotId = DisplayScriptCanvasSlot(graphCanvasNodeId, slot);

            GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetName, slot.GetName());
            GraphCanvas::SlotRequestBus::Event(graphCanvasSlotId, &GraphCanvas::SlotRequests::SetTooltip, slot.GetToolTip());

            CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), graphCanvasSlotId);
        }

        if (asset)
        {
            GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetTitle, asset->GetData().m_name);
        }

        GraphCanvas::NodeTitleRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, "MethodNodeTitlePalette");

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplayFunctionDefinitionNode(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Nodes::Core::FunctionDefinitionNode* functionDefinitionNode)
    {
        NodeConfiguration nodeConfiguration;

        nodeConfiguration.PopulateComponentDescriptors<IconComponent, FunctionDefinitionNodeDescriptorComponent>();

        if (functionDefinitionNode->RequiresDynamicSlotOrdering())
        {
            nodeConfiguration.PopulateComponentDescriptors<DynamicOrderingDynamicSlotComponent>();
        }
        else
        {
            nodeConfiguration.PopulateComponentDescriptors<DynamicSlotComponent>();
        }

        nodeConfiguration.m_nodeSubStyle = ".nodeling";
        nodeConfiguration.m_titlePalette = "NodelingTitlePalette";
        nodeConfiguration.m_scriptCanvasId = functionDefinitionNode->GetEntityId();

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

        if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(azrtti_typeid(functionDefinitionNode)))
        {
            AZStd::string nodeContext = GetContextName(*classData);
            nodeConfiguration.m_translationContext = TranslationHelper::GetUserDefinedContext(nodeContext);

            nodeConfiguration.m_titleFallback = (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name;
            nodeConfiguration.m_tooltipFallback = (classData->m_editData && classData->m_editData->m_description) ? classData->m_editData->m_description : "";

            GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeContext, nodeConfiguration.m_translationContext);
            subtitleKeyedString.m_key = TranslationHelper::GetUserDefinedNodeKey(nodeContext, nodeConfiguration.m_titleFallback, ScriptCanvasEditor::TranslationKeyId::Category);

            nodeConfiguration.m_subtitleFallback = subtitleKeyedString.GetDisplayString();

            nodeConfiguration.m_translationKeyName = nodeConfiguration.m_titleFallback;
            nodeConfiguration.m_translationKeyContext = nodeContext;

            nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;


            ScriptCanvas::GraphScopedNodeId nodelingId;

            nodelingId.m_identifier = nodeConfiguration.m_scriptCanvasId;
            nodelingId.m_scriptCanvasId = functionDefinitionNode->GetOwningScriptCanvasId();

            AZStd::string nodelingName;
            ScriptCanvas::NodelingRequestBus::EventResult(nodelingName, nodelingId, &ScriptCanvas::NodelingRequests::GetDisplayName);

            if (classData->m_editData)
            {
                const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);

                if (elementData)
                {
                    if (auto nodeTypeAttribute = elementData->FindAttribute(ScriptCanvas::Attributes::Node::NodeType))
                    {
                        if (auto nodeTypeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<NodeType>*>(nodeTypeAttribute))
                        {
                            nodeConfiguration.m_nodeType = nodeTypeAttributeData->Get(nullptr);
                        }
                    }
                }
            }
        }

        nodeConfiguration.m_subtitleFallback = "";

        // Because of how the extender slots are registered, there isn't an easy way to only create one or the other based on
        // the type of nodeling, so instead they both get created and we need to remove the inapplicable one
        GraphCanvas::ConnectionType typeToRemove = (functionDefinitionNode->IsExecutionEntry()) ? GraphCanvas::CT_Input : GraphCanvas::CT_Output;

        AZ::EntityId graphCanvasNodeId = DisplayGeneralScriptCanvasNode(graphCanvasGraphId, functionDefinitionNode, nodeConfiguration);


        AZStd::vector<GraphCanvas::SlotId> extenderSlotIds, executionSlotIds;
        GraphCanvas::NodeRequestBus::EventResult(extenderSlotIds, graphCanvasNodeId, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, typeToRemove, GraphCanvas::SlotTypes::ExtenderSlot);
        if (!extenderSlotIds.empty())
        {
            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::RemoveSlot, *extenderSlotIds.begin());
        }

        GraphCanvas::NodeRequestBus::EventResult(executionSlotIds, graphCanvasNodeId, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, typeToRemove, GraphCanvas::SlotTypes::ExecutionSlot);
        if (!executionSlotIds.empty())
        {
            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::RemoveSlot, *executionSlotIds.begin());
        }

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplayNodeling(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Nodes::Core::Internal::Nodeling* nodeling)
    {
        NodeConfiguration nodeConfiguration;

        nodeConfiguration.PopulateComponentDescriptors<IconComponent, NodelingDescriptorComponent>();

        if (nodeling->RequiresDynamicSlotOrdering())
        {
            nodeConfiguration.PopulateComponentDescriptors<DynamicOrderingDynamicSlotComponent>();
        }
        else
        {
            nodeConfiguration.PopulateComponentDescriptors<DynamicSlotComponent>();
        }

        nodeConfiguration.m_nodeSubStyle = ".nodeling";
        nodeConfiguration.m_titlePalette = "NodelingTitlePalette";
        nodeConfiguration.m_scriptCanvasId = nodeling->GetEntityId();

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(azrtti_typeid(nodeling));

        if (classData)
        {
            AZStd::string nodeContext = GetContextName(*classData);
            nodeConfiguration.m_translationContext = TranslationHelper::GetUserDefinedContext(nodeContext);

            nodeConfiguration.m_titleFallback = (classData->m_editData && classData->m_editData->m_name) ? classData->m_editData->m_name : classData->m_name;
            nodeConfiguration.m_tooltipFallback = (classData->m_editData && classData->m_editData->m_description) ? classData->m_editData->m_description : "";

            GraphCanvas::TranslationKeyedString subtitleKeyedString(nodeContext, nodeConfiguration.m_translationContext);
            subtitleKeyedString.m_key = TranslationHelper::GetUserDefinedNodeKey(nodeContext, nodeConfiguration.m_titleFallback, ScriptCanvasEditor::TranslationKeyId::Category);

            nodeConfiguration.m_subtitleFallback = subtitleKeyedString.GetDisplayString();

            nodeConfiguration.m_translationKeyName = nodeConfiguration.m_titleFallback;
            nodeConfiguration.m_translationKeyContext = nodeContext;

            nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;

            if (classData->m_editData)
            {
                const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);

                if (elementData)
                {
                    if (auto nodeTypeAttribute = elementData->FindAttribute(ScriptCanvas::Attributes::Node::NodeType))
                    {
                        if (auto nodeTypeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<NodeType>*>(nodeTypeAttribute))
                        {
                            nodeConfiguration.m_nodeType = nodeTypeAttributeData->Get(nullptr);
                        }
                    }
                }
            }
        }

        nodeConfiguration.m_subtitleFallback = "";

        return DisplayGeneralScriptCanvasNode(graphCanvasGraphId, nodeling, nodeConfiguration);
    }

    AZ::EntityId DisplayGetVariableNode(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Nodes::Core::GetVariableNode* variableNode)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        NodeConfiguration nodeConfiguration;
        nodeConfiguration.PopulateComponentDescriptors<IconComponent, DynamicSlotComponent, GetVariableNodeDescriptorComponent>();
        nodeConfiguration.m_nodeSubStyle = ".getVariable";
        nodeConfiguration.m_titlePalette = "GetVariableNodeTitlePalette";
        nodeConfiguration.m_scriptCanvasId = variableNode->GetEntityId();

        // <Translation>
        nodeConfiguration.m_translationContext = TranslationHelper::GetContextName(TranslationContextGroup::ClassMethod, "CORE");

        nodeConfiguration.m_translationKeyContext = "CORE";
        nodeConfiguration.m_translationKeyName = "GETVARIABLE";

        nodeConfiguration.m_titleFallback = "Get Variable";
        nodeConfiguration.m_subtitleFallback = "";
        nodeConfiguration.m_tooltipFallback = "Gets the specified Variable or one of it's properties.";

        nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;
        // </Translation>

        AZ::EntityId graphCanvasNodeId = DisplayGeneralScriptCanvasNode(graphCanvasGraphId, variableNode, nodeConfiguration);

        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(0));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(1));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(2));

        return graphCanvasNodeId;
    }

    AZ::EntityId DisplaySetVariableNode(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Nodes::Core::SetVariableNode* variableNode)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        NodeConfiguration nodeConfiguration;
        nodeConfiguration.PopulateComponentDescriptors<IconComponent, DynamicSlotComponent, SetVariableNodeDescriptorComponent>();
        nodeConfiguration.m_nodeSubStyle = ".setVariable";
        nodeConfiguration.m_titlePalette = "SetVariableNodeTitlePalette";
        nodeConfiguration.m_scriptCanvasId = variableNode->GetEntityId();

        // <Translation>

        nodeConfiguration.m_translationContext = TranslationHelper::GetContextName(TranslationContextGroup::ClassMethod, "CORE");

        nodeConfiguration.m_translationKeyContext = "CORE";
        nodeConfiguration.m_translationKeyName = "SETVARIABLE";

        nodeConfiguration.m_titleFallback = "Set Variable";
        nodeConfiguration.m_subtitleFallback = "";
        nodeConfiguration.m_tooltipFallback = "Sets the specified Variable.";

        nodeConfiguration.m_translationGroup = TranslationContextGroup::ClassMethod;
        // </Translation>

        AZ::EntityId graphCanvasId = DisplayGeneralScriptCanvasNode(graphCanvasGraphId, variableNode, nodeConfiguration);

        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::ExecutionGroup, GraphCanvas::SlotGroupConfiguration(0));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::PropertyGroup, GraphCanvas::SlotGroupConfiguration(1));
        GraphCanvas::SlotLayoutRequestBus::Event(graphCanvasId, &GraphCanvas::SlotLayoutRequests::ConfigureSlotGroup, GraphCanvas::SlotGroups::DataGroup, GraphCanvas::SlotGroupConfiguration(2));

        return graphCanvasId;
    }

///////////////////
// Header Methods
///////////////////
    AZ::EntityId DisplayScriptCanvasNode(AZ::EntityId graphCanvasGraphId, const ScriptCanvas::Node* node)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        AZ::EntityId graphCanvasNodeId;

        if (azrtti_istypeof<ScriptCanvas::Nodes::Core::SetVariableNode>(node))
        {
            graphCanvasNodeId = DisplaySetVariableNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::SetVariableNode*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::GetVariableNode>(node))
        {
            graphCanvasNodeId = DisplayGetVariableNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::GetVariableNode*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::Method>(node))
        {
            graphCanvasNodeId = DisplayMethodNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::Method*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::EBusEventHandler>(node))
        {
            graphCanvasNodeId = DisplayEbusWrapperNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::EBusEventHandler*>(node));
        }
        else if (auto azEventHandlerNode{ azrtti_cast<const ScriptCanvas::Nodes::Core::AzEventHandler*>(node) }; azEventHandlerNode != nullptr)
        {
            graphCanvasNodeId = DisplayAzEventHandlerNode(graphCanvasGraphId, azEventHandlerNode);
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::ReceiveScriptEvent>(node))
        {
            graphCanvasNodeId = DisplayScriptEventWrapperNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::SendScriptEvent>(node))
        {
            graphCanvasNodeId = DisplayScriptEventSenderNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::SendScriptEvent*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::FunctionCallNode>(node))
        {
            graphCanvasNodeId = DisplayFunctionNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::FunctionDefinitionNode>(node))
        {
            graphCanvasNodeId = DisplayFunctionDefinitionNode(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(node));
        }
        else if (azrtti_istypeof<ScriptCanvas::Nodes::Core::Internal::Nodeling>(node))
        {
            graphCanvasNodeId = DisplayNodeling(graphCanvasGraphId, static_cast<const ScriptCanvas::Nodes::Core::Internal::Nodeling*>(node));
        }
        else if (node)
        {
            graphCanvasNodeId = DisplayNode(graphCanvasGraphId, node);
        }

        return graphCanvasNodeId;
    }

    static void RegisterAndActivateGraphCanvasSlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::SlotId& slotId, AZ::Entity* slotEntity)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        if (slotEntity)
        {
            slotEntity->Init();
            slotEntity->Activate();

            // Set the user data on the GraphCanvas slot to be the SlotId of the ScriptCanvas
            AZStd::any* slotUserData = nullptr;
            GraphCanvas::SlotRequestBus::EventResult(slotUserData, slotEntity->GetId(), &GraphCanvas::SlotRequests::GetUserData);

            if (slotUserData)
            {
                *slotUserData = slotId;
            }

            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
        }
    }

    static GraphCanvas::ConnectionType ToGraphCanvasConnectionType(ScriptCanvas::ConnectionType connectionType)
    {
        GraphCanvas::ConnectionType graphCanvasConnectionType = GraphCanvas::CT_Invalid;
        switch (connectionType)
        {
        case ScriptCanvas::ConnectionType::Input:
            graphCanvasConnectionType = GraphCanvas::CT_Input;
            break;
        case ScriptCanvas::ConnectionType::Output:
            graphCanvasConnectionType = GraphCanvas::CT_Output;
            break;
        default:
            break;
        }

        return graphCanvasConnectionType;
    }

    AZ::EntityId DisplayScriptCanvasSlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::Slot& slot, GraphCanvas::SlotGroup slotGroup)
    {
        if (!slot.IsVisible())
        {
            return AZ::EntityId();
        }

        AZ_PROFILE_FUNCTION(ScriptCanvas);
        AZ::Entity* slotEntity = nullptr;

        AZ::Uuid typeId = ScriptCanvas::Data::ToAZType(slot.GetDataType());

        if (slot.IsExecution())
        {
            GraphCanvas::ExecutionSlotConfiguration executionConfiguration;
            executionConfiguration.m_name = slot.GetName();
            executionConfiguration.m_tooltip = slot.GetToolTip();
            executionConfiguration.m_slotGroup = slotGroup;

            if (slot.IsLatent())
            {
                executionConfiguration.m_textDecoration = u8"\U0001f552";
                executionConfiguration.m_textDecorationToolTip = "This slot will not be executed immediately.";
            }

            if (slotGroup == GraphCanvas::SlotGroups::Invalid && slot.GetDisplayGroup() != AZ::Crc32())
            {
                executionConfiguration.m_slotGroup = slot.GetDisplayGroup();
            }

            executionConfiguration.m_connectionType = ToGraphCanvasConnectionType(slot.GetConnectionType());

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, graphCanvasNodeId, executionConfiguration);
        }
        else if (slot.IsData())
        {
            GraphCanvas::DataSlotConfiguration dataSlotConfiguration;
            dataSlotConfiguration.m_typeId = typeId;
            dataSlotConfiguration.m_dataSlotType = GraphCanvas::DataSlotType::Value;

            dataSlotConfiguration.m_name = slot.GetName();
            dataSlotConfiguration.m_tooltip = slot.GetToolTip();
            dataSlotConfiguration.m_slotGroup = slotGroup;

            if (slot.IsLatent())
            {
                dataSlotConfiguration.m_textDecoration = u8"\U0001f552";
                dataSlotConfiguration.m_textDecorationToolTip = "This slot will not be executed immediately.";
            }

            if (slotGroup == GraphCanvas::SlotGroups::Invalid && slot.GetDisplayGroup() != AZ::Crc32())
            {
                dataSlotConfiguration.m_slotGroup = slot.GetDisplayGroup();
            }

            dataSlotConfiguration.m_connectionType = ToGraphCanvasConnectionType(slot.GetConnectionType());

            if (ScriptCanvas::Data::IsContainerType(typeId))
            {
                dataSlotConfiguration.m_dataValueType = GraphCanvas::DataValueType::Container;
                dataSlotConfiguration.m_containerTypeIds = ScriptCanvas::Data::GetContainedTypes(typeId);
            }

            switch (slot.GetDynamicDataType())
            {
            case ScriptCanvas::DynamicDataType::Container:
                dataSlotConfiguration.m_dataValueType = GraphCanvas::DataValueType::Container;
                break;
            default:
                break;
            }

            if (slot.IsVariableReference())
            {
                dataSlotConfiguration.m_dataSlotType = GraphCanvas::DataSlotType::Reference;
            }

            dataSlotConfiguration.m_canConvertTypes = slot.CanConvertTypes();

            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, graphCanvasNodeId, dataSlotConfiguration);
        }

        if (slotEntity)
        {
            RegisterAndActivateGraphCanvasSlot(graphCanvasNodeId, slot.GetId(), slotEntity);
            CopyTranslationKeyedNameToDatumLabel(graphCanvasNodeId, slot.GetId(), slotEntity->GetId());
            return slotEntity->GetId();
        }
        else
        {
            return AZ::EntityId();
        }
    }
}

namespace ScriptCanvasEditor::Nodes::SlotDisplayHelper
{
    AZ::EntityId DisplayPropertySlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& propertyConfiguration)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        GraphCanvas::SlotConfiguration graphCanvasConfiguration;

        graphCanvasConfiguration.m_name = propertyConfiguration.m_name;
        graphCanvasConfiguration.m_tooltip = propertyConfiguration.m_tooltip;
        graphCanvasConfiguration.m_slotGroup = GraphCanvas::SlotGroup(propertyConfiguration.m_displayGroup);

        graphCanvasConfiguration.m_connectionType = ToGraphCanvasConnectionType(propertyConfiguration.m_connectionType);

        AZ::Entity* slotEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreatePropertySlot, graphCanvasNodeId, propertyConfiguration.m_identifier, graphCanvasConfiguration);

        if (slotEntity)
        {
            slotEntity->Init();
            slotEntity->Activate();

            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
        }

        return slotEntity ? slotEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId DisplayExtendableSlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& extenderConfiguration)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);

        GraphCanvas::ExtenderSlotConfiguration graphCanvasConfiguration;

        graphCanvasConfiguration.m_name = extenderConfiguration.m_name;
        graphCanvasConfiguration.m_tooltip = extenderConfiguration.m_tooltip;
        graphCanvasConfiguration.m_slotGroup = GraphCanvas::SlotGroup(extenderConfiguration.m_displayGroup);

        graphCanvasConfiguration.m_connectionType = ToGraphCanvasConnectionType(extenderConfiguration.m_connectionType);

        graphCanvasConfiguration.m_extenderId = extenderConfiguration.m_identifier;

        AZ::Entity* slotEntity = nullptr;

        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(slotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, graphCanvasNodeId, graphCanvasConfiguration);

        if (slotEntity)
        {
            slotEntity->Init();
            slotEntity->Activate();

            GraphCanvas::NodeRequestBus::Event(graphCanvasNodeId, &GraphCanvas::NodeRequests::AddSlot, slotEntity->GetId());
        }

        return slotEntity ? slotEntity->GetId() : AZ::EntityId();
    }

    AZ::EntityId DisplayVisualExtensionSlot(AZ::EntityId graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& extensionConfiguration)
    {
        AZ::EntityId slotId;
        switch (extensionConfiguration.m_extensionType)
        {
        case ScriptCanvas::VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot:
            slotId = SlotDisplayHelper::DisplayExtendableSlot(graphCanvasNodeId, extensionConfiguration);
            break;
        case ScriptCanvas::VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot:
            slotId = SlotDisplayHelper::DisplayPropertySlot(graphCanvasNodeId, extensionConfiguration);
            break;
        default:
            break;
        }

        return slotId;
    }
}
