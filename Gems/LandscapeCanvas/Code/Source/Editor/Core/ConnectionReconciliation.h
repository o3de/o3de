/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphModel/Model/Graph.h>

namespace LandscapeCanvasEditor
{
    using NodeSlotPair = AZStd::pair<GraphModel::NodePtr, GraphModel::SlotPtr>;
    using ConnectionsList = AZStd::vector<AZStd::pair<NodeSlotPair, NodeSlotPair>>;

    struct ResolvedConnectionSource
    {
        GraphModel::NodePtr m_node;
        GraphModel::SlotPtr m_slot;
    };

    bool IsVisualOnlyNode(const GraphModel::NodePtr& node);

    ResolvedConnectionSource ResolveConnectionSource(GraphModel::SlotPtr sourceSlot);

    // Leaves only missing connections in expectedConnections and returns obsolete existing wires.
    GraphModel::Graph::ConnectionList ReconcileConnections(
        GraphModel::ConstNodePtr node,
        ConnectionsList& expectedConnections);
} // namespace LandscapeCanvasEditor
