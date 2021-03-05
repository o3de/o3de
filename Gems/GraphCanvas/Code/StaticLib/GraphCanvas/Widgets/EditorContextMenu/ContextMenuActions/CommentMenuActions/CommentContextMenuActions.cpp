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

    ContextMenuAction::SceneReaction EditCommentMenuAction::TriggerAction([[maybe_unused]] const AZ::Vector2& scenePos)
    {
        const AZ::EntityId& targetId = GetTargetId();

        CommentUIRequestBus::Event(targetId, &CommentUIRequests::SetEditable, true);

        return SceneReaction::Nothing;
    }
}
