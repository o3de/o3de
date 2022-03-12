/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/CommentConstructMenuActions.h>

#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <GraphCanvas/Utils/ConversionUtils.h>


namespace GraphCanvas
{
    /////////////////////////
    // AddCommentMenuAction
    /////////////////////////
    
    AddCommentMenuAction::AddCommentMenuAction(QObject* parent)
        : ConstructContextMenuAction("Add comment", parent)
    {
    }

    ContextMenuAction::SceneReaction AddCommentMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);

        AZ::Entity* graphCanvasEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(graphCanvasEntity, &GraphCanvasRequests::CreateCommentNodeAndActivate);

        AZ_Assert(graphCanvasEntity, "Unable to create GraphCanvas Bus Node");

        if (graphCanvasEntity)
        {
            SceneRequestBus::Event(graphId, &SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos, false);
            CommentUIRequestBus::Event(graphCanvasEntity->GetId(), &CommentUIRequests::SetEditable, true);
        }
        
        if (graphCanvasEntity != nullptr)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }
}
