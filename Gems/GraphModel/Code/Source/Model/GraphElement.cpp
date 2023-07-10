/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/GraphElement.h>

namespace GraphModel
{
    void GraphElement::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphElement>()
                ->Version(0)
                ;

            serializeContext->RegisterGenericType<GraphElementPtr>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GraphElement>("GraphModelGraphElement")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Method("GetGraph", &GraphElement::GetGraph)
                ->Method("GetGraphContext", &GraphElement::GetGraphContext)
                ;
        }
    }

    GraphElement::GraphElement(GraphPtr graph)
        : m_graph(graph)
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
