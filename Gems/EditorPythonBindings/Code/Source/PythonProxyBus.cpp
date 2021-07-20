/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonProxyBus.h>

#include <Source/PythonUtility.h>
#include <Source/PythonTypeCasters.h>

#include <Source/PythonCommon.h>
#include <Source/PythonSymbolsBus.h>
#include <pybind11/embed.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/std/optional.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        enum class EventType
        {
            Broadcast,
            Event,
            QueueBroadcast,
            QueueEvent
        };

        pybind11::object InvokeEbus(AZ::BehaviorEBus& behaviorEBus, EventType eventType, AZStd::string_view eventName, pybind11::args pythonArgs)
        {
            auto eventIterator = behaviorEBus.m_events.find(eventName);
            AZ_Warning("python", eventIterator != behaviorEBus.m_events.end(), "Event %.*s does not exist in EBus %s", aznumeric_cast<int>(eventName.size()), eventName.data(), behaviorEBus.m_name.c_str());
            if (eventIterator == behaviorEBus.m_events.end())
            {
                return pybind11::cast<pybind11::none>(Py_None);
            }

            auto& behaviorEBusEventSender = eventIterator->second;

            switch (eventType)
            {
                case EventType::Broadcast:
                {
                    AZ_Warning("python", behaviorEBusEventSender.m_broadcast, "EventSender: function %.*s in EBus %s does not support the bus.Broadcast event type.", static_cast<int>(eventName.size()), eventName.data(), behaviorEBus.m_name.c_str());
                    if (behaviorEBusEventSender.m_broadcast)
                    {
                        return Call::StaticMethod(behaviorEBusEventSender.m_broadcast, pythonArgs);
                    }
                    break;
                }
                case EventType::Event:
                {
                    AZ_Warning("python", behaviorEBusEventSender.m_event, "EventSender: function %.*s in EBus %s does not support the bus.Event event type.", static_cast<int>(eventName.size()), eventName.data(), behaviorEBus.m_name.c_str());
                    if (behaviorEBusEventSender.m_event)
                    {
                        return Call::StaticMethod(behaviorEBusEventSender.m_event, pythonArgs);
                    }
                    break;
                }
                case EventType::QueueBroadcast:
                {
                    AZ_Warning("python", behaviorEBusEventSender.m_queueBroadcast, "EventSender: function %.*s in EBus %s does not support the bus.QueueBroadcast event type.", static_cast<int>(eventName.size()), eventName.data(), behaviorEBus.m_name.c_str());
                    if (behaviorEBusEventSender.m_queueBroadcast)
                    {
                        return Call::StaticMethod(behaviorEBusEventSender.m_queueBroadcast, pythonArgs);
                    }
                    break;
                }
                case EventType::QueueEvent:
                {
                    AZ_Warning("python", behaviorEBusEventSender.m_queueEvent, "EventSender: function %.*s in EBus %s does not support the bus.QueueEvent event type.", static_cast<int>(eventName.size()), eventName.data(), behaviorEBus.m_name.c_str());
                    if (behaviorEBusEventSender.m_queueEvent)
                    {
                        return Call::StaticMethod(behaviorEBusEventSender.m_queueEvent, pythonArgs);
                    }
                    break;
                }
                default:
                    AZ_Error("python", false, "Unknown EBus call type %d", eventType);
                    break;
            }

            return pybind11::cast<pybind11::none>(Py_None);
        }
        
        class PythonProxyNotificationHandler final
        {
        public:
            AZ_CLASS_ALLOCATOR(PythonProxyNotificationHandler, AZ::SystemAllocator, 0);

            PythonProxyNotificationHandler(AZStd::string_view busName)
            {
                AZ::BehaviorContext* behaviorContext(nullptr);
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("python", false, "A behavior context is required to bind the buses!");
                    return;
                }

                auto behaviorEBusEntry = behaviorContext->m_ebuses.find(busName);
                if (behaviorEBusEntry == behaviorContext->m_ebuses.end())
                {
                    AZ_Error("python", false, "There is no EBus by the name of %.*s", static_cast<int>(busName.size()), busName.data());
                    return;
                }

                AZ_Assert(behaviorEBusEntry->second, "A null EBus:%s is in the Behavior Context!", behaviorEBusEntry->first.c_str());
                m_ebus = behaviorEBusEntry->second;
            }

            ~PythonProxyNotificationHandler()
            {
                Disconnect();
            }

            bool IsConnected() const
            {
                if (m_handler)
                {
                    return m_handler->IsConnected();
                }
                return false;
            }

            bool Connect(pybind11::object busId)
            {
                if (!m_ebus)
                {
                    AZ_Error("python", false, "EBus not set.");
                    return false;
                }

                if (!CreateHandler(*m_ebus))
                {
                    AZ_Error("python", false, "Could not create a handler for ebus");
                    return false;
                }

                // does the EBus require an address to connect?
                if (m_ebus->m_idParam.m_typeId.IsNull())
                {
                    AZ_Warning("python", busId.is_none(), "Connecting to an singleton EBus but was given a non-None busId(%s)", pybind11::cast<AZStd::string>(busId).c_str());
                    return m_handler->Connect();
                }
                else if (busId.is_none())
                {
                    AZ_Warning("python", busId.is_none(), "Connecting to an EBus that requires an address but was given a None busId");
                    return false;
                }

                Convert::StackVariableAllocator stackVariableAllocator;
                AZ::BehaviorValueParameter busAddress;

                if (!Convert::PythonToBehaviorValueParameter(m_ebus->m_idParam, busId, busAddress, stackVariableAllocator))
                {
                    AZ_Warning("python", busId.is_none(), "Could not convert busId(%s) to address type (%s)",
                        pybind11::cast<AZStd::string>(busId).c_str(), m_ebus->m_idParam.m_typeId.ToString<AZStd::string>().c_str());
                    return false;
                }

                return m_handler->Connect(&busAddress);
            }

            bool Disconnect()
            {
                if (!m_handler)
                {
                    return false;
                }
                m_handler->Disconnect();

                if (m_ebus)
                {
                    DestroyHandler(*m_ebus);
                }
                return true;
            }

            bool AddCallback(AZStd::string_view eventName, pybind11::function callback)
            {
                if (!PyCallable_Check(callback.ptr()))
                {
                    AZ_Error("python", false, "The callback needs to be a callable python function.");
                    return false;
                }

                if (!m_handler)
                {
                    AZ_Error("python", false, "No EBus connection deteced; missing call or failed call to connect()?");
                    return false;
                }

                const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                for (int iEvent = 0; iEvent < static_cast<int>(events.size()); ++iEvent)
                {
                    const AZ::BehaviorEBusHandler::BusForwarderEvent& e = events[iEvent];
                    if (eventName == e.m_name)
                    {
                        AZStd::string eventNameValue{ eventName };
#if defined(AZ_ENABLE_TRACING)
                        const auto& callbackIt = m_callbackMap.find(eventNameValue);
#endif
                        AZ_Warning("python", m_callbackMap.end() == callbackIt, "Replacing callback for eventName:%s", eventNameValue.c_str());
                        m_callbackMap[eventNameValue] = callback;
                        return true;
                    }
                }
                return false;
            }

        protected:
            void DestroyHandler(const AZ::BehaviorEBus& ebus)
            {
                if (m_handler)
                {
                    AZ_Warning("python", ebus.m_destroyHandler, "Ebus (%s) does not have a handler destroyer.", ebus.m_name.c_str());
                    if (ebus.m_destroyHandler)
                    {
                        ebus.m_destroyHandler->Invoke(m_handler);
                    }
                }
                m_handler = nullptr;
                m_callbackMap.clear();
            }

            bool CreateHandler(const AZ::BehaviorEBus& ebus)
            {
                DestroyHandler(ebus);
                
                AZ_Warning("python", ebus.m_createHandler, "Ebus (%s) does not have a handler creator.", ebus.m_name.c_str());
                if (!ebus.m_createHandler)
                {
                    return false;
                }
                
                if (!ebus.m_createHandler->InvokeResult(m_handler))
                {
                    AZ_Warning("python", ebus.m_createHandler, "Ebus (%s) failed to create a handler.", ebus.m_name.c_str());
                    return false;
                }

                if (m_handler)
                {
                    const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                    for (int iEvent = 0; iEvent < static_cast<int>(events.size()); ++iEvent)
                    {
                        m_handler->InstallGenericHook(iEvent, &PythonProxyNotificationHandler::OnEventGenericHook, this);
                    }
                }

                return true;
            }

            static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                reinterpret_cast<PythonProxyNotificationHandler*>(userData)->OnEventGenericHook(eventName, eventIndex, result, numParameters, parameters);
            }

            void OnEventGenericHook(const char* eventName, [[maybe_unused]] int eventIndex, [[maybe_unused]] AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                // find the callback for the event
                const auto& callbackEntry = m_callbackMap.find(eventName);
                if (callbackEntry == m_callbackMap.end())
                {
                    return;
                }
                pybind11::function callback = callbackEntry->second;

                // build the parameters to send to callback
                Convert::StackVariableAllocator stackVariableAllocator;
                pybind11::tuple pythonParamters(numParameters);
                for (int index = 0; index < numParameters; ++index)
                {
                    AZ::BehaviorValueParameter& behaviorValueParameter{ *(parameters + index) };
                    pythonParamters[index] = Convert::BehaviorValueParameterToPython(behaviorValueParameter, stackVariableAllocator);
                    
                    if (pythonParamters[index].is_none())
                    {
                        AZ_Warning("python", false, "Ebus(%s) event(%s) failed to convert parameter at index(%d)", m_ebus->m_name.c_str(), eventName, index);
                        return;
                    }
                }

                try
                {
                    pybind11::object pyResult = callback(pythonParamters);

                    // store the result 
                    if (result && pyResult.is_none() == false)
                    {
                        // reset/prepare the stack allocator
                        m_stackVariableAllocator = {};

                        AZ::BehaviorValueParameter coverted;
                        const AZ::u32 traits = result->m_traits;
                        if (Convert::PythonToBehaviorValueParameter(*result, pyResult, coverted, m_stackVariableAllocator))
                        {
                            result->Set(coverted);
                            result->m_value = coverted.GetValueAddress();
                            if ((traits & AZ::BehaviorParameter::TR_POINTER) == AZ::BehaviorParameter::TR_POINTER)
                            {
                                result->m_value = &result->m_value;
                            }
                        }
                    }
                }
                catch ([[maybe_unused]] const std::exception& e)
                {
                    AZ_Error("python", false, "Python callback threw an exception %s", e.what());
                }
            }

        private:
            const AZ::BehaviorEBus* m_ebus = nullptr;
            AZ::BehaviorEBusHandler* m_handler = nullptr;
            AZStd::unordered_map<AZStd::string, pybind11::function> m_callbackMap;
            Convert::StackVariableAllocator m_stackVariableAllocator;
        };
    }

    namespace PythonProxyBusManagement
    {
        void CreateSubmodule(pybind11::module baseModule)
        {
            AZ::BehaviorContext* behaviorContext(nullptr);
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
            if (!behaviorContext)
            {
                AZ_Error("python", false, "A behavior context is required to bind the buses!");
                return;
            }

            auto busModule = baseModule.def_submodule("bus");
            Module::PackageMapType modulePackageMap;

            // export possible ways an EBus can be invoked
            pybind11::enum_<Internal::EventType>(busModule, "EventType")
                .value("Event", Internal::EventType::Event)
                .value("Broadcast", Internal::EventType::Broadcast)
                .value("QueueEvent", Internal::EventType::QueueEvent)
                .value("QueueBroadcast", Internal::EventType::QueueBroadcast)
                .export_values();

            // export the EBuses flagged for Automation or Common scope
            for (auto&& busEntry : behaviorContext->m_ebuses)
            {
                AZStd::string& ebusName = busEntry.first;
                AZ::BehaviorEBus* behaviorEBus = busEntry.second;
                if (Scope::IsBehaviorFlaggedForEditor(behaviorEBus->m_attributes))
                {
                    auto busCaller = pybind11::cpp_function([behaviorEBus](Internal::EventType eventType, AZStd::string_view eventName, pybind11::args pythonArgs)
                    {
                        return Internal::InvokeEbus(*behaviorEBus, eventType, eventName, pythonArgs);
                    });

                    auto createPythonProxyNotificationHandler = pybind11::cpp_function([behaviorEBus]()
                    {
                        return aznew Internal::PythonProxyNotificationHandler(behaviorEBus->m_name.c_str());
                    });

                    pybind11::module thisBusModule = busModule;
                    auto moduleName = Module::GetName(behaviorEBus->m_attributes);
                    if (moduleName)
                    {
                        // this will place the bus into either:
                        // 1) if the module is valid, then azlmbr.<module name>.<ebus name>
                        // 2) or, then azlmbr.bus.<ebus name>
                        thisBusModule = Module::DeterminePackageModule(modulePackageMap, *moduleName, baseModule, busModule, true);
                    }

                    // for each notification handler type, make a convenient Python type to make the script more Python-ic
                    if (behaviorEBus->m_createHandler && behaviorEBus->m_destroyHandler)
                    {
                        AZStd::string ebusNotificationName{ AZStd::string::format("%sHandler", ebusName.c_str()) };
                        thisBusModule.attr(ebusNotificationName.c_str()) = createPythonProxyNotificationHandler;
                    }

                    // is a request EBus
                    thisBusModule.attr(ebusName.c_str()) = busCaller;

                    // log the bus symbol
                    AZStd::string subModuleName = pybind11::cast<AZStd::string>(thisBusModule.attr("__name__"));
                    PythonSymbolEventBus::Broadcast(&PythonSymbolEventBus::Events::LogBus, subModuleName, ebusName, behaviorEBus);
                }
            }

            // export possible ways an EBus can be invoked
            pybind11::class_<Internal::PythonProxyNotificationHandler>(busModule, "NotificationHandler")
                .def(pybind11::init<AZStd::string_view>())
                .def("is_connected", &Internal::PythonProxyNotificationHandler::IsConnected)
                .def("connect", &Internal::PythonProxyNotificationHandler::Connect, pybind11::arg("busId") = pybind11::none())
                .def("disconnect", &Internal::PythonProxyNotificationHandler::Disconnect)
                .def("add_callback", &Internal::PythonProxyNotificationHandler::AddCallback)
                ;
        }
    }
}
