/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Contract.h"
#include "ContractBus.h"
#include "Slot.h"

#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    void Contract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Contract>()
                ->Version(0)
                ;
        }
    }
    
    AZ::Outcome<void, AZStd::string> Contract::Evaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        return OnEvaluate(sourceSlot, targetSlot);
    }

    AZ::Outcome<void, AZStd::string> Contract::EvaluateForType(const Data::Type& dataType) const
    {
        if (!dataType.IsValid())
        {            
            return AZ::Failure<AZStd::string>("No valid contract match for Invalid Data Type");
        }

        return OnEvaluateForType(dataType);
    }
}
