/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include <GraphModel/Model/Common.h>

namespace AtomToolsFramework
{
    //! The logic for this event is executed whenever a node is dragged from the node palette to the graph view. Will attempt to create a
    //! DynamicNode wherever the mouse drop event occurred.
    class CreateDynamicNodeMimeEvent : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(CreateDynamicNodeMimeEvent, "{9FF18F29-50B9-46A2-B661-9FD81BF9BD2F}", GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateDynamicNodeMimeEvent, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        CreateDynamicNodeMimeEvent() = default;
        CreateDynamicNodeMimeEvent(const AZ::Crc32& toolId, const AZ::Uuid& configId);
        bool ExecuteEvent(const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId) override;

    protected:
        AZ::Crc32 m_toolId = {};
        AZ::Uuid m_configId;
    };

    //! Item displayed in the node palette for creating a DynamicNode using the configuration settings passed into the constructor
    class DynamicNodePaletteItem : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicNodePaletteItem, AZ::SystemAllocator);

        //! Constructor
        //! @param toolId Unique ID of the client system editor (ex: AZ_CRC_CE("MaterialCanvas"))
        //! @param config The DynamicNodeConfig used to populate the DynamicNode.
        DynamicNodePaletteItem(const AZ::Crc32& toolId, const DynamicNodeConfig& config);
        ~DynamicNodePaletteItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

    protected:
        AZ::Crc32 m_toolId = {};
        AZ::Uuid m_configId;
    };
} // namespace AtomToolsFramework
