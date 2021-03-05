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
#include "EBusEventHandler.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            const char* EBusEventHandler::c_busIdName = "Source";
            const char* EBusEventHandler::c_busIdTooltip = "ID used to connect on a specific Event address";

            /*static*/ bool EBusEventHandler::EBusEventHandlerVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() <= 3)
                {
                    auto element = rootElement.FindSubElement(AZ_CRC("m_ebusName", 0x74a03d75));

                    if (element)
                    {
                        AZStd::string ebusName;
                        element->GetData(ebusName);

                        if (!ebusName.empty())
                        {
                            ScriptCanvas::EBusBusId busId = ScriptCanvas::EBusBusId(ebusName.c_str());

                            if (rootElement.AddElementWithData(serializeContext, "m_busId", busId) == -1)
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        return false;
                    }
                }

                if (rootElement.GetVersion() <= 2)
                {
                    // Renamed "BusId" to "Source"

                    auto slotContainerElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0)});
                    if (!slotContainerElements.empty())
                    {
                        // This contains the pair elements stored in the SlotNameMap(Name -> Index)
                        auto slotNameToIndexElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slotNameSlotMap", 0x69040afb), AZ_CRC("element", 0x41405e39)});

                        // This contains the Slot class elements stored in Node class
                        auto slotElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slots", 0x84838ab4), AZ_CRC("element", 0x41405e39)});

                        AZStd::string slotName;
                        for (auto slotNameToIndexElement : slotNameToIndexElements)
                        {
                            int index = -1;
                            if (slotNameToIndexElement->GetChildData(AZ_CRC("value1", 0xa2756c5a), slotName) && slotName == "BusId" && slotNameToIndexElement->GetChildData(AZ_CRC("value2", 0x3b7c3de0), index))
                            {
                                CombinedSlotType slotType;
                                if (slotElements.size() > static_cast<size_t>(index) && slotElements[index]->GetChildData(AZ_CRC("type", 0x8cde5729), slotType) && slotType == CombinedSlotType::DataIn)
                                {
                                    AZ::SerializeContext::DataElementNode& slotElement = *slotElements[index];

                                    slotName = "Source";
                                    slotElement.RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                                    if (slotElement.AddElementWithData(serializeContext, "slotName", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }

                                    if (slotNameToIndexElement->AddElementWithData(serializeContext, "value1", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }

                if (rootElement.GetVersion() == 2)
                {
                    auto element = rootElement.FindSubElement(AZ_CRC("m_eventMap", 0x8b413178));

                    AZStd::unordered_map<AZ::Crc32, EBusEventEntry> entryMap;
                    element->GetDataHierarchy(serializeContext, entryMap);

                    EBusEventHandler::EventMap eventMap;

                    for (const auto& entryElement : entryMap)
                    {
                        AZ_Assert(eventMap.find(entryElement.first) == eventMap.end(), "Duplicated event found while converting EBusEventHandler from version 2 to 3");

                        eventMap[entryElement.first] = entryElement.second;
                    }

                    rootElement.RemoveElementByName(AZ_CRC("m_eventMap", 0x8b413178));
                    if (rootElement.AddElementWithData(serializeContext, "m_eventMap", eventMap) == -1)
                    {
                        return false;
                    }

                    return true;
                }
                else if (rootElement.GetVersion() <= 1)
                {
                    // Changed:
                    //  using Events = AZStd::vector<EBusEventEntry>;
                    //  to:
                    //  using EventMap = AZStd::map<AZ::Crc32, EBusEventEntry>;

                    auto ebusEventEntryElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("m_events", 0x191405b4), AZ_CRC("element", 0x41405e39)});

                    EBusEventHandler::EventMap eventMap;
                    for (AZ::SerializeContext::DataElementNode* ebusEventEntryElement : ebusEventEntryElements)
                    {
                        EBusEventEntry eventEntry;
                        if (!ebusEventEntryElement->GetData(eventEntry))
                        {
                            return false;
                        }
                        AZ::Crc32 key = AZ::Crc32(eventEntry.m_eventName.c_str());
                        AZ_Assert(eventMap.find(key) == eventMap.end(), "Duplicated event found while converting EBusEventHandler from version 1 to 3.");
                        eventMap[key] = eventEntry;
                    }

                    rootElement.RemoveElementByName(AZ_CRC("m_events", 0x191405b4));
                    if (rootElement.AddElementWithData(serializeContext, "m_eventMap", eventMap) == -1)
                    {
                        return false;
                    }

                    return true;
                }

                if (rootElement.GetVersion() == 0)
                {
                    auto slotContainerElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0)});
                    if (!slotContainerElements.empty())
                    {
                        // This contains the pair elements stored in the SlotNameMap(Name -> Index)
                        auto slotNameToIndexElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slotNameSlotMap", 0x69040afb), AZ_CRC("element", 0x41405e39)});
                        // This contains the Slot class elements stored in Node class
                        auto slotElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slots", 0x84838ab4), AZ_CRC("element", 0x41405e39)});

                        for (auto slotNameToIndexElement : slotNameToIndexElements)
                        {
                            AZStd::string slotName;
                            int index = -1;
                            if (slotNameToIndexElement->GetChildData(AZ_CRC("value1", 0xa2756c5a), slotName) && slotName == "EntityId" && slotNameToIndexElement->GetChildData(AZ_CRC("value2", 0x3b7c3de0), index))
                            {
                                CombinedSlotType slotType;
                                if (slotElements.size() > static_cast<size_t>(index) && slotElements[index]->GetChildData(AZ_CRC("type", 0x8cde5729), slotType) && slotType == CombinedSlotType::DataIn)
                                {
                                    AZ::SerializeContext::DataElementNode& slotElement = *slotElements[index];

                                    slotName = "BusId";
                                    slotElement.RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                                    if (slotElement.AddElementWithData(serializeContext, "slotName", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }

                                    if (slotNameToIndexElement->AddElementWithData(serializeContext, "value1", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }

                return true;
            }

            EBusEventHandler::~EBusEventHandler()
            {
                if (m_ebus && m_ebus->m_destroyHandler)
                {
                    m_ebus->m_destroyHandler->Invoke(m_handler);
                }

                EBusHandlerNodeRequestBus::Handler::BusDisconnect();
            }

            void EBusEventHandler::OnInit()
            {
                if (m_ebus && m_handler)
                {
                    for (int eventIndex(0); eventIndex < m_handler->GetEvents().size(); ++eventIndex)
                    {
                        InitializeEvent(eventIndex);
                    }
                }
            }

            void EBusEventHandler::OnActivate()
            {
                // Set the auto connect value to the serialized value to give the setter 
                // the chance to overrule it if the node's Connect slot is manually connected.
                SetAutoConnectToGraphOwner(m_autoConnectToGraphOwner);
            }

            void EBusEventHandler::OnPostActivate()
            {
                if (m_ebus && m_handler && m_autoConnectToGraphOwner)
                {
                    bool allowAutoConnect = GetExecutionType() == ExecutionType::Runtime;

                    if (IsIDRequired())
                    {
                        // If we are hooked up to data, wait for that to signal before connecting
                        if (IsConnected(FindSlotIdForDescriptor(c_busIdName, SlotDescriptors::DataIn())))
                        {
                            allowAutoConnect = false;
                        }
                    }

                    if (allowAutoConnect)
                    {
                        Connect();
                    }
                }

                if (GetExecutionType() == ExecutionType::Runtime)
                {
                    for (auto& busEntry : m_eventMap)
                    {
                        busEntry.second.m_shouldHandleEvent = IsEventConnected(busEntry.second);
                    }
                }
            }

            void EBusEventHandler::OnDeactivate()
            {
                Disconnect();
            }

            void EBusEventHandler::OnGraphSet()
            {
                if (GetExecutionType() == ExecutionType::Editor)
                {
                    GraphScopedNodeId scopedNodeId(GetOwningScriptCanvasId(), GetEntityId());
                    EBusHandlerNodeRequestBus::Handler::BusConnect(scopedNodeId);
                }
            }

            void EBusEventHandler::CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (IsIDRequired())
                {
                    const Datum* datum = FindDatum(GetSlotId(c_busIdName));

                    const ScriptCanvas::GraphScopedVariableId* scopedVariableId = datum->GetAs<ScriptCanvas::GraphScopedVariableId>();

                    if (scopedVariableId)
                    {
                        variableIds.insert(scopedVariableId->m_identifier);
                    }
                }
            }

            bool EBusEventHandler::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (IsIDRequired())
                {
                    const Datum* datum = FindDatum(GetSlotId(c_busIdName));

                    const ScriptCanvas::GraphScopedVariableId* scopedVariableId = datum->GetAs<ScriptCanvas::GraphScopedVariableId>();

                    if (scopedVariableId)
                    {
                        return variableIds.find(scopedVariableId->m_identifier) != variableIds.end();
                    }
                }

                return false;
            }

            void EBusEventHandler::SetAddressId(const Datum& datumValue)
            {
                if (IsIDRequired())
                {
                    ModifiableDatumView datumView;
                    FindModifiableDatumView(GetSlotId(c_busIdName), datumView);

                    if (datumView.IsValid())
                    {
                        datumView.HardCopyDatum(datumValue);
                        OnDatumEdited(datumView.GetDatum());
                    }
                }
            }

            void EBusEventHandler::Connect()
            {
                if (!m_handler)
                {
                    AZ_Error("Script Canvas", false, "Cannot connect to an EBus Handler for EBus %s, This is most likely due to the EBus %s interface changing", m_ebusName.data(), m_ebusName.data());
                    return;
                }

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
                                AZ::EntityId* notificationId = static_cast<AZ::EntityId*>(busIdParameter.m_value);
                                (*notificationId) = GetRuntimeBus()->GetRuntimeEntityId();
                            }
                        }
                    }
                    else if (busIdType == azrtti_typeid<GraphScopedVariableId>())
                    {                        
                        GraphScopedVariableId* notificationId = static_cast<GraphScopedVariableId*>(busIdParameter.m_value);
                        notificationId->m_scriptCanvasId = GetOwningScriptCanvasId();
                    }

                }
                if (!IsIDRequired() || busIdParameter.GetValueAddress())
                {
                    // Ensure we disconnect if this bus is already connected, this could happen if a different bus Id is provided
                    // and this node is connected through the Connect slot.
                    m_handler->Disconnect();

                    AZ_VerifyError("Script Canvas", m_handler->Connect(&busIdParameter),
                        "Unable to connect to EBus with BusIdType %s. The BusIdType of the EBus (%s) does not match the BusIdType stored in the Datum",
                        busIdType.ToString<AZStd::string>().data(), m_ebusName.data());
                }
            }

            void EBusEventHandler::Disconnect()
            {
                if (!m_handler)
                {
                    AZ_Error("Script Canvas", false, "Cannot disconnect from an EBus Handler for EBus %s, This is most likely due to the EBus %s interface changing", m_ebusName.data(), m_ebusName.data());
                    return;
                }
                m_handler->Disconnect();
            }

            const AZ::BehaviorEBusHandler::BusForwarderEvent* GetEventHandlerFromName(int& eventIndexOut, AZ::BehaviorEBusHandler& handler, const AZStd::string& eventName)
            {
                const AZ::BehaviorEBusHandler::EventArray& events(handler.GetEvents());
                int eventIndex(0);

                for (auto& event : events)
                {
                    if (!(eventName.compare(event.m_name)))
                    {
                        eventIndexOut = eventIndex;
                        return &event;
                    }

                    ++eventIndex;
                }

                return nullptr;
            }

            AZStd::vector<SlotId> EBusEventHandler::GetEventSlotIds() const
            {
                AZStd::vector<SlotId> eventSlotIds;

                for (const auto& iter : m_eventMap)
                {
                    eventSlotIds.push_back(iter.second.m_eventSlotId);
                }

                return eventSlotIds;
            }

            AZStd::vector<SlotId> EBusEventHandler::GetNonEventSlotIds() const
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

            bool EBusEventHandler::IsEventSlotId(const SlotId& slotId) const
            {
                for (const auto& eventItem : m_eventMap)
                {
                    const auto& event = eventItem.second;
                    if (slotId == event.m_eventSlotId
                        || slotId == event.m_resultSlotId
                        || event.m_parameterSlotIds.end() != AZStd::find_if(event.m_parameterSlotIds.begin(), event.m_parameterSlotIds.end(), [&slotId](const SlotId& candidate) { return slotId == candidate; }))
                    {
                        return true;
                    }
                }

                return false;
            }

            const EBusEventEntry* EBusEventHandler::FindEvent(const AZStd::string& name) const
            {
                AZ::Crc32 key = AZ::Crc32(name.c_str());
                auto iter = m_eventMap.find(key);
                return (iter != m_eventMap.end()) ? &iter->second : nullptr;
            }

            bool EBusEventHandler::CreateHandler(AZStd::string_view ebusName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                if (m_handler)
                {
                    AZ_Warning("Script Canvas", false, "Handler %s is already initialized", ebusName.data());
                    return true;
                }

                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus handler without a behavior context!");
                    return false;
                }

                const auto& ebusIterator = behaviorContext->m_ebuses.find(ebusName);
                if (ebusIterator == behaviorContext->m_ebuses.end())
                {
                    AZ_Error("Script Canvas", false, "No ebus by name of %s in the behavior context!", ebusName.data());
                    return false;
                }

                m_ebus = ebusIterator->second;
                AZ_Assert(m_ebus, "ebus == nullptr in %s", ebusName.data());

                if (!m_ebus->m_createHandler)
                {
                    AZ_Error("Script Canvas", false, "The ebus %s has no create handler!", ebusName.data());
                }

                if (!m_ebus->m_destroyHandler)
                {
                    AZ_Error("Script Canvas", false, "The ebus %s has no destroy handler!", ebusName.data());
                }

                if (m_ebus->m_name.compare(m_ebusName) != 0)
                {
                    m_ebusName = m_ebus->m_name;
                }

                AZ_Verify(m_ebus->m_createHandler->InvokeResult(m_handler), "Ebus handler creation failed %s", ebusName.data());
                AZ_Assert(m_handler, "Ebus create handler failed %s", ebusName.data());

                return true;
            }

            void EBusEventHandler::InitializeBus(const AZStd::string& ebusName)
            {
                if (!CreateHandler(ebusName))
                {
                    return;
                }

                const bool wasConfigured = IsConfigured();

                if (!wasConfigured)
                {
                    if (IsIDRequired())
                    {
                        const auto busToolTip = AZStd::string::format("%s (Type: %s)", c_busIdTooltip, m_ebus->m_idParam.m_name);
                        const AZ::TypeId& busId = m_ebus->m_idParam.m_typeId;

                        DataSlotConfiguration config;

                        config.m_name = c_busIdName;
                        config.m_toolTip = busToolTip;
                        config.SetConnectionType(ConnectionType::Input);
                        
                        if (busId == azrtti_typeid<AZ::EntityId>())
                        {
                            config.SetDefaultValue(GraphOwnerId);
                        }
                        else
                        {
                            Data::Type busIdType(AZ::BehaviorContextHelper::IsStringParameter(m_ebus->m_idParam) ? Data::Type::String() : Data::FromAZType(busId));
                            
                            config.ConfigureDatum(AZStd::move(Datum(busIdType, Datum::eOriginality::Original)));
                        }

                        AddSlot(config);
                    }
                }

                m_ebusName = m_ebus->m_name;
                m_busId = ScriptCanvas::EBusBusId(m_ebusName.c_str());

                const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                for (int eventIndex(0); eventIndex < events.size(); ++eventIndex)
                {
                    InitializeEvent(eventIndex);
                }

                PopulateNodeType();
            }

            void EBusEventHandler::InitializeEvent(int eventIndex)
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
                AZ_Assert(!event.m_parameters.empty(), "No parameters in event!");
                m_handler->InstallGenericHook(events[eventIndex].m_name, &OnEventGenericHook, this);

                if (m_eventMap.find(AZ::Crc32(event.m_name)) != m_eventMap.end())
                {
                    // Event is already associated with the EBusEventHandler
                    return;
                }

                const size_t sentinel(event.m_parameters.size());

                EBusEventEntry ebusEventEntry;
                ebusEventEntry.m_numExpectedArguments = static_cast<int>(sentinel - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst);

                if (event.HasResult())
                {
                    const AZ::BehaviorParameter& argument(event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result]);
                    Data::Type inputType(AZ::BehaviorContextHelper::IsStringParameter(argument) ? Data::Type::String() : Data::FromAZType(argument.m_typeId));
                    const AZStd::string argName(AZStd::string::format("Result: %s", Data::GetName(inputType).data()).data());

                    DataSlotConfiguration resultConfiguration;

                    resultConfiguration.m_name = argName.c_str();
                    resultConfiguration.m_toolTip = "";
                    resultConfiguration.SetConnectionType(ConnectionType::Input);
                    resultConfiguration.m_addUniqueSlotByNameAndType = false;

                    resultConfiguration.ConfigureDatum(AZStd::move(Datum(inputType, Datum::eOriginality::Copy)));

                    ebusEventEntry.m_resultSlotId = AddSlot(resultConfiguration);
                }

                for (size_t parameterIndex(AZ::eBehaviorBusForwarderEventIndices::ParameterFirst)
                    ; parameterIndex < sentinel
                    ; ++parameterIndex)
                {
                    const AZ::BehaviorParameter& parameter(event.m_parameters[parameterIndex]);
                    Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(parameter) ? Data::Type::String() : Data::FromAZType(parameter.m_typeId));
                    // multiple outs will need out value names
                    AZStd::string argName = event.m_metadataParameters[parameterIndex].m_name;
                    if (argName.empty())
                    {
                        argName = Data::GetName(outputType);
                    }
                    const AZStd::string& argToolTip = event.m_metadataParameters[parameterIndex].m_toolTip;

                    DataSlotConfiguration slotConfiguration;
                    slotConfiguration.m_name = argName;
                    slotConfiguration.m_toolTip = argToolTip;
                    slotConfiguration.SetConnectionType(ConnectionType::Output);
                    slotConfiguration.m_addUniqueSlotByNameAndType = false;
                    slotConfiguration.SetType(outputType);

                    ebusEventEntry.m_parameterSlotIds.push_back(AddSlot(slotConfiguration));
                }

                const AZStd::string eventID(AZStd::string::format("ExecutionSlot:%s", event.m_name));

                {
                    ExecutionSlotConfiguration slotConfiguration(eventID, ConnectionType::Output);
                    ebusEventEntry.m_eventSlotId = AddSlot(slotConfiguration);
                }

                AZ_Assert(ebusEventEntry.m_eventSlotId.IsValid(), "the event execution out slot must be valid");
                ebusEventEntry.m_eventName = event.m_name;
                ebusEventEntry.m_eventId = event.m_eventId;

                m_eventMap[AZ::Crc32(event.m_name)] = ebusEventEntry;
            }

            bool EBusEventHandler::IsEventConnected(const EBusEventEntry& entry) const
            {
                auto eventSlot = GetSlot(entry.m_eventSlotId);
                auto resultSlot = GetSlot(entry.m_resultSlotId);

                return (eventSlot && Node::IsConnected(*eventSlot))
                    || (resultSlot && Node::IsConnected(*resultSlot))
                    || AZStd::any_of(entry.m_parameterSlotIds.begin(), entry.m_parameterSlotIds.end(), [this](const SlotId& id) { return this->Node::IsConnected(id); });
            }

            bool EBusEventHandler::IsEventHandler() const
            {
                return true;
            }
            
            bool EBusEventHandler::IsValid() const
            {
                return !m_eventMap.empty();
            }

            void EBusEventHandler::SetAutoConnectToGraphOwner(bool enabled)
            {
                ScriptCanvas::Slot* connectSlot = EBusEventHandlerProperty::GetConnectSlot(this);

                if (connectSlot)
                {
                    m_autoConnectToGraphOwner = enabled && !IsConnected(*connectSlot);
                }
            }

            void EBusEventHandler::OnEvent(const char* eventName, [[maybe_unused]] const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "EBusEventHandler::OnEvent %s", eventName);

                SCRIPTCANVAS_RETURN_IF_ERROR_STATE((*this));

                auto iter = m_eventMap.find(AZ::Crc32(eventName));
                if (iter == m_eventMap.end())
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Invalid event index in Ebus handler");
                    return;
                }

                EBusEventEntry& ebusEventEntry = iter->second;

                if (!ebusEventEntry.m_shouldHandleEvent || ebusEventEntry.m_isHandlingEvent)
                {
                    AZ_Warning("ScriptCanvas", !ebusEventEntry.m_isHandlingEvent, "Found situation where in handling event(%s::%s) triggered the same event. Possible infinite loop, not handling second call.", GetEBusName(), eventName);
                    return;
                }

                ebusEventEntry.m_isHandlingEvent = true;

                ebusEventEntry.m_resultEvaluated = !ebusEventEntry.IsExpectingResult();

                AZ_Assert(ebusEventEntry.m_eventName.compare(eventName) == 0, "Wrong event handled by this EBusEventHandler! received %s, expected %s", eventName, ebusEventEntry.m_eventName.c_str());
                AZ_Assert(numParameters == ebusEventEntry.m_numExpectedArguments, "Wrong number of events passed into EBusEventHandler %s", eventName);

                // route my parameters to the connect nodes input
                AZ_Assert(ebusEventEntry.m_parameterSlotIds.size() == numParameters, "Node %s-%s Num parameters = %d, but num output slots = %d", m_ebusName.c_str(), ebusEventEntry.m_eventName.c_str(), numParameters, ebusEventEntry.m_parameterSlotIds.size());

                for (int parameterIndex(0); parameterIndex < numParameters; ++parameterIndex)
                {
                    const Slot* slot = GetSlot(ebusEventEntry.m_parameterSlotIds[parameterIndex]);
                    const auto& value = *(parameters + parameterIndex);
                    const Datum input(value);

                    PushOutput(input, (*slot));
                }

                // now, this should pass execution off to the nodes that will push their output into this result input
                SignalOutput(ebusEventEntry.m_eventSlotId, ExecuteMode::UntilNodeIsFoundInStack);

                // route executed nodes output to my input, and my input to the result
                if (ebusEventEntry.IsExpectingResult())
                {
                    if (result)
                    {
                        if (const Datum* resultInput = FindDatum(ebusEventEntry.m_resultSlotId))
                        {
                            ebusEventEntry.m_resultEvaluated = resultInput->ToBehaviorContext(*result);
                            AZ_Warning("Script Canvas", ebusEventEntry.m_resultEvaluated, "Script Canvas failed to write a value back to the caller!");
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
                AZ_Warning("Script Canvas", (result != nullptr) == ebusEventEntry.IsExpectingResult(), "Node %s-%s mismatch between expecting a result and getting one!", m_ebusName.c_str(), ebusEventEntry.m_eventName.c_str());
                AZ_Warning("Script Canvas", ebusEventEntry.m_resultEvaluated, "Node %s-%s result not evaluated properly!", m_ebusName.c_str(), ebusEventEntry.m_eventName.c_str());

                ebusEventEntry.m_isHandlingEvent = false;
            }

            void EBusEventHandler::OnInputSignal(const SlotId& slotId)
            {
                SlotId connectSlot = EBusEventHandlerProperty::GetConnectSlotId(this);
                SlotId disconnectSlot = EBusEventHandlerProperty::GetDisconnectSlotId(this);

                if (connectSlot == slotId)
                {
                    if (IsIDRequired())
                    {
                        const Datum* busIdDatum = FindDatum(GetSlotId(c_busIdName));

                        if (!busIdDatum || busIdDatum->Empty())
                        {
                            SlotId failureSlot = EBusEventHandlerProperty::GetOnFailureSlotId(this);
                            SignalOutput(failureSlot);
                            SCRIPTCANVAS_REPORT_ERROR((*this), "In order to connect this node, a valid BusId must be provided.");
                            return;
                        }
                    }
                    
                    Disconnect();
                    Connect();

                    SlotId onConnectSlotId = EBusEventHandlerProperty::GetOnConnectedSlotId(this);
                    SignalOutput(onConnectSlotId);
                    return;                    
                }
                else if (disconnectSlot == slotId)
                {
                    Disconnect();

                    SlotId onDisconnectSlotId = EBusEventHandlerProperty::GetOnDisconnectedSlotId(this);
                    SignalOutput(onDisconnectSlotId);
                    return;
                }
            }

            void EBusEventHandler::OnInputChanged([[maybe_unused]] const Datum& input, const SlotId& slotId)
            {
                if (GetExecutionType() == ExecutionType::Runtime 
                    && m_autoConnectToGraphOwner 
                    && slotId == FindSlotIdForDescriptor(c_busIdName, SlotDescriptors::DataIn()))
                {
                    Disconnect();
                    Connect();
                }
            }

            void EBusEventHandler::OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                EBusEventHandler* handler(reinterpret_cast<EBusEventHandler*>(userData));
                handler->OnEvent(eventName, eventIndex, result, numParameters, parameters);
            }

            void EBusEventHandler::OnWriteEnd()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
                if (!m_ebus)
                {
                    CreateHandler(m_ebusName);
                }
            }

            NodeTypeIdentifier EBusEventHandler::GetOutputNodeType(const SlotId& slotId) const
            {
                for (auto mapPair : m_eventMap)
                {
                    bool isEvent = mapPair.second.m_eventSlotId == slotId;

                    if (!isEvent)
                    {
                        for (auto paramSlotId : mapPair.second.m_parameterSlotIds)
                        {
                            if (paramSlotId == slotId)
                            {
                                isEvent = true;
                                break;
                            }
                        }
                    }

                    if (isEvent)
                    {
                        return NodeUtils::ConstructEBusEventReceiverIdentifier(m_busId, mapPair.second.m_eventId);
                    }
                }

                // If we don't match any of the output slots for our events, just return our base type as it's
                // one of the control pins firing.
                return GetNodeType();
            }

            bool EBusEventEntry::EBusEventEntryVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() == 0)
                {
                    auto element = rootElement.FindSubElement(AZ_CRC("m_eventName", 0x5c560197));

                    AZStd::string eventName;
                    if (element->GetData(eventName))
                    {
                        AZ::Crc32 eventId = AZ::Crc32(eventName.c_str());

                        if (rootElement.AddElementWithData(serializeContext, "m_eventId", eventId) == -1)
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            void EBusEventEntry::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<AZ::Crc32, EBusEventEntry> >::GetGenericInfo();
                    if (genericClassInfo)
                    {
                        genericClassInfo->Reflect(serialize);
                    }

                    serialize->Class<EBusEventEntry>()
                        ->Version(1, EBusEventEntryVersionConverter)
                        ->Field("m_eventName", &EBusEventEntry::m_eventName)
                        ->Field("m_eventId", &EBusEventEntry::m_eventId)
                        ->Field("m_eventSlotId", &EBusEventEntry::m_eventSlotId)
                        ->Field("m_resultSlotId", &EBusEventEntry::m_resultSlotId)
                        ->Field("m_parameterSlotIds", &EBusEventEntry::m_parameterSlotIds)
                        ->Field("m_numExpectedArguments", &EBusEventEntry::m_numExpectedArguments)
                        ->Field("m_resultEvaluated", &EBusEventEntry::m_resultEvaluated)
                        ;
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
