/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VersioningUtils.h"

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Connection.h>

namespace ScriptCanvas
{
    AZStd::vector<Endpoint> GraphUpdateReport::Convert(const Endpoint& oldEndpoint) const
    {
        auto iter = m_oldSlotsToNewSlots.find(oldEndpoint);
        return iter != m_oldSlotsToNewSlots.end() ? iter->second : AZStd::vector<Endpoint>{ oldEndpoint };
    }

    bool GraphUpdateReport::IsEmpty() const
    {
        return m_deletedOldSlots.empty() && m_oldSlotsToNewSlots.empty();
    }

    void NodeUpdateReport::Clear()
    {
        m_newNode = nullptr;
        m_deletedOldSlots.clear();
        m_oldSlotsToNewSlots.clear();
    }

    bool NodeUpdateReport::IsEmpty() const
    {
        return !m_newNode && m_deletedOldSlots.empty() && m_oldSlotsToNewSlots.empty();
    }

    void VersioningUtils::CopyOldValueToDataSlot(Slot* newSlot, const VariableId& oldVariableReference, const Datum* oldDatum)
    {
        if (oldVariableReference.IsValid())
        {
            newSlot->SetVariableReference(oldVariableReference);
        }
        else if (oldDatum && !oldDatum->Empty())
        {
            newSlot->ConvertToValue();
            ScriptCanvas::ModifiableDatumView datumView;
            newSlot->FindModifiableDatumView(datumView);
            if (datumView.IsValid())
            {
                auto newDatumLabel = newSlot->FindDatum()->GetLabel();
                datumView.SetDataType(oldDatum->GetType());
                datumView.HardCopyDatum(*oldDatum);
                datumView.RelabelDatum(newDatumLabel);
            }
        }
    }

    void MergeUpdateSlotReport(const AZ::EntityId& scriptCanvasNodeId, GraphUpdateReport& report, const NodeUpdateReport& source)
    {
        report.m_deletedOldSlots.reserve(source.m_deletedOldSlots.size());

        for (auto& slotId : source.m_deletedOldSlots)
        {
            report.m_deletedOldSlots.insert({ scriptCanvasNodeId, slotId });
        }

        report.m_oldSlotsToNewSlots.reserve(source.m_oldSlotsToNewSlots.size());

        for (auto& oldToNewIter : source.m_oldSlotsToNewSlots)
        {
            AZStd::vector<Endpoint> newEndpoints;
            newEndpoints.reserve(oldToNewIter.second.size());

            for (auto& targetSlotId : oldToNewIter.second)
            {
                newEndpoints.push_back({ scriptCanvasNodeId, targetSlotId });
            }

            report.m_oldSlotsToNewSlots[{ scriptCanvasNodeId, oldToNewIter.first}] = AZStd::move(newEndpoints);
        }
    }

    AZStd::vector<AZStd::pair<Endpoint, Endpoint>> CollectEndpoints(const AZStd::vector<AZ::Entity*>& connections, bool logEntityNames)
    {
        AZStd::vector<AZStd::string> names;
        AZStd::vector<AZStd::pair<Endpoint, Endpoint>> endpoints;

        for (auto& connectionEntity : connections)
        {
            if (logEntityNames)
            {
                names.push_back(connectionEntity->GetName());
            }

            if (auto connection = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Connection>(connectionEntity->GetId()))
            {
                endpoints.push_back(AZStd::make_pair(connection->GetSourceEndpoint(), connection->GetTargetEndpoint()));
            }
        }

        if (logEntityNames)
        {
            AZStd::sort(names.begin(), names.end());

            AZStd::string result = "\nConnection Name list:\n";
            for (auto& name : names)
            {
                result += "\n";
                result += name;
            }

            AZ_TracePrintf("ScriptCanvas", result.c_str());
        }

        return endpoints;
    }

