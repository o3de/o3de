/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SlotTypeContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    static const char* GetConnectionFailureReason(const SlotDescriptor& sourceDescriptor, const SlotDescriptor& targetDescriptor)
    {
        if (sourceDescriptor.m_slotType != targetDescriptor.m_slotType)
        {
            return "Cannot connect Execution slots to Data slots.";
        }
        else if (sourceDescriptor.m_connectionType == targetDescriptor.m_connectionType)
        {
            if (sourceDescriptor.IsInput())
            {
                return "Cannot connect Input slots to other Input slots";
            }
            else if (sourceDescriptor.IsOutput())
            {
                return "Cannot connect Output slots to other Output slots";
            }
        }

        AZ_Assert(false, "Unknown reason for Connection Failure");
        return "Unknown reason for Connection Failure";
    }

    AZ::Outcome<void, AZStd::string> SlotTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        const auto sourceDescriptor = sourceSlot.GetDescriptor();
        const auto targetDescriptor = targetSlot.GetDescriptor();
        
        if (sourceDescriptor.CanConnectTo(targetDescriptor))
        {
            return AZ::Success();
        }
        
        return AZ::Failure(AZStd::string::format
            ( "(%s) - %s "            
            , RTTI_GetTypeName()
            , GetConnectionFailureReason(sourceDescriptor, targetDescriptor)
            ));
    }

    void SlotTypeContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<SlotTypeContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
