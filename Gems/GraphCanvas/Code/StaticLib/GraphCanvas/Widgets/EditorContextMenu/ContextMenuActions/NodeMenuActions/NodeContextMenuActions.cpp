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
