/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
