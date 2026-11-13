/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_set.h>
#include <Editor/Core/ConnectionReconciliation.h>
#include <Editor/Nodes/BaseNode.h>
#include <Editor/Nodes/RerouteNode.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Slot.h>

namespace LandscapeCanvasEditor
{
    bool IsVisualOnlyNode(const GraphModel::NodePtr& node)
    {
        const auto* baseNode = azrtti_cast<const LandscapeCanvas::BaseNode*>(node.get());
        return baseNode && baseNode->IsVisualOnly();
    }

    ResolvedConnectionSource ResolveConnectionSource(GraphModel::SlotPtr sourceSlot)
    {
        AZStd::unordered_set<const GraphModel::Node*> visited;
        while (sourceSlot)
        {
            GraphModel::NodePtr sourceNode = sourceSlot->GetParentNode();
            if (!IsVisualOnlyNode(sourceNode))
            {
                return { sourceNode, sourceSlot };
            }

            if (!visited.insert(sourceNode.get()).second)
            {
                return {};
            }

            GraphModel::SlotPtr inputSlot = sourceNode->GetSlot(LandscapeCanvas::RerouteNode::IN_SLOT_ID);
            if (!inputSlot)
            {
                return {};
            }

            sourceSlot.reset();
            for (const auto& connection : inputSlot->GetConnections())
            {
                if (connection && connection->GetTargetSlot() == inputSlot)
                {
                    sourceSlot = connection->GetSourceSlot();
                    break;
                }
            }
        }

        return {};
    }

    GraphModel::Graph::ConnectionList ReconcileConnections(
        GraphModel::ConstNodePtr node,
        ConnectionsList& expectedConnections)
    {
        GraphModel::Graph::ConnectionList obsolete;
        for (const auto& [slotId, slot] : node->GetSlots())
        {
            if (!slot->Is(GraphModel::SlotDirection::Input, GraphModel::SlotType::Data))
            {
                continue;
            }

            const auto connections = slot->GetConnections();
            if (connections.empty())
            {
                continue;
            }

            const auto& connection = *connections.begin();
            const ResolvedConnectionSource source = ResolveConnectionSource(connection->GetSourceSlot());
            const auto expected = AZStd::find_if(
                expectedConnections.begin(),
                expectedConnections.end(),
                [&slot](const auto& endpoints)
                {
                    return endpoints.second.second == slot;
                });

            if (expected != expectedConnections.end()
                && expected->first.first == source.m_node
                && expected->first.second == source.m_slot)
            {
                expectedConnections.erase(expected);
            }
            else if (expected == expectedConnections.end()
                && !source.m_node
                && IsVisualOnlyNode(connection->GetSourceNode()))
            {
                // Incomplete routing is editor data with no corresponding component property.
                continue;
            }
            else
            {
                obsolete.push_back(connection);
            }
        }

        return obsolete;
    }
} // namespace LandscapeCanvasEditor
