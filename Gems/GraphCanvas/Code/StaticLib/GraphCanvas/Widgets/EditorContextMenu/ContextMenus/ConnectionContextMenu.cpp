/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>

namespace GraphCanvas
{
    //////////////////////////
    // ConnectionContextMenu
    //////////////////////////
    
    ConnectionContextMenu::ConnectionContextMenu(EditorId editorId, QWidget* parent)
        : EditorContextMenu(editorId, parent)
    {
        m_editActionsGroup.PopulateMenu(this);

        m_alignmentActionsGroup.PopulateMenu(this);
    }
    
    void ConnectionContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);

        m_editActionsGroup.SetCutEnabled(false);
        m_editActionsGroup.SetCopyEnabled(false);
        m_editActionsGroup.SetPasteEnabled(false);
        m_editActionsGroup.SetDuplicateEnabled(false);
    }
}
