/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    class NodeGroupActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeGroupActionsMenuGroup, AZ::SystemAllocator);
        
        NodeGroupActionsMenuGroup();
        ~NodeGroupActionsMenuGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);

        void RefreshPresets();
    
        void SetUngroupNodesEnabled(bool enabled);
        void SetCollapseGroupEnabled(bool enabled);
        void SetExpandGroupEnabled(bool enabled);
        
    private:
    
        CreateNodeGroupPresetMenuActionGroup m_createNodeGroupPreset;
        ContextMenuAction* m_ungroupAction;
        ContextMenuAction* m_collapseAction;
        ContextMenuAction* m_expandAction;
    };
}
