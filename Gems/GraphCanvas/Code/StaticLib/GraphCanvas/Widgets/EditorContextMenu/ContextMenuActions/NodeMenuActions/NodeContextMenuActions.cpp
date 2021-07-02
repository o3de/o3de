/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeMenuActions/NodeContextMenuActions.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // ManageUnusedSlotsMenuAction
    ////////////////////////////////
    
    ManageUnusedSlotsMenuAction::ManageUnusedSlotsMenuAction(QObject* parent, bool hideSlots)
        : NodeContextMenuAction(hideSlots ? "Hide Unused Slots" : "Show Unused Slots", parent)
        , m_hideSlots(hideSlots)
    {
        
    }
    
    void ManageUnusedSlotsMenuAction::RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId)
    {
        if (m_hideSlots)
        {
            bool hasHideableSlots = true;
            NodeRequestBus::EventResult(hasHideableSlots, targetId, &NodeRequests::HasHideableSlots);
            setEnabled(hasHideableSlots);
        }
        else
        {
            setEnabled(false);

            AZStd::vector<AZ::EntityId> selectedNodes;
            SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

            for (auto nodeId : selectedNodes)
            {
                bool isHidingUnusedSlots = false;
                NodeRequestBus::EventResult(isHidingUnusedSlots, nodeId, &NodeRequests::IsHidingUnusedSlots);

                if (isHidingUnusedSlots)
                {
                    setEnabled(true);
                    break;
                }
            }
        }
    }
    
    NodeContextMenuAction::SceneReaction ManageUnusedSlotsMenuAction::TriggerAction(const GraphId& graphId, const AZ::Vector2& /*scenePos*/)
    {
        AZStd::vector<AZ::EntityId> selectedNodes;
        SceneRequestBus::EventResult(selectedNodes, graphId, &SceneRequests::GetSelectedNodes);

        for (auto nodeId : selectedNodes)
        {
            if (m_hideSlots)
            {
                NodeRequestBus::Event(nodeId, &NodeRequests::HideUnusedSlots);
            }
            else
            {
                NodeRequestBus::Event(nodeId, &NodeRequests::ShowAllSlots);
            }
        }

        if (selectedNodes.empty())
        {
            return SceneReaction::Nothing;
        }

        return SceneReaction::PostUndo;
    }
}
