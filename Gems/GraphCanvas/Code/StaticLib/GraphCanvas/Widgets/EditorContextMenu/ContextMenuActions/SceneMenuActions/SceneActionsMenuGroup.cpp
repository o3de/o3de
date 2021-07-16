/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
