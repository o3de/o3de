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

#include "DynamicTypeContract.h"

#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> DynamicTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        Data::Type targetType = targetSlot.GetDataType();

        bool acceptsType = sourceSlot.GetNode()->SlotAcceptsType(sourceSlot.GetId(), targetType);
        
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
