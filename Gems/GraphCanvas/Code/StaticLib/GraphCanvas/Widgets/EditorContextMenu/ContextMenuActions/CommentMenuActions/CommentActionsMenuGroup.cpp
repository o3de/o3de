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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/CommentMenuActions/CommentActionsMenuGroup.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/CommentMenuActions/CommentContextMenuActions.h>

namespace GraphCanvas
{
    ////////////////////////////
    // CommentActionsMenuGroup
    ////////////////////////////
    
    CommentActionsMenuGroup::CommentActionsMenuGroup()
        : m_editCommentAction(nullptr)
    {

    }
    
    CommentActionsMenuGroup::~CommentActionsMenuGroup()
    {
    }

    void CommentActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(CommentContextMenuAction::GetCommentContextMenuActionGroupId());

        m_editCommentAction = aznew EditCommentMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_editCommentAction);
    }

    void CommentActionsMenuGroup::SetEditCommentActionEnabled(bool enabled)
    {
        m_editCommentAction->setEnabled(enabled);
    }
}
