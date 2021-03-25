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
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/utils.h>

#include <ScriptCanvas/Variable/VariableCore.h>

namespace AZ
{
    class EntityId;
}

namespace ScriptCanvas
{
    class Datum;
    class Endpoint;
    class Graph;
    class Slot;

    using ReplacementEndpointPairs = AZStd::unordered_set<AZStd::pair<ScriptCanvas::Endpoint, ScriptCanvas::Endpoint>>;
    using ReplacementConnectionMap = AZStd::unordered_map<AZ::EntityId, ReplacementEndpointPairs>;

    class VersioningUtils
    {
    public:
        static void CopyOldValueToDataSlot(Slot* newSlot, const VariableId& oldVariableReference, const Datum* oldDatum);

        static void CreateRemapConnectionsForSourceEndpoint(const Graph& graph, const Endpoint& oldSourceEndpoint, const Endpoint& newSourceEndpoint, ReplacementConnectionMap&);
        static void CreateRemapConnectionsForTargetEndpoint(const Graph& graph, const Endpoint& oldTargetEndpoint, const Endpoint& newTargetEndpoint, ReplacementConnectionMap&);
    };
}
