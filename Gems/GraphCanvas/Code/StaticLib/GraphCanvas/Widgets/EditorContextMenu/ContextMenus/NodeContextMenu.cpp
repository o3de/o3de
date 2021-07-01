/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeMenuActions/NodeContextMenuActions.h>

namespace GraphCanvas
{
    ////////////////////
    // NodeContextMenu
    ////////////////////
    
    NodeContextMenu::NodeContextMenu(EditorId editorId, QWidget* parent)
        : EditorContextMenu(editorId, parent)
    {
        m_editActionGroup.PopulateMenu(this);
        m_nodeGroupActionGroup.PopulateMenu(this);
        m_disableActionGroup.PopulateMenu(this);
        m_alignmentActionGroup.PopulateMenu(this);

        AddActionGroup(NodeContextMenuAction::GetNodeContextMenuActionGroupId());

        const bool showUnusedSlots = false;
        const bool hideUnusedSlots = true;

        AddMenuAction(aznew ManageUnusedSlotsMenuAction(this, showUnusedSlots));
        AddMenuAction(aznew ManageUnusedSlotsMenuAction(this, hideUnusedSlots));
    }
    
    void NodeContextMenu::OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId)
    {
        AZ_UNUSED(graphId);
        AZ_UNUSED(targetMemberId);

        m_editActionGroup.SetPasteEnabled(false);

        m_nodeGroupActionGroup.RefreshPresets();
        m_disableActionGroup.RefreshActions(graphId);
    }
}
