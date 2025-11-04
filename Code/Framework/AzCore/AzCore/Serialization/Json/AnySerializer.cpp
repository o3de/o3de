/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/AnySerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonAnySerializer, SystemAllocator);
    JsonSerializationResult::Result JsonAnySerializer::Load(
        void* outputValue,
        [[maybe_unused]] const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.
        AZ_Assert(outputValueTypeId == azrtti_typeid<AZStd::any>(), "JsonAnySerializer::Load against value typeID that was not AZStd::any");

        if (inputValue.GetType() != rapidjson::kObjectType)
        {
            return context.Report(
                JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "JsonAnySerializer::Load only supports rapidjson::kObjectType");
        }

        AZ::Uuid anyTypeId = AZ::Uuid::CreateNull();
        auto typeIdMember = inputValue.FindMember(JsonSerialization::TypeIdFieldIdentifier);
        if (typeIdMember == inputValue.MemberEnd())
        {
            return context.Report(
                JSR::Tasks::ReadField,
                JSR::Outcomes::DefaultsUsed, // passes one test breaks another?
                "JsonAnySerializer::Load created a default, empty AZStd::any");
        }

        JsonSerializationResult::ResultCode result(JSR::Tasks::ReadField);
        result.Combine(LoadTypeId(anyTypeId, typeIdMember->value, context));

        if (anyTypeId.IsNull())
        {
            return context.Report(
                JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "JsonAnySerializer::Load created a default, empty AZStd::any");
        }
        else
        {
            auto outputAnyPtr = reinterpret_cast<AZStd::any*>(outputValue);
            *outputAnyPtr = context.GetSerializeContext()->CreateAny(anyTypeId);

            if (outputAnyPtr->type() != anyTypeId)
            {
                return context.Report(
                    result,
                    "JsonAnySerializer::Load failed to load a value matched the reported AZ TypeId. "
                    "The C++ declaration may have been deleted or changed.");
            }

            // Allowing the serializer to read from objects contained in members "Value" and "value" for backward compatibility
            result.Combine(ContinueLoadingFromJsonObjectField(
                AZStd::any_cast<void>(outputAnyPtr), anyTypeId, inputValue, inputValue.HasMember("Value") ? "Value" : "value", context));
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "JsonAnySerializer::Load finished loading AZStd::any"
                                                              : "JsonAnySerializer::Load failed to load AZStd::any");
    }

    JsonSerializationResult::Result JsonAnySerializer::Store(
        rapidjson::Value& outputValue,
        const void* inputValue,
        const void* defaultValue,
        [[maybe_unused]] const Uuid& valueTypeId,
        JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.
        AZ_Assert(valueTypeId == azrtti_typeid<AZStd::any>(), "JsonAnySerializer::Store against value typeID that was not AZStd::any");

        JSR::ResultCode result(JSR::Tasks::WriteValue);
        outputValue.SetObject();

        auto inputAnyPtr = reinterpret_cast<const AZStd::any*>(inputValue);
        const auto anyTypeId = inputAnyPtr->type();

        if (!anyTypeId.IsNull())
        {
            rapidjson::Value typeValue;
            result.Combine(StoreTypeId(typeValue, anyTypeId, context));
            outputValue.AddMember(
                rapidjson::StringRef(JsonSerialization::TypeIdFieldIdentifier), AZStd::move(typeValue), context.GetJsonAllocator());

            auto defaultAnyPtr = reinterpret_cast<const AZStd::any*>(defaultValue);
            const void* defaultValueSource =
                defaultAnyPtr && defaultAnyPtr->type() == anyTypeId ? AZStd::any_cast<void>(defaultAnyPtr) : nullptr;

            // The serializer will save the object in "Value" with an uppercase "V", which is standard in other serializers
            result.Combine(ContinueStoringToJsonObjectField(
                outputValue, "Value", AZStd::any_cast<void>(inputAnyPtr), defaultValueSource, anyTypeId, context));
        }

        return context.Report(
            result,
            result.GetProcessing() != JSR::Processing::Halted ? "JsonAnySerializer::Store finished storing AZStd::any"
                                                              : "JsonAnySerializer::Store failed to store AZStd::any");
    }

} // namespace AZ
