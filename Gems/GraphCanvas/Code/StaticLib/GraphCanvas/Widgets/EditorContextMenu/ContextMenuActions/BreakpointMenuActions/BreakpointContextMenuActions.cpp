/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/BreakpointMenuActions/BreakpointContextMenuActions.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

namespace GraphCanvas
{
    //////////////////////////////
    // AddBreakpointMenuAction
    //////////////////////////////

    AddBreakpointMenuAction::AddBreakpointMenuAction(QObject* parent)
        : BreakpointContextMenuAction("Add Breakpoint", parent)
    {
    }

    ContextMenuAction::SceneReaction AddBreakpointMenuAction::TriggerAction(
        const GraphId& graphId, [[maybe_unused]] const AZ::Vector2& scenePos)
    {
        AZStd::vector<AZ::EntityId> selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

        if (selectedNodes.empty())
        {
            return SceneReaction::Nothing;
        }

        AZStd::unordered_set<NodeId> nodeIds;
        for (const AZ::EntityId& nodeId : selectedNodes)
        {
            nodeIds.insert(nodeId);
        }

        GraphModelRequestBus::Event(graphId, &GraphModelRequests::AddBreakpoints, nodeIds);
        return SceneReaction::PostUndo;
    }

} // namespace GraphCanvas
