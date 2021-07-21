
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/SceneMenuActions/SceneContextMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    ///////////////////////////////////
    // RemoveUnusedElementsMenuAction
    ///////////////////////////////////
    
    RemoveUnusedElementsMenuAction::RemoveUnusedElementsMenuAction(QObject* parent)
        : SceneContextMenuAction("All", parent)
    {
        setToolTip("Removes all of the unused elements from the active graph");
    }

    bool RemoveUnusedElementsMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string RemoveUnusedElementsMenuAction::GetSubMenuPath() const
    {
        return "Remove Unused";
    }

    ContextMenuAction::SceneReaction RemoveUnusedElementsMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::RemoveUnusedElements);
        return SceneReaction::PostUndo;
    }

    ////////////////////////////////
    // RemoveUnusedNodesMenuAction
    ////////////////////////////////

    RemoveUnusedNodesMenuAction::RemoveUnusedNodesMenuAction(QObject* parent)
        : SceneContextMenuAction("Nodes", parent)
    {
        setToolTip("Removes all of the unused nodes from the active graph");
    }

    bool RemoveUnusedNodesMenuAction::IsInSubMenu() const
    {
        return true;
    }

    AZStd::string RemoveUnusedNodesMenuAction::GetSubMenuPath() const
    {
        return "Remove Unused";
    }

    ContextMenuAction::SceneReaction RemoveUnusedNodesMenuAction::TriggerAction(const AZ::Vector2& /*scenePos*/)
    {
        const GraphId& graphId = GetGraphId();

        SceneRequestBus::Event(graphId, &SceneRequests::RemoveUnusedNodes);
        return SceneReaction::PostUndo;
    }
}
