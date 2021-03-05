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
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ConstructMenuActions/ConstructPresetMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    class NodeGroupActionsMenuGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(NodeGroupActionsMenuGroup, AZ::SystemAllocator, 0);
        
        NodeGroupActionsMenuGroup();
        ~NodeGroupActionsMenuGroup();

        void PopulateMenu(EditorContextMenu* contextMenu);

        void RefreshPresets();
    
        void SetUngroupNodesEnabled(bool enabled);
        void SetCollapseGroupEnabled(bool enabled);
        void SetExpandGroupEnabled(bool enabled);
        
    private:
    
        NodeGroupPresetsMenuActionGroup m_nodeGroupPresets;
        ContextMenuAction* m_ungroupAction;
        ContextMenuAction* m_collapseAction;
        ContextMenuAction* m_expandAction;
    };
}