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
    template<class EBus, class Event, class... Args, size_t... Is>
    void CallEBusMethod(Event eventFunc, BehaviorEventType eventType,
        BehaviorArgument* arguments, BehaviorArgument* result, AZStd::Internal::pack_traits_arg_sequence<Args...>, AZStd::index_sequence<Is...>)
    {
        using R = typename  AZStd::function_traits<Event>::return_type;
        switch (eventType)
        {
        case BehaviorEventType::BE_BROADCAST:
            if constexpr (AZStd::is_void_v<R>)
            {
                EBus::Broadcast(eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
            }
            else
            {
                if (result)
                {
                    EBus::BroadcastResult(*result, eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
                }
                else
                {
                    EBus::Broadcast(eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
                }
            }
            break;
        case BehaviorEventType::BE_EVENT_ID:
            if constexpr (!AZStd::is_same_v<typename EBus::BusIdType, AZ::NullBusId>)
            {
                BehaviorArgument& id = *arguments++;
                if constexpr (AZStd::is_void_v<R>)
                {
                    EBus::Event(*id.GetAsUnsafe<typename EBus::BusIdType>(), eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
                }
                else
                {
                    if (result)
                    {
                        EBus::EventResult(*result, *id.GetAsUnsafe<typename EBus::BusIdType>(), eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
                    }
                    else
                    {
                        EBus::Event(*id.GetAsUnsafe<typename EBus::BusIdType>(), eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
                    }
                }
            }
            break;
        case BehaviorEventType::BE_QUEUE_BROADCAST:
            if constexpr (!AZStd::is_same_v<typename EBus::QueuePolicy::BusMessageCall, AZ::Internal::NullBusMessageCall>)
            {
                EBus::QueueBroadcast(eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
            }
            break;
        case BehaviorEventType::BE_QUEUE_EVENT_ID:
            if constexpr (!AZStd::is_same_v<typename EBus::BusIdType, AZ::NullBusId>
                && !AZStd::is_same_v<typename EBus::QueuePolicy::BusMessageCall, AZ::Internal::NullBusMessageCall>)
            {
                BehaviorArgument& id = *arguments++;
                EBus::QueueEvent(*id.GetAsUnsafe<typename EBus::BusIdType>(), eventFunc, *arguments[Is].GetAsUnsafe<Args>()...);
            }
            break;
        }
    }

    template<class EBus, class R, class BusType, class... Args>
    BehaviorEBusEvent::BehaviorEBusEvent(R(*functionPointer)(BusType*, Args...), BehaviorContext* context,
        BehaviorEventType eventType, AZStd::type_identity<EBus>)
        : BehaviorMethod(context)
        , m_eventType(eventType)
        , m_hasNonVoidReturn(!AZStd::is_void_v<R>)
    {
        m_functor = [functionPointer, this](BehaviorArgument* result, AZStd::span<BehaviorArgument> arguments)
        {
            CallEBusMethod<EBus>(functionPointer, m_eventType, arguments.data(), result,
                AZStd::Internal::pack_traits_arg_sequence<Args...>{}, AZStd::make_index_sequence<sizeof...(Args)>());
        };

        InitParameters<EBus, R, BusType, Args...>();
    }

    template<class EBus, class R, class C, class... Args>
    BehaviorEBusEvent::BehaviorEBusEvent(R(C::*functionPointer)(Args...), BehaviorContext* context,
        BehaviorEventType eventType, AZStd::type_identity<EBus>)
        : BehaviorMethod(context)
        , m_eventType(eventType)
        , m_hasNonVoidReturn(!AZStd::is_void_v<R>)
    {
        m_functor = [functionPointer, this](BehaviorArgument* result, AZStd::span<BehaviorArgument> arguments)
        {
            CallEBusMethod<EBus>(functionPointer, m_eventType, arguments.data(), result,
                AZStd::Internal::pack_traits_arg_sequence<Args...>{}, AZStd::make_index_sequence<sizeof...(Args)>());
        };

        InitParameters<EBus, R, C, Args...>();
    }

    template<class EBus, class R, class C, class... Args>
    BehaviorEBusEvent::BehaviorEBusEvent(R(C::*functionPointer)(Args...) const, BehaviorContext* context,
        BehaviorEventType eventType, AZStd::type_identity<EBus>)
        : BehaviorEBusEvent(reinterpret_cast<R(C::*)(Args...)>(functionPointer), context,
            eventType, AZStd::type_identity<EBus>{})
    {
        m_isConst = true;
    }

#if __cpp_noexcept_function_type
    // Delegate to the non-noexcept constructors
    template<class EBus, class R, class BusType, class... Args>
    BehaviorEBusEvent::BehaviorEBusEvent(R(*functionPointer)(BusType*, Args...) noexcept, BehaviorContext* context,
        BehaviorEventType eventType, AZStd::type_identity<EBus>)
        : BehaviorEBusEvent(reinterpret_cast<R(*)(Args...)>(functionPointer), context,
            eventType, AZStd::type_identity<EBus>{})
    {}

    template<class EBus, class R, class C, class... Args>
    BehaviorEBusEvent::BehaviorEBusEvent(R(C::* functionPointer)(Args...) noexcept, BehaviorContext* context,
        BehaviorEventType eventType, AZStd::type_identity<EBus>)
        : BehaviorEBusEvent(reinterpret_cast<R(C::*)(Args...)>(functionPointer), context,
            eventType, AZStd::type_identity<EBus>{})
    {}

    template<class EBus, class R, class C, class... Args>
    BehaviorEBusEvent::BehaviorEBusEvent(R(C::* functionPointer)(Args...) const noexcept, BehaviorContext* context,
        BehaviorEventType eventType, AZStd::type_identity<EBus>)
        : BehaviorEBusEvent(reinterpret_cast<R(C::*)(Args...) const>(functionPointer),
            eventType, AZStd::type_identity<EBus>{})
    {}
#endif

    template<class EBus, class R, class BusType, class... Args>
    void BehaviorEBusEvent::InitParameters()
    {
        size_t parameterArraySize = 1 + sizeof...(Args);
        constexpr bool hasBusId = !AZStd::is_same_v<typename EBus::BusIdType, AZ::NullBusId>;
        const bool addressById = m_eventType == BehaviorEventType::BE_EVENT_ID
            || m_eventType == BehaviorEventType::BE_QUEUE_EVENT_ID;
        if constexpr (hasBusId)
        {
            if (addressById)
            {
                // There is an additional parameter for the BusId
                ++parameterArraySize;
                // The start of the non-bus id parameters is now at offset 2
                ++m_startNamedArgumentIndex;
            }
        }

        // Resize the parameters and meta associated with the parameters
        m_parameters.resize(parameterArraySize);
        m_metadataParameters.resize(parameterArraySize);

        SetParameters<R>(m_parameters.data(), this);
        if constexpr (hasBusId)
        {
            if (addressById)
            {
                // configure optional ID parameter
                SetParameters<typename EBus::BusIdType>(m_parameters.data() + s_startArgumentIndex);
            }

        }
        SetParameters<Args...>(m_parameters.data() + m_startNamedArgumentIndex, this);
    }
} // namespace AZ::Internal
