/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DefaultEventHandler.h"
#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventDefinition.h>
#include <ScriptEvents/Internal/VersionedProperty.h>
#include "ScriptEventsBindingBus.h"

namespace ScriptEvents
{
    DefaultBehaviorHandler::~DefaultBehaviorHandler()
    {
        Disconnect();
    }

    DefaultBehaviorHandler::DefaultBehaviorHandler(AZ::BehaviorEBus* ebus, const ScriptEvents::ScriptEvent* scriptEventDefinition)
        : m_ebus(ebus)
    {
        m_busNameId = AZ::Uuid::CreateName(m_ebus->m_name.c_str());

        for (const auto& eventPair : m_ebus->m_events)
        {
            auto* event = (eventPair.second.m_event != nullptr) ? eventPair.second.m_event : eventPair.second.m_broadcast;

            if (event)
            {
                AZ::BehaviorEBusHandler::BusForwarderEvent eventForwarder;
                eventForwarder.m_name = event->m_name.c_str();
                eventForwarder.m_parameters.push_back(*event->GetResult());

                // If a definition is provided, for each method check all the versions until we find the one that matches.
                // This is necessary because we need to use the versioned property's ID as the m_eventId of the forwarder
                // for Script Canvas.
                if (scriptEventDefinition)
                {
                    for (auto& method : scriptEventDefinition->GetMethods())
                    {
                        AZStd::string name = method.GetName();
                        if (name.compare(event->m_name) == 0)
                        {
                            // We need the of this property as they will all have the same ID.
                            eventForwarder.m_eventId = AZ::Crc32(method.GetNameProperty().GetId().ToString<AZStd::string>().c_str());
                        }
                        else
                        {
                            // If the event name doesn't match the current version, check all other versions.
                            for (const auto& version : method.GetNameProperty().GetVersions())
                            {
                                AZStd::string versionName;
                                if (version.Get<AZStd::string>(versionName))
                                {
                                    if (versionName.compare(event->m_name) == 0)
                                    {
                                        // We need the ID of any version of this property as they will all have the same ID.
                                        eventForwarder.m_eventId = AZ::Crc32(version.GetId().ToString<AZStd::string>().c_str());
                                    }
                                }
                            }
                        }
                    }
                }

                // As a fallback, we'll use the Crc32 of the event name
                if (eventForwarder.m_eventId == AZ::Crc32())
                {
                    eventForwarder.m_eventId = AZ::Crc32(event->m_name.c_str());
                }

                // this extra parameter is only needed for broadcast only buses
                bool isAddressable = Types::IsAddressableType(m_ebus->m_idParam.m_typeId);
                if (!isAddressable)
                {
                    eventForwarder.m_parameters.push_back(m_ebus->m_idParam);
                }

                size_t argumentCount = event->GetNumArguments();
                for (size_t i = 0; i < argumentCount; ++i)
                {
                    eventForwarder.m_parameters.push_back(*event->GetArgument(i));
                }

                eventForwarder.m_isFunctionGeneric = true;
                m_events.push_back(eventForwarder);
            }
        }
    }

    int DefaultBehaviorHandler::GetFunctionIndex(const char* name) const
    {
        if (m_ebus->m_events.find(name) == m_ebus->m_events.end())
        {
            AZ_Error("Script Events", false, "No function with the name %s found.", name);
            return -1;
        }

        return static_cast<int>(AZStd::distance(m_ebus->m_events.begin(), m_ebus->m_events.find(name)));
    }


    bool DefaultBehaviorHandler::Connect(AZ::BehaviorArgument* address)
    {
        if (address)
        {
            AZ_Assert(address->m_typeId == m_ebus->m_idParam.m_typeId, "Error EBus %s requires an address of type %s (%s), received %s (%s)",
                m_ebus->m_name.c_str(), m_ebus->m_idParam.m_name, m_ebus->m_idParam.m_typeId.ToString<AZStd::string>().c_str(), address->m_name, address->m_typeId.ToString<AZStd::string>().c_str());

            if (!m_address.m_value && address->m_typeId != azrtti_typeid<void>())
            {
                // get the behavior class for our address
                const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(address->m_typeId);
                AZ_Warning("DefaultBehaviorHandler", behaviorClass, "%s is not a valid reflected class", address->m_name);

                if (behaviorClass)
                {
                    // cache a copy of the bus address for invoking events
                    static_cast<AZ::BehaviorParameter&>(m_address) = m_ebus->m_idParam;
                    m_address.m_value = behaviorClass->Allocate();
                    behaviorClass->m_cloner(m_address.m_value, address->m_value, nullptr);
                }
            }
        }

        Internal::BindingRequestBus::Event(m_busNameId, &Internal::BindingRequest::Connect, &m_address, this);

        return true;
    }

    void DefaultBehaviorHandler::Disconnect(AZ::BehaviorArgument* address [[maybe_unused]])
    {
        // script doesn't support multihandler buses, ignore the optional address parameter
        Internal::BindingRequestBus::Event(m_busNameId, &Internal::BindingRequest::Disconnect, &m_address, this);

        if (m_address.m_value)
        {
            const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(m_address.m_typeId);
            AZ_Assert(behaviorClass, "Did not find class %s when disconnecting DefaultBehaviorHandler", m_ebus->m_name.c_str());

            behaviorClass->m_destructor(m_address.m_value, behaviorClass->m_userData);

            behaviorClass->Deallocate(m_address.m_value);
            m_address.m_value = nullptr;
        }
    }
}
