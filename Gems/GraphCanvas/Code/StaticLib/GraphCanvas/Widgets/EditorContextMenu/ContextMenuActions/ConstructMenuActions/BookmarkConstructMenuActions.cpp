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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/BookmarkConstructMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

namespace GraphCanvas
{
    //////////////////////////
    // AddBookmarkMenuAction
    //////////////////////////
    AddBookmarkMenuAction::AddBookmarkMenuAction(QObject* parent)
        : ConstructContextMenuAction("Add bookmark", parent)
    {
    }

    ContextMenuAction::SceneReaction AddBookmarkMenuAction::TriggerAction(const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::ClearSelection);

        AZ::Entity* bookmarkEntity = nullptr;
        GraphCanvasRequestBus::BroadcastResult(bookmarkEntity, &GraphCanvasRequests::CreateBookmarkAnchorAndActivate);

        AZ_Assert(bookmarkEntity, "Unable to create GraphCanvas Bookmark Entity");

        if (bookmarkEntity)
        {
            SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::AddBookmarkAnchor, bookmarkEntity->GetId(), scenePos);
        }

        if (bookmarkEntity)
        {
            return SceneReaction::PostUndo;
        }
        else
        {
            return SceneReaction::Nothing;
        }
    }
}