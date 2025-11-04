/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/AZStdAnyDataContainer.inl>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Serialization/BehaviorContextObjectSerializer.h>

using namespace ScriptCanvas;

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(BehaviorContextObjectSerializer, SystemAllocator);

    JsonSerializationResult::Result BehaviorContextObjectSerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(outputValueTypeId == azrtti_typeid<BehaviorContextObject>(), "BehaviorContextObjectSerializer Load against output typeID"
            "that was not BehaviorContextObject");
        AZ_Assert(outputValue, "BehaviorContextObjectSerializer Load against null output");

        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
        auto outputBehaviorContextObject = reinterpret_cast<BehaviorContextObject*>(outputValue);

        bool isOwned = false;
        result.Combine(ContinueLoadingFromJsonObjectField
                ( &isOwned
                , azrtti_typeid<bool>()
                , inputValue
                , "isOwned"
                , context));

        if (isOwned)
        {
            AZStd::any storage;
            { // any storage begin

                auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
                if (typeIdMember == inputValue.MemberEnd())
                {
                    return context.Report
                        (JSR::Tasks::ReadField
                        , JSR::Outcomes::Missing
                        , AZStd::string::format("BehaviorContextObjectSerializer::Load failed to load the %s member"
                        , JsonSerialization::TypeIdFieldIdentifier));
                }

                AZ::Uuid typeId;
                result.Combine(LoadTypeId(typeId, typeIdMember->value, context));
                if (typeId.IsNull())
                {
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic
                        , "BehaviorContextObjectSerializer::Load failed to load the AZ TypeId of the value");
                }

                storage = context.GetSerializeContext()->CreateAny(typeId);
                if (storage.empty() || storage.type() != typeId)
                {
                    return context.Report(result, "BehaviorContextObjectSerializer::Load failed to load a value matched the reported AZ "
                        "TypeId. The C++ declaration may have been deleted or changed.");
                }

                result.Combine(ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(&storage), typeId, inputValue, "value", context));
            } // any storage end

            auto bcClass = AZ::BehaviorContextHelper::GetClass(storage.type());
            BehaviorContextObject::Deserialize(*outputBehaviorContextObject, *bcClass, storage);
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "BehaviorContextObjectSerializer Load finished loading BehaviorContextObject"
            : "BehaviorContextObjectSerializer Load failed to load BehaviorContextObject");
    }

    JsonSerializationResult::Result BehaviorContextObjectSerializer::Store
        ( rapidjson::Value& outputValue
        , const void* inputValue
        , const void* defaultValue
        , [[maybe_unused]] const Uuid& valueTypeId
        , JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult;

        AZ_Assert(valueTypeId == azrtti_typeid<BehaviorContextObject>(), "BehaviorContextObjectSerializer Store against value typeID that "
            "was not BehaviorContextObject");
        AZ_Assert(inputValue, "BehaviorContextObjectSerializer Store against null inputValue pointer ");

        auto defaultScriptDataPtr = reinterpret_cast<const BehaviorContextObject*>(defaultValue);
        auto inputScriptDataPtr = reinterpret_cast<const BehaviorContextObject*>(inputValue);

        if (defaultScriptDataPtr)
        {
            if (AZ::Helpers::CompareAnyValue(inputScriptDataPtr->ToAny(), defaultScriptDataPtr->ToAny()))
            {
                return context.Report
                    ( JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "BehaviorContextObjectSerializer Store used defaults "
                        "for BehaviorContextObject");
            }
        }

        outputValue.SetObject();
        JSR::ResultCode result(JSR::Tasks::WriteValue);
        const bool isInputOwned = inputScriptDataPtr->IsOwned();
        const bool isDefaultOwned = defaultScriptDataPtr ? defaultScriptDataPtr->IsOwned() : false;
        result.Combine(ContinueStoringToJsonObjectField
                ( outputValue
                , "isOwned"
                , &isInputOwned
                , &isDefaultOwned
                , azrtti_typeid<bool>()
                , context));

        if (isInputOwned)
        {
            // any storage begin
            {
                rapidjson::Value typeValue;
                result.Combine(StoreTypeId(typeValue, inputScriptDataPtr->ToAny().type(), context));
                outputValue.AddMember
                    ( rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier)
                    , AZStd::move(typeValue)
                    , context.GetJsonAllocator());
            }

            result.Combine(ContinueStoringToJsonObjectField
                    ( outputValue
                    , "value"
                    , inputScriptDataPtr->Get()
                    , defaultScriptDataPtr ? defaultScriptDataPtr->Get() : nullptr
                    , inputScriptDataPtr->ToAny().type()
                    , context));
            // datum storage end       
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "BehaviorContextObjectSerializer Store finished saving BehaviorContextObject"
            : "BehaviorContextObjectSerializer Store failed to save BehaviorContextObject");
    }
}
