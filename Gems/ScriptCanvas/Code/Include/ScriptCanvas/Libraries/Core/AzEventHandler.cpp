/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/RestrictedNodeContract.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EventHandlerTranslationUtility.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

namespace ScriptCanvas::Nodes::Core
{
    AzEventHandler::~AzEventHandler() = default;

    bool AzEventHandler::InitEventFromMethod(const AZ::BehaviorMethod& methodWhichReturnsEvent)
    {
        // If the event has already been initialized, then the event Data slots have already been added
        if (m_azEventEntry.m_azEventInputSlotId.IsValid())
        {
            return true;
        }
        // Make sure the method returns an AZ::Event
        const AZ::BehaviorParameter* resultParameter = methodWhichReturnsEvent.GetResult();
        if (resultParameter == nullptr || resultParameter->m_azRtti == nullptr
            || resultParameter->m_azRtti->GetGenericTypeId() != azrtti_typeid<AZ::Event>())
        {
            return false;
        }

        // Read in AZ Event Description data to retrieve the event name and parameter names
        AZ::Attribute* azEventDescAttribute = AZ::FindAttribute(AZ::Script::Attributes::AzEventDescription, methodWhichReturnsEvent.m_attributes);
        AZ::BehaviorAzEventDescription behaviorAzEventDesc;
        AZ::AttributeReader azEventDescAttributeReader(nullptr, azEventDescAttribute);
        azEventDescAttributeReader.Read<decltype(behaviorAzEventDesc)>(behaviorAzEventDesc);

        // Retrieve the AZ TypeId for each parameter of the event
        AZStd::vector<AZ::BehaviorParameter> eventParameterTypes;
        auto azEventClass = AZ::BehaviorContextHelper::GetClass(resultParameter->m_azRtti->GetTypeId());
        if (azEventClass)
        {
            AZ::Attribute* eventParameterTypesAttribute = AZ::FindAttribute(AZ::Script::Attributes::EventParameterTypes,
                azEventClass->m_attributes);
            AZ::AttributeReader(nullptr, eventParameterTypesAttribute).Read<decltype(eventParameterTypes)>(eventParameterTypes);
        }

        if (behaviorAzEventDesc.m_parameterNames.size() != eventParameterTypes.size())
        {
            AZ_Assert(false, "The number of parameters (%zu) that"
                " the AZ Event %s accepts is not equal to the number of parameter names provided to the AzEventDescription attribute (%zu)",
                eventParameterTypes.size(), behaviorAzEventDesc.m_eventName.c_str(), behaviorAzEventDesc.m_parameterNames.size());
            return false;
        }
        // Add Event Parameter slots as output slots on the right of the node
        for (size_t paramIndex{}; paramIndex < eventParameterTypes.size(); ++paramIndex)
        {
            auto outputType{ AZ::BehaviorContextHelper::IsStringParameter(eventParameterTypes[paramIndex])
                ? Data::Type::String() : Data::FromAZType(eventParameterTypes[paramIndex].m_typeId) };
            // multiple outs will need out value names

            DataSlotConfiguration slotConfiguration;
            slotConfiguration.m_name = AZStd::move(behaviorAzEventDesc.m_parameterNames[paramIndex]); // Currently doesn't support a tooltip
            slotConfiguration.SetConnectionType(ConnectionType::Output);
            slotConfiguration.SetType(outputType);

            m_azEventEntry.m_parameterSlotIds.push_back(AddSlot(slotConfiguration));
        }

        // Store the name of the event in the AzEventEntry structure
        m_azEventEntry.m_eventName = AZStd::move(behaviorAzEventDesc.m_eventName);

        // Add a DataSlot which accepts the aliased AZ::Event<Params...> type by reference;
        DataSlotConfiguration azEventSlotConfiguration;
        azEventSlotConfiguration.m_name = m_azEventEntry.m_eventName;
        azEventSlotConfiguration.SetConnectionType(ConnectionType::Input);
        azEventSlotConfiguration.SetType(Data::FromAZType(resultParameter->m_typeId));
        // Add a connection limit contract to allow only one connection to the data input slot
        azEventSlotConfiguration.m_contractDescs.emplace_back([]() -> Contract* { return aznew ConnectionLimitContract(1); });
        azEventSlotConfiguration.m_contractDescs.emplace_back([]() -> Contract* { return aznew RestrictedNodeContract{}; });
        m_azEventEntry.m_azEventInputSlotId = AddSlot(azEventSlotConfiguration);

        return true;
    }

