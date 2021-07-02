/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupActionsMenuGroup.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuActions.h>

namespace GraphCanvas
{
    //////////////////////////////
    // NodeGroupActionsMenuGroup
    //////////////////////////////
    
    NodeGroupActionsMenuGroup::NodeGroupActionsMenuGroup()
        : m_ungroupAction(nullptr)
        , m_collapseAction(nullptr)
        , m_expandAction(nullptr)
    {
    }
    
    NodeGroupActionsMenuGroup::~NodeGroupActionsMenuGroup()
    {
    }

    void NodeGroupActionsMenuGroup::PopulateMenu(EditorContextMenu* contextMenu)
    {
        contextMenu->AddActionGroup(ConstructContextMenuAction::GetConstructContextMenuActionGroupId());
        contextMenu->AddActionGroup(NodeGroupContextMenuAction::GetNodeGroupContextMenuActionGroupId());        

        m_createNodeGroupPreset.PopulateMenu(contextMenu);

        m_ungroupAction = aznew UngroupNodeGroupMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_ungroupAction);

        m_collapseAction = aznew CollapseNodeGroupMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_collapseAction);

        m_expandAction = aznew ExpandNodeGroupMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_expandAction);
    }

    void NodeGroupActionsMenuGroup::RefreshPresets()
    {
        m_createNodeGroupPreset.RefreshPresets();
    }

    void NodeGroupActionsMenuGroup::SetUngroupNodesEnabled(bool enabled)
    {
        m_ungroupAction->setEnabled(enabled);
    }

    void NodeGroupActionsMenuGroup::SetCollapseGroupEnabled(bool enabled)
    {
        m_collapseAction->setEnabled(enabled);
    }

    void NodeGroupActionsMenuGroup::SetExpandGroupEnabled(bool enabled)
    {
        m_expandAction->setEnabled(enabled);
    }
}
