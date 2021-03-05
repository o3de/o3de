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

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>

namespace GraphCanvas
{
    class GraphCanvasConstructActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasConstructActionsMenuGroup, AZ::SystemAllocator, 0);
        
        GraphCanvasConstructActionsMenuGroup();
        ~GraphCanvasConstructActionsMenuGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);
        void RefreshGroup();

        void SetAddBookmarkEnabled(bool enabled);
        void SetCommentsEnabled(bool enabled);
        
    private:
    
        ContextMenuAction* m_createBookmark;
        CommentPresetsMenuActionGroup m_commentPresets;
    };
}
