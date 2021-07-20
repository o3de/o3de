/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReceiveScriptEvent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetManager.h>

#include <ScriptCanvas/Execution/ErrorBus.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Libraries/Core/EventHandlerTranslationUtility.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            const char* ReceiveScriptEvent::c_busIdName = "Source";
            const char* ReceiveScriptEvent::c_busIdTooltip = "ID used to connect on a specific Event address";

            ReceiveScriptEvent::ReceiveScriptEvent()
                : m_connected(false)
            {}

            ReceiveScriptEvent::~ReceiveScriptEvent()
            {
                if (m_connected && m_handler)
                {
                    m_handler->Disconnect();
                }

                if (m_ebus && m_handler && m_ebus->m_destroyHandler)
                {
                    m_ebus->m_destroyHandler->Invoke(m_handler);
                }

                m_ebus = nullptr;
                m_handler = nullptr;
            }

            void ReceiveScriptEvent::CompleteInitialize(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset)
            {
                bool wasConfigured = IsConfigured();

                SlotIdMapping populationMapping;
                PopulateAsset(asset, populationMapping);

                // If we are configured, but we added more elements. We want to update.
                if (wasConfigured && !populationMapping.empty())
                {
                    m_eventSlotMapping.insert(populationMapping.begin(), populationMapping.end());
                }
                else if (!wasConfigured)
                {
                    m_eventSlotMapping = populationMapping;
                }
            }

            void ReceiveScriptEvent::PopulateAsset(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, SlotIdMapping& populationMapping)
            {
                if (CreateHandler(asset))
                {
                    if (!CreateEbus())
                    {
                        // Asset version is likely out of date with this event - for now prompt to open and re-save, TODO: auto fix graph.
                        AZ::Data::AssetInfo assetInfo;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, asset.GetId());

                        AZStd::string graphAssetName;
                        RuntimeRequestBus::EventResult(graphAssetName, GetOwningScriptCanvasId(), &RuntimeRequests::GetAssetName);

                        AZ_Error("Script Event", false, "The Script Event asset (%s) has been modified. Open the graph (%s) and re-save it.", assetInfo.m_relativePath.c_str(), graphAssetName.c_str());
                        return;
                    }

                    const bool wasConfigured = IsConfigured();

                    if (!wasConfigured)
                    {
                        AZ::Uuid addressTypeId = m_definition.GetAddressType();
                        AZ::Uuid addressId = m_definition.GetAddressTypeProperty().GetId();

                        if (m_definition.IsAddressRequired())
                        {
                            bool isNewSlot = true;

                            const auto busToolTip = AZStd::string::format("%s (Type: %s)", c_busIdTooltip, m_ebus->m_idParam.m_name);
                            const AZ::TypeId& busId = m_ebus->m_idParam.m_typeId;                            

                            SlotId addressSlotId;

                            DataSlotConfiguration config;
                            auto remappingIter = m_eventSlotMapping.find(addressId);

                            if (remappingIter != m_eventSlotMapping.end())
                            {
                                isNewSlot = false;
                                config.m_slotId = remappingIter->second;
                            }

                            config.m_name = c_busIdName;
                            config.m_toolTip = busToolTip;
                            config.SetConnectionType(ConnectionType::Input);

                            if (busId == azrtti_typeid<AZ::EntityId>())
                            {
                                config.SetDefaultValue(GraphOwnerId);
                                config.m_contractDescs = { { []() { return aznew RestrictedTypeContract({ Data::Type::EntityID() }); } } };
                            }
                            else
                            {
                                Data::Type busIdType(AZ::BehaviorContextHelper::IsStringParameter(m_ebus->m_idParam) ? Data::Type::String() : Data::FromAZType(busId));                                
                                
                                config.ConfigureDatum(AZStd::move(Datum(busIdType, Datum::eOriginality::Original)));
                                config.m_contractDescs = { { [busIdType]() { return aznew RestrictedTypeContract({ busIdType }); } } };
                            }

                            addressSlotId = AddSlot(config, isNewSlot);

                            populationMapping[addressId] = addressSlotId;
                        }
                    }

                    if (CreateEbus())
                    {
                        const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                        for (int eventIndex(0); eventIndex < events.size(); ++eventIndex)
                        {
                            InitializeEvent(asset, eventIndex, populationMapping);
                        }
                    }
                }
            }


            void ReceiveScriptEvent::InitializeEvent(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset, int eventIndex, SlotIdMapping& populationMapping)
            {
                if (!m_handler)
                {
                    AZ_Error("Script Canvas", false, "BehaviorEBusHandler is nullptr. Cannot initialize event");
                    return;
                }

                const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                if (eventIndex >= events.size())
                {
                    AZ_Error("Script Canvas", false, "Event index %d is out of range. Total number of events: %zu", eventIndex, events.size());
                    return;
                }

                const AZ::BehaviorEBusHandler::BusForwarderEvent& event = events[eventIndex];

                if (m_version == 0)
                {
                    m_version = m_definition.GetVersion();
                }

                auto methodDefinition = AZStd::find_if(m_definition.GetMethods().begin(), m_definition.GetMethods().end(), [&event](const ScriptEvents::Method& methodDefinition)->bool { return methodDefinition.GetEventId() == event.m_eventId; });
                if (methodDefinition == m_definition.GetMethods().end())
                {
                    AZ_Assert(false, "The script event definition does not have the event for which this method was created.");
                    return;
                }

                AZ::Uuid namePropertyId = methodDefinition->GetNameProperty().GetId();
                auto eventId = AZ::Crc32(namePropertyId.ToString<AZStd::string>().c_str());

                m_userData.m_handler = this;
                m_userData.m_methodDefinition = methodDefinition;

                AZ_Assert(!event.m_parameters.empty(), "No parameters in event!");
                if (!events[eventIndex].m_function)
                {
                    m_handler->InstallGenericHook(events[eventIndex].m_name, &OnEventGenericHook, &m_userData);
                }

                if (m_eventMap.find(eventId) == m_eventMap.end())
                {
                    m_eventMap[eventId] = ConfigureEbusEntry(*methodDefinition, event, populationMapping);
                }

                PopulateNodeType();
            }

            Internal::ScriptEventEntry ReceiveScriptEvent::ConfigureEbusEntry(const ScriptEvents::Method& methodDefinition, const AZ::BehaviorEBusHandler::BusForwarderEvent& event, SlotIdMapping& populationMapping)
            {
                const size_t sentinel(event.m_parameters.size());

                Internal::ScriptEventEntry eBusEventEntry;
                eBusEventEntry.m_scriptEventAssetId = m_scriptEventAssetId;
                eBusEventEntry.m_numExpectedArguments = static_cast<int>(sentinel - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst);

                if (event.HasResult())
                {
                    const AZ::BehaviorParameter& argument(event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result]);
                    Data::Type inputType(AZ::BehaviorContextHelper::IsStringParameter(argument) ? Data::Type::String() : Data::FromAZType(argument.m_typeId));
                    const AZStd::string argumentTypeName = Data::GetName(inputType);

                    AZ::Uuid resultIdentifier = methodDefinition.GetReturnTypeProperty().GetId();

                    bool isNewSlot = true;

                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = AZStd::string::format("Result: %s", argumentTypeName.c_str());

                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    slotConfiguration.ConfigureDatum(AZStd::move(Datum(inputType, Datum::eOriginality::Copy, nullptr, AZ::Uuid::CreateNull())));
                    slotConfiguration.m_addUniqueSlotByNameAndType = false;

                    auto remappingIdIter = m_eventSlotMapping.find(resultIdentifier);

                    if (remappingIdIter != m_eventSlotMapping.end())
                    {
                        isNewSlot = false;
                        slotConfiguration.m_slotId = remappingIdIter->second;
                    }

                    SlotId slotId = AddSlot(slotConfiguration, isNewSlot);

                    populationMapping[resultIdentifier] = slotId;
                    eBusEventEntry.m_resultSlotId = slotId;
                }

                for (size_t parameterIndex(AZ::eBehaviorBusForwarderEventIndices::ParameterFirst)
                    ; parameterIndex < sentinel
                    ; ++parameterIndex)
                {
                    bool isNewSlot = true;

                    const AZ::BehaviorParameter& parameter(event.m_parameters[parameterIndex]);
                    Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(parameter) ? Data::Type::String() : Data::FromAZType(parameter.m_typeId));

                    // Get the name and tooltip from the script event definition
                    size_t eventParamIndex = parameterIndex - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst;
                    const auto& parameterDefinitions = methodDefinition.GetParameters();
                    if (!parameterDefinitions.empty())
                    {
                        AZStd::string argName = parameterDefinitions[eventParamIndex].GetName();
                        AZStd::string argToolTip = parameterDefinitions[eventParamIndex].GetTooltip();
                        AZ::Uuid argIdentifier = parameterDefinitions[eventParamIndex].GetNameProperty().GetId();

                        if (argName.empty())
                        {
                            argName = AZStd::string::format("%s", Data::GetName(outputType).c_str());
                        }

                        DataSlotConfiguration slotConfiguration;
                        slotConfiguration.m_name = argName;
                        slotConfiguration.m_toolTip = argToolTip;
                        slotConfiguration.SetConnectionType(ConnectionType::Output);
                        slotConfiguration.m_addUniqueSlotByNameAndType = false;

                        auto remappingIdIter = m_eventSlotMapping.find(argIdentifier);

                        if (remappingIdIter != m_eventSlotMapping.end())
                        {
                            isNewSlot = false;
                            slotConfiguration.m_slotId = remappingIdIter->second;
                        }

                        slotConfiguration.SetType(outputType);

                        SlotId slotId = AddSlot(slotConfiguration, isNewSlot);

                        AZ_Error("ScriptCanvas", populationMapping.find(argIdentifier) == populationMapping.end(), "Trying to create the same slot twice. Unable to create sane mapping.");
                    
                        populationMapping[argIdentifier] = slotId;
                        eBusEventEntry.m_parameterSlotIds.push_back(slotId);
                    }
                }

                const AZStd::string eventID(AZStd::string::format("%s%s", k_EventOutPrefix, event.m_name));

                {
                    AZ::Uuid outputSlotId = methodDefinition.GetNameProperty().GetId();
                    ExecutionSlotConfiguration slotConfiguration;

                    slotConfiguration.m_isLatent = true;
                    slotConfiguration.m_name = eventID;
                    slotConfiguration.SetConnectionType(ConnectionType::Output);                    
                    slotConfiguration.m_addUniqueSlotByNameAndType = true;

                    auto remappingIter = m_eventSlotMapping.find(outputSlotId);

                    if (remappingIter != m_eventSlotMapping.end())
                    {
                        slotConfiguration.m_slotId = remappingIter->second;
                    }

                    SlotId slotId = AddSlot(slotConfiguration);

                    populationMapping[outputSlotId] = slotId;
                    eBusEventEntry.m_eventSlotId = slotId;

                    AZ_Assert(eBusEventEntry.m_eventSlotId.IsValid(), "the event execution out slot must be valid");
                }

                eBusEventEntry.m_eventName = event.m_name;

                return eBusEventEntry;
            }

            void ReceiveScriptEvent::OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                ReceiveScriptEvent::EventHookUserData* eventHookUserData(reinterpret_cast<ReceiveScriptEvent::EventHookUserData*>(userData));
                eventHookUserData->m_handler->OnEvent(eventName, eventIndex, result, numParameters, parameters);
            }

            void ReceiveScriptEvent::SetAutoConnectToGraphOwner(bool enabled)
            {
                ScriptCanvas::Slot* connectSlot = ReceiveScriptEventProperty::GetConnectSlot(this);

                if (connectSlot)
                {
                    m_autoConnectToGraphOwner = enabled && !IsConnected(*connectSlot);
                }
            }

            void ReceiveScriptEvent::OnEvent(const char* eventName, const int, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ReceiveScriptEvent::OnEvent %s", eventName);

                SCRIPTCANVAS_RETURN_IF_ERROR_STATE((*this));

                ScriptEvents::Method method;
                if (!GetScriptEvent().FindMethod(eventName, method))
                {
                    AZ_Warning("Script Events", false, "Failed to find method: %s when trying to handle it.", eventName);
                    return;
                }

                auto iter = m_eventMap.find(AZ::Crc32(method.GetNameProperty().GetId().ToString<AZStd::string>().c_str()));
                if (iter == m_eventMap.end())
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Invalid event index in Ebus handler");
                    return;
                }

                Internal::ScriptEventEntry& scriptEventEntry = iter->second;

                if (!scriptEventEntry.m_shouldHandleEvent || scriptEventEntry.m_isHandlingEvent)
                {
                    AZ_Warning("ScriptCanvas", !scriptEventEntry.m_isHandlingEvent, "Found situation where in handling event(%s::%s) triggered the same event. Possible infinite loop, not handling second call.", GetScriptEvent().GetName().c_str(), eventName);
                    return;
                }

                scriptEventEntry.m_isHandlingEvent = true;

                scriptEventEntry.m_resultEvaluated = !scriptEventEntry.IsExpectingResult();

                // route the parameters to the connect nodes input
                if (scriptEventEntry.m_parameterSlotIds.size() != numParameters)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "The Script Event, %s, has %d parameters. While the ScriptCanvas Node (%s) has (%zu) slots. Make sure the node is updated in the Script Canvas graph.", eventName, numParameters, GetNodeName().c_str(), scriptEventEntry.m_parameterSlotIds.size());
                }
                else
                {
                    for (int parameterIndex(0); parameterIndex < numParameters; ++parameterIndex)
                    {
                        const Slot* slot = GetSlot(scriptEventEntry.m_parameterSlotIds[parameterIndex]);
                        const auto& value = *(parameters + parameterIndex);
                        const Datum input(value);

                        if (slot->IsTypeMatchFor(input.GetType()))
                        {
                            PushOutput(input, *slot);
                        }
                        else
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Type mismatch in Script Event %s on ScriptCanvas Node (%s). Make sure the nodes is updated in the Script Canvas graph", eventName, GetNodeName().c_str());
                        }
                    }
                }

                {
                    // now, this should pass execution off to the nodes that will push their output into this result input
                    SignalOutput(scriptEventEntry.m_eventSlotId, ExecuteMode::UntilNodeIsFoundInStack);
                }

                // route executed nodes output to my input, and my input to the result
                if (scriptEventEntry.IsExpectingResult())
                {
                    if (result)
                    {
                        if (const Datum* resultInput = FindDatum(scriptEventEntry.m_resultSlotId))
                        {
                            scriptEventEntry.m_resultEvaluated = resultInput->ToBehaviorContext(*result);
                            AZ_Warning("Script Canvas", scriptEventEntry.m_resultEvaluated, "%s expects a result value of type %s to be provided, got %s", scriptEventEntry.m_eventName.c_str(), Data::GetName(resultInput->GetType()).c_str(), result->m_name);
                        }
                        else
                        {
                            AZ_Warning("Script Canvas", false, "Script Canvas handler expecting a result, but had no ability to return it");
                        }
                    }
                    else
                    {
                        AZ_Warning("Script Canvas", false, "Script Canvas handler is expecting a result, but was called without receiving one!");
                    }   
                }
                else
                {
                    AZ_Warning("Script Canvas", !result, "Script Canvas handler is not expecting a result, but was called receiving one!");
                }

                // route executed nodes output to my input, and my input to the result
                AZ_Warning("Script Canvas", (result != nullptr) == scriptEventEntry.IsExpectingResult(), "Node %s-%s mismatch between expecting a result and getting one!", m_ebus->m_name.c_str(), scriptEventEntry.m_eventName.c_str());
                AZ_Warning("Script Canvas", scriptEventEntry.m_resultEvaluated, "Node %s-%s result not evaluated properly!", m_ebus->m_name.c_str(), scriptEventEntry.m_eventName.c_str());

                scriptEventEntry.m_isHandlingEvent = false;
            }

            void ReceiveScriptEvent::OnActivate()
            {
                SetAutoConnectToGraphOwner(m_autoConnectToGraphOwner);
                ScriptEventBase::OnActivate();
            }

            void ReceiveScriptEvent::OnPostActivate()
            {
                
            }

            void ReceiveScriptEvent::OnDeactivate()
            {
                Disconnect(false);

                ScriptEventBase::OnDeactivate();
            }
            
            void ReceiveScriptEvent::Connect()
            {
                AZ::EntityId connectToEntityId;
                AZ::BehaviorValueParameter busIdParameter;
                busIdParameter.Set(m_ebus->m_idParam);
                const AZ::Uuid busIdType = m_ebus->m_idParam.m_typeId;

                const Datum* busIdDatum = IsIDRequired() ? FindDatum(GetSlotId(c_busIdName)) : nullptr;
                if (busIdDatum && !busIdDatum->Empty())
                {
                    if (busIdDatum->IS_A(Data::FromAZType(busIdType)) || busIdDatum->IsConvertibleTo(Data::FromAZType(busIdType)))
                    {
                        auto busIdOutcome = busIdDatum->ToBehaviorValueParameter(m_ebus->m_idParam);
                        if (busIdOutcome.IsSuccess())
                        {
                            busIdParameter = busIdOutcome.TakeValue();
                        }
                    }

                    if (busIdType == azrtti_typeid<AZ::EntityId>())
                    {
                        if (auto busEntityId = busIdDatum->GetAs<AZ::EntityId>())
                        {
                            if (!busEntityId->IsValid() || *busEntityId == ScriptCanvas::GraphOwnerId)
                            {
                                RuntimeRequestBus::EventResult(connectToEntityId, GetOwningScriptCanvasId(), &RuntimeRequests::GetRuntimeEntityId);
                                busIdParameter.m_value = &connectToEntityId;
                            }
                        }
                    }
                }

                if (!IsIDRequired() || busIdParameter.GetValueAddress())
                {
                    if (m_connected)
                    {
                        // Ensure we disconnect if this bus is already connected, this could happen if a different bus Id is provided
                        // and this node is connected through the Connect slot.
                        m_handler->Disconnect();
                    }

                    AZ_VerifyError("Script Canvas", m_handler->Connect(&busIdParameter),
                        "Unable to connect to EBus with BusIdType %s. The BusIdType of the Script Event (%s) does not match the BusIdType provided.",
                        busIdType.ToString<AZStd::string>().data(), m_definition.GetName().c_str());

                    m_connected = true;
                }
            }

            void ReceiveScriptEvent::CompleteDisconnection()
            {
                m_handler->Disconnect();
                m_connected = false;

                if (IsActivated())
                {
                    SlotId onDisconnectSlotId = ReceiveScriptEventProperty::GetOnDisconnectedSlotId(this);
                    SignalOutput(onDisconnectSlotId);
                }
            }

            void ReceiveScriptEvent::Disconnect(bool queueDisconnect)
            {
                if (m_connected && m_handler)
                {
                    if (queueDisconnect)
                    {
                        AZ::SystemTickBus::QueueFunction(
                            [this]()
                            {
                                CompleteDisconnection();
                            }
                        );
                    }
                    else
                    {
                        CompleteDisconnection();
                    }
                }
            }

            const Internal::ScriptEventEntry* ReceiveScriptEvent::FindEventWithSlot(const Slot& slot) const
            {
                auto slotId = slot.GetId();
                auto iter = AZStd::find_if(m_eventMap.begin(), m_eventMap.end(), [slotId](const auto& iter) { return iter.second.ContainsSlot(slotId); });
                return iter != m_eventMap.end() ? &iter->second : nullptr;
            }

            AZStd::string ReceiveScriptEvent::GetEBusName() const
            {
                if (!m_asset || !m_asset.IsReady())
                {
                    AZ_Error("ScriptCanvas", false, "Script Event asset %s is not ready.", m_scriptEventAssetId.ToString<AZStd::string>().c_str());
                }
                return m_asset.Get()->m_definition.GetName();
            }

            const Slot* ReceiveScriptEvent::GetEBusConnectSlot() const
            {
                return EBusEventHandlerProperty::GetConnectSlot(this);
            }

            const Slot* ReceiveScriptEvent::GetEBusDisconnectSlot() const
            {
                return EBusEventHandlerProperty::GetDisconnectSlot(this);
            }

            AZStd::optional<size_t> ReceiveScriptEvent::GetEventIndex(AZStd::string eventName) const
            {
                return m_handler->GetFunctionIndex(eventName.c_str());;
            }

            AZStd::vector<SlotId> ReceiveScriptEvent::GetEventSlotIds() const
            {
                AZStd::vector<SlotId> eventSlotIds;

                for (const auto& iter : m_eventMap)
                {
                    if (IsEventConnected(iter.second))
                    {
                        eventSlotIds.push_back(iter.second.m_eventSlotId);
                    }
                }

                return eventSlotIds;
            }

            AZ::Outcome<AZStd::string, void> ReceiveScriptEvent::GetFunctionCallName(const Slot* slot) const
            {
                const auto slotId = slot->GetId();
                auto mutableThis = const_cast<ReceiveScriptEvent*>(this);

                if (slotId == EBusEventHandlerProperty::GetConnectSlotId(mutableThis))
                {
                    if (IsIDRequired())
                    {
                        return AZ::Success(AZStd::string(Grammar::k_EBusHandlerConnectToName));
                    }
                    else
                    {
                        return AZ::Success(AZStd::string(Grammar::k_EBusHandlerConnectName));
                    }
                }
                else if (slotId == EBusEventHandlerProperty::GetDisconnectSlotId(mutableThis))
                {
                    return AZ::Success(AZStd::string(Grammar::k_EBusHandlerDisconnectName));
                }
                else
                {
                    return AZ::Failure();
                }
            }

            const Datum* ReceiveScriptEvent::GetHandlerStartAddress() const
            {
                return FindDatum(GetSlotId(c_busIdName));
            }

            const Slot* ReceiveScriptEvent::GetEBusConnectAddressSlot() const
            {
                return GetSlot(GetSlotId(c_busIdName));
            }

            AZStd::vector<const Slot*> ReceiveScriptEvent::GetOnVariableHandlingDataSlots() const
            {
                return { GetSlot(GetSlotId(c_busIdName)) };
            }

            AZStd::vector<const Slot*> ReceiveScriptEvent::GetOnVariableHandlingExecutionSlots() const
            {
                return { EBusEventHandlerProperty::GetConnectSlot(this), EBusEventHandlerProperty::GetDisconnectSlot(this) };
            }

            AZ::Outcome<AZStd::string> ReceiveScriptEvent::GetInternalOutKey(const Slot& slot) const
            {
                if (const auto entry = FindEventWithSlot(slot))
                {
                    return AZ::Success(entry->m_eventName);
                }

                return AZ::Failure();
            }

            ConstSlotsOutcome ReceiveScriptEvent::GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const
            {
                return EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*this, executionSlot, targetSlotType);
            }

            AZStd::vector<SlotId> ReceiveScriptEvent::GetNonEventSlotIds() const
            {
                AZStd::vector<SlotId> nonEventSlotIds;

                for (const auto& slot : GetSlots())
                {
                    const SlotId slotId = slot.GetId();

                    if (!IsEventSlotId(slotId))
                    {
                        nonEventSlotIds.push_back(slotId);
                    }
                }

                return nonEventSlotIds;
            }

            bool ReceiveScriptEvent::IsAutoConnected() const
            {
                return m_autoConnectToGraphOwner;
            }

            bool ReceiveScriptEvent::IsEBusAddressed() const
            {
                return IsIDRequired();
            }

            bool ReceiveScriptEvent::IsEventConnected(const Internal::ScriptEventEntry& entry) const
            {
                return Node::IsConnected(*GetSlot(entry.m_eventSlotId))
                    || (entry.m_resultSlotId.IsValid() && Node::IsConnected(*GetSlot(entry.m_resultSlotId)))
                    || AZStd::any_of(entry.m_parameterSlotIds.begin(), entry.m_parameterSlotIds.end(), [this](const SlotId& id) { return this->Node::IsConnected(*GetSlot(id)); });
            }

            bool ReceiveScriptEvent::IsEventHandler() const
            {
                return true;
            }

            bool ReceiveScriptEvent::IsEventSlotId(const SlotId& slotId) const
            {
                for (const auto& eventItem : m_eventMap)
                {
                    if (eventItem.second.ContainsSlot(slotId))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool ReceiveScriptEvent::IsIDRequired() const
            {
                auto slot = GetEBusConnectAddressSlot();
                return m_definition.IsAddressRequired() || (slot && slot->GetDataType().IsValid());
            }

            bool ReceiveScriptEvent::CreateHandler(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                if (m_handler)
                {
                    return true;
                }

                if (!asset)
                {
                    return false;
                }

                m_definition = asset.Get()->m_definition;

                if (m_version == 0)
                {
                    m_version = m_definition.GetVersion();
                }

                if (!m_scriptEvent && m_scriptEventAssetId.IsValid())
                {
                    ScriptEvents::ScriptEventBus::BroadcastResult(m_scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, m_scriptEventAssetId, m_version);
                    m_scriptEvent->Init(m_scriptEventAssetId);
                }

                return true;

            }

            void ReceiveScriptEvent::OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>& asset)
            {
                if (CreateHandler(asset))
                {
                    CompleteInitialize(asset);
                }
            }

            bool ReceiveScriptEvent::CreateEbus()
            {
                if (!m_ebus)
                {
                    AZ::BehaviorContext* behaviorContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
                    
                    const auto& ebusIterator = behaviorContext->m_ebuses.find(m_definition.GetName());
                    if (ebusIterator == behaviorContext->m_ebuses.end())
                    {
                        AZ_Error("Script Canvas", false, "ReceiveScriptEvent::CreateHandler - No ebus by name of %s in the behavior context!", m_definition.GetName().c_str());
                        return false;
                    }

                    m_ebus = ebusIterator->second;
                    AZ_Assert(m_ebus, "Behavior Context EBus does not exist: %s", m_definition.GetName().c_str());
                    AZ_Assert(m_ebus->m_createHandler, "The ebus %s has no create handler!", m_definition.GetName().c_str());
                    AZ_Assert(m_ebus->m_destroyHandler, "The ebus %s has no destroy handler!", m_definition.GetName().c_str());

                    AZ_Verify(m_ebus->m_createHandler->InvokeResult(m_handler, &m_definition), "Behavior Context EBus handler creation failed %s", m_definition.GetName().c_str());

                    AZ_Assert(m_handler, "Ebus create handler failed %s", m_definition.GetName().c_str());
                }

                return true;
            }

            bool ReceiveScriptEvent::SetupHandler()
            {
                if (!m_handler)
                {
                    if (!m_asset.IsReady() && m_scriptEventAssetId.IsValid())
                    {
                        m_asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                        m_asset.BlockUntilLoadComplete();
                        CreateHandler(m_asset);
                        CreateEbus();
                    }

                    if (!m_handler)
                    {
                        AZStd::string error = AZStd::string::format("Script Event receiver node was not initialized (%s)!", m_definition.GetName().c_str());
                        SCRIPTCANVAS_REPORT_ERROR((*this), error.c_str());
                        return false;
                    }
                }

                return true;
            }

            void ReceiveScriptEvent::OnInputSignal(const SlotId& slotId)
            {
                SlotId connectSlot = ReceiveScriptEventProperty::GetConnectSlotId(this);

                if (connectSlot == slotId)
                {
                    const Datum* busIdDatum = FindDatum(GetSlotId(c_busIdName));
                    if (IsIDRequired() && (!busIdDatum || busIdDatum->Empty()))
                    {
                        SlotId failureSlot = ReceiveScriptEventProperty::GetOnFailureSlotId(this);
                        SignalOutput(failureSlot);
                        SCRIPTCANVAS_REPORT_ERROR((*this), "In order to connect this node, a valid BusId must be provided.");
                        return;
                    }
                    else
                    {
                        Connect();
                        SlotId onConnectSlotId = ReceiveScriptEventProperty::GetOnConnectedSlotId(this);
                        SignalOutput(onConnectSlotId);
                        return;
                    }
                }

                SlotId disconnectSlot = ReceiveScriptEventProperty::GetDisconnectSlotId(this);
                if (disconnectSlot == slotId)
                {
                    Disconnect();

                    return;
                }
            }

            void ReceiveScriptEvent::OnInputChanged(const Datum&, const SlotId&)
            {
                /*
                if (GetExecutionType() == ExecutionType::Runtime
                    && m_autoConnectToGraphOwner 
                    && slotId == FindSlotIdForDescriptor(c_busIdName, SlotDescriptors::DataIn()))
                {
                    Disconnect(false);
                    Connect();
                }
                */
            }

            bool ReceiveScriptEvent::IsOutOfDate(const VersionData& graphVersion) const
            {
                AZ_UNUSED(graphVersion);
                return IsAssetOutOfDate().second;
            }

            UpdateResult ReceiveScriptEvent::OnUpdateNode()
            {
                for (auto mapPair : m_eventSlotMapping)
                {
                    const bool removeConnections = false;
                    RemoveSlot(mapPair.second, removeConnections);
                }

                m_eventMap.clear();
                m_scriptEvent.reset();

                delete m_handler;
                m_handler = nullptr;
                m_ebus = nullptr;

                m_version = 0;

                SlotIdMapping populationMapping;

                AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                m_asset.BlockUntilLoadComplete();
                PopulateAsset(asset, populationMapping);

                m_eventSlotMapping = AZStd::move(populationMapping);

                if (m_ebus == nullptr)
                {
                    return UpdateResult::DeleteNode;
                }
                else
                {
                    return UpdateResult::DirtyGraph;
                }
            }


            AZStd::string ReceiveScriptEvent::GetUpdateString() const
            {
                if (m_ebus)
                {
                    return AZStd::string::format("Updated ScriptEvent (%s)", m_definition.GetName().c_str());
                }
                else
                {
                    return AZStd::string::format("Deleted ScriptEvent (%s)", m_asset.GetId().ToString<AZStd::string>().c_str());
                }
            }
        }
    }
}
