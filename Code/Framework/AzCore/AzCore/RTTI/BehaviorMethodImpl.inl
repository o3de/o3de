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
    // call helper
    template<class R, class... Args, AZStd::size_t... Is>
    inline void CallMethodGlobal(R(*functionPtr)(Args...) , BehaviorArgument* arguments, BehaviorArgument* result, AZStd::index_sequence<Is...>)
    {
        (void)arguments;
        if constexpr (AZStd::is_void_v<R>)
        {
            functionPtr(*arguments[Is].GetAsUnsafe<Args>()...);
        }
        else
        {
            if (result)
            {
                result->StoreResult<R>(functionPtr(*arguments[Is].GetAsUnsafe<Args>()...));
            }
            else
            {
                functionPtr(*arguments[Is].GetAsUnsafe<Args>()...);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class R, class C, class... Args, AZStd::size_t... Is>
    inline void CallMethodMember(R(C::* functionPtr)(Args...), C* thisPtr, BehaviorArgument* arguments, BehaviorArgument* result, AZStd::index_sequence<Is...>)
    {
        (void)arguments;
        if constexpr (AZStd::is_void_v<R>)
        {
            (thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...);
        }
        else
        {
            if (result)
            {
                result->StoreResult<R>((thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...));
            }
            else
            {
                (thisPtr->*functionPtr)(*arguments[Is].GetAsUnsafe<Args>()...);
            }
        }
    }

    // BehaviorMethodImpl constructors
    template<class R, class... Args>
    BehaviorMethodImpl::BehaviorMethodImpl(R(*functionPointer)(Args...), BehaviorContext* context, AZStd::string name)
        : BehaviorMethod(context)
        , m_hasNonVoidReturn(!AZStd::is_void_v<R>)
    {
        // Wrap the function pointer object in a type erased lambda
        auto CallFreeFunction = [functionPointer](BehaviorArgument* result, AZStd::span<BehaviorArgument> arguments)
        {
            CallMethodGlobal(functionPointer, arguments.data(), result, AZStd::make_index_sequence<sizeof...(Args)>());
        };
        m_functor = CallFreeFunction;
        m_name = AZStd::move(name);

        // The +1 is for the return type
        constexpr size_t parameterArraySize = 1 + sizeof...(Args);
        m_parameters.resize(parameterArraySize);
        m_metadataParameters.resize(parameterArraySize);
        SetParameters<R>(m_parameters.data(), this);
        SetParameters<Args...>(m_parameters.data() + m_startNamedArgumentIndex, this);
    }


    template<class R, class C, class... Args>
    BehaviorMethodImpl::BehaviorMethodImpl(R(C::* functionPointer)(Args...), BehaviorContext* context, AZStd::string name)
        : BehaviorMethod(context)
        , m_startNamedArgumentIndex(s_startArgumentIndex + 1)
        , m_hasNonVoidReturn(!AZStd::is_void_v<R>)
        , m_isMemberFunc(true)
    {
        // Wrap the member function pointer object in a type erased lambda
        auto CallMemberFunction = [functionPointer, this](BehaviorArgument* result, AZStd::span<BehaviorArgument> arguments)
        {
            C* thisPtr = *arguments[0].GetAsUnsafe<C*>();
            CallMethodMember(functionPointer, thisPtr, arguments.data() + 1, result, AZStd::make_index_sequence<sizeof...(Args)>());

            BehaviorObjectSignals::Event(thisPtr, &BehaviorObjectSignals::Events::OnMemberMethodCalled, this);
        };
        m_functor = CallMemberFunction;
        // Increment the start named argument index since the function is a member
        m_name = AZStd::move(name);

        // The +2 is for the return type and class type
        constexpr size_t parameterArraySize = 2 + sizeof...(Args);
        m_parameters.resize(parameterArraySize);
        m_metadataParameters.resize(parameterArraySize);

        SetParameters<R>(m_parameters.data(), this);
        SetParameters<C*>(m_parameters.data() + s_startArgumentIndex, this);
        m_parameters[s_startArgumentIndex].m_traits |= BehaviorParameter::TR_THIS_PTR;
        SetParameters<Args...>(m_parameters.data() + m_startNamedArgumentIndex, this);
    }

    template<class R, class C, class... Args>
    BehaviorMethodImpl::BehaviorMethodImpl(R(C::* functionPointer)(Args...) const, BehaviorContext* context, AZStd::string name)
        : BehaviorMethodImpl(reinterpret_cast<R(C::*)(Args...)>(functionPointer), context, AZStd::move(name))
    {
        m_isConst = true;
    }

#if __cpp_noexcept_function_type
    // Delegate to the non-noexcept constructors
    template <class R, class... Args>
    BehaviorMethodImpl::BehaviorMethodImpl(R(*functionPointer)(Args...) noexcept, BehaviorContext* context, AZStd::string name)
        : BehaviorMethodImpl(reinterpret_cast<R(*)(Args...)>(functionPointer), context, AZStd::move(name))
    {}

    template <class R, class C, class... Args>
    BehaviorMethodImpl::BehaviorMethodImpl(R(C::* functionPointer)(Args...) noexcept, BehaviorContext* context, AZStd::string name)
        : BehaviorMethodImpl(reinterpret_cast<R(C::*)(Args...)>(functionPointer), context, AZStd::move(name))
    {}

    template <class R, class C, class... Args>
    BehaviorMethodImpl::BehaviorMethodImpl(R(C::* functionPointer)(Args...) const noexcept, BehaviorContext* context, AZStd::string name)
        : BehaviorMethodImpl(reinterpret_cast<R(C::*)(Args...) const>(functionPointer), context, AZStd::move(name))
    {}
#endif
} // namespace AZ::Internal
