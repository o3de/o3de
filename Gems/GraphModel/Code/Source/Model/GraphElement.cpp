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

    IGraphContextPtr GraphElement::GetGraphContext() const
    {
        GraphPtr graph = m_graph.lock();
        return graph ? graph->GetContext() : nullptr;
    }

} // namespace GraphModel
