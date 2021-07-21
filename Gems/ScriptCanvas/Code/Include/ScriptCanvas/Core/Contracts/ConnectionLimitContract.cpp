/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ConnectionLimitContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> ConnectionLimitContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        if (m_limit < 0)
        {
            return AZ::Success();
        }

        AZStd::vector<Endpoint> connectedEndpoints = sourceSlot.GetNode()->GetGraph()->GetConnectedEndpoints(sourceSlot.GetEndpoint());

        if (connectedEndpoints.size() < static_cast<AZ::u32>(m_limit))
        {
            return AZ::Success();
        }
        else
        {
            AZStd::string errorMessage = AZStd::string::format
                ( "Connection cannot be created between source slot \"%s\" and target slot \"%s\" as the source slot has a Connection Limit of %d. (%s)"
                , sourceSlot.GetName().data()
                , targetSlot.GetName().data()
                , m_limit
                , RTTI_GetTypeName());

            return AZ::Failure(errorMessage);
        }
    }

    ConnectionLimitContract::ConnectionLimitContract(AZ::s32 limit) 
        : m_limit(AZStd::GetMax(-1, limit)) 
    {}

    void ConnectionLimitContract::SetLimit(AZ::s32 limit) 
    { 
        m_limit = AZStd::GetMax(-1, limit); 
    }

    void ConnectionLimitContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ConnectionLimitContract, Contract>()
                ->Version(0)
                ->Field("limit", &ConnectionLimitContract::m_limit)
                ;
        }
    }
}
