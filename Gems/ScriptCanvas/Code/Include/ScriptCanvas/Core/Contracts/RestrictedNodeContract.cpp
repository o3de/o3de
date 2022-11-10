/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RestrictedNodeContract.h"
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvas
{
    RestrictedNodeContract::RestrictedNodeContract(AZ::EntityId nodeId) 
        : m_nodeId{ nodeId }
    {}

    void RestrictedNodeContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection); serializeContext != nullptr)
        {
            serializeContext->Class<RestrictedNodeContract, Contract>()
                ->Version(0)
                ->Field("m_nodeId", &RestrictedNodeContract::m_nodeId)
                ;
        }
    }

    void RestrictedNodeContract::SetNodeId(AZ::EntityId nodeId) 
    { 
        m_nodeId = nodeId;
    }

    AZ::Outcome<void, AZStd::string> RestrictedNodeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        // Validate that the Node which contains the target slot matches the stored nodeId in order validate
        // the restricted node contract
        AZ::Outcome<void, AZStd::string> outcome(AZStd::unexpect, AZStd::string::format("Connection cannot be created between source slot"
            R"( "%s" and target slot "%s". Connections to source slot can be only be made from a node with node ID)"
            R"([%s])", sourceSlot.GetName().c_str(), targetSlot.GetName().c_str(), m_nodeId.ToString().c_str()));
        return targetSlot.GetNodeId() == m_nodeId ? AZ::Success() : outcome;
    }
}
