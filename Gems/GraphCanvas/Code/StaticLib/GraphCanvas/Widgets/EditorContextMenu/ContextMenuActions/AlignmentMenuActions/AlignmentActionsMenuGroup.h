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
    class AlignmentActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(AlignmentActionsMenuGroup, AZ::SystemAllocator, 0);
        
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
