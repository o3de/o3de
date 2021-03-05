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
