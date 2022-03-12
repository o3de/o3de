/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DynamicTypeContract.h"

#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> DynamicTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        Data::Type targetType = targetSlot.GetDataType();

        AZ::Outcome<void, AZStd::string> acceptsType = sourceSlot.GetNode()->SlotAcceptsType(sourceSlot.GetId(), targetType);
        
        if (acceptsType)
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\", slot does not support type: %s."
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , Data::GetName(targetType).c_str()
        );

        return AZ::Failure(errorMessage);

    }

    void DynamicTypeContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<DynamicTypeContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
