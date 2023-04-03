/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


// Graph Canvas
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphModelIntegration
{
    //! Provides a common interface for instantiating Graph Canvas support nodes like comments through the Node Palette
    class CreateGraphCanvasNodeMimeEvent
        : public GraphCanvas::GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(CreateGraphCanvasNodeMimeEvent, "{7171A847-7405-459F-A031-CC9AE50745B6}", GraphCanvas::GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateGraphCanvasNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateGraphCanvasNodeMimeEvent() = default;
        ~CreateGraphCanvasNodeMimeEvent() = default;

        bool ExecuteEvent([[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& dropPosition, const AZ::EntityId& graphCanvasSceneId) override final
        {
            AZ::Entity* graphCanvasNode = CreateNode();

            if (graphCanvasNode)
            {
                AZ::EntityId graphCanvasNodeId = graphCanvasNode->GetId();

                GraphCanvas::SceneRequestBus::Event(graphCanvasSceneId, &GraphCanvas::SceneRequests::AddNode, graphCanvasNodeId, dropPosition, false);
                GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

                AZ::EntityId gridId;
                GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasSceneId, &GraphCanvas::SceneRequests::GetGrid);

                AZ::Vector2 offset;
                GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

                dropPosition += offset;
                return true;
            }

            return false;
        }

    protected:
        virtual AZ::Entity* CreateNode() const = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////
    // Comment Node

    class CreateCommentNodeMimeEvent
        : public CreateGraphCanvasNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateCommentNodeMimeEvent, "{1060EE7B-DBC2-4B7F-BC4C-4AB4651A3812}", CreateGraphCanvasNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateCommentNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateCommentNodeMimeEvent() = default;
        ~CreateCommentNodeMimeEvent() = default;

        virtual AZ::Entity* CreateNode() const override
        {
            AZ::Entity* graphCanvasNode = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasNode, &GraphCanvas::GraphCanvasRequests::CreateCommentNodeAndActivate);
            return graphCanvasNode;
        }
    };

    class CommentNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(CommentNodePaletteTreeItem, AZ::SystemAllocator);
        CommentNodePaletteTreeItem(AZStd::string_view nodeName, GraphCanvas::EditorId editorId)
            : DraggableNodePaletteTreeItem(nodeName, editorId)
        {
            SetToolTip("Comment box for notes. Does not affect script execution or data.");
        }
        ~CommentNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override
        {
            return aznew CreateCommentNodeMimeEvent();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////
    // Node Group Node

    class CreateNodeGroupNodeMimeEvent
        : public CreateGraphCanvasNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateNodeGroupNodeMimeEvent, "{1451A2F2-640B-4CB3-BF48-DD77E97EC900}", CreateGraphCanvasNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateNodeGroupNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateNodeGroupNodeMimeEvent() = default;
        ~CreateNodeGroupNodeMimeEvent() = default;

        virtual AZ::Entity* CreateNode() const override
        {
            AZ::Entity* graphCanvasNode = nullptr;
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasNode, &GraphCanvas::GraphCanvasRequests::CreateNodeGroupAndActivate);
            return graphCanvasNode;
        }
    };

    class NodeGroupNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeGroupNodePaletteTreeItem, AZ::SystemAllocator);
        NodeGroupNodePaletteTreeItem(AZStd::string_view nodeName, GraphCanvas::EditorId editorId)
            : DraggableNodePaletteTreeItem(nodeName, editorId)
        {}
        ~NodeGroupNodePaletteTreeItem() = default;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override
        {
            return aznew CreateNodeGroupNodeMimeEvent();
        }
    };

    /// Add common utilities to a specific Node Palette tree.
    void AddCommonNodePaletteUtilities(GraphCanvas::GraphCanvasTreeItem* rootItem, const GraphCanvas::EditorId& editorId);
}
