/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExclusivePureDataContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> ExclusivePureDataContract::HasNoPureDataConnection(const Slot& dataInputSlot) const
    {
        bool isOnPureDataThread(true);
        NodeRequestBus::EventResult(isOnPureDataThread, dataInputSlot.GetNodeId(), &NodeRequests::IsOnPureDataThread, dataInputSlot.GetId());

        if (!isOnPureDataThread)
        {
            return AZ::Success();
        }
        
        return AZ::Failure(AZStd::string("There is already a pure data input into this slot"));
    }

    AZ::Outcome<void, AZStd::string> ExclusivePureDataContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        if (sourceSlot.GetDescriptor().CanConnectTo(targetSlot.GetDescriptor()))
        {
            if (sourceSlot.GetDescriptor().IsInput())
            {
                return HasNoPureDataConnection(sourceSlot);
            }
            else if (targetSlot.GetDescriptor().IsInput())
            {
                return HasNoPureDataConnection(targetSlot);
            }
        }

        return AZ::Failure(AZStd::string("invalid data connection attempted"));
    }

    void ExclusivePureDataContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<ExclusivePureDataContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
