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

    void SceneContextMenu::OnRefreshActions([[maybe_unused]] const GraphId& graphId, [[maybe_unused]] const AZ::EntityId& targetMemberId)
    {
        m_graphCanvasConstructGroups.RefreshGroup();
        m_nodeGroupPresets.RefreshPresets();
    }
}