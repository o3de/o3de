/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RestrictedNodeContract.h"
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>

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
        if (targetSlot.GetNodeId() == m_nodeId)
        {
            return AZ::Success();
        }

        if (const Node* node = targetSlot.GetNode(); node && node->IsConnectionTransparentForTranslation())
        {
            const EndpointsResolved endpoints = node->GetNonTransparentEndpoints(targetSlot);
            if (!endpoints.empty()
                && AZStd::all_of(endpoints.begin(), endpoints.end(), [this](const auto& endpoint)
                    {
                        return endpoint.first->GetEntityId() == m_nodeId;
                    }))
            {
                return AZ::Success();
            }
        }

        return AZ::Failure(AZStd::string::format(
            "Slot \"%s\" only accepts connections from node [%s], so it cannot connect to slot \"%s\".",
            sourceSlot.GetName().c_str(), m_nodeId.ToString().c_str(), targetSlot.GetName().c_str()));
    }
}
