/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    template<class BusId>
    bool BehaviorEBusHandler::Connect(BusId id)
    {
        BehaviorArgument p(&id);
        return Connect(&p);
    }

    template<class BusId>
    void BehaviorEBusHandler::Disconnect(BusId id)
    {
        BehaviorArgument p(&id);
        Disconnect(&p);
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Hook>
    bool BehaviorEBusHandler::InstallHook(int index, Hook h, void* userData)
    {
        if (index != -1)
        {
            // Check parameters
            if (!Internal::SetFunctionParameters<typename AZStd::remove_pointer<Hook>::type>::Check(m_events[index].m_parameters))
            {
                return false;
            }

            m_events[index].m_isFunctionGeneric = false;
            m_events[index].m_function = reinterpret_cast<void*>(h);
            m_events[index].m_userData = userData;
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Hook>
    bool BehaviorEBusHandler::InstallHook(const char* name, Hook h, void* userData)
    {
        return InstallHook(GetFunctionIndex(name), h, userData);
    };

    //////////////////////////////////////////////////////////////////////////
    template<class Event>
    void BehaviorEBusHandler::SetEvent(Event e, const char* name)
    {
        (void)e;
        int i = GetFunctionIndex(name);
        if (i != -1)
        {
            m_events.resize(i + 1);
            m_events[i].m_name = name;
            m_events[i].m_eventId = AZ::Crc32(name);
            m_events[i].m_function = nullptr;
            AZ::Internal::SetFunctionParameters<typename AZStd::remove_pointer<Event>::type>::Set(m_events[i].m_parameters);
            m_events[i].m_metadataParameters.resize(m_events[i].m_parameters.size());
        }
    }

    template<class Event>
    void BehaviorEBusHandler::SetEvent(Event e, AZStd::string_view name, const BehaviorParameterOverridesArray<Event>& args)
    {
        (void)e;
        int i = GetFunctionIndex(name.data());
        if (i != -1)
        {
            m_events.resize(i + 1);
            m_events[i].m_name = name.data();
            m_events[i].m_eventId = AZ::Crc32(name.data());
            m_events[i].m_function = nullptr;
            AZ::Internal::SetFunctionParameters<typename AZStd::remove_pointer<Event>::type>::Set(m_events[i].m_parameters);
            m_events[i].m_metadataParameters.resize(m_events[i].m_parameters.size());
            for (size_t argIndex = 0; argIndex < args.size(); ++argIndex)
            {
                m_events[i].m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::ParameterFirst + argIndex] = { args[argIndex].m_name, args[argIndex].m_toolTip };
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    template<class... Args>
    void BehaviorEBusHandler::Call(int index, Args&&... args) const
    {
        const BusForwarderEvent& e = m_events[index];
        if (e.m_function)
        {
            if (e.m_isFunctionGeneric)
            {
                BehaviorArgument arguments[sizeof...(args) + 1] = { &args... };
                reinterpret_cast<GenericHookType>(e.m_function)(const_cast<void*>(e.m_userData), e.m_name, index, nullptr, AZ_ARRAY_SIZE(arguments) - 1, arguments);
            }
            else
            {
                typedef void(*FunctionType)(void*, Args...);
                reinterpret_cast<FunctionType>(e.m_function)(const_cast<void*>(e.m_userData), args...);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    template<class R, class... Args>
    void BehaviorEBusHandler::CallResult(R& result, int index, Args&&... args) const
    {
        const BusForwarderEvent& e = m_events[index];
        if (e.m_function)
        {
            if (e.m_isFunctionGeneric)
            {
                BehaviorArgument arguments[sizeof...(args) + 1] = { &args... };
                BehaviorArgument r(&result);
                reinterpret_cast<GenericHookType>(e.m_function)(const_cast<void*>(e.m_userData), e.m_name, index, &r, AZ_ARRAY_SIZE(arguments) - 1, arguments);
                // Assign on top of the the value if the param isn't a pointer
                // (otherwise the pointer just gets overridden and no value is returned).
                if ((r.m_traits & BehaviorParameter::TR_POINTER) == 0 && r.GetAsUnsafe<R>())
                {
                    result = *r.GetAsUnsafe<R>();
                }
            }
            else
            {
                typedef R(*FunctionType)(void*, Args...);
                result = reinterpret_cast<FunctionType>(e.m_function)(const_cast<void*>(e.m_userData), args...);
            }
        }
    }
} // namespace AZ
