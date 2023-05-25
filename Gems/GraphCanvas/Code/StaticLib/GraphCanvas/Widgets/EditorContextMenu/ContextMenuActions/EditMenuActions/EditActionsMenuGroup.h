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
    class EditActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(EditActionsMenuGroup, AZ::SystemAllocator);
        
        EditActionsMenuGroup();
        ~EditActionsMenuGroup();

        void PopulateMenu(EditorContextMenu* parent);
    
        void SetCutEnabled(bool enabled);
        void SetCopyEnabled(bool enabled);
        void SetPasteEnabled(bool enabled);
        void SetDeleteEnabled(bool enabled);
        void SetDuplicateEnabled(bool enabled);
        
    private:
    
        ContextMenuAction* m_cutAction;
        ContextMenuAction* m_copyAction;
        ContextMenuAction* m_pasteAction;
        ContextMenuAction* m_deleteAction;
        ContextMenuAction* m_duplicateAction;
    };
}
