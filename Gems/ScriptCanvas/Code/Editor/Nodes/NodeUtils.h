/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

#include <Editor/Translation/TranslationHelper.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

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

            AZ::EntityId  m_scriptCanvasId;
        };

        struct StyleConfiguration
        {
            AZStd::string m_nodeSubStyle;
            AZStd::string m_titlePalette;
        };

        AZStd::string GetContextName(const AZ::SerializeContext::ClassData& classData);

        // Copies the slot name to the underlying ScriptCanvas Data Slot which matches the slot Id
        void UpdateSlotDatumLabels(AZ::EntityId graphCanvasNodeId);
        void UpdateSlotDatumLabel(const AZ::EntityId& graphCanvasNodeId, ScriptCanvas::SlotId scSlotId, const AZStd::string& name);

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
