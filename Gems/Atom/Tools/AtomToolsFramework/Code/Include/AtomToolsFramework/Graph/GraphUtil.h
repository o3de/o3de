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

        // Pre-calculate and cache sorting scores for all nodes to avoid reprocessing during the sort
        AZStd::mutex nodeScoreMapMutex;
        AZStd::unordered_map<GraphModel::NodeId, AZStd::tuple<bool, bool, uint32_t>> nodeScoreMap;
        nodeScoreMap.reserve(nodes.size());

        AZ::parallel_for_each(nodes.begin(), nodes.end(), [&](NodeValueTypeRef node) {
            AZStd::scoped_lock lock(nodeScoreMapMutex);
            nodeScoreMap.emplace(node->GetId(), AZStd::make_tuple(node->HasInputSlots(), !node->HasOutputSlots(), node->GetMaxInputDepth()));
        });

        AZStd::stable_sort(nodes.begin(), nodes.end(), [&](NodeValueTypeRef nodeA, NodeValueTypeRef nodeB) {
            return nodeScoreMap[nodeA->GetId()] < nodeScoreMap[nodeB->GetId()];
        });
    }
} // namespace AtomToolsFramework
