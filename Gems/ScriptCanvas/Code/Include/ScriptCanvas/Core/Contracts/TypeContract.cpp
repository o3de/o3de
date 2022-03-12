/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TypeContract.h"
#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Slot.h>

namespace ScriptCanvas
{
    void RestrictedTypeContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RestrictedTypeContract, Contract>()
                ->Version(1)
                ->Field("flags", &RestrictedTypeContract::m_flags)
                ->Field("types", &RestrictedTypeContract::m_types)
                ;
        }
    }

    AZ::Outcome<void, AZStd::string> RestrictedTypeContract::OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const
    {
        bool valid = false;

        if (m_types.empty())
        {
            valid = m_flags != Exclusive;
        }
        else
        {
            if (m_flags == Inclusive)
            {
                for (const auto& type : m_types)
                {
                    if (targetSlot.IsTypeMatchFor(type))
                    {
                        valid = true;
                        break;
                    }
                }
            }
            else
            {
                valid = true;
                for (const auto& type : m_types)
                {
                    if (targetSlot.IsTypeMatchFor(type))
                    {
                        if (!targetSlot.IsDynamicSlot())
                        {
                            valid = false;
                            break;
                        }
                        else if (targetSlot.HasDisplayType())
                        {
                            valid = false;
                            break;
                        }
                    }
                }
            }
        }

        if (valid)
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("Connection cannot be created between source slot \"%s\" and target slot \"%s\" as the types do not satisfy the type requirement. (%s)\n\rValid types are:\n\r"
            , sourceSlot.GetName().data()
            , targetSlot.GetName().data()
            , RTTI_GetTypeName());

        for (const auto& type : m_types)
        {
            if (Data::IsValueType(type))
            {
                errorMessage.append(AZStd::string::format("%s\n", Data::GetName(type).data()));
            }
            else
            {
                errorMessage.append(Data::GetBehaviorClassName(type.GetAZType()));
            }
        }

        return AZ::Failure(errorMessage);
    }

    AZ::Outcome<void, AZStd::string> RestrictedTypeContract::OnEvaluateForType(const Data::Type& dataType) const
    {
        bool valid = false;

        if (m_flags == Inclusive)
        {            
            valid = m_types.empty();

            for (const auto& type : m_types)
            {
                if (dataType.IS_A(type))
                {
                    valid = true;
                    break;
                }
            }
        }
        else
        {
            valid = true;
            for (const auto& type : m_types)
            {
                if (dataType.IS_A(type))
                {
                    valid = false;
                    break;
                }
            }
        }

        if (valid)
        {
            return AZ::Success();
        }

        AZStd::string errorMessage = AZStd::string::format("The supplied type(%s) does not satisfy the Type Requirement.", Data::GetName(dataType).data());
        return AZ::Failure(errorMessage);
    }
}
