/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentActionsMenuGroup.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/GraphCanvasConstructActionsMenuGroup.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/EditMenuActions/EditActionsMenuGroup.h>

namespace GraphCanvas
{
    class SceneContextMenu
        : public EditorContextMenu
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneContextMenu, AZ::SystemAllocator)
        SceneContextMenu(EditorId editorId, QWidget* parent = nullptr);
        ~SceneContextMenu() override = default;

        void OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetMemberId) override;

    protected:

        EditActionsMenuGroup m_editorActionsGroup;
        GraphCanvasConstructActionsMenuGroup m_graphCanvasConstructGroups;
        CreateNodeGroupPresetMenuActionGroup m_nodeGroupPresets;
        AlignmentActionsMenuGroup m_alignmentActionsGroups;
    };
}
