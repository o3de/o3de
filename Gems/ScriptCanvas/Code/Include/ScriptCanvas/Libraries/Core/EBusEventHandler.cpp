/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EBusEventHandler.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Grammar/Primitives.h>
#include <ScriptCanvas/Grammar/ParsingUtilities.h>
#include <ScriptCanvas/Libraries/Core/EventHandlerTranslationUtility.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            const char* EBusEventHandler::c_busIdName = "Source";
            const char* EBusEventHandler::c_busIdTooltip = "ID used to connect on a specific Event address";

            EBusEventHandler::~EBusEventHandler()
            {
                if (m_ebus && m_ebus->m_destroyHandler)
                {
                    m_ebus->m_destroyHandler->Invoke(m_handler);
                }

                EBusHandlerNodeRequestBus::Handler::BusDisconnect();
            }

            AZ::BehaviorEBus* EBusEventHandler::GetBus() const
            {
                return m_ebus;
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

            void EBusEventHandler::OnGraphSet()
            {
                GraphScopedNodeId scopedNodeId(GetOwningScriptCanvasId(), GetEntityId());
                EBusHandlerNodeRequestBus::Handler::BusConnect(scopedNodeId);
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
                
                Node::CollectVariableReferences(variableIds);
            }

            bool EBusEventHandler::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (IsIDRequired())
                {
                    const Datum* datum = FindDatum(GetSlotId(c_busIdName));

                    const ScriptCanvas::GraphScopedVariableId* scopedVariableId = datum->GetAs<ScriptCanvas::GraphScopedVariableId>();

                    if (scopedVariableId)
                    {
                        if(variableIds.find(scopedVariableId->m_identifier) != variableIds.end())
                        {
                            return true;
                        }
                    }
                }

                return Node::ContainsReferencesToVariables(variableIds);
            }

            size_t EBusEventHandler::GenerateFingerprint() const
            {
                return BehaviorContextUtils::GenerateFingerprintForMethod(MethodType::Event, "", GetEBusName());
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

            AZ::Outcome<DependencyReport, void> EBusEventHandler::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }

            AZStd::string EBusEventHandler::GetEBusName() const
            {
                return m_ebusName;
            }

            bool EBusEventHandler::IsEBusAddressed() const
            {
                return IsIDRequired();
            }

            bool EBusEventHandler::IsAutoConnected() const
            {
                return m_autoConnectToGraphOwner;
            }

            const Datum* EBusEventHandler::GetHandlerStartAddress() const
            {
                return FindDatum(GetSlotId(c_busIdName));
            }

            const Slot* EBusEventHandler::GetEBusConnectAddressSlot() const
            {
                return GetSlot(GetSlotId(c_busIdName));
            }

            AZStd::vector<const Slot*> EBusEventHandler::GetOnVariableHandlingDataSlots() const
            {
                return { GetSlot(GetSlotId(c_busIdName)) };
            }

            AZStd::vector<const Slot*> EBusEventHandler::GetOnVariableHandlingExecutionSlots() const
            {
                return { EBusEventHandlerProperty::GetConnectSlot(this), EBusEventHandlerProperty::GetDisconnectSlot(this) };
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

            const Slot* EBusEventHandler::GetEBusConnectSlot() const
            {
                return EBusEventHandlerProperty::GetConnectSlot(this);
            }

            const Slot* EBusEventHandler::GetEBusDisconnectSlot() const
            {
                return EBusEventHandlerProperty::GetDisconnectSlot(this);
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

            AZ::Outcome<AZStd::string> EBusEventHandler::GetInternalOutKey(const Slot& slot) const
            {
                if (const auto entry = FindEventWithSlot(slot))
                {
                    return AZ::Success(entry->m_eventName);
                }

                return AZ::Failure();
            }

            const EBusEventEntry* EBusEventHandler::FindEventWithSlot(const Slot& slot) const
            {
                auto slotId = slot.GetId();
                auto iter = AZStd::find_if(m_eventMap.begin(), m_eventMap.end(), [slotId](const auto& iter) { return iter.second.ContainsSlot(slotId); });
                return iter != m_eventMap.end() ? &iter->second : nullptr;
            }

            AZ::Outcome<AZStd::string, void> EBusEventHandler::GetFunctionCallName(const Slot* slot) const
            {
                const auto slotId = slot->GetId();
                auto mutableThis = const_cast<EBusEventHandler*>(this);

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

            ConstSlotsOutcome EBusEventHandler::GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const
            {
                return EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(*this, executionSlot, targetSlotType);
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
                    if (eventItem.second.ContainsSlot(slotId))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool EBusEventHandler::IsOutOfDate([[maybe_unused]] const VersionData& graphVersion) const
            {
                bool isUnitTestingInProgress = false;
                ScriptCanvas::SystemRequestBus::BroadcastResult(isUnitTestingInProgress, &ScriptCanvas::SystemRequests::IsScriptUnitTestingInProgress);

                if (isUnitTestingInProgress)
                {
                    return false;
                }

                AZ_UNUSED(graphVersion);

                if (IsVariableWriteHandler())
                {
                    return false;
                }

                if (!m_handler || !m_ebus)
                {
                    return true;
                }

                for (const auto& eventEntryIter : m_eventMap)
                {
                    // If event is not connected we can skip for now
                    if (!IsEventConnected(eventEntryIter.second))
                    {
                        continue;
                    }

                    int eventIndex = m_handler->GetFunctionIndex(eventEntryIter.second.m_eventName.data());
                    if (eventIndex == -1)
                    {
                        return true;
                    }

                    const AZ::BehaviorEBusHandler::BusForwarderEvent& event = m_handler->GetEvents()[eventIndex];

                    // Compare output type
                    const bool eventHasOutput = !event.m_parameters.empty() && !event.m_parameters.front().m_typeId.IsNull() && event.m_parameters.front().m_typeId != azrtti_typeid<void>();
                    bool nodeHasOutput = false;
                    bool outputDataTypeMatch = true;
                    const auto* outputSlot = GetSlot(eventEntryIter.second.m_resultSlotId);
                    if (outputSlot && outputSlot->IsData())
                    {
                        nodeHasOutput = true;
                        outputDataTypeMatch = BehaviorContextUtils::IsSameDataType(&event.m_parameters[0], outputSlot->GetDataType());
                    }

                    // Compare input type
                    int startArgumentIndex = 2; // first two are result and userdata
                    const size_t eventInputNumber = event.m_parameters.size() - startArgumentIndex;
                    size_t nodeInputNumber = 0;
                    bool inputDataTypeMatch = true;
                    if (!eventEntryIter.second.m_parameterSlotIds.empty())
                    {
                        for (const auto& inputParameterSlotId : eventEntryIter.second.m_parameterSlotIds)
                        {
                            const auto* inputParameterSlot = GetSlot(inputParameterSlotId);
                            if (inputParameterSlot && inputParameterSlot->IsData())
                            {
                                ++nodeInputNumber;
                                if (nodeInputNumber > eventInputNumber)
                                {
                                    break;
                                }
                                inputDataTypeMatch = BehaviorContextUtils::IsSameDataType(&event.m_parameters[nodeInputNumber - 1 + startArgumentIndex], inputParameterSlot->GetDataType());
                            }
                        }
                    }

                    // Compare address type
                    const bool ebusHasAddress = !m_ebus->m_idParam.m_typeId.IsNull();
                    bool nodeHasAddress = false;
                    bool addressTypeMatch = true;
                    const auto* sourceSlot = GetSlotByName(GetSourceSlotName());
                    if (sourceSlot && sourceSlot->IsData())
                    {
                        nodeHasAddress = true;
                        addressTypeMatch = sourceSlot->GetDataType().GetAZType() == m_ebus->m_idParam.m_typeId;
                    }

                    const bool nodeAndEventOutputMatch = (nodeHasOutput == eventHasOutput) && outputDataTypeMatch;
                    const bool nodeAndEventInputMatch = (nodeInputNumber == eventInputNumber) && inputDataTypeMatch;
                    const bool nodeAndEbusAddressMatch = (nodeHasAddress == ebusHasAddress) && addressTypeMatch;
                    if (!nodeAndEventOutputMatch || !nodeAndEventInputMatch || !nodeAndEbusAddressMatch)
                    {
                        return true;
                    }
                }

                return false;
            }

            AZStd::optional<size_t> EBusEventHandler::GetEventIndex(AZStd::string eventName) const
            {
                return m_handler->GetFunctionIndex(eventName.c_str());
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

                    resultConfiguration.ConfigureDatum(AZStd::move(Datum(inputType, Datum::eOriginality::Original)));

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

                const AZStd::string eventID(AZStd::string::format("%s%s", k_EventOutPrefix, event.m_name));

                {
                    ExecutionSlotConfiguration slotConfiguration(eventID, ConnectionType::Output);
                    slotConfiguration.m_isLatent = true;
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

            bool EBusEventHandler::IsVariableWriteHandler() const
            {
                const auto ebusName = GetEBusName();
                return ebusName == k_OnVariableWriteEbusName;
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

            void EBusEventHandler::OnDeserialize()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
                if (!m_ebus && !m_ebusName.empty())
                {
                    CreateHandler(m_ebusName);
                }

                Node::OnDeserialize();
            }

#if defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)////
            void EBusEventHandler::OnWriteEnd()
            {
                OnDeserialize();
            }
#endif//defined(OBJECT_STREAM_EDITOR_ASSET_LOADING_SUPPORT_ENABLED)

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

            bool EBusEventEntry::ContainsSlot(SlotId slotId) const
            {
                if (m_eventSlotId == slotId)
                {
                    return true;
                }
                else if (m_resultSlotId == slotId)
                {
                    return true;
                }
                else
                {
                    // for grammar purposes, this will never get called, but is here for completeness
                    return AZStd::find(m_parameterSlotIds.begin(), m_parameterSlotIds.end(), slotId) != m_parameterSlotIds.end();
                }
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

            bool EBusEventEntry::IsExpectingResult() const
            {
                return m_resultSlotId.IsValid();
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

        }
    }
}
