/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Connection.h"
#include "Slot.h"
#include "Node.h"
#include "NodeBus.h"
#include "Contracts/DisallowReentrantExecutionContract.h"
#include "Contracts/RestrictedNodeContract.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> MatchContracts(const Slot& firstSlot, const Slot& secondSlot)
    {
        for (const auto& contract : firstSlot.GetContracts())
        {
            if (!contract)
            {
                break;
            }

            if (auto outcome = contract->Evaluate(firstSlot, secondSlot); !outcome)
            {
                return outcome;
            }
        }

        return AZ::Success();
    }

    namespace
    {
        AZ::Outcome<void, AZStd::string> MatchEndpointContracts(const Slot& firstSlot, const Slot& secondSlot)
        {
            for (const auto& contract : firstSlot.GetContracts())
            {
                if (azrtti_istypeof<RestrictedNodeContract>(contract.get())
                    || azrtti_istypeof<DisallowReentrantExecutionContract>(contract.get()))
                {
                    if (auto outcome = contract->Evaluate(firstSlot, secondSlot); !outcome)
                    {
                        return outcome;
                    }
                }
            }

            return AZ::Success();
        }
    }

    Connection::Connection(const ID& fromNode, const SlotId& fromSlot, const ID& toNode, const SlotId& toSlot)
        : m_sourceEndpoint(fromNode, fromSlot)
        , m_targetEndpoint(toNode, toSlot)
    {
    }

    Connection::Connection(const Endpoint& fromConnection, const Endpoint& toConnection)
        : m_sourceEndpoint(fromConnection)
        , m_targetEndpoint(toConnection)
    {
    }

    Connection::~Connection()
    {
        ConnectionRequestBus::Handler::BusDisconnect();
    }

    void Connection::Reflect(AZ::ReflectContext* reflection)
    {
        Endpoint::Reflect(reflection);
        NamedEndpoint::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Connection, AZ::Component>()
                ->Version(0)
                ->Field("sourceEndpoint", &Connection::m_sourceEndpoint)
                ->Field("targetEndpoint", &Connection::m_targetEndpoint)
                ;
        }

    }

    void Connection::Init()
    {
        ConnectionRequestBus::Handler::BusConnect(GetEntityId());
    }

    void Connection::Activate()
    {
    }

    void Connection::Deactivate()
    {
    }

    AZ::Outcome<void, AZStd::string> Connection::ValidateEndpoints(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint)
    {
        Slot* sourceSlot{};
        NodeRequestBus::EventResult(sourceSlot, sourceEndpoint.GetNodeId(), &NodeRequests::GetSlot, sourceEndpoint.GetSlotId());

        Slot* targetSlot{};
        NodeRequestBus::EventResult(targetSlot, targetEndpoint.GetNodeId(), &NodeRequests::GetSlot, targetEndpoint.GetSlotId());

        if (!sourceSlot)
        {
            return AZ::Failure(AZStd::string("Source slot does not exist."));
        }

        if (!targetSlot)
        {
            return AZ::Failure(AZStd::string("Target slot does not exist."));
        }

        return ValidateConnection((*sourceSlot), (*targetSlot));
    }

    AZ::Outcome<void, AZStd::string> Connection::ValidateConnection(const Slot& sourceSlot, const Slot& targetSlot)
    {
        if (sourceSlot.IsData())
        {
            auto typeMatchCheck = sourceSlot.IsTypeMatchFor(targetSlot);
            if (!typeMatchCheck)
            {
                return typeMatchCheck;
            }
        }

        auto connectionSourceToTarget = MatchContracts(sourceSlot, targetSlot);
        if (!connectionSourceToTarget.IsSuccess())
        {
            return connectionSourceToTarget;
        }

        auto connectionTargetToSource = MatchContracts(targetSlot, sourceSlot);
        if (!connectionTargetToSource.IsSuccess())
        {
            return connectionTargetToSource;
        }

        const Node* sourceNode = sourceSlot.GetNode();
        const Node* targetNode = targetSlot.GetNode();
        if ((sourceNode && sourceNode->IsConnectionTransparentForTranslation())
            || (targetNode && targetNode->IsConnectionTransparentForTranslation()))
        {
            if (!sourceNode || !targetNode)
            {
                return AZ::Failure(AZStd::string("Reroute connections require both slots to have an owning node."));
            }

            const auto sources = sourceNode->GetNonTransparentEndpoints(sourceSlot);
            const auto targets = targetNode->GetNonTransparentEndpoints(targetSlot);

            // Connection limits count physical wires. Node identity checks also apply across reroutes.
            for (const auto& sourceEndpoint : sources)
            {
                for (const auto& targetEndpoint : targets)
                {
                    const Slot* source = sourceEndpoint.second;
                    const Slot* target = targetEndpoint.second;

                    if (auto outcome = MatchEndpointContracts(*source, *target); !outcome)
                    {
                        return outcome;
                    }
                    if (auto outcome = MatchEndpointContracts(*target, *source); !outcome)
                    {
                        return outcome;
                    }
                }
            }
        }

        return AZ::Success();
    }

    bool Connection::ContainsEndpoint(const Endpoint& endpoint)
    {
        return m_sourceEndpoint == endpoint
            || m_targetEndpoint == endpoint;
    }

    const SlotId& Connection::GetSourceSlot() const
    {
        return m_sourceEndpoint.GetSlotId();
    }

    const SlotId& Connection::GetTargetSlot() const
    {
        return m_targetEndpoint.GetSlotId();
    }

    const ID& Connection::GetTargetNode() const
    {
        return m_targetEndpoint.GetNodeId();
    }

    const ID& Connection::GetSourceNode() const
    {
        return m_sourceEndpoint.GetNodeId();
    }

    const Endpoint& Connection::GetTargetEndpoint() const
    {
        return m_targetEndpoint;
    }

    const Endpoint& Connection::GetSourceEndpoint() const
    {
        return m_sourceEndpoint;
    }

    void Connection::OnNodeRemoved(const ID& nodeId)
    {
        if (nodeId == m_sourceEndpoint.GetNodeId() || nodeId == m_targetEndpoint.GetNodeId())
        {
            Slot* sourceSlot{};
            NodeRequestBus::EventResult(sourceSlot, m_sourceEndpoint.GetNodeId(), &NodeRequests::GetSlot, m_sourceEndpoint.GetSlotId());
            if (sourceSlot)
            {
                GraphRequestBus::Event(sourceSlot->GetNode()->GetOwningScriptCanvasId(), &GraphRequests::DisconnectById, GetEntityId());
            }
        }
    }
}
