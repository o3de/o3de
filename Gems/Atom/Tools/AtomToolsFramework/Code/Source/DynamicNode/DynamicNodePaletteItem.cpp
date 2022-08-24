/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodePaletteItem.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Model/Common.h>

namespace AtomToolsFramework
{
    void CreateDynamicNodeMimeEvent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CreateDynamicNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("toolId", &CreateDynamicNodeMimeEvent::m_toolId)
                ->Field("configId", &CreateDynamicNodeMimeEvent::m_configId);
        }
    }

    CreateDynamicNodeMimeEvent::CreateDynamicNodeMimeEvent(
        const AZ::Crc32& toolId, const AZ::Uuid& configId)
        : m_toolId(toolId)
        , m_configId(configId)
    {
    }

    bool CreateDynamicNodeMimeEvent::ExecuteEvent(
        [[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId)
    {
        GraphModel::GraphPtr graph;
        GraphModelIntegration::GraphManagerRequestBus::BroadcastResult(
            graph, &GraphModelIntegration::GraphManagerRequests::GetGraph, graphCanvasSceneId);
        if (graph)
        {
            AZStd::shared_ptr<GraphModel::Node> node = AZStd::make_shared<DynamicNode>(graph, m_toolId, m_configId);
            if (node)
            {
                GraphModelIntegration::GraphControllerRequestBus::Event(
                    graphCanvasSceneId, &GraphModelIntegration::GraphControllerRequests::AddNode, node, dropPosition);
                return true;
            }
        }

        return false;
    }

    DynamicNodePaletteItem::DynamicNodePaletteItem(
        const AZ::Crc32& toolId, const DynamicNodeConfig& config)
        : DraggableNodePaletteTreeItem(config.m_title.c_str(), toolId)
        , m_toolId(toolId)
        , m_configId(config.m_id)
    {
    }

    GraphCanvas::GraphCanvasMimeEvent* DynamicNodePaletteItem::CreateMimeEvent() const
    {
        return aznew CreateDynamicNodeMimeEvent(m_toolId, m_configId);
    }
} // namespace AtomToolsFramework
