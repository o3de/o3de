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
            SceneRequestBus::Event(graphId, &SceneRequests::AddNode, graphCanvasEntity->GetId(), scenePos);
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