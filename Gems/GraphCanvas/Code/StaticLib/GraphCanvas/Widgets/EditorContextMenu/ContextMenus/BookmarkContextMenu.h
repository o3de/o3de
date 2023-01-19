/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditActionsMenuGroup.h>

namespace GraphCanvas
{
    class BookmarkContextMenu
        : public EditorContextMenu
    {
    public:
        AZ_CLASS_ALLOCATOR(BookmarkContextMenu, AZ::SystemAllocator)
        BookmarkContextMenu(EditorId editorId, QWidget* parent = nullptr);
        ~BookmarkContextMenu() override = default;

    protected:

        void OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetId) override;

        EditActionsMenuGroup m_editActionMenuGroup;
    };
}