    void UpdateConnectionStatus(Graph& graph, const GraphUpdateReport& report)
    {
         GraphData* graphData = graph.GetGraphData();
         if (!graphData)
         {
             AZ_Error("ScriptCanvas", false, "Graph was missing graph data to update");
             return;
         }
 
         AZStd::unordered_set<SlotId> oldConnectedSlots;
         AZ_TracePrintf("ScriptCanvas", "Connections list before: ");
         auto endpoints = CollectEndpoints(graphData->m_connections, true);
         graph.RemoveAllConnections();

         for (auto& iter : endpoints)
         {
             const AZStd::vector<Endpoint>& sources = report.Convert(iter.first);
             const AZStd::vector<Endpoint>& targets = report.Convert(iter.second);

             for (const auto& source : sources)
             {
                 for (const auto& target : targets)
                 {
                     graph.ConnectByEndpoint(source, target);
                 }
             }
         }

         graphData->BuildEndpointMap();
         AZ_TracePrintf("ScriptCanvas", "Connections list after: ");
         CollectEndpoints(graphData->m_connections, true);
    }

    void VersioningUtils::CreateRemapConnectionsForSourceEndpoint(const Graph& graph, const Endpoint& oldSourceEndpoint, const Endpoint& newSourceEndpoint,
        ReplacementConnectionMap& connectionMap)
    {
        auto otherEndpoints = graph.GetConnectedEndpoints(oldSourceEndpoint);
        for (const auto& otherEndpoint : otherEndpoints)
        {
            AZ::Entity* connection;
            graph.FindConnection(connection, oldSourceEndpoint, otherEndpoint);
            if (connection)
            {
                auto connectionId = connection->GetId();
                if (newSourceEndpoint.IsValid())
                {
                    auto connectionIter = connectionMap.find(connectionId);
                    if (connectionIter != connectionMap.end())
                    {
                        ReplacementEndpointPairs newPairs;
                        for (auto& newEndpointPair : connectionIter->second)
                        {
                            if (newEndpointPair.first == oldSourceEndpoint)
                            {
                                newEndpointPair.first = newSourceEndpoint;
                            }
                            else
                            {
                                newPairs.emplace(AZStd::make_pair(newSourceEndpoint, newEndpointPair.second));
                            }
                        }
                        if (!newPairs.empty())
                        {
                            connectionMap[connectionId].insert(newPairs.begin(), newPairs.end());
                        }
                    }
                    else
                    {
                        connectionMap.emplace(connectionId, ReplacementEndpointPairs{ AZStd::make_pair(newSourceEndpoint, otherEndpoint) });
                    }
                }
                else
                {
                    connectionMap.emplace(connectionId, ReplacementEndpointPairs{});
                }
            }
        }
    }

    void VersioningUtils::CreateRemapConnectionsForTargetEndpoint(const Graph& graph, const Endpoint& oldTargetEndpoint, const Endpoint& newTargetEndpoint,
        ReplacementConnectionMap& connectionMap)
    {
        auto otherEndpoints = graph.GetConnectedEndpoints(oldTargetEndpoint);
        for (const auto& otherEndpoint : otherEndpoints)
        {
            AZ::Entity* connection;
            graph.FindConnection(connection, otherEndpoint, oldTargetEndpoint);
            if (connection)
            {
                auto connectionId = connection->GetId();
                if (newTargetEndpoint.IsValid())
                {
                    auto connectionIter = connectionMap.find(connectionId);
                    if (connectionIter != connectionMap.end())
                    {
                        ReplacementEndpointPairs newPairs;
                        for (auto& newEndpointPair : connectionIter->second)
                        {
                            if (newEndpointPair.second == oldTargetEndpoint)
                            {
                                newEndpointPair.second = newTargetEndpoint;
                            }
                            else
                            {
                                newPairs.emplace(AZStd::make_pair(newEndpointPair.first, newTargetEndpoint));
                            }
                        }
                        if (!newPairs.empty())
                        {
                            connectionMap[connectionId].insert(newPairs.begin(), newPairs.end());
                        }
                    }
                    else
                    {
                        connectionMap.emplace(connectionId, ReplacementEndpointPairs{ AZStd::make_pair(otherEndpoint, newTargetEndpoint) });
                    }
                }
                else
                {
                    connectionMap.emplace(connectionId, ReplacementEndpointPairs{});
                }
            }
        }
    }
}
