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
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    class EditActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(EditActionsMenuGroup, AZ::SystemAllocator, 0);
        
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