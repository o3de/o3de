/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/
#pragma once

namespace AZ::Internal
{
    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::BehaviorEBusEvent(FunctionPointer functionPointer, BehaviorContext* context)
        : BehaviorMethod(context)
        , m_functionPtr(functionPointer)
    {
        SetParameters<R>(m_parameters, this);
        if constexpr (s_isBusIdParameter == 1)
        {
            // optional ID parameter
            SetParameters<typename EBus::BusIdType>(&m_parameters[s_startArgumentIndex]);
        }
        SetParameters<Args...>(&m_parameters[s_startNamedArgumentIndex], this);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    bool BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::Call(BehaviorArgument* arguments, unsigned int numArguments, BehaviorArgument* result) const
    {
        size_t totalArguments = GetNumArguments();
        if (numArguments < totalArguments)
        {
            // We are cloning all arguments on the stack, since Call is called only from Invoke we can reserve bigger "arguments" array
            // that can always handle all parameters. So far the don't use default values that ofter, so we will optimize for the common case first.
            BehaviorArgument* newArguments = reinterpret_cast<BehaviorArgument*>(alloca(sizeof(BehaviorArgument) * totalArguments));
            // clone the input parameters (we don't need to clone temp buffers, etc. as they will be still on the stack)
            size_t argIndex = 0;
            for (; argIndex < numArguments; ++argIndex)
            {
                new(&newArguments[argIndex]) BehaviorArgument(arguments[argIndex]);
            }

            // clone the default parameters if they exist
            for (; argIndex < totalArguments; ++argIndex)
            {
                BehaviorDefaultValuePtr defaultValue = GetDefaultValue(argIndex);
                if (!defaultValue)
                {
                    AZ_Warning("Behavior", false, "Not enough arguments to make a call! %d needed %d", numArguments, totalArguments);
                    return false;
                }
                new(&newArguments[argIndex]) BehaviorArgument(defaultValue->GetValue());
            }

            arguments = newArguments;
        }

        if constexpr (s_isBusIdParameter)
        {
            if (!arguments[0].ConvertTo(m_parameters[1].m_typeId))
            {
                AZ_Warning("Behavior", false, "Invalid BusIdType type can't convert! %s -> %s", arguments[0].m_name, m_parameters[1].m_name);
                return false;
            }
        }

        for (size_t i = s_startNamedArgumentIndex; i < AZ_ARRAY_SIZE(m_parameters); ++i)
        {
            if (!arguments[i - 1].ConvertTo(m_parameters[i].m_typeId))
            {
                AZ_Warning("Behavior", false, "Invalid parameter type for method '%s'! Can not convert method parameter %d from %s(%s) to %s(%s)", m_name.c_str(), i - 1, arguments[i - 1].m_name, arguments[i - 1].m_typeId.template ToString<AZStd::string>().c_str(), m_parameters[i].m_name, m_parameters[i].m_typeId.template ToString<AZStd::string>().c_str());
                return false;
            }
        }

        AZ::Internal::EBusCaller<EventType, EBus, R, Args...>::Call(
            m_functionPtr, arguments, result, AZStd::make_index_sequence<sizeof...(Args)>());

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    bool BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::HasResult() const
    {
        return !AZStd::is_same<R, void>::value;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    bool BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::IsMember() const
    {
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    bool BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::HasBusId() const
    {
        return s_isBusIdParameter != 0;
    }

    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetBusIdArgument() const
    {
        return HasBusId() ? GetArgument(0) : nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    size_t BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetNumArguments() const
    {
        return AZ_ARRAY_SIZE(m_parameters) - s_startArgumentIndex;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    size_t BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetMinNumberOfArguments() const
    {
        // Iterate from end of MetadataParameters and count the number of consecutive valid BehaviorValue objects
        size_t numDefaultArguments = 0;
        for (size_t i = GetNumArguments() - 1; i >= 0 && GetDefaultValue(i); --i, ++numDefaultArguments)
        {
        }
        return GetNumArguments() - numDefaultArguments;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetArgument(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return &m_parameters[index + s_startArgumentIndex];
        }
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    void BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::OverrideParameterTraits(size_t index, AZ::u32 addTraits, AZ::u32 removeTraits)
    {
        if (index < GetNumArguments())
        {
            m_parameters[index + s_startArgumentIndex].m_traits = (m_parameters[index + s_startArgumentIndex].m_traits & ~removeTraits) | addTraits;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    const AZStd::string* BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetArgumentName(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return &m_metadataParameters[index + s_startArgumentIndex].m_name;
        }
        return nullptr;
    }

    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    void BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::SetArgumentName(size_t index, const AZStd::string& name)
    {
        if (index < GetNumArguments())
        {
            m_metadataParameters[index + s_startArgumentIndex].m_name = name;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    const AZStd::string* BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetArgumentToolTip(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return &m_metadataParameters[index + s_startArgumentIndex].m_toolTip;
        }
        return nullptr;
    }

    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    void BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::SetArgumentToolTip(size_t index, const AZStd::string& toolTip)
    {
        if (index < GetNumArguments())
        {
            m_metadataParameters[index + s_startArgumentIndex].m_toolTip = toolTip;
        }
    }

    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    void BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::SetDefaultValue(size_t index, BehaviorDefaultValuePtr defaultValue)
    {
        if (index < GetNumArguments())
        {
            if (defaultValue && defaultValue->GetValue().m_typeId != GetArgument(index)->m_typeId)
            {
                AZ_Assert(false, "Argument %zu default value type, doesn't match! Default value should be the same type! Current type %s!", index, defaultValue->GetValue().m_name);
                return;
            }
            m_metadataParameters[index + s_startArgumentIndex].m_defaultValue = defaultValue;
        }
    }

    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    BehaviorDefaultValuePtr BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetDefaultValue(size_t index) const
    {
        if (index < GetNumArguments())
        {
            return m_metadataParameters[index + s_startArgumentIndex].m_defaultValue;
        }
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class EBus, BehaviorEventType EventType, class R, class BusType, class... Args>
    const BehaviorParameter* BehaviorEBusEvent<EBus, EventType, R(BusType*, Args...)>::GetResult() const
    {
        return &m_parameters[0];
    }
}

