/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNode.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include <GraphModel/Model/Common.h>

namespace AtomToolsFramework
{
    class CreateDynamicNodeMimeEvent : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(CreateDynamicNodeMimeEvent, "{9FF18F29-50B9-46A2-B661-9FD81BF9BD2F}", GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateDynamicNodeMimeEvent, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        CreateDynamicNodeMimeEvent() = default;
        CreateDynamicNodeMimeEvent(const DynamicNodeConfig& config);
        bool ExecuteEvent(const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId) override;

    protected:
        DynamicNodeConfig m_config;
    };

    class DynamicNodePaletteItem : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicNodePaletteItem, AZ::SystemAllocator, 0);

        //! Constructor
        //! \param editorId Unique name of the client system editor (ex: AZ_CRC_CE("MaterialCanvas"))
        //! \param config The configuration structure used to populate the dynamic node.
        DynamicNodePaletteItem(GraphCanvas::EditorId editorId, const DynamicNodeConfig& config);
        ~DynamicNodePaletteItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

    protected:
        DynamicNodeConfig m_config;
    };
} // namespace AtomToolsFramework