    const Slot* AzEventHandler::GetEventInputSlot() const
    {
        return GetSlot(m_azEventEntry.m_azEventInputSlotId);
    }

    void AzEventHandler::SetRestrictedNodeId(AZ::EntityId methodNodeId)
    {
        Slot* connectSlot = AzEventHandlerProperty::GetConnectSlot(this);
        if (auto restrictedNodeContract{ connectSlot->FindContract<RestrictedNodeContract>() }; restrictedNodeContract != nullptr)
        {
            restrictedNodeContract->SetNodeId(methodNodeId);
        }
        Slot* azEventDataInSlot = GetSlotByName(m_azEventEntry.m_eventName);
        if (auto restrictedNodeContract{ azEventDataInSlot->FindContract<RestrictedNodeContract>() }; restrictedNodeContract != nullptr)
        {
            restrictedNodeContract->SetNodeId(methodNodeId);
        }
    }

    void AzEventHandler::CollectVariableReferences(AZStd::unordered_set<ScriptCanvas::VariableId>& variableIds) const
    {
        const Datum* datum = FindDatum(GetSlotId(m_azEventEntry.m_eventName));

        if (auto scopedVariableId = datum->GetAs<ScriptCanvas::GraphScopedVariableId>(); scopedVariableId != nullptr)
        {
            variableIds.insert(scopedVariableId->m_identifier);
        }
    }

    bool AzEventHandler::ContainsReferencesToVariables(const AZStd::unordered_set<ScriptCanvas::VariableId>& variableIds) const
    {
        const Datum* datum = FindDatum(GetSlotId(m_azEventEntry.m_eventName));

        if (auto scopedVariableId = datum->GetAs<ScriptCanvas::GraphScopedVariableId>(); scopedVariableId != nullptr)
        {
            return variableIds.contains(scopedVariableId->m_identifier);
        }

        return false;
    }

    size_t AzEventHandler::GenerateFingerprint() const
    {
        return BehaviorContextUtils::GenerateFingerprintForMethod(MethodType::Event, "", m_azEventEntry.m_eventName);
    }

    AZ::Outcome<DependencyReport, void> AzEventHandler::GetDependencies() const
    {
        return AZ::Success(DependencyReport{});
    }

    const AzEventEntry& AzEventHandler::GetEventEntry() const
    {
        return m_azEventEntry;
    }

    AZ::Outcome<AZStd::string> AzEventHandler::GetInternalOutKey(const Slot& slot) const
    {
        if (&slot == AzEventHandlerProperty::GetOnEventSlot(this))
        {
            return AZ::Success(AZStd::string(slot.GetName()));
        }

        return AZ::Failure();
    }

    AZ::Outcome<Grammar::LexicalScope, void> AzEventHandler::GetFunctionCallLexicalScope([[maybe_unused]] const Slot* slot) const
    {
        return AZ::Success(Grammar::LexicalScope::Variable());
    }

    AZ::Outcome<AZStd::string, void> AzEventHandler::GetFunctionCallName(const Slot* slot) const
    {
        const auto slotId = slot->GetId();
        auto mutableThis = const_cast<AzEventHandler*>(this);

        if (slotId == AzEventHandlerProperty::GetConnectSlotId(mutableThis))
        {
            return AZ::Success(AZStd::string(Grammar::k_AzEventHandlerConnectName));
        }
        else if (slotId == AzEventHandlerProperty::GetDisconnectSlotId(mutableThis))
        {
            return AZ::Success(AZStd::string(Grammar::k_AzEventHandlerDisconnectName));
        }
        else
        {
            return AZ::Failure();
        }
    }

