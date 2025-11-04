/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

// Graph Model
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Graph.h>

namespace GraphModel
{
    Connection::Connection(GraphPtr graph, SlotPtr sourceSlot, SlotPtr targetSlot)
        : GraphElement(graph)
        , m_sourceSlot(sourceSlot)
        , m_targetSlot(targetSlot)
    {
        AZ_Assert(sourceSlot->SupportsConnections(), "sourceSlot type does not support connections to other slots");
        AZ_Assert(targetSlot->SupportsConnections(), "targetSlot type does not support connections to other slots");

        const NodeId sourceNodeId = sourceSlot->GetParentNode()->GetId();
        const NodeId targetNodeId = targetSlot->GetParentNode()->GetId();
        m_sourceEndpoint = AZStd::make_pair(sourceNodeId, sourceSlot->GetSlotId());
        m_targetEndpoint = AZStd::make_pair(targetNodeId, targetSlot->GetSlotId());
    }

    void Connection::PostLoadSetup(GraphPtr graph)
    {
        m_graph = graph;

        m_sourceSlot = azrtti_cast<Slot*>(graph->FindSlot(m_sourceEndpoint));
        m_targetSlot = azrtti_cast<Slot*>(graph->FindSlot(m_targetEndpoint));
    }

    void Connection::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Connection, GraphElement>()
                ->Version(0)
                ->Field("m_sourceEndpoint", &Connection::m_sourceEndpoint)
                ->Field("m_targetEndpoint", &Connection::m_targetEndpoint)
                ;

            serializeContext->RegisterGenericType<ConnectionPtr>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Connection>("ConnectionModelConnection")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Method("GetSourceNode", &Connection::GetSourceNode)
                ->Method("GetTargetNode", &Connection::GetTargetNode)
                ->Method("GetSourceSlot", &Connection::GetSourceSlot)
                ->Method("GetTargetSlot", &Connection::GetTargetSlot)
                ->Method("GetSourceEndpoint", &Connection::GetSourceEndpoint)
                ->Method("GetTargetEndpoint", &Connection::GetTargetEndpoint)
            ;
        }
    }

    NodePtr Connection::GetSourceNode() const
    {
        auto sourceSlot = GetSourceSlot();
        return sourceSlot ? sourceSlot->GetParentNode() : nullptr;
    }

    NodePtr Connection::GetTargetNode() const
    {
        auto targetSlot = GetTargetSlot();
        return targetSlot ? targetSlot->GetParentNode() : nullptr;
    }


    SlotPtr Connection::GetSourceSlot() const
    {
        return m_sourceSlot.lock();
    }

    SlotPtr Connection::GetTargetSlot() const
    {
        return m_targetSlot.lock();
    }

    const Endpoint& Connection::GetSourceEndpoint() const
    {
        return m_sourceEndpoint;
    }

    const Endpoint& Connection::GetTargetEndpoint() const
    {
        return m_targetEndpoint;
    }

} // namespace GraphModel
