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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupActionsMenuGroup.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/AlignmentMenuActions/AlignmentActionsMenuGroup.h>

namespace GraphCanvas
{
    class NodeGroupContextMenu
        : public EditorContextMenu
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeGroupContextMenu, AZ::SystemAllocator)
        NodeGroupContextMenu(EditorId editorId, QWidget* parent = nullptr);
        ~NodeGroupContextMenu() override = default;
        
    protected:
    
        void OnRefreshActions(const GraphId& graphId, const AZ::EntityId& targetId) override;

    private:

        EditActionsMenuGroup m_editActionsGroup;
        NodeGroupActionsMenuGroup m_nodeGroupActionsGroup;
        ApplyNodeGroupPresetMenuActionGroup m_applyNodeGroupPresets;
        AlignmentActionsMenuGroup m_alignmentActionsGroup;

        ContextMenuAction* m_createPresetFrom;
    };
}
