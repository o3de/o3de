/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DisallowReentrantExecutionContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> DisallowReentrantExecutionContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        bool valid = sourceSlot.GetNodeId() != targetSlot.GetNodeId();
        if (valid)
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\", this slot does not allow connections from the same node. (%s)"
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , RTTI_GetTypeName()
        );

        return AZ::Failure(errorMessage);

    }

    void DisallowReentrantExecutionContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<DisallowReentrantExecutionContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
