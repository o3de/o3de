/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/BreakpointMenuActions/BreakpointActionsMenuGroup.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/BreakpointMenuActions/BreakpointContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/BreakpointMenuActions/BreakpointContextMenuActions.h>

namespace GraphCanvas
{
    void BreakpointActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(BreakpointContextMenuAction::GetBreakpointContextMenuActionGroupId());

        m_addBreakpointAction = aznew AddBreakpointMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_addBreakpointAction);
    }
} // namespace GraphCanvas
