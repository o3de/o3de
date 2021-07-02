/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/BookmarkContextMenu.h>

namespace GraphCanvas
{
    ////////////////////////
    // BookmarkContextMenu
    ////////////////////////

    BookmarkContextMenu::BookmarkContextMenu(EditorId editorId, QWidget* parent)
        : GraphCanvas::EditorContextMenu(editorId, parent)
    {
        m_editActionMenuGroup.PopulateMenu(this);
    }

    void BookmarkContextMenu::OnRefreshActions(const GraphCanvas::GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);

        m_editActionMenuGroup.SetPasteEnabled(false);
    }
}
