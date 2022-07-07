/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/AnySerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonAnySerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonAnySerializer::Load
        ( void* outputValue
        , [[maybe_unused]] const Uuid& outputValueTypeId
        , const rapidjson::Value& inputValue
        , JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        AZ_Assert(outputValueTypeId == azrtti_typeid<AZStd::any>(), "JsonAnySerializer::Load against value typeID that was not AZStd::any");

        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);

        AZ::Uuid typeId = AZ::Uuid::CreateNull();
        auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
        if (typeIdMember == inputValue.MemberEnd())
        {
            return context.Report
            (JSR::Tasks::ReadField
                , JSR::Outcomes::Missing
                , AZStd::string::format("JsonAnySerializer::Load failed to load the %s member"
                    , JsonSerialization::TypeIdFieldIdentifier));
        }

        result.Combine(LoadTypeId(typeId, typeIdMember->value, context));

        if (!typeId.IsNull())
        {
            auto outputAnyPtr = reinterpret_cast<AZStd::any*>(outputValue);
            *outputAnyPtr = context.GetSerializeContext()->CreateAny(typeId);

            if (outputAnyPtr->empty() || outputAnyPtr->type() != typeId)
            {
                return context.Report(result, "JsonAnySerializer::Load failed to load a value matched the reported AZ TypeId. "
                    "The C++ declaration may have been deleted or changed.");
            }

            result.Combine(ContinueLoadingFromJsonObjectField(AZStd::any_cast<void>(outputAnyPtr), typeId, inputValue, "value", context));
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "JsonAnySerializer::Load finished loading AZStd::any"
            : "JsonAnySerializer::Load failed to load AZStd::any");;
    }

    JsonSerializationResult::Result JsonAnySerializer::Store
        ( rapidjson::Value& outputValue
        , const void* inputValue
        , const void* defaultValue
        , [[maybe_unused]] const Uuid& valueTypeId
        , JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        AZ_Assert(valueTypeId == azrtti_typeid<AZStd::any>(), "JsonAnySerializer::Store against value typeID that was not AZStd::any");

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        outputValue.SetObject();
        rapidjson::Value typeValue;

        auto inputAnyPtr = reinterpret_cast<const AZStd::any*>(inputValue);
        const auto typeId = inputAnyPtr->type();
        result.Combine(StoreTypeId(typeValue, typeId, context));
        outputValue.AddMember
            ( rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier)
            , AZStd::move(typeValue)
            , context.GetJsonAllocator());

        if (!typeId.IsNull() && AZStd::any_cast<void>(inputAnyPtr) != nullptr)
        {
            auto defaultAnyPtr = reinterpret_cast<const AZStd::any*>(defaultValue);
            const void* defaultValueSource = defaultAnyPtr->type() == typeId ? AZStd::any_cast<void>(defaultAnyPtr) : nullptr;

            result.Combine(ContinueStoringToJsonObjectField
                ( outputValue
                , "value"
                , AZStd::any_cast<void>(inputAnyPtr)
                , defaultValueSource
                , typeId
                , context));
        }

        return context.Report(result, result.GetProcessing() != JSR::Processing::Halted
            ? "JsonAnySerializer::Store finished storing AZStd::any"
            : "JsonAnySerializer::Store failed to store AZStd::any");
    }

} // namespace AZ
