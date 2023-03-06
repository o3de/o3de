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
    // Contains the EBusBuilderBase non-function template member definitions
    EBusBuilderBase::EBusBuilderBase(BehaviorContext* context, BehaviorEBus* ebus)
        : Base(context)
        , m_ebus(ebus)
    {
        Base::m_currentAttributes = &ebus->m_attributes;
    }

    //////////////////////////////////////////////////////////////////////////
    EBusBuilderBase::~EBusBuilderBase()
    {
        // process all on demand queued reflections
        Base::m_context->ExecuteQueuedOnDemandReflections();

        if (!Base::m_context->IsRemovingReflection())
        {
            for (auto&& [eventName, eventSender] : m_ebus->m_events)
            {
                if (MethodReturnsAzEventByReferenceOrPointer(*eventSender.m_broadcast))
                {
                    ValidateAzEventDescription(*Base::m_context, *eventSender.m_broadcast);
                }
            }
            BehaviorContextBus::Event(Base::m_context, &BehaviorContextBus::Events::OnAddEBus, m_ebus->m_name.c_str(), m_ebus);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    auto EBusBuilderBase::operator->() -> EBusBuilderBase*
    {
        return this;
    }

    auto EBusBuilderBase::VirtualProperty(const char* name, const char* getterEvent, const char* setterEvent)
        -> EBusBuilderBase*
    {
        if (m_ebus)
        {
            BehaviorEBusEventSender* getter = nullptr;
            BehaviorEBusEventSender* setter = nullptr;
            if (getterEvent)
            {
                auto getterIt = m_ebus->m_events.find(getterEvent);
                getter = &getterIt->second;
                if (getterIt == m_ebus->m_events.end())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter event %s is not reflected. Make sure VirtualProperty is reflected after the Event!", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }

                // we should always have the broadcast present, so use it for our checks
                if (!getter->m_broadcast->HasResult())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s should return result", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }
                if (getter->m_broadcast->GetNumArguments() != 0)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s can not have arguments only result", m_ebus->m_name.c_str(), name, getterEvent);
                    return this;
                }
            }
            if (setterEvent)
            {
                auto setterIt = m_ebus->m_events.find(setterEvent);
                if (setterIt == m_ebus->m_events.end())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter event %s is not reflected. Make sure VirtualProperty is reflected after the Event!", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
                setter = &setterIt->second;
                if (setter->m_broadcast->HasResult())
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter %s should not return result", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
                if (setter->m_broadcast->GetNumArguments() != 1)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s setter %s can have only one argument", m_ebus->m_name.c_str(), name, setterEvent);
                    return this;
                }
            }

            if (getter && setter)
            {
                if (getter->m_broadcast->GetResult()->m_typeId != setter->m_broadcast->GetArgument(0)->m_typeId)
                {
                    AZ_Error("BehaviorContext", false, "EBus %s, VirtualProperty %s getter %s return [%s] and setter %s input argument [%s] types don't match ", m_ebus->m_name.c_str(), name, setterEvent, getter->m_broadcast->GetResult()->m_typeId.ToString<AZStd::string>().c_str(), setter->m_broadcast->GetArgument(0)->m_typeId.ToString<AZStd::string>().c_str());
                    return this;
                }
            }

            m_ebus->m_virtualProperties.insert(AZStd::make_pair(name, BehaviorEBus::VirtualProperty(getter, setter)));
        }
        return this;
    }

    EBusAttributes::EBusAttributes(BehaviorContext* context)
        : Base(context)
    {
    }

    void EBusAttributes::SetEBusEventSender(BehaviorEBusEventSender* ebusSender)
    {
        m_currentEBusSender = ebusSender;
    }
} // namespace AZ
