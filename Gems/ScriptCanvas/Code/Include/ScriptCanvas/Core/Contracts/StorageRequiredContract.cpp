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
#include "precompiled.h"
#include "StorageRequiredContract.h"

#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> StorageRequiredContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        if (sourceSlot.GetType() == SlotType::DataOut && targetSlot.GetType() == SlotType::DataIn)
        {
            bool isSlotValidStorage{};
            NodeRequestBus::EventResult(isSlotValidStorage, targetSlot.GetNodeId(), &NodeRequests::IsSlotValidStorage, targetSlot.GetId());
            if (isSlotValidStorage)
            {
                return AZ::Success();
            }
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\", Storage requirement is not met. (%s)"
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , RTTI_GetTypeName()
        );

        return AZ::Failure(errorMessage);
    }

    void StorageRequiredContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<StorageRequiredContract, Contract>()
                ->Version(0)
                ;
        }
    }
}
