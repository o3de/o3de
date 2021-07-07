/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "IsReferenceTypeContract.h"

#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> IsReferenceTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        Data::Type targetType = targetSlot.GetDataType();

        if (EvaluateForType(targetType))
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\", slot type must be a reference type, but is: %s."
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , Data::GetName(targetType).c_str()
        );

        return AZ::Failure(errorMessage);
    }

    AZ::Outcome<void, AZStd::string> IsReferenceTypeContract::OnEvaluateForType(const Data::Type& dataType) const
    {
        if (!Data::IsValueType(dataType))
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("Type %s is not a reference type."
            , Data::GetName(dataType).c_str()
        );

        return AZ::Failure(errorMessage);
    }

    void IsReferenceTypeContract::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<IsReferenceTypeContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
