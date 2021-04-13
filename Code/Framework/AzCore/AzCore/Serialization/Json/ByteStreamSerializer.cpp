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

#include "ByteStreamSerializer.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    namespace ByteSerializerInternal
    {
        static JsonSerializationResult::Result Load(void* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            using JsonSerializationResult::Outcomes;
            using JsonSerializationResult::Tasks;

            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            switch (inputValue.GetType())
            {
            case rapidjson::kStringType: {
                JsonByteStream* valAsByteStream = static_cast<JsonByteStream*>(outputValue);
                JsonByteStream buffer;
                buffer.resize(inputValue.GetStringLength());
                AZStd::copy(inputValue.GetString(), inputValue.GetString() + inputValue.GetStringLength(), buffer.begin());
                *valAsByteStream = AZStd::move(buffer);
                return context.Report(Tasks::ReadField, Outcomes::Success, "Successfully read ByteStream.");
            }
            case rapidjson::kArrayType:
            case rapidjson::kObjectType:
            case rapidjson::kNullType:
            case rapidjson::kFalseType:
            case rapidjson::kTrueType:
            case rapidjson::kNumberType:
                return context.Report(
                    Tasks::ReadField, Outcomes::Unsupported,
                    "Unsupported type. ByteStream values cannot be read from arrays, objects, nulls, booleans or numbers.");
            default:
                return context.Report(Tasks::ReadField, Outcomes::Unknown, "Unknown json type encountered for ByteStream value.");
            }
        }

        static JsonSerializationResult::Result StoreWithDefault(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, JsonSerializerContext& context)
        {
            using JsonSerializationResult::Outcomes;
            using JsonSerializationResult::Tasks;

            const JsonByteStream& valAsByteStream = *static_cast<const JsonByteStream*>(inputValue);
            if (context.ShouldKeepDefaults() || !defaultValue ||
                (valAsByteStream != *static_cast<const JsonByteStream*>(defaultValue)))
            {
                outputValue.SetString(
                    reinterpret_cast<const char*>(valAsByteStream.data()), aznumeric_caster(valAsByteStream.size()),
                    context.GetJsonAllocator());
                return context.Report(Tasks::WriteValue, Outcomes::Success, "ByteStream successfully stored.");
            }

            return context.Report(Tasks::WriteValue, Outcomes::DefaultsUsed, "Default ByteStream used.");
        }
    } // namespace ByteSerializerInternal

    AZ_CLASS_ALLOCATOR_IMPL(JsonByteStreamSerializer, SystemAllocator, 0);

    JsonSerializationResult::Result JsonByteStreamSerializer::Load(
        void* outputValue, [[maybe_unused]] const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(
            azrtti_typeid<JsonByteStream>() == outputValueTypeId,
            "Unable to deserialize AZStd::vector<AZ::u8>> to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());

        return ByteSerializerInternal::Load(outputValue, inputValue, context);
    }

    JsonSerializationResult::Result JsonByteStreamSerializer::Store(
        rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, [[maybe_unused]] const Uuid& valueTypeId,
        JsonSerializerContext& context)
    {
        AZ_Assert(
            azrtti_typeid<JsonByteStream>() == valueTypeId,
            "Unable to serialize AZStd::vector<AZ::u8> to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());

        return ByteSerializerInternal::StoreWithDefault(outputValue, inputValue, defaultValue, context);
    }
} // namespace AZ
