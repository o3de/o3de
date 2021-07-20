/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/CommentMenuActions/CommentContextMenuActions.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/GraphCanvasBus.h>

#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    //////////////////////////
    // EditCommentMenuAction
    //////////////////////////

    EditCommentMenuAction::EditCommentMenuAction(QObject* parent)
        : CommentContextMenuAction("Edit comment", parent)
    {
        setToolTip("Edits the selected comment");
    }

    void EditCommentMenuAction::RefreshAction()
    {
        const AZ::EntityId& targetId = GetTargetId();

        setEnabled(GraphUtils::IsComment(targetId));
    }

    ContextMenuAction::SceneReaction EditCommentMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const AZ::EntityId& targetId = GetTargetId();

        CommentUIRequestBus::Event(targetId, &CommentUIRequests::SetEditable, true);

        return SceneReaction::Nothing;
    }
}
