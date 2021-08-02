/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SupportsMethodContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    AZ::Outcome<void, AZStd::string> SupportsMethodContract::OnEvaluate([[maybe_unused]] const Slot& sourceSlot, const Slot& targetSlot) const
    {   
        if (targetSlot.IsDynamicSlot() && targetSlot.HasDisplayType())
        {
            Data::Type dataType = targetSlot.GetDataType();

            return EvaluateForType(dataType);
        }

        // If the target slot is dynamic. We can assume they are a type match from the regular type matching system.
        // So we can just return success and let the dynamic typing system handle ensuring this contract is successfully
        // fulfiled.
        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> SupportsMethodContract::OnEvaluateForType(const Data::Type& dataType) const
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (!behaviorContext)
        {
            AZ_Assert(false, "A behavior context is required!");
            return AZ::Failure(AZStd::string::format("No Behavior Context"));
        }

        const AZ::TypeId azDataType = ToAZType(dataType);

        const auto classIter(behaviorContext->m_typeToClassMap.find(azDataType));
        if (classIter != behaviorContext->m_typeToClassMap.end())
        {
            AZ::BehaviorClass* behaviorClass = classIter->second;
            if (behaviorClass)
            {
                if (behaviorClass->m_methods.find(m_methodName) != behaviorClass->m_methods.end())
                {
                    return AZ::Success();
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Behavior Context does not contain reflection for method %s on class %s", m_methodName.c_str(), azDataType.ToString<AZStd::string>().c_str()));
                }
            }
        }

        return AZ::Failure(AZStd::string::format("Behavior Context does not contain reflection for type provided: %s", azDataType.ToString<AZStd::string>().c_str()));
    }

    void SupportsMethodContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<SupportsMethodContract, Contract>()
                ->Version(0)
                ->Field("m_methodName", &SupportsMethodContract::m_methodName)
                ;
        }
    }
}
