/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/Nodes/NodeDisplayUtils.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/NodelingBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/MethodOverloaded.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>

namespace ScriptCanvasEditor::Nodes
{

    NodeIdPair CreateFunctionDefinitionNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, bool isInput, AZStd::string rootName)
    {
        ScriptCanvasEditor::Nodes::StyleConfiguration styleConfiguration;
        auto nodeAndPair = CreateAndGetNode(azrtti_typeid<ScriptCanvas::Nodes::Core::FunctionDefinitionNode>(), scriptCanvasId, styleConfiguration, [isInput](ScriptCanvas::Node* node)
            {
                ScriptCanvas::Nodes::Core::FunctionDefinitionNode* fdn = azrtti_cast<ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(node);
                if (!isInput)
                {
                    fdn->MarkExecutionExit();
                }
            }
        );
        NodeIdPair createdPair = nodeAndPair.second;

        auto functionDefinitionNode = azrtti_cast<ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(nodeAndPair.first);

        if (functionDefinitionNode && createdPair.m_scriptCanvasId.IsValid())
        {

            AZStd::unordered_set<AZStd::string> nodelingNames;

            auto enumerationFunction = [&nodelingNames, scriptCanvasId](ScriptCanvas::NodelingRequests* nodelingRequests)
            {
                if (nodelingRequests->GetGraphScopedNodeId().m_scriptCanvasId == scriptCanvasId)
                {
                    nodelingNames.insert(nodelingRequests->GetDisplayName());
                }

                return true;
            };

            ScriptCanvas::NodelingRequestBus::EnumerateHandlers(enumerationFunction);

            int counter = 1;

            AZStd::string nodelingName = rootName;

            while (nodelingNames.find(nodelingName) != nodelingNames.end())
            {
                nodelingName = AZStd::string::format("%.*s %i", aznumeric_cast<int>(rootName.size()), rootName.data(), counter);
                ++counter;
            }

            ScriptCanvas::GraphScopedNodeId nodelingId;

            nodelingId.m_identifier = createdPair.m_scriptCanvasId;
            nodelingId.m_scriptCanvasId = scriptCanvasId;

            ScriptCanvas::NodelingRequestBus::Event(nodelingId, &ScriptCanvas::NodelingRequests::SetDisplayName, nodelingName);

            if (!isInput)
            {
                functionDefinitionNode->MarkExecutionExit();
            }
            
            GraphCanvas::NodeTitleRequestBus::Event(createdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetSubTitle, "Function");

        }

        return createdPair;
    }

    NodeIdPair CreateNode(const AZ::Uuid& classId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const StyleConfiguration& styleConfiguration)
    {
        return CreateAndGetNode(classId, scriptCanvasId, styleConfiguration).second;
    }

    AZStd::pair<ScriptCanvas::Node*, NodeIdPair> CreateAndGetNode(const AZ::Uuid& classId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const StyleConfiguration& styleConfiguration, AZStd::function<void(ScriptCanvas::Node*)> onCreateCallback)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIdPair;

        ScriptCanvas::Node* node{};
        AZ::Entity* scriptCanvasEntity{ aznew AZ::Entity };
        scriptCanvasEntity->Init();
        nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();
        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, classId);
        
        if (onCreateCallback)
        {
            onCreateCallback(node);
        }
        scriptCanvasEntity->SetName(AZStd::string::format("SC-Node(%s)", scriptCanvasEntity->GetName().data()));

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIdPair.m_graphCanvasId = DisplayScriptCanvasNode(graphCanvasGraphId, node);

        if (nodeIdPair.m_graphCanvasId.IsValid())
        {
            if (!styleConfiguration.m_titlePalette.empty())
            {
                GraphCanvas::NodeTitleRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, styleConfiguration.m_titlePalette);
            }

            // TODO: Add support for this, but should be mostly irrelevant, as the display node should be the one that handles setting that up.
            //nodeConfiguration.m_nodeSubStyle = styleConfiguration.m_nodeSubStyle;
        }

        return AZStd::make_pair(node, nodeIdPair);
    }

    NodeIdPair CreateObjectMethodNode(AZStd::string_view className, AZStd::string_view methodName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, ScriptCanvas::PropertyStatus propertyStatus)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIds;

        ScriptCanvas::Node* node = nullptr;
        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
        scriptCanvasEntity->Init();
        nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, ScriptCanvas::Nodes::Core::Method::RTTI_Type());
        auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node);

        ScriptCanvas::NamespacePath emptyNamespacePath;
        methodNode->InitializeBehaviorMethod(emptyNamespacePath, className, methodName, propertyStatus);

        AZStd::string_view displayName = methodNode->GetName();
        scriptCanvasEntity->SetName(AZStd::string::format("SC-Node(%.*s)", aznumeric_cast<int>(displayName.size()), displayName.data()));

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIds.m_graphCanvasId = DisplayMethodNode(graphCanvasGraphId, methodNode);

        return nodeIds;
    }

    NodeIdPair CreateObjectMethodOverloadNode(AZStd::string_view className, AZStd::string_view methodName, const ScriptCanvas::ScriptCanvasId& scriptCanvasGraphId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIds;

        ScriptCanvas::Node* node = nullptr;
        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
        scriptCanvasEntity->Init();
        nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Core::MethodOverloaded::RTTI_Type());
        auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::MethodOverloaded*>(node);

        ScriptCanvas::NamespacePath emptyNamespacePath;
        methodNode->InitializeBehaviorMethod(emptyNamespacePath, className, methodName, ScriptCanvas::PropertyStatus::None);

        AZStd::string_view displayName = methodNode->GetName();
        scriptCanvasEntity->SetName(AZStd::string::format("SC-Node(%.*s)", aznumeric_cast<int>(displayName.size()), displayName.data()));

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIds.m_graphCanvasId = DisplayMethodNode(graphCanvasGraphId, methodNode);

        return nodeIds;
    }

    NodeIdPair CreateGlobalMethodNode(AZStd::string_view methodName, bool isProperty, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIds;

        ScriptCanvas::Node* node = nullptr;
        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
        scriptCanvasEntity->Init();
        nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, ScriptCanvas::Nodes::Core::Method::RTTI_Type());
        auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node);

        ScriptCanvas::NamespacePath emptyNamespacePath;
        methodNode->InitializeFree(emptyNamespacePath, methodName);

        AZStd::string displayName = methodNode->GetName();
        scriptCanvasEntity->SetName(AZStd::string::format("SC-Node(%.*s)", aznumeric_cast<int>(displayName.size()), displayName.data()));

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIds.m_graphCanvasId = DisplayMethodNode(graphCanvasGraphId, methodNode, isProperty);

        return nodeIds;
    }

    NodeIdPair CreateEbusWrapperNode(AZStd::string_view busName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIdPair;

        ScriptCanvas::Node* node = nullptr;

        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Node(%.*s)", aznumeric_cast<int>(busName.size()), busName.data()));
        scriptCanvasEntity->Init();

        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type());
        auto* busNode = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(node);
        busNode->InitializeBus(busName);

        nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIdPair.m_graphCanvasId = DisplayEbusWrapperNode(graphCanvasGraphId, busNode);

        return nodeIdPair;
    }

    NodeIdPair CreateScriptEventReceiverNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::Data::AssetId& assetId)
    {
        AZ_Assert(assetId.IsValid(), "CreateScriptEventReceiverNode asset Id must be valid");

        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIdPair;

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
        if (!asset)
        {
            AZ_Error("GraphCanvas", asset, "Unable to CreateScriptEventReceiverNode, asset %s not found.", assetId.ToString<AZStd::string>().c_str());
            return nodeIdPair;
        }

        asset.BlockUntilLoadComplete();

        ScriptCanvas::Node* node = nullptr;

        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Node(%s)", asset.Get()->m_definition.GetName().data()));
        scriptCanvasEntity->Init();

        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, ScriptCanvas::Nodes::Core::ReceiveScriptEvent::RTTI_Type());
        auto* busNode = azrtti_cast<ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(node);
        busNode->Initialize(assetId);

        nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIdPair.m_graphCanvasId = DisplayScriptEventWrapperNode(graphCanvasGraphId, busNode);

        return nodeIdPair;
    }

    NodeIdPair CreateScriptEventSenderNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId)
    {
        AZ_Assert(assetId.IsValid(), "CreateScriptEventSenderNode asset Id must be valid");

        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIdPair;

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Node(%s)", asset.Get()->m_definition.GetName().data()));
        scriptCanvasEntity->Init();

        ScriptCanvas::Node* node = nullptr;
        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, ScriptCanvas::Nodes::Core::SendScriptEvent::RTTI_Type());
        auto* senderNode = azrtti_cast<ScriptCanvas::Nodes::Core::SendScriptEvent*>(node);

        senderNode->ConfigureNode(assetId, eventId);

        nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIdPair.m_graphCanvasId = DisplayScriptEventSenderNode(graphCanvasGraphId, senderNode);

        return nodeIdPair;
    }

    CreateNodeResult CreateGetVariableNodeResult(const ScriptCanvas::VariableId& variableId, ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        const AZ::Uuid k_VariableNodeTypeId = azrtti_typeid<ScriptCanvas::Nodes::Core::GetVariableNode>();

        NodeIdPair nodeIds;

        ScriptCanvas::Node* node = nullptr;
        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
        scriptCanvasEntity->Init();
        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, k_VariableNodeTypeId);

        ScriptCanvas::Nodes::Core::GetVariableNode* variableNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::GetVariableNode>(scriptCanvasEntity);

        if (variableNode)
        {
            variableNode->SetId(variableId);
        }

        nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIds.m_graphCanvasId = DisplayGetVariableNode(graphCanvasGraphId, variableNode);

        scriptCanvasEntity->SetName(AZStd::string::format("SC Node(GetVariable)"));

        CreateNodeResult result;
        result.node = node;
        result.nodeIdPair = nodeIds;
        return result;
    }

    CreateNodeResult CreateSetVariableNodeResult(const ScriptCanvas::VariableId& variableId, ScriptCanvas::ScriptCanvasId scriptCanvasId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        const AZ::Uuid k_VariableNodeTypeId = azrtti_typeid<ScriptCanvas::Nodes::Core::SetVariableNode>();

        NodeIdPair nodeIds;

        ScriptCanvas::Node* node = nullptr;
        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity();
        scriptCanvasEntity->Init();
        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasId, k_VariableNodeTypeId);

        ScriptCanvas::Nodes::Core::SetVariableNode* variableNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Nodes::Core::SetVariableNode>(scriptCanvasEntity);

        if (variableNode)
        {
            variableNode->SetId(variableId);
        }

        nodeIds.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIds.m_graphCanvasId = DisplaySetVariableNode(graphCanvasGraphId, variableNode);

        scriptCanvasEntity->SetName(AZStd::string::format("SC Node(SetVariable)"));

        CreateNodeResult result;
        result.node = node;
        result.nodeIdPair = nodeIds;
        return result;
    }

    NodeIdPair CreateGetVariableNode(const ScriptCanvas::VariableId& variableId, ScriptCanvas::ScriptCanvasId scriptCanvasGraphId)
    {
        return CreateGetVariableNodeResult(variableId, scriptCanvasGraphId).nodeIdPair;
    }

    NodeIdPair CreateSetVariableNode(const ScriptCanvas::VariableId& variableId, ScriptCanvas::ScriptCanvasId scriptCanvasGraphId)
    {
        return CreateSetVariableNodeResult(variableId, scriptCanvasGraphId).nodeIdPair;
    }

    NodeIdPair CreateFunctionNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasGraphId, const AZ::Data::AssetId& assetId, const ScriptCanvas::Grammar::FunctionSourceId& sourceId)
    {
        AZ_Assert(assetId.IsValid(), "CreateFunctionNode source asset Id must be valid");

        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIdPair;

        AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvas::SubgraphInterfaceAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        asset.BlockUntilLoadComplete();

        AZ::Entity* scriptCanvasEntity = aznew AZ::Entity(AZStd::string::format("SC-Function (%s)", asset.GetId().ToString<AZStd::string>().c_str()));
        scriptCanvasEntity->Init();

        ScriptCanvas::Node* node = nullptr;
        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, scriptCanvasEntity->GetId(), scriptCanvasGraphId, ScriptCanvas::Nodes::Core::FunctionCallNode::RTTI_Type());
        auto functionNode = azrtti_cast<ScriptCanvas::Nodes::Core::FunctionCallNode*>(node);
        functionNode->Initialize(assetId, sourceId);
        functionNode->BuildNode();

        nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIdPair.m_graphCanvasId = DisplayFunctionNode(graphCanvasGraphId, functionNode);

        return nodeIdPair;
    }

    NodeIdPair CreateAzEventHandlerNode(const AZ::BehaviorMethod& methodWithAzEventReturn, ScriptCanvas::ScriptCanvasId scriptCanvasId,
        AZ::EntityId connectingMethodNodeId)
    {
        AZ_PROFILE_FUNCTION(ScriptCanvas);
        NodeIdPair nodeIdPair;

        // Make sure the method returns an AZ::Event by reference or pointer
        if (!AZ::MethodReturnsAzEventByReferenceOrPointer(methodWithAzEventReturn))
        {
            return {};
        }

        // Read in AZ Event Description data to retrieve the event name and parameter names
        AZ::Attribute* azEventDescAttribute = AZ::FindAttribute(AZ::Script::Attributes::AzEventDescription, methodWithAzEventReturn.m_attributes);
        AZ::BehaviorAzEventDescription behaviorAzEventDesc;
        AZ::AttributeReader azEventDescAttributeReader(nullptr, azEventDescAttribute);
        azEventDescAttributeReader.Read<decltype(behaviorAzEventDesc)>(behaviorAzEventDesc);
        if (behaviorAzEventDesc.m_eventName.empty())
        {
            AZ_Error("NodeUtils", false, "Cannot create an AzEvent node with empty event name")
            return {};
        }

        auto scriptCanvasEntity = aznew AZ::Entity{ AZStd::string::format("SC-EventNode(%s)", behaviorAzEventDesc.m_eventName.c_str()) };
        scriptCanvasEntity->Init();
        auto azEventHandler = scriptCanvasEntity->CreateComponent<ScriptCanvas::Nodes::Core::AzEventHandler>();
        ScriptCanvas::GraphRequestBus::Event(scriptCanvasId, &ScriptCanvas::GraphRequests::AddNode, scriptCanvasEntity->GetId());
        azEventHandler->InitEventFromMethod(methodWithAzEventReturn);
        azEventHandler->SetRestrictedNodeId(connectingMethodNodeId);

        nodeIdPair.m_scriptCanvasId = scriptCanvasEntity->GetId();

        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasId, &EditorGraphRequests::GetGraphCanvasGraphId);

        nodeIdPair.m_graphCanvasId = DisplayAzEventHandlerNode(graphCanvasGraphId, azEventHandler);

        return nodeIdPair;
    }
}
