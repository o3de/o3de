/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
        using NodeValueType = typename NodeContainer::value_type;
        using NodeValueTypeRef = typename NodeContainer::const_reference;

        // Pre-calculate and cache sorting scores for all nodes to avoid reprocessing during the sort
        AZStd::map<NodeValueType, AZStd::tuple<bool, bool, uint32_t>> nodeScoreTable;
        for (NodeValueTypeRef node : nodes)
        {
            nodeScoreTable.emplace(node, AZStd::make_tuple(node->HasInputSlots(), !node->HasOutputSlots(), node->GetMaxInputDepth()));
        }

        AZStd::stable_sort(nodes.begin(), nodes.end(), [&](NodeValueTypeRef nodeA, NodeValueTypeRef nodeB) {
            return nodeScoreTable[nodeA] < nodeScoreTable[nodeB];
        });
    }
} // namespace AtomToolsFramework
