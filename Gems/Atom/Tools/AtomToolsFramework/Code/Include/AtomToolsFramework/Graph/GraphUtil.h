/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/tuple.h>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>

namespace AtomToolsFramework
{
    //! Get the value from a slot as a string
    AZStd::string GetStringValueFromSlot(GraphModel::ConstSlotPtr slot, const AZStd::string& defaultValue = {});

    // Sort a container of nodes by depth, considering the number and state of input and output slots and connections, for consistent
    // display and execution order. The function is templatized to account for different container types and constness. 
    template<typename NodeContainer>
    void SortNodesInExecutionOrder(NodeContainer& nodes)
    {
        using NodeValueTypeRef = typename NodeContainer::const_reference;

        // Pre-calculate and cache sorting scores for all nodes to avoid reprocessing during the sort.
        //
        // The node ID is the last component so that the ordering is total. Everything before it ties readily: two nodes with the same
        // slot shape at the same depth score identically, which is the ordinary case for sibling branches of a graph. The sort below is
        // stable, so a tie leaves them in whatever order the caller supplied, and callers build their containers by iterating
        // GraphModel::Graph::GetNodes(), which is an unordered_map. Inserting any node rehashes it, siblings come back the other way
        // round, and the generated shader source changes line order without the graph meaning anything different. The Asset Processor
        // sees a changed file and rebuilds every shader, so dropping an unconnected node on the canvas triggered a full recompile.
        AZStd::mutex nodeScoreMapMutex;
        AZStd::unordered_map<GraphModel::NodeId, AZStd::tuple<bool, bool, uint32_t, GraphModel::NodeId>> nodeScoreMap;
        nodeScoreMap.reserve(nodes.size());

        AZ::parallel_for_each(nodes.begin(), nodes.end(), [&](NodeValueTypeRef node) {
            AZStd::scoped_lock lock(nodeScoreMapMutex);
            nodeScoreMap.emplace(
                node->GetId(),
                AZStd::make_tuple(node->HasInputSlots(), !node->HasOutputSlots(), node->GetMaxInputDepth(), node->GetId()));
        });

        AZStd::stable_sort(nodes.begin(), nodes.end(), [&](NodeValueTypeRef nodeA, NodeValueTypeRef nodeB) {
            return nodeScoreMap[nodeA->GetId()] < nodeScoreMap[nodeB->GetId()];
        });
    }
} // namespace AtomToolsFramework
