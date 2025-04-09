/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(GraphCanvasConstructActionsMenuGroup, AZ::SystemAllocator);
        
        GraphCanvasConstructActionsMenuGroup();
        ~GraphCanvasConstructActionsMenuGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);
        void RefreshGroup();

        void SetAddBookmarkEnabled(bool enabled);
        void SetCommentsEnabled(bool enabled);
        
    private:
    
        ContextMenuAction* m_createBookmark;
        CreateCommentPresetMenuActionGroup m_commentPresets;
    };
}
