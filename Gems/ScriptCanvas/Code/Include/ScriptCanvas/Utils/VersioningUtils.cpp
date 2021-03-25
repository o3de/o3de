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
#include "VersioningUtils.h"

#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvas
{
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
