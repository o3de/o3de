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
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneActionsMenuGroup.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuAction.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

namespace GraphCanvas
{
    //////////////////////////
    // SceneActionsMenuGroup
    //////////////////////////
    
    SceneActionsMenuGroup::SceneActionsMenuGroup(EditorContextMenu* contextMenu)
        : m_removeUnusedNodesAction(nullptr)
    {
        contextMenu->AddActionGroup(SceneContextMenuAction::GetSceneContextMenuActionGroupId());

        m_removeUnusedNodesAction = aznew RemoveUnusedNodesMenuAction(contextMenu);
        contextMenu->AddMenuAction(m_removeUnusedNodesAction);
    }
    
    SceneActionsMenuGroup::~SceneActionsMenuGroup()
    {
    }
    
    void SceneActionsMenuGroup::SetCleanUpGraphEnabled(bool enabled)
    {
        m_removeUnusedNodesAction->setEnabled(enabled);
    }    
}