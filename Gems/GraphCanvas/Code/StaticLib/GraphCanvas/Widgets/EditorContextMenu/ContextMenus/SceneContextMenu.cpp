/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuActions.h>

namespace GraphCanvas
{
    /////////////////////
    // SceneContextMenu
    /////////////////////
    
    SceneContextMenu::SceneContextMenu(EditorId editorId, QWidget* parent)
        : EditorContextMenu(editorId, parent)
    {
        m_editorActionsGroup.PopulateMenu(this);
        m_graphCanvasConstructGroups.PopulateMenu(this);        
        m_nodeGroupPresets.PopulateMenu(this);        

        m_alignmentActionsGroups.PopulateMenu(this);
    }

    void SceneContextMenu::OnRefreshActions(const GraphId& /*graphId*/, const AZ::EntityId& /*targetMemberId*/)
    {
        m_graphCanvasConstructGroups.RefreshGroup();
        m_nodeGroupPresets.RefreshPresets();
    }
}
