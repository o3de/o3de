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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/GraphCanvasConstructActionsMenuGroup.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/BookmarkConstructMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/CommentConstructMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>

#include <GraphCanvas/Types/ConstructPresets.h>

namespace GraphCanvas
{
    /////////////////////////////////////////
    // GraphCanvasConstructActionsMenuGroup
    /////////////////////////////////////////

    GraphCanvasConstructActionsMenuGroup::GraphCanvasConstructActionsMenuGroup()
        : m_createBookmark(nullptr)
    {
    }

    GraphCanvasConstructActionsMenuGroup::~GraphCanvasConstructActionsMenuGroup()
    {
    }

    void GraphCanvasConstructActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(ConstructContextMenuAction::GetConstructContextMenuActionGroupId());

        m_createBookmark = aznew AddBookmarkMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_createBookmark);

        m_commentPresets.PopulateMenu(contextMenu);
    }

    void GraphCanvasConstructActionsMenuGroup::RefreshGroup()
    {
        m_commentPresets.RefreshPresets();
    }

    void GraphCanvasConstructActionsMenuGroup::SetAddBookmarkEnabled(bool enabled)
    {
        m_createBookmark->setEnabled(enabled);
    }

    void GraphCanvasConstructActionsMenuGroup::SetCommentsEnabled(bool enabled)
    {
        m_commentPresets.SetEnabled(enabled);
    }
}
