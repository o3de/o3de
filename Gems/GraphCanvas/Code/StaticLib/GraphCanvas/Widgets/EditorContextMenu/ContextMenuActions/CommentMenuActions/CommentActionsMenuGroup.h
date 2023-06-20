/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    class CommentActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(CommentActionsMenuGroup, AZ::SystemAllocator);
        
        CommentActionsMenuGroup();
        ~CommentActionsMenuGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);
    
        void SetEditCommentActionEnabled(bool enabled);
        
    private:    

        ContextMenuAction* m_editCommentAction;        
    };
}
