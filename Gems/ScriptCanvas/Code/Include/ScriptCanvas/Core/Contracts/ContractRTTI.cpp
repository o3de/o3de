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

#include "ContractRTTI.h"

#include <Core/ContractBus.h>
#include <Core/Node.h>
#include <Core/Slot.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> ContractRTTI::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        AZ::Entity* nodeEntity{};
        AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, targetSlot.GetNodeId());
        const auto node = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity) : nullptr;

        bool valid{};
        if (m_flags == Inclusive)
        {
            if (node)
            {
                for (const auto& type : m_types)
                {
                    if (node->RTTI_IsTypeOf(type))
                    {
                        valid = true;
                        break;
                    }
                }
            }            
        }
        else
        {
            if (node)
            {
                valid = true;
                
                for (const auto& type : m_types)
                {
                    if (node->RTTI_IsTypeOf(type))
                    {
                        valid = false;
                        break;
                    }
                }
            }
        }

        if (valid)
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\" as the types do not satisfy the RTTI requirement. (%s)"
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , RTTI_GetTypeName());

        return AZ::Failure(errorMessage);
    }

    void ContractRTTI::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ContractRTTI, Contract>()
                ->Version(1)
                ->Field("flags", &ContractRTTI::m_flags)
                ->Field("types", &ContractRTTI::m_types)
                ;
        }
    }
}
