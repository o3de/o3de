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