    AZStd::vector<SlotId> AzEventHandler::GetEventSlotIds() const
    {
        return { AzEventHandlerProperty::GetOnEventSlotId(this) };
    }

    AZStd::vector<SlotId> AzEventHandler::GetNonEventSlotIds() const
    {
        AZStd::vector<SlotId> nonEventSlotIds;

        for (const auto& slot : GetSlots())
        {
            if (!IsEventSlotId(slot.GetId()))
            {
                nonEventSlotIds.push_back(slot.GetId());
            }
        }

        return nonEventSlotIds;
    }

    bool AzEventHandler::IsEventSlotId(const SlotId& slotId) const
    {
        return m_azEventEntry.m_azEventInputSlotId == slotId ||
            AZStd::find(m_azEventEntry.m_parameterSlotIds.begin(), m_azEventEntry.m_parameterSlotIds.end(), slotId)
            != m_azEventEntry.m_parameterSlotIds.end();
    }

    bool AzEventHandler::IsEventHandler() const
    {
        return true;
    }

    NodeTypeIdentifier AzEventHandler::GetOutputNodeType(const SlotId& slotId) const
    {
        const bool isAzEvent = slotId == AzEventHandlerProperty::GetOnEventSlotId(this)
            || AZStd::find(m_azEventEntry.m_parameterSlotIds.begin(), m_azEventEntry.m_parameterSlotIds.end(), slotId) != m_azEventEntry.m_parameterSlotIds.end();

        if (isAzEvent)
        {
            auto azEventIdentifier = static_cast<AzEventIdentifier>(AZStd::hash<AZStd::string_view>{}(m_azEventEntry.m_eventName));
            return NodeUtils::ConstructAzEventIdentifier(azEventIdentifier);
        }

        // If we don't match any of the output slots for our events, just return our base type as it's
        // one of the control pins firing.
        return GetNodeType();
    }

    ConstSlotsOutcome AzEventHandler::GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, [[maybe_unused]] const Slot* executionChildSlot) const
    {
        AZStd::vector<const Slot*> slots;

        const auto sourceType = executionSlot.GetType();
        switch (sourceType)
        {
        case CombinedSlotType::ExecutionIn:
        {
            if (targetSlotType == CombinedSlotType::DataIn)
            {
                if (&executionSlot == AzEventHandlerProperty::GetConnectSlot(this))
                {
                    auto eventInputSlot = GetSlot(m_azEventEntry.m_azEventInputSlotId);
                    if (!eventInputSlot)
                    {
                        return AZ::Failure(AZStd::string("AZ::EventHandler failed to return an input event slot"));
                    }

                    slots.push_back(eventInputSlot);
                }
            }
            else if (targetSlotType == CombinedSlotType::ExecutionOut)
            {
                if (&executionSlot == AzEventHandlerProperty::GetConnectSlot(this))
                {
                    slots.push_back(AzEventHandlerProperty::GetOnConnectedSlot(this));
                }
                else if (&executionSlot == AzEventHandlerProperty::GetDisconnectSlot(this))
                {
                    slots.push_back(AzEventHandlerProperty::GetOnDisconnectedSlot(this));
                }
            }
        }
        break;
        case CombinedSlotType::LatentOut:
            if (targetSlotType == CombinedSlotType::DataOut)
            {
                slots = GetSlotsByType(CombinedSlotType::DataOut);
            }
            break;
        }

        return AZ::Success(slots);
    }

    void AzEventEntry::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<AzEventEntry>()
                ->Version(0)
                ->Field("m_eventName", &AzEventEntry::m_eventName)
                ->Field("m_parameterSlotIds", &AzEventEntry::m_parameterSlotIds)
                ->Field("m_parameterNames", &AzEventEntry::m_parameterSlotIds)
                ->Field("m_eventSlotId", &AzEventEntry::m_azEventInputSlotId)
                ;
        }
    }

    AZStd::string AzEventHandler::GetNodeName() const
    {
        return GetDebugName();
    }

    AZStd::string AzEventHandler::GetDebugName() const
    {
        return m_azEventEntry.m_eventName;
    }
}
