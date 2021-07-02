/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Connection>()
                ->Version(0)
                ->Field("m_sourceEndpoint", &Connection::m_sourceEndpoint)
                ->Field("m_targetEndpoint", &Connection::m_targetEndpoint)
                ;
        }
    }

    NodePtr Connection::GetSourceNode() const
    {
        if (GetSourceSlot())
        {
            return GetSourceSlot()->GetParentNode();
        }

        return nullptr;
    }

    NodePtr Connection::GetTargetNode() const
    {
        if (GetTargetSlot())
        {
            return GetTargetSlot()->GetParentNode();
        }

        return nullptr;
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
