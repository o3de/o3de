/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Serialization/DatumSerializer.h>

using namespace ScriptCanvas;

namespace DatumSerializerCpp
{
    bool IsEventInput(const AZ::Uuid& inputType)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Can't serialize data properly without checking the type, for which we need behavior context!");
        auto bcClassIter = behaviorContext->m_typeToClassMap.find(inputType);
        return bcClassIter != behaviorContext->m_typeToClassMap.end()
            && bcClassIter->second->m_azRtti
            && bcClassIter->second->m_azRtti->GetGenericTypeId() == azrtti_typeid<AZ::Event>();
    }
}

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(DatumSerializer, SystemAllocator);

    JsonSerializationResult::Result DatumSerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<Datum>(), "DatumSerializer Load against output typeID that was not Datum");
        AZ_Assert(outputValue, "DatumSerializer Load against null output");

        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
        auto outputDatum = reinterpret_cast<Datum*>(outputValue);

        bool isOverloadedStorage = false;
        AZ_Assert(azrtti_typeid<decltype(outputDatum->m_isOverloadedStorage)>() == azrtti_typeid<decltype(isOverloadedStorage)>()
            , "overloaded storage type changed and won't load properly");
        result.Combine(ContinueLoadingFromJsonObjectField
            ( &isOverloadedStorage
            , azrtti_typeid<decltype(outputDatum->m_isOverloadedStorage)>()
            , inputValue
            , "isOverloadedStorage"
            , context));

        ScriptCanvas::Data::Type scType;
        AZ_Assert(azrtti_typeid<decltype(outputDatum->m_type)>() == azrtti_typeid<decltype(scType)>()
            , "ScriptCanvas::Data::Type type changed and won't load properly");
        result.Combine(ContinueLoadingFromJsonObjectField
            ( &scType
            , azrtti_typeid<decltype(outputDatum->m_type)>()
            , inputValue
            , "scriptCanvasType"
            , context));

        // datum storage begin
        auto isNullPointerMember = inputValue.FindMember("isNullPointer");
        if (isNullPointerMember == inputValue.MemberEnd())
        {
            return context.Report
                ( JSR::Tasks::ReadField
                , JSR::Outcomes::Missing
                , "DatumSerializer::Load failed to load the 'isNullPointer' member");
        }

        if (isNullPointerMember->value.GetBool())
        {
            *outputDatum = Datum(scType, Datum::eOriginality::Original);
        }
        else
        {
            AZ::Uuid typeId = AZ::Uuid::CreateNull();
            auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
            if (typeIdMember == inputValue.MemberEnd())
            {
                return context.Report
                    ( JSR::Tasks::ReadField
                    , JSR::Outcomes::Missing
                    , AZStd::string::format("DatumSerializer::Load failed to load the %s member"
                        , JsonSerialization::TypeIdFieldIdentifier));
            }

            result.Combine(LoadTypeId(typeId, typeIdMember->value, context));
            if (typeId.IsNull())
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic
                    , "DatumSerializer::Load failed to load the AZ TypeId of the value");
            }

            AZStd::any storage = context.GetSerializeContext()->CreateAny(typeId);
            if (storage.empty() || storage.type() != typeId)
            {
                return context.Report(result, "DatumSerializer::Load failed to load a value matched the reported AZ TypeId. "
                    "The C++ declaration may have been deleted or changed.");
            }

            result.Combine(ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&storage), typeId, inputValue, "value", context));
            outputDatum->ReconfigureDatumTo(
                Datum(scType, Datum::eOriginality::Original, AZStd::any_cast<void>(&storage), scType.GetAZType()));
        } // datum storage end

        AZStd::string label;
        AZ_Assert(azrtti_typeid<decltype(outputDatum->m_datumLabel)>() == azrtti_typeid<decltype(label)>()
            , "m_datumLabel type changed and won't load properly");
        result.Combine(ContinueLoadingFromJsonObjectField
            ( &label
            , azrtti_typeid<decltype(outputDatum->m_datumLabel)>()
            , inputValue
            , "label"
            , context));
        outputDatum->SetLabel(label);

        if (auto listeners = context.GetMetadata().Find<SerializationListeners>())
        {
            listeners->push_back(outputDatum);
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "DatumSerializer Load finished loading Datum"
            : "DatumSerializer Load failed to load Datum");
    }

    JsonSerializationResult::Result DatumSerializer::Store
        ( rapidjson::Value& outputValue
        , const void* inputValue
        , const void* defaultValue
        , [[maybe_unused]] const Uuid& valueTypeId
        , JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(valueTypeId == azrtti_typeid<Datum>(), "DatumSerializer Store against value typeID that was not Datum");
        AZ_Assert(inputValue, "DatumSerializer Store against null inputValue pointer ");

        auto inputScriptDataPtr = reinterpret_cast<const Datum*>(inputValue);
        auto defaultScriptDataPtr = reinterpret_cast<const Datum*>(defaultValue);

        if (defaultScriptDataPtr)
        {
            if (*inputScriptDataPtr == *defaultScriptDataPtr)
            {
                return context.Report
                (JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "DatumSerializer Store used defaults for Datum");
            }
        }

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        outputValue.SetObject();

        result.Combine(ContinueStoringToJsonObjectField
            ( outputValue
            , "isOverloadedStorage"
            , &inputScriptDataPtr->m_isOverloadedStorage
            , defaultScriptDataPtr ? &defaultScriptDataPtr->m_isOverloadedStorage : nullptr
            , azrtti_typeid<decltype(inputScriptDataPtr->m_isOverloadedStorage)>()
            , context));

        result.Combine(ContinueStoringToJsonObjectField
            ( outputValue
            , "scriptCanvasType"
            , &inputScriptDataPtr->GetType()
            , defaultScriptDataPtr ? &defaultScriptDataPtr->GetType() : nullptr
            , azrtti_typeid<decltype(inputScriptDataPtr->GetType())>()
            , context));


        // datum storage begin
        auto inputObjectSource = inputScriptDataPtr->GetAsDanger();
        const bool isNullPointer = inputObjectSource == nullptr || DatumSerializerCpp::IsEventInput(inputScriptDataPtr->GetType().GetAZType());
        outputValue.AddMember("isNullPointer", rapidjson::Value(isNullPointer), context.GetJsonAllocator());

        if (!isNullPointer)
        {
            rapidjson::Value typeValue;
            result.Combine(StoreTypeId(typeValue, inputScriptDataPtr->GetType().GetAZType(), context));
            outputValue.AddMember
                ( rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier)
                , AZStd::move(typeValue)
                , context.GetJsonAllocator());

            auto defaultObjectSource = defaultScriptDataPtr ? defaultScriptDataPtr->GetAsDanger() : nullptr;

            result.Combine(ContinueStoringToJsonObjectField
                ( outputValue
                , "value"
                , inputObjectSource
                , defaultObjectSource
                , inputScriptDataPtr->GetType().GetAZType()
                , context));
        }
        // datum storage end

        result.Combine(ContinueStoringToJsonObjectField
            ( outputValue
            , "label"
            , &inputScriptDataPtr->m_datumLabel
            , defaultScriptDataPtr ? &defaultScriptDataPtr->m_datumLabel : nullptr
            , azrtti_typeid<decltype(inputScriptDataPtr->m_datumLabel)>()
            , context));

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "DatumSerializer Store finished saving Datum"
            : "DatumSerializer Store failed to save Datum");
    }
}
