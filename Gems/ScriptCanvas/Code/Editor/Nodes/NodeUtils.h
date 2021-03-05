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

#include <AzCore/Component/EntityId.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Libraries/Core/FunctionNode.h>

#include <ScriptEvents/ScriptEventsAsset.h>

#include <Editor/Translation/TranslationHelper.h>

namespace ScriptCanvas
{
    class Slot;
}

namespace ScriptCanvasEditor
{
    namespace Nodes
    {
        namespace Internal
        {
            template<class... ComponentTypes>
            struct PopulateHelper;

            template<class ComponentType, class... ComponentTypes>
            struct PopulateHelper<ComponentType, ComponentTypes...>
            {
            public:
                static void PopulateComponentDescriptors(AZStd::vector< AZ::Uuid >& componentDescriptors)
                {
                    componentDescriptors.emplace_back(azrtti_typeid<ComponentType>());
                    PopulateHelper<ComponentTypes...>::PopulateComponentDescriptors(componentDescriptors);
                }
            };

            template<>
            struct PopulateHelper<>
            {
            public:
                static void PopulateComponentDescriptors([[maybe_unused]] AZStd::vector< AZ::Uuid >& componentDescriptors)
                {
                }
            };
        }

        enum class NodeType
        {
            WrapperNode,
            GeneralNode
        };

        struct NodeConfiguration
        {
            NodeConfiguration()
                : m_nodeType(NodeType::GeneralNode)
            {
            }

            template<class... ComponentTypes>
            void PopulateComponentDescriptors()
            {
                m_customComponents.reserve(sizeof...(ComponentTypes));
                Internal::PopulateHelper<ComponentTypes...>::PopulateComponentDescriptors(m_customComponents);
            }

            NodeType m_nodeType;

            AZStd::string m_nodeSubStyle;
            AZStd::string m_titlePalette;
            AZStd::vector< AZ::Uuid > m_customComponents;

            // Translation Information for the Node
            AZStd::string m_translationContext;

            AZStd::string m_translationKeyName;
            AZStd::string m_translationKeyContext;

            TranslationContextGroup m_translationGroup;

            AZStd::string m_titleFallback;
            AZStd::string m_subtitleFallback;
            AZStd::string m_tooltipFallback;

            AZ::EntityId  m_scriptCanvasId;
        };

        struct StyleConfiguration
        {
            AZStd::string m_nodeSubStyle;
            AZStd::string m_titlePalette;
        };

        AZStd::string GetContextName(const AZ::SerializeContext::ClassData& classData);
        AZStd::string GetCategoryName(const AZ::SerializeContext::ClassData& classData);

        AZ::EntityId DisplayEbusEventNode(const AZ::EntityId& graphCanvasGraphId, const AZStd::string& busName, const AZStd::string& eventName, const ScriptCanvas::EBusEventId& eventId);

        // Generic method of displaying a node.
        AZ::EntityId DisplayScriptCanvasNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Node* node);

        // Specific create methods which will also handle displaying the node.
        NodeIdPair CreateNode(const AZ::Uuid& classData, const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const StyleConfiguration& styleConfiguration);
        NodeIdPair CreateEntityNode(const AZ::EntityId& sourceId, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        NodeIdPair CreateObjectMethodNode(const AZStd::string& className, const AZStd::string& methodName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        NodeIdPair CreateEbusWrapperNode(const AZStd::string& busName, const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        
        // Script Events
        AZ::EntityId DisplayScriptEventNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition);
        NodeIdPair CreateScriptEventReceiverNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::Data::AssetId& assetId);
        NodeIdPair CreateScriptEventSenderNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::Data::AssetId& assetId, const ScriptCanvas::EBusEventId& eventId);

        NodeIdPair CreateGetVariableNode(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId);
        NodeIdPair CreateSetVariableNode(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId);

        NodeIdPair CreateFunctionNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasGraphId, const AZ::Data::AssetId& assetId);
        AZ::EntityId DisplayFunctionNode(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::FunctionNode* functionNode);
        AZ::EntityId DisplayFunctionNode(const AZ::EntityId& graphCanvasGraphId, ScriptCanvas::Nodes::Core::FunctionNode* functionNode);

        AZ::EntityId DisplayNodeling(const AZ::EntityId& graphCanvasGraphId, const ScriptCanvas::Nodes::Core::Internal::Nodeling* nodeling);
        NodeIdPair CreateExecutionNodeling(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, AZStd::string rootName = "New Nodeling");

        // SlotGroup will control how elements are grouped.
        // Invalid will cause the slots to put themselves into whatever category they belong to by default.
        AZ::EntityId DisplayScriptCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::Slot& slot, GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid);
        AZ::EntityId DisplayVisualExtensionSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::VisualExtensionSlotConfiguration& extenderConfiguration);

        void CopySlotTranslationKeyedNamesToDatums(AZ::EntityId graphCanvasNodeId);

        template <typename NodeType>
        NodeType* GetNode(AZ::EntityId scriptCanvasGraphId, NodeIdPair nodeIdPair)
        { 
            ScriptCanvas::Node* node = nullptr;

            AZ::Entity* sourceEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, nodeIdPair.m_scriptCanvasId);
            if (sourceEntity)
            {
                node = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(sourceEntity);

                if (node == nullptr)
                {
                    ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::CreateNodeOnEntity, sourceEntity->GetId(), scriptCanvasGraphId, azrtti_typeid<NodeType>());
                }
            }

            return azrtti_cast<NodeType*>(node);
        }
    }
}
