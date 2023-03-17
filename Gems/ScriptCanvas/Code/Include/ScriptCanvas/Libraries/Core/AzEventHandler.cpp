/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/RestrictedNodeContract.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EventHandlerTranslationUtility.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

namespace AzEventHandlerCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Nodes;
    using namespace ScriptCanvas::Nodes::Core;
    
    struct AzEventEntryData_v0
    {
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
            {
                serializeContext->Class<AzEventEntryData_v0>()
                    ->Field("eventName", &AzEventEntryData_v0::m_eventName)
                    ->Field("parameterSlotIds", &AzEventEntryData_v0::m_parameterSlotIds)
                    ->Field("parameterNames", &AzEventEntryData_v0::m_parameterNames)
                    ->Field("azEventInputSlotId", &AzEventEntryData_v0::m_azEventInputSlotId)
                    ;
            }
        }

        AZ_TYPE_INFO(AzEventEntryData_v0, "{D17AE86E-48D3-4187-A4A9-2594CCA034E6}");

        AZStd::string m_eventName;
        AZStd::vector<SlotId> m_parameterSlotIds;
        AZStd::vector<AZStd::string> m_parameterNames;
        SlotId m_azEventInputSlotId;
    };

    class AzEventEntrySerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(AzEventEntrySerializer, "{8FD61AF4-8EBF-4DDB-9251-9A62C05AEA1C}", AZ::BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR_DECL;

    private:

        AZ::JsonSerializationResult::Result Load
            ( void* outputValue
            , [[maybe_unused]] const AZ::Uuid& outputValueTypeId
            , const rapidjson::Value& inputValue
            , AZ::JsonDeserializerContext& context) override
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(outputValueTypeId == azrtti_typeid<AzEventEntry>()
                , "AzEventEntrySerializer Load against output typeID that was not RuntimeVariable");
            AZ_Assert(outputValue, "AzEventEntrySerializer Load against null output");

            auto outputEntry = reinterpret_cast<AzEventEntry*>(outputValue);
            JSR::ResultCode result(JSR::Tasks::ReadField);
            
            auto data_v0 = inputValue.FindMember("AzEventEntryData_v0");
            if (data_v0 != inputValue.MemberEnd())
            {
                // latest version detected, continue loading automatically
                AzEventEntryData_v0 target;
                result.Combine(ContinueLoading(&target, azrtti_typeid<AzEventEntryData_v0>(), data_v0->value, context));
                outputEntry->m_eventName = target.m_eventName;
                outputEntry->m_parameterSlotIds = target.m_parameterSlotIds;
                outputEntry->m_parameterNames = target.m_parameterNames;
                outputEntry->m_azEventInputSlotId = target.m_azEventInputSlotId;
            }
            else
            {
                // older version detected
                // read manually and do not read multiple copies of m_parameterSlotIds, nor erroneously reflected parameter names
                result.Combine(ContinueLoadingFromJsonObjectField
                    ( &outputEntry->m_eventName
                    , azrtti_typeid(outputEntry->m_eventName)
                    , inputValue
                    , "m_eventName"
                    , context));
                result.Combine(ContinueLoadingFromJsonObjectField
                    ( &outputEntry->m_parameterSlotIds
                    , azrtti_typeid(outputEntry->m_parameterSlotIds)
                    , inputValue, "m_parameterSlotIds"
                    , context));
                result.Combine(ContinueLoadingFromJsonObjectField
                    ( &outputEntry->m_azEventInputSlotId
                    , azrtti_typeid(outputEntry->m_azEventInputSlotId)
                    , inputValue
                    , "m_eventSlotId"
                    , context));

                for (size_t newSize = 1; newSize < outputEntry->m_parameterSlotIds.size(); ++newSize)
                {
                    if (outputEntry->m_parameterSlotIds.front() == outputEntry->m_parameterSlotIds[newSize])
                    {
                        outputEntry->m_parameterSlotIds.resize(newSize);
                        break;
                    }
                }
            }

            return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
                ? "AzEventEntrySerializer Load finished loading AzEventEntry"
                : "AzEventEntrySerializer Load failed to load AzEventEntry");
        }

        AZ::JsonSerializationResult::Result Store
            ( rapidjson::Value& outputValue
            , const void* inputValue
            , [[maybe_unused]] const void* defaultValue
            , [[maybe_unused]] const AZ::Uuid& valueTypeId
            , AZ::JsonSerializerContext& context) override
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(valueTypeId == azrtti_typeid<AzEventEntry>()
                , "AzEventEntrySerializer Load against input typeID that was not AzEventEntry");
            AZ_Assert(inputValue, "AzEventEntrySerializer Load against null inputValue");

            auto inputEntry = reinterpret_cast<const AzEventEntry*>(inputValue);
            AzEventEntryData_v0 defaultData;
            AzEventEntryData_v0 target;
            target.m_eventName = inputEntry->m_eventName;
            target.m_parameterSlotIds = inputEntry->m_parameterSlotIds;
            target.m_parameterNames = inputEntry->m_parameterNames;
            target.m_azEventInputSlotId = inputEntry->m_azEventInputSlotId;

            // save out version data as a psuedo member variable
            JSR::ResultCode result(JSR::Tasks::WriteValue);
            outputValue.SetObject();
            {
                rapidjson::Value versionData;
                versionData.SetObject();
                result.Combine(ContinueStoring(versionData, &target, &defaultData, azrtti_typeid<AzEventEntryData_v0>(), context));
                outputValue.AddMember(rapidjson::StringRef("AzEventEntryData_v0"), AZStd::move(versionData), context.GetJsonAllocator());
            }

            return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
                ? "AzEventEntrySerializer Load finished loading Datum"
                : "AzEventEntrySerializer Load failed to load Datum");
        }
    };

    AZ_CLASS_ALLOCATOR_IMPL(AzEventEntrySerializer, AZ::SystemAllocator);

}

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

    AZStd::string AzEventHandler::GetDebugName() const
    {
        return m_azEventEntry.m_eventName;
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

    AZStd::string AzEventHandler::GetNodeName() const
    {
        return GetDebugName();
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
                ->Field("m_eventName", &AzEventEntry::m_eventName)
                ->Field("m_parameterSlotIds", &AzEventEntry::m_parameterSlotIds)
                ->Field("m_parameterNames", &AzEventEntry::m_parameterNames)
                ->Field("m_eventSlotId", &AzEventEntry::m_azEventInputSlotId)
                ;
        }

        AzEventHandlerCpp::AzEventEntryData_v0::Reflect(context);

        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<AzEventHandlerCpp::AzEventEntrySerializer>()->HandlesType<AzEventEntry>();
        }
    }
}
