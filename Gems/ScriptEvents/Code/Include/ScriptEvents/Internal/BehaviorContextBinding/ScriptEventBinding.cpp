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

#include "precompiled.h"

#include "DefaultEventHandler.h"
#include <ScriptEvents/ScriptEventTypes.h>
#include <ScriptEvents/Internal/BehaviorContextBinding/ScriptEventBinding.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptEvents
{
    namespace
    {
        void Call(const AZ::BehaviorEBusHandler::BusForwarderEvent& forwarderEvent, int functionIndex, const Internal::BindingRequest::BindingParameters& parameter)
        {
            const AZ::u32 referenceTypes = AZ::BehaviorParameter::TR_POINTER & AZ::BehaviorParameter::TR_REFERENCE;

            if (parameter.m_returnValue && !(parameter.m_returnValue->m_traits & referenceTypes))
            {
                AZ::BehaviorValueParameter returnValue(*parameter.m_returnValue);
                reinterpret_cast<AZ::BehaviorEBusHandler::GenericHookType>(forwarderEvent.m_function)(forwarderEvent.m_userData, forwarderEvent.m_name, functionIndex, &returnValue, parameter.m_parameterCount, parameter.m_parameters);

                // check for BehaviorClass type that must be cloned back to storage pointed to by m_returnValue.GetValueAddress(), regardless if GenericHookType() returned a pointer
                if (returnValue.GetValueAddress() != parameter.m_returnValue->GetValueAddress())
                {
                    if (returnValue.GetValueAddress())
                    {
                        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(parameter.m_returnValue->m_typeId);
                        if (behaviorClass && behaviorClass->m_cloner)
                        {
                            behaviorClass->m_cloner(parameter.m_returnValue->GetValueAddress(), returnValue.GetValueAddress(), nullptr);
                        }
                        else if (behaviorClass)
                        {
                            AZ_Error("ScriptEvents", false, "A ScriptEvent returned a class without a supported cloning function. Supply a cloning function for: %s.", behaviorClass->m_name.data());
                        }
                        else
                        {
                            AZ_Error("ScriptEvents", false, "A ScriptEvent returned a class that is not exposed to BehaviorContext.");
                        }
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", false, "A ScriptEvent call was supposed to return a value and returned none.");
                    }
                }
            }
            else
            {
                reinterpret_cast<AZ::BehaviorEBusHandler::GenericHookType>(forwarderEvent.m_function)(forwarderEvent.m_userData, forwarderEvent.m_name, functionIndex, parameter.m_returnValue, parameter.m_parameterCount, parameter.m_parameters);
            }
        }

        //! Searches the behavior context for a specified equal operator implementation
        //! for the given behavior class
        AZ::BehaviorMethod* FindEqualityOperatorMethod(const AZ::BehaviorClass* behaviorClass)
        {
            AZ_Assert(behaviorClass, "Invalid AZ::BehaviorClass submitted to FindEqualityOperatorMethod");
            for (const auto& equalMethodCandidatePair : behaviorClass->m_methods)
            {
                const AZ::AttributeArray& attributes = equalMethodCandidatePair.second->m_attributes;
                for (const auto& attributePair : attributes)
                {
                    if (attributePair.second->RTTI_IsTypeOf(AZ::AzTypeInfo<AZ::AttributeData<AZ::Script::Attributes::OperatorType>>::Uuid()))
                    {
                        const auto& attributeData = AZ::RttiCast<AZ::AttributeData<AZ::Script::Attributes::OperatorType>*>(attributePair.second);
                        if (attributeData->Get(nullptr) == AZ::Script::Attributes::OperatorType::Equal)
                        {
                            return equalMethodCandidatePair.second;
                        }
                    }
                }
            }
            return nullptr;
        }

    }

    ScriptEventBinding::ScriptEventBinding(AZ::BehaviorContext* context, AZStd::string_view scriptEventName, const AZ::Uuid& addressType)
        : m_context(context)
        , m_scriptEventName(scriptEventName)
    {
        m_busBindingAddress = AZ::Uuid::CreateName(scriptEventName.data());
        Internal::BindingRequestBus::Handler::BusConnect(m_busBindingAddress);

        if (ScriptEvents::Types::IsAddressableType(addressType))
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(addressType);
            m_equalityOperatorMethod = FindEqualityOperatorMethod(behaviorClass);
            AZ_Assert(m_equalityOperatorMethod, "Address type %s for %s must implement an equality operator, see AZ::Script::Attributes::OperatorType::Equal", addressType.ToString<AZStd::string>().c_str(), scriptEventName.data());
        }
    }

    ScriptEventBinding::~ScriptEventBinding()
    {
        Internal::BindingRequestBus::Handler::BusDisconnect(m_busBindingAddress);
    }

    void ScriptEventBinding::Bind(const BindingParameters& parameter)
    {
        // If an address is not provided, this script event will be broadcast
        if (!parameter.m_address || parameter.m_address->m_typeId.IsNull())
        {
                for (ScriptEventsHandler* eventHandler : m_broadcasts)
                {
                    int functionIndex = eventHandler->GetFunctionIndex(parameter.m_eventName.data());
                    if (functionIndex >= 0 && functionIndex < eventHandler->GetEvents().size())
                    {
                        const AZ::BehaviorEBusHandler::BusForwarderEvent& forwarderEvent = eventHandler->GetEvents()[functionIndex];
                        if (!forwarderEvent.m_function)
                        {
                            // Note, this may be OK if it happened in Script Canvas, we can't reasonably expect every event to be handled, I just need to be sure that
                            // if there is a handler node that we don't get this.
                            AZ_WarningOnce("Script Events", forwarderEvent.m_function, "Function %s not found for event: %s in script: %s - if needed, provide an implementation.", parameter.m_eventName.data(), m_scriptEventName.data(), eventHandler->GetScriptPath().c_str());
                        }
                        else
                        {
                            Call(forwarderEvent, functionIndex, parameter);
                        }
                    }
                }

            // Broadcast event to all
            for (EventBindingEntry& handlerEntry : m_events)
            {
                for (ScriptEventsHandler* eventHandler : handlerEntry.second)
                {
                    int functionIndex = eventHandler->GetFunctionIndex(parameter.m_eventName.data());
                    if (functionIndex >= 0 && functionIndex < eventHandler->GetEvents().size())
                    {
                        const AZ::BehaviorEBusHandler::BusForwarderEvent& forwarderEvent = eventHandler->GetEvents()[functionIndex];
                        if (!forwarderEvent.m_function)
                        {
                            AZ_WarningOnce("Script Events", forwarderEvent.m_function, "Function %s not found for event: %s in script: %s - if needed, provide an implementation.", parameter.m_eventName.data(), m_scriptEventName.data(), eventHandler->GetScriptPath().c_str());
                        }
                        else
                        {
                            Call(forwarderEvent, functionIndex, parameter);
                        }
                    }
                }
            }
        }
        else
        {
            size_t addressHash = GetAddressHash(parameter.m_address);
            EventMap::iterator eventIterator = m_events.find(addressHash);

            if (eventIterator != m_events.end())
            {
                // look for exact matches within the hash bucket

                // Handlers may be disconnected as a result of this operation, making a copy here to avoid iterating over removed elements of m_events
                EventSet events = eventIterator->second;
                for (ScriptEventsHandler* handler : events)
                {
                    AZ::BehaviorClass* addressTypeClass = m_context->m_typeToClassMap.at(parameter.m_address->m_typeId);

                    bool isEqual = true;

                    // use the default comparer for classes exposed through behaviorContext->Class<SomeType>(
                    if (m_equalityOperatorMethod)
                    {
                        AZ::BehaviorValueParameter addresses[2];
                        // we are going to call an equality operator on this, but the behavior method expects the args to be continuous

                        // capture the value of the address
                        if (m_equalityOperatorMethod->GetArgument(0)->m_traits & AZ::BehaviorParameter::TR_POINTER)
                        {
                            addresses[0].m_value = &parameter.m_address->m_value;
                        }
                        else
                        {
                            addresses[0].m_value = parameter.m_address->m_value;
                        }
                        *static_cast<AZ::BehaviorParameter*>(&addresses[0]) = static_cast<const AZ::BehaviorParameter&>(*parameter.m_address);
                        addresses[0].m_tempData = parameter.m_address->m_tempData;
                        addresses[0].m_traits = m_equalityOperatorMethod->GetArgument(0)->m_traits;

                        // capture the value stored in the handler
                        if (m_equalityOperatorMethod->GetArgument(1)->m_traits & AZ::BehaviorParameter::TR_POINTER)
                        {
                            addresses[1].m_value = &handler->GetBusId()->m_value;
                        }
                        else
                        {
                            addresses[1].m_value = handler->GetBusId()->m_value;
                        }
                        *static_cast<AZ::BehaviorParameter*>(&addresses[1]) = static_cast<const AZ::BehaviorParameter&>(*handler->GetBusId());
                        addresses[1].m_tempData = handler->GetBusId()->m_tempData;
                        addresses[1].m_traits = m_equalityOperatorMethod->GetArgument(1)->m_traits;

                        AZ::BehaviorValueParameter addressMatch;
                        addressMatch.Set(&isEqual);

                        m_equalityOperatorMethod->Call(addresses, 2, &addressMatch);
                    }
                    else if (addressTypeClass->m_equalityComparer)
                    {
                        isEqual = addressTypeClass->m_equalityComparer(parameter.m_address->m_value, handler->GetBusId()->m_value, nullptr);
                    }

                    if (isEqual)
                    {
                        int functionIndex = handler->GetFunctionIndex(parameter.m_eventName.data());
                        if (functionIndex >= 0 && functionIndex < handler->GetEvents().size())
                        {
                            AZ::BehaviorEBusHandler::BusForwarderEvent forwarderEvent = handler->GetEvents()[functionIndex];
                            AZ_WarningOnce("Script Events", forwarderEvent.m_function, "Function %s not found for event: %s in script: %s - if needed, provide an implementation.", parameter.m_eventName.data(), m_scriptEventName.data(), handler->GetScriptPath().c_str());

                            if (forwarderEvent.m_function)
                            {
                                Call(forwarderEvent, functionIndex, parameter);
                            }
                        }
                    }
                }
            }
        }
    }

    void ScriptEventBinding::Connect(const AZ::BehaviorValueParameter* address, ScriptEventsHandler* handler)
    {
        AZ_Warning("Script Event", address, "%s: Address was not specified when connecting,", m_scriptEventName.data());
        if (address && address->m_value)
        {
            size_t addressHash = GetAddressHash(address);
            m_events[addressHash].insert(handler);
        }
        else
        {
            m_broadcasts.insert(handler);
        }
    }

    void ScriptEventBinding::Disconnect(const AZ::BehaviorValueParameter* address, ScriptEventsHandler* handler)
    {
        if (address->m_value)
        {
            size_t addressHash = GetAddressHash(address);

            auto eventIter = m_events.find(addressHash);

            if (eventIter != m_events.end())
            {
                eventIter->second.erase(handler);

                if (eventIter->second.empty())
                {
                    m_events.erase(addressHash);
                }
            }
        }
        else
        {
            m_broadcasts.erase(handler);
            for (EventBindingEntry& eventBindingEntry : m_events)
            {
                eventBindingEntry.second.erase(handler);
            }
        }
    }

    void ScriptEventBinding::RemoveHandler(ScriptEventsHandler* handler)
    {
        m_broadcasts.erase(handler);
    }

    size_t ScriptEventBinding::GetAddressHash(const AZ::BehaviorValueParameter* address)
    {
        const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(address->m_typeId);
        AZ_Assert(behaviorClass, "The specified type %s is not in the Behavior Context, make sure it is reflected.", address->m_name);
        return behaviorClass->m_valueHasher(address->m_value);
    }

}
