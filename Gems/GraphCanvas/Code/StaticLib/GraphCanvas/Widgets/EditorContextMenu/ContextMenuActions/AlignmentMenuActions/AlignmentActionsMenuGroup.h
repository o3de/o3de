/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    class AlignmentActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(AlignmentActionsMenuGroup, AZ::SystemAllocator);
        
        AlignmentActionsMenuGroup();
        ~AlignmentActionsMenuGroup() = default;

        void PopulateMenu(EditorContextMenu* contextMenu);

        void SetEnabled(bool enabled);
        
    private:

        EditorContextMenu* m_editorMenu;
    
        ContextMenuAction* m_alignTop;
        ContextMenuAction* m_alignBottom;
        ContextMenuAction* m_alignRight;
        ContextMenuAction* m_alignLeft;
    };
}
