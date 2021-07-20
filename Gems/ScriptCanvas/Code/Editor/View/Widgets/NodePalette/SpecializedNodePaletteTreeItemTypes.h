/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include "CreateNodeMimeEvent.h"

namespace ScriptCanvasEditor
{
    // <EntityRefNode>
    class CreateEntityRefNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateEntityRefNodeMimeEvent, "{20CD5AF5-216E-4A41-9630-191C2803899B}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateEntityRefNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateEntityRefNodeMimeEvent() = default;
        CreateEntityRefNodeMimeEvent(const AZ::EntityId& entityId);
        ~CreateEntityRefNodeMimeEvent() = default;

    protected:
        ScriptCanvasEditor::NodeIdPair CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

    private:
        AZ::EntityId m_entityId;
    };

    class EntityRefNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityRefNodePaletteTreeItem, AZ::SystemAllocator, 0);
        EntityRefNodePaletteTreeItem(AZStd::string_view nodeName, const QString& iconPath);
        ~EntityRefNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    };
    // </EntityRefNode>    
    
    // <CommentNode>
    class CreateCommentNodeMimeEvent
        : public SpecializedCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateCommentNodeMimeEvent, "{AF5BB1C0-E5CF-40B1-A037-1500C2BAC787}", SpecializedCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateCommentNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateCommentNodeMimeEvent() = default;
        ~CreateCommentNodeMimeEvent() = default;

        NodeIdPair ConstructNode(const AZ::EntityId& sceneId, const AZ::Vector2& scenePosition);
        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId) override;
    };

    class CommentNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(CommentNodePaletteTreeItem, AZ::SystemAllocator, 0);
        CommentNodePaletteTreeItem(AZStd::string_view nodeName, const QString& iconPath);
        ~CommentNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    };    
    // </CommentNode>

    // <NodeGroup>
    class CreateNodeGroupMimeEvent
        : public SpecializedCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateNodeGroupMimeEvent, "{FD969A58-404E-4B97-8A62-57C2B5EAC686}", SpecializedCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateNodeGroupMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateNodeGroupMimeEvent() = default;
        ~CreateNodeGroupMimeEvent() = default;

        NodeIdPair ConstructNode(const GraphCanvas::GraphId& sceneId, const AZ::Vector2& scenePosition);
        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const GraphCanvas::GraphId& sceneId) override;
    };

    class NodeGroupNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeGroupNodePaletteTreeItem, AZ::SystemAllocator, 0);
        NodeGroupNodePaletteTreeItem(AZStd::string_view nodeName, const QString& iconPath);
        ~NodeGroupNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    };
    // </NodeGroup>
}
