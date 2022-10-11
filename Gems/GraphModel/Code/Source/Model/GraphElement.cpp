/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphElement.h>

namespace GraphModel
{

    GraphElement::GraphElement(GraphPtr graph) : m_graph(graph)
    {
    }

    GraphPtr GraphElement::GetGraph() const
    {
        return m_graph.lock();
    }

    GraphContextPtr GraphElement::GetGraphContext() const
    {
        GraphPtr graph = m_graph.lock();
        return graph ? graph->GetContext() : nullptr;
    }

} // namespace GraphModel
