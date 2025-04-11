/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventMethod.h"
#include "ScriptEventsBindingBus.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetCommon.h>

#include <ScriptEvents/ScriptEvent.h>
#include <ScriptEvents/Internal/VersionedProperty.h>

namespace ScriptEvents
{
    ScriptEventMethod::ScriptEventMethod(AZ::BehaviorContext* behaviorContext, const ScriptEvent& definition, const AZStd::string eventName)
        : AZ::BehaviorMethod(behaviorContext)
        , m_busIdType(AZ::Uuid::CreateNull())
        , m_returnType(AZ::Uuid::CreateNull())
        , m_busBindingId()
    {
        m_name = eventName;

        const AZStd::string& busName = definition.GetName();
        m_busBindingId = AZ::Uuid::CreateName(busName.c_str());
        m_busIdType = definition.GetAddressType();

        Method method;
        if (!definition.FindMethod(eventName, method))
        {
            AZ_Warning("Script Events", false, "Method %s not found in Script Event %s", eventName.data(), busName.c_str());
        }

        m_result.m_name = "Result";

        if (!method.GetReturnTypeProperty().IsEmpty())
        {
            method.GetReturnTypeProperty().Get(m_returnType);
            Internal::Utils::BehaviorParameterFromType(m_returnType, false, m_result);
        }

        ReserveArguments(method.GetParameters().size() + 1);

        size_t index = 0;

        // BehaviorContext Ebus Events require an Id, it is passed in as the first parameter to the method.
        if (!m_busIdType.IsNull())
        {
            AZ::BehaviorArgument busId;
            Internal::Utils::BehaviorParameterFromType(m_busIdType, true, busId);
            m_behaviorParameters.emplace_back(busId);
            SetArgumentName(index, busId.m_name);
            SetArgumentToolTip(index, busId.m_name);

            ++index;
        }

        AZStd::vector<AZStd::string> tests;
        for (const Parameter& parameter : method.GetParameters())
        {
            const AZStd::string& argumentName = parameter.GetName();
            SetArgumentName(index, argumentName);

            m_behaviorParameters.emplace_back();
            Internal::Utils::BehaviorParameterFromParameter(behaviorContext, parameter, m_argumentNames[index].c_str(), m_behaviorParameters.back());

            const AZStd::string& tooltip = parameter.GetTooltip();
            if (!tooltip.empty())
            {
                SetArgumentToolTip(index, tooltip.data());
            }

            ++index;
        }

        //AZ_TracePrintf("Script Events", "Script Method: %s %s::%s (Arguments: %d)\n", m_returnType.ToString<AZStd::string>().c_str(), busName.c_str(), eventName.data(), method.GetParameters().size());

    }

    bool ScriptEventMethod::Call(AZStd::span<AZ::BehaviorArgument> params, AZ::BehaviorArgument* returnValue) const
    {
        Internal::BindingRequest::BindingParameters parameters;
        parameters.m_eventName = m_name;
        parameters.m_address = &params[0];  // The address is stored in the first parameter
        parameters.m_parameters = params.data() + 1;
        parameters.m_parameterCount = AZ::u32(params.size() - 1); // Minus the address
        parameters.m_returnValue = returnValue;

        Internal::BindingRequestBus::Event(m_busBindingId, &Internal::BindingRequest::Bind, parameters);

        if (returnValue && returnValue->m_onAssignedResult)
        {
            returnValue->m_onAssignedResult();
        }

        return true;
    }

    auto ScriptEventMethod::IsCallable([[maybe_unused]] AZStd::span<AZ::BehaviorArgument> params, [[maybe_unused]] AZ::BehaviorArgument* returnValue) const
        -> ResultOutcome
    {
        return AZ::Success();
    }

    void ScriptEventMethod::ReserveArguments(size_t numArguments)
    {
        m_behaviorParameters.reserve(numArguments);
        m_argumentNames.resize(numArguments);
        m_argumentToolTips.resize(numArguments);
    }

    void ScriptEventMethod::SetArgumentName(size_t index, AZStd::string name)
    {
        if (index >= m_argumentNames.size())
        {
            m_argumentNames.resize(index + 1);
        }

        m_argumentNames[index] = AZStd::move(name);
    }

    size_t ScriptEventMethod::GetMinNumberOfArguments() const
    {
        // Iterate from end of parameters and count the number of consecutive valid BehaviorValue objects
        size_t numDefaultArguments = 0;
        for (int i = static_cast<int>(GetNumArguments()) - 1; i >= 0 && GetDefaultValue(static_cast<size_t>(i)); --i, ++numDefaultArguments)
        {
        }
        return GetNumArguments() - numDefaultArguments;
    }

    AZ::BehaviorDefaultValuePtr ScriptEventMethod::GetDefaultValue(size_t) const
    {
        // Default values for Script Events are not implemented.
        return nullptr;
    }

}
