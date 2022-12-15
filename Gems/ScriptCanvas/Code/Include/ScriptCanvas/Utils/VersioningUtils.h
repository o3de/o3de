/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/utils.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace AZ
{
    class EntityId;
}

namespace ScriptCanvas
{
    class Datum;
    class Graph;
    class Slot;

    using ReplacementEndpointPairs = AZStd::unordered_set<AZStd::pair<Endpoint, Endpoint>>;
    using ReplacementConnectionMap = AZStd::unordered_map<AZ::EntityId, ReplacementEndpointPairs>;

    struct NodeUpdateReport
    {
        ScriptCanvas::Node* m_newNode = nullptr;
        AZStd::unordered_set<ScriptCanvas::SlotId> m_deletedOldSlots;
        AZStd::unordered_map<ScriptCanvas::SlotId, AZStd::vector<ScriptCanvas::SlotId>> m_oldSlotsToNewSlots;

        void Clear();
        bool IsEmpty() const;
    };

    struct GraphUpdateReport
    {
        AZStd::unordered_set<Endpoint> m_deletedOldSlots;
        AZStd::unordered_map<Endpoint, AZStd::vector<Endpoint>> m_oldSlotsToNewSlots;

        AZStd::vector<Endpoint> Convert(const Endpoint& oldEndpoint) const;

        bool IsEmpty() const;
    };

    void MergeUpdateSlotReport(const AZ::EntityId& scriptCanvasNodeId, GraphUpdateReport& report, const NodeUpdateReport& source);

    AZStd::vector<AZStd::pair<Endpoint, Endpoint>> CollectEndpoints(const AZStd::vector<AZ::Entity*>& connections, bool logEntityNames = false);

    void UpdateConnectionStatus(Graph& graph, const GraphUpdateReport& report);

    class VersioningUtils
    {
    public:
        static void CopyOldValueToDataSlot(Slot* newSlot, const VariableId& oldVariableReference, const Datum* oldDatum);

        static void CreateRemapConnectionsForSourceEndpoint(const Graph& graph, const Endpoint& oldSourceEndpoint, const Endpoint& newSourceEndpoint, ReplacementConnectionMap&);
        static void CreateRemapConnectionsForTargetEndpoint(const Graph& graph, const Endpoint& oldTargetEndpoint, const Endpoint& newTargetEndpoint, ReplacementConnectionMap&);
    };
}
