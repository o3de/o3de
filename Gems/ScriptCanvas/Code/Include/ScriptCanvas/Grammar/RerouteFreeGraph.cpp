/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RerouteFreeGraph.h"

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <ScriptCanvas/Core/Connection.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas::Grammar
{
    namespace RerouteFreeGraphCpp
    {
        using EndpointMap = AZStd::unordered_multimap<Endpoint, Endpoint>;
        using NodesById = AZStd::unordered_map<AZ::EntityId, const Node*>;

        NodesById CollectNodes(const GraphData& graphData)
        {
            NodesById nodes;
            for (AZ::Entity* entity : graphData.m_nodes)
            {
                if (entity)
                {
                    if (const Node* node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(entity))
                    {
                        nodes.emplace(entity->GetId(), node);
                    }
                }
            }
            return nodes;
        }

        const Node* FindTransparentNode(const NodesById& nodes, const Endpoint& endpoint)
        {
            const auto found = nodes.find(endpoint.GetNodeId());
            return found != nodes.end() && found->second->IsConnectionTransparentForTranslation() ? found->second : nullptr;
        }

        bool HasTransparentNode(const NodesById& nodes)
        {
            return AZStd::any_of(nodes.begin(), nodes.end(), [](const auto& entry)
            {
                return entry.second->IsConnectionTransparentForTranslation();
            });
        }

        //! Follows a connection that enters a reroute at `endpoint` through the chain, appending the real endpoints it reaches.
        void ResolveThroughReroutes(
            const NodesById& nodes,
            const EndpointMap& endpointMap,
            const Endpoint& endpoint,
            AZStd::unordered_set<Endpoint>& visited,
            AZStd::vector<Endpoint>& resolved)
        {
            if (!visited.insert(endpoint).second)
            {
                return;
            }

            const Node* reroute = FindTransparentNode(nodes, endpoint);
            if (!reroute)
            {
                resolved.push_back(endpoint);
                return;
            }

            const Slot* enteredSlot = reroute->GetSlot(endpoint.GetSlotId());
            if (!enteredSlot || !reroute->IsNodeEnabled())
            {
                return;
            }

            for (const Slot& oppositeSlot : reroute->GetSlots())
            {
                if (!enteredSlot->GetDescriptor().CanConnectTo(oppositeSlot.GetDescriptor()))
                {
                    continue;
                }

                const auto [begin, end] = endpointMap.equal_range(oppositeSlot.GetEndpoint());
                for (auto iter = begin; iter != end; ++iter)
                {
                    ResolveThroughReroutes(nodes, endpointMap, iter->second, visited, resolved);
                }
            }
        }

        bool IsInputEndpoint(const NodesById& nodes, const Endpoint& endpoint)
        {
            const auto found = nodes.find(endpoint.GetNodeId());
            const Slot* slot = found != nodes.end() ? found->second->GetSlot(endpoint.GetSlotId()) : nullptr;
            return slot && slot->IsInput();
        }

        //! Built in connection order like GraphData::BuildEndpointMap, so fan-outs keep the order the user made them in.
        //! A connection through reroutes takes the place of the hop that ends the chain at a real input.
        EndpointMap MakeRerouteFreeEndpointMap(const NodesById& nodes, const GraphData& graphData)
        {
            EndpointMap rerouteFree;
            rerouteFree.rehash(graphData.m_endpointMap.bucket_count());
            AZStd::unordered_set<AZStd::pair<Endpoint, Endpoint>> added;
            auto add = [&](const Endpoint& source, const Endpoint& target, const Endpoint& output, const Endpoint& input)
            {
                // Two routes between the same pair of endpoints are one direct connection.
                if (added.emplace(output, input).second)
                {
                    rerouteFree.emplace(source, target);
                    rerouteFree.emplace(target, source);
                }
            };

            for (AZ::Entity* connectionEntity : graphData.m_connections)
            {
                const auto* connection = connectionEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Connection>(connectionEntity) : nullptr;
                if (!connection)
                {
                    continue;
                }

                const Endpoint& source = connection->GetSourceEndpoint();
                const Endpoint& target = connection->GetTargetEndpoint();
                const bool sourceIsReroute = FindTransparentNode(nodes, source) != nullptr;
                const bool targetIsReroute = FindTransparentNode(nodes, target) != nullptr;
                if (!sourceIsReroute && !targetIsReroute)
                {
                    const bool reversed = IsInputEndpoint(nodes, source);
                    add(source, target, reversed ? target : source, reversed ? source : target);
                    continue;
                }

                if (sourceIsReroute == targetIsReroute)
                {
                    // Reroute to reroute: part of a chain, emitted where the chain reaches a real input.
                    continue;
                }

                const Endpoint& real = sourceIsReroute ? target : source;
                if (!IsInputEndpoint(nodes, real))
                {
                    continue;
                }

                AZStd::unordered_set<Endpoint> visited;
                AZStd::vector<Endpoint> outputs;
                ResolveThroughReroutes(nodes, graphData.m_endpointMap, sourceIsReroute ? source : target, visited, outputs);
                for (const Endpoint& output : outputs)
                {
                    add(output, real, output, real);
                }
            }
            return rerouteFree;
        }
    } // namespace RerouteFreeGraphCpp

    AZStd::unordered_multimap<Endpoint, Endpoint> MakeRerouteFreeEndpointMap(const GraphData& graphData)
    {
        return RerouteFreeGraphCpp::MakeRerouteFreeEndpointMap(RerouteFreeGraphCpp::CollectNodes(graphData), graphData);
    }

    ScopedRerouteFreeGraph::ScopedRerouteFreeGraph(const Graph* graph)
    {
        if (!graph)
        {
            return;
        }

        // The parser takes the graph as const. The graph's data is restored before this scope ends,
        // so the swap is invisible to everything outside the parse.
        GraphData* graphData = const_cast<Graph*>(graph)->GetGraphData();
        if (!graphData)
        {
            return;
        }

        const RerouteFreeGraphCpp::NodesById nodes = RerouteFreeGraphCpp::CollectNodes(*graphData);
        if (!RerouteFreeGraphCpp::HasTransparentNode(nodes))
        {
            // A graph without reroutes is parsed exactly as it is.
            return;
        }

        m_graphData = graphData;
        m_editorEndpointMap = RerouteFreeGraphCpp::MakeRerouteFreeEndpointMap(nodes, *graphData);
        AZStd::swap(m_editorEndpointMap, graphData->m_endpointMap);

        // The parser visits nodes in the node set's (address hashed) order. Erasing the reroutes in place keeps
        // every other node where it was, so the parser sees the same order as for the graph wired directly.
        for (const auto& [nodeId, node] : nodes)
        {
            if (node->IsConnectionTransparentForTranslation())
            {
                m_reroutes.push_back(node->GetEntity());
                graphData->m_nodes.erase(node->GetEntity());
            }
        }
    }

    ScopedRerouteFreeGraph::~ScopedRerouteFreeGraph()
    {
        if (m_graphData)
        {
            m_graphData->m_nodes.insert(m_reroutes.begin(), m_reroutes.end());
            AZStd::swap(m_editorEndpointMap, m_graphData->m_endpointMap);
        }
    }
} // namespace ScriptCanvas::Grammar
