/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
