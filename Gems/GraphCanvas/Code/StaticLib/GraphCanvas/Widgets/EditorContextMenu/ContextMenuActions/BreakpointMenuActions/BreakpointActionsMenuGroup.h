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
    class BreakpointActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(BreakpointActionsMenuGroup, AZ::SystemAllocator);

        BreakpointActionsMenuGroup() = default;
        ~BreakpointActionsMenuGroup() = default;

        void PopulateMenu(EditorContextMenu* contextMenu);

    private:
        ContextMenuAction* m_addBreakpointAction;
    };
} // namespace GraphCanvas
