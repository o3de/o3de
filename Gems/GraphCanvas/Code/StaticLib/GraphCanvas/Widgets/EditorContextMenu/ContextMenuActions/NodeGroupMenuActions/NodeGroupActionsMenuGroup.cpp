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

        m_nodeGroupPresets.PopulateMenu(contextMenu);

        m_ungroupAction = aznew UngroupNodeGroupMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_ungroupAction);

        m_collapseAction = aznew CollapseNodeGroupMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_collapseAction);

        m_expandAction = aznew ExpandNodeGroupMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_expandAction);
    }

    void NodeGroupActionsMenuGroup::RefreshPresets()
    {
        m_nodeGroupPresets.RefreshPresets();
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