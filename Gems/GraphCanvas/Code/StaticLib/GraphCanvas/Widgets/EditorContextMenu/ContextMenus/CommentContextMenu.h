/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentActionsMenuGroup.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditActionsMenuGroup.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupActionsMenuGroup.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/CommentMenuActions/CommentActionsMenuGroup.h>

namespace GraphCanvas
{
    class CommentContextMenu
        : public EditorContextMenu
    {
    public:
        AZ_CLASS_ALLOCATOR(CommentContextMenu, AZ::SystemAllocator)
        CommentContextMenu(EditorId editorId, QWidget* parent = nullptr);
        ~CommentContextMenu() override = default;
        
    protected:
    
        void OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId) override;

    private:

        EditActionsMenuGroup m_editActionGroup;
        CommentActionsMenuGroup m_commentActionGroup;
        NodeGroupActionsMenuGroup m_nodeGroupActionGroup;
        AlignmentActionsMenuGroup m_alignmentActionGroup;

        ContextMenuAction* m_createPresetFrom;
        ApplyCommentPresetMenuActionGroup m_applyCommentPresets;
    };
}
