/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>


namespace AZ::Internal
{
    bool BehaviorMethodImpl::Call(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result) const
    {
        size_t totalArguments = GetNumArguments();
        if (arguments.size() < totalArguments)
        {
            // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array
            // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
            AZStd::span<BehaviorArgument> newArguments(reinterpret_cast<BehaviorArgument*>(alloca(sizeof(BehaviorArgument) * totalArguments)), totalArguments);
            // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
            size_t argIndex = 0;
            for (; argIndex < arguments.size(); ++argIndex)
            {
                new(&newArguments[argIndex]) BehaviorArgument(arguments[argIndex]);
            }

            // clone the default parameters if they exist
            for (; argIndex < totalArguments; ++argIndex)
            {
                BehaviorDefaultValuePtr defaultValue = GetDefaultValue(argIndex);
                if (!defaultValue)
                {
                    AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", arguments.size(), totalArguments);
                    return false;
                }
                new(&newArguments[argIndex]) BehaviorArgument(defaultValue->GetValue());
            }

            arguments = newArguments;
        }

        auto memberArgument = IsMember() ? GetArgument(0) : nullptr;
        if (memberArgument != nullptr && !arguments[0].ConvertTo(memberArgument->m_typeId))
        {
            // this pointer is invalid
            AZ_Warning("Behavior", false, "First parameter should be the 'this' pointer for the member function! %s", m_name.c_str());
            return false;
        }

        for (size_t i = m_startNamedArgumentIndex; i < m_parameters.size(); ++i)
        {
            // Verify that argument is a pointer for pointer type parameters and a value or reference for value type or reference type parameters
            bool isArgumentPointer = arguments[i - 1].m_traits & BehaviorParameter::Traits::TR_POINTER;
            bool isParameterPointer = m_parameters[i].m_traits & BehaviorParameter::Traits::TR_POINTER;
            if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
            {
                AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)",
                    m_name.c_str(), i - 1, arguments[i - 1].m_name,
                    arguments[i - 1].m_typeId.ToFixedString().c_str(),
                    m_parameters[i].m_name, m_parameters[i].m_typeId.ToFixedString().c_str());
                return false;
            }
            else if (isArgumentPointer != isParameterPointer)
            {
                AZ_Warning("Behavior", false, R"(Argument is a "%s" type of %s, while parameter accepts a "%s" type of %s )",
                    isArgumentPointer ? "pointer" : "value", arguments[i - 1].m_name,
                    isParameterPointer ? "pointer" : "value", m_parameters[i].m_name);
                return false;
            }
        }

        m_functor(result, arguments);

        return true;
    }


    auto BehaviorMethodImpl::IsCallable(AZStd::span<BehaviorArgument> arguments, BehaviorArgument* result) const
        -> ResultOutcome
    {
        size_t totalArguments = GetNumArguments();
        if (arguments.size() < totalArguments)
        {
            // Clone the arguments on the stack and validate that the method can be invoked with the arguments
            AZStd::span<BehaviorArgument> newArguments(reinterpret_cast<BehaviorArgument*>(alloca(sizeof(BehaviorArgument) * totalArguments)), totalArguments);
            // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
            size_t argIndex = 0;
            for (; argIndex < arguments.size(); ++argIndex)
            {
                new(&newArguments[argIndex]) BehaviorArgument(arguments[argIndex]);
            }

            // clone the default parameters if they exist
            for (; argIndex < totalArguments; ++argIndex)
            {
                BehaviorDefaultValuePtr defaultValue = GetDefaultValue(argIndex);
                if (!defaultValue)
                {
                    return ResultOutcome{ AZStd::unexpect, ResultOutcome::ErrorType::format(
                        "Not enough arguments to make call to method %s. %zu supplied, needed %zu",
                        m_name.c_str(), arguments.size(), newArguments.size()) };
                }
                new(&newArguments[argIndex]) BehaviorArgument(defaultValue->GetValue());
            }

            arguments = newArguments;
        }
        else if (arguments.size() > totalArguments)
        {
            return ResultOutcome{ AZStd::unexpect, ResultOutcome::ErrorType::format(
                "Too many arguments supplied to method '%s'. Expects %zu arguments, provided %zu",
                m_name.c_str(), totalArguments, arguments.size()) };
        }

        auto memberArgument = IsMember() ? GetArgument(0) : nullptr;
        if (memberArgument != nullptr && !arguments[0].ConvertTo(memberArgument->m_typeId))
        {
            return ResultOutcome{ AZStd::unexpect, ResultOutcome::ErrorType::format(
                "First parameter should be the 'this' pointer for the member function! %s", m_name.c_str()) };
        }

        for (size_t i = m_startNamedArgumentIndex; i < m_parameters.size(); ++i)
        {
            // Verify that argument is a pointer for pointer type parameters and a value or reference for value type or reference type parameters
            bool isArgumentPointer = arguments[i - 1].m_traits & BehaviorParameter::Traits::TR_POINTER;
            bool isParameterPointer = m_parameters[i].m_traits & BehaviorParameter::Traits::TR_POINTER;
            if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
            {
                return ResultOutcome{ AZStd::unexpect, ResultOutcome::ErrorType::format(
                    "Invalid parameter type for method '%s'!"
                    " Can not convert method parameter %zu from %s(%s) to %s(%s)",
                    m_name.c_str(), i - 1,
                    arguments[i - 1].m_name, arguments[i - 1].m_typeId.ToFixedString().c_str(),
                    m_parameters[i].m_name, m_parameters[i].m_typeId.ToFixedString().c_str()
                ) };
            }
            else if (isArgumentPointer != isParameterPointer)
            {
                return ResultOutcome{ AZStd::unexpect, ResultOutcome::ErrorType::format(
                    R"(Argument is a "%s" type of %s, while parameter accepts a "%s" type of %s )",
                    isArgumentPointer ? "pointer" : "value", arguments[i - 1].m_name,
                    isParameterPointer ? "pointer" : "value", m_parameters[i].m_name) };
            }
        }

        if (result != nullptr)
        {
            const AZ::BehaviorParameter* returnParam = GetResult();
            if (bool canStoreResult = result->m_typeId.IsNull() || result->m_typeId == returnParam->m_typeId ||
                (returnParam->m_azRtti != nullptr && returnParam->m_azRtti->IsTypeOf(result->m_typeId));
                !canStoreResult)
            {
                return ResultOutcome{ AZStd::unexpect, ResultOutcome::ErrorType::format(
                    "The behavior argument for the return value cannot store the return type %s",
                    returnParam->m_typeId.ToFixedString().c_str()) };
            }
        }

        return AZ::Success();
    }

    bool BehaviorMethodImpl::HasResult() const
    {
        return m_hasNonVoidReturn;
    }

    bool BehaviorMethodImpl::IsMember() const
    {
        return m_isMemberFunc;
    }

    bool BehaviorMethodImpl::HasBusId() const
    {
        return false;
    }

    const BehaviorParameter* BehaviorMethodImpl::GetBusIdArgument() const
    {
        return nullptr;
    }

    size_t BehaviorMethodImpl::GetNumArguments() const
    {
        return m_parameters.size() - s_startArgumentIndex;
    }

    size_t BehaviorMethodImpl::GetMinNumberOfArguments() const
    {
        // Iterate from end of MetadataParameters and count the number of consecutive valid BehaviorValue objects
        size_t numDefaultArguments = 0;
        for (size_t i = GetNumArguments(); i > 0 && GetDefaultValue(i - 1); --i, ++numDefaultArguments)
        {
        }
        return GetNumArguments() - numDefaultArguments;
    }

    const BehaviorParameter* BehaviorMethodImpl::GetArgument(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return &m_parameters[index + s_startArgumentIndex];
        }
        return nullptr;
    }

    void BehaviorMethodImpl::OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits)
    {
        if (index < GetNumArguments())
        {
            m_parameters[index + s_startArgumentIndex].m_traits = (m_parameters[index + s_startArgumentIndex].m_traits & ~removeTraits) | addTraits;
        }
    }

    const AZStd::string* BehaviorMethodImpl::GetArgumentName(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return &m_metadataParameters[index + s_startArgumentIndex].m_name;
        }
        return nullptr;
    }

    void BehaviorMethodImpl::SetArgumentName(size_t index, AZStd::string name)
    {
        if (index < GetNumArguments())
        {
            m_metadataParameters[index + s_startArgumentIndex].m_name = name;
        }
    }

    const AZStd::string* BehaviorMethodImpl::GetArgumentToolTip(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return &m_metadataParameters[index + s_startArgumentIndex].m_toolTip;
        }
        return nullptr;
    }

    void BehaviorMethodImpl::SetArgumentToolTip(size_t index, AZStd::string toolTip)
    {
        if (index < GetNumArguments())
        {
            m_metadataParameters[index + s_startArgumentIndex].m_toolTip = AZStd::move(toolTip);
        }
    }

    void BehaviorMethodImpl::SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue)
    {
        if (index < GetNumArguments())
        {
            if (defaultValue && defaultValue->GetValue().m_typeId != GetArgument(index)->m_typeId)
            {
                AZ_Assert(false, "Argument %zu default value type, doesn't match! Default value should be the same type! Current type %s!", index, defaultValue->GetValue().m_name);
                return;
            }
            m_metadataParameters[index + s_startArgumentIndex].m_defaultValue = AZStd::move(defaultValue);
        }
    }

    BehaviorDefaultValuePtr BehaviorMethodImpl::GetDefaultValue(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return m_metadataParameters[index + s_startArgumentIndex].m_defaultValue;
        }
        return nullptr;
    }

    const BehaviorParameter* BehaviorMethodImpl::GetResult() const
    {
        return &m_parameters[0];
    }

} // namespace AZ::Internal
