/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>


#include "SpecializedNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeCreateUtils.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"
#include "Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h"

#include <Core/Attributes.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////
    // CreateCommentNodeMimeEvent
    ///////////////////////////////

    void CreateCommentNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateCommentNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    NodeIdPair CreateCommentNodeMimeEvent::ConstructNode(const GraphCanvas::GraphId& sceneId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair retVal;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateCommentNodeAndActivate);

        if (graphCanvasEntity)
        {
            retVal.m_graphCanvasId = graphCanvasEntity->GetId();
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePosition, false);
            GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        }

        return retVal;
    }

    bool CreateCommentNodeMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const GraphCanvas::GraphId& graphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        NodeIdPair nodeId = ConstructNode(graphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ///////////////////////////////
    // CommentNodePaletteTreeItem
    ///////////////////////////////

    CommentNodePaletteTreeItem::CommentNodePaletteTreeItem(AZStd::string_view nodeName, [[maybe_unused]] const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("Comment box for notes. Does not affect script execution or data.");

        SetTitlePalette("CommentNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* CommentNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateCommentNodeMimeEvent();
    }

    /////////////////////////////
    // CreateNodeGroupMimeEvent
    /////////////////////////////

    void CreateNodeGroupMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateNodeGroupMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }

    NodeIdPair CreateNodeGroupMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair retVal;

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvas::GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvas::GraphCanvasRequests::CreateNodeGroupAndActivate);

        if (graphCanvasEntity)
        {
            retVal.m_graphCanvasId = graphCanvasEntity->GetId();
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePosition, false);
            GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasEntity->GetId(), &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
        }

        return retVal;
    }

    bool CreateNodeGroupMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        NodeIdPair nodeId = ConstructNode(graphCanvasGraphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ////////////////////////////////////
    // NodeGroupNodePaletteTreeItem
    ////////////////////////////////////

    NodeGroupNodePaletteTreeItem::NodeGroupNodePaletteTreeItem(AZStd::string_view nodeName, [[maybe_unused]] const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, ScriptCanvasEditor::AssetEditorId)
    {
    }

    GraphCanvas::GraphCanvasMimeEvent* NodeGroupNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateNodeGroupMimeEvent();
    }
}
