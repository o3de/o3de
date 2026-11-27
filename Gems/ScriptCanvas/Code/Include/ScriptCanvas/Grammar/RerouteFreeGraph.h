/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/GraphData.h>

namespace ScriptCanvas
{
    class Graph;

    namespace Grammar
    {
        //! Builds the graph's endpoint map with every reroute spliced out: a chain becomes direct connections, and a
        //! chain that reaches no real node, or ends at a disabled reroute, leaves its real endpoint unconnected.
        AZStd::unordered_multimap<Endpoint, Endpoint> MakeRerouteFreeEndpointMap(const GraphData& graphData);

        //! While alive, the parser and translator see the graph without reroutes: they leave the node list, and
        //! connection queries answer from MakeRerouteFreeEndpointMap. Both are restored on destruction.
        class ScopedRerouteFreeGraph
        {
        public:
            explicit ScopedRerouteFreeGraph(const Graph* graph);
            ~ScopedRerouteFreeGraph();

            ScopedRerouteFreeGraph(const ScopedRerouteFreeGraph&) = delete;
            ScopedRerouteFreeGraph& operator=(const ScopedRerouteFreeGraph&) = delete;

        private:
            GraphData* m_graphData = nullptr;
            AZStd::vector<AZ::Entity*> m_reroutes;
            AZStd::unordered_multimap<Endpoint, Endpoint> m_editorEndpointMap;
        };
    } // namespace Grammar
} // namespace ScriptCanvas
