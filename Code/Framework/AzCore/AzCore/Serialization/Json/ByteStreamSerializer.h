/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    template<class Allocator>
    using JsonByteStream = AZStd::vector<AZ::u8, Allocator>; //!< Alias for AZStd::vector<AZ::u8>.

    inline constexpr AZ::Uuid JsonByteStreamSerializerId{ "{30F0EA5A-CD13-4BA7-BAE1-D50D851CAC45}" };

    //! Serialize a stream of bytes (usually binary data) as a json string value.
    //! @note Related to GenericClassByteStream (part of SerializeGenericTypeInfo<AZStd::vector<AZ::u8>> - see AZStdContainers.inl for more
    //! details).
    template<class Allocator>
    class JsonByteStreamSerializer : public BaseJsonSerializer
    {
    public:
        AZ_RTTI((JsonByteStreamSerializer, JsonByteStreamSerializerId, Allocator), BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR(JsonByteStreamSerializer, AZ::SystemAllocator, 0);

        JsonSerializationResult::Result Load(
            void* outputValue,
            [[maybe_unused]] const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            JsonDeserializerContext& context) override
        {
            using JsonSerializationResult::Outcomes;
            using JsonSerializationResult::Tasks;

            AZ_Assert(
                azrtti_typeid<JsonByteStream<Allocator>>() == outputValueTypeId,
                "Unable to deserialize AZStd::vector<AZ::u8>> to json because the "
                "provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            switch (inputValue.GetType())
            {
            case rapidjson::kStringType:
                {
                    JsonByteStream<AZStd::allocator> buffer; // Need to use standard allocator: Decode does not supports other allocators
                    if (AZ::StringFunc::Base64::Decode(buffer, inputValue.GetString(), inputValue.GetStringLength()))
                    {
                        auto* valAsByteStream = static_cast<JsonByteStream<Allocator>*>(outputValue);
                        if constexpr (std::is_same_v<Allocator, AZStd::allocator>)
                        {
                            *valAsByteStream = AZStd::move(buffer); // Same allocator -> Move
                        }
                        else
                        {
                            valAsByteStream->assign(buffer.begin(), buffer.end()); // Different allocators -> copy
                        }
                        return context.Report(Tasks::ReadField, Outcomes::Success, "Successfully read ByteStream.");
                    }
                    return context.Report(Tasks::ReadField, Outcomes::Invalid, "Decode of Base64 encoded ByteStream failed.");
                }
            case rapidjson::kArrayType:
            case rapidjson::kObjectType:
            case rapidjson::kNullType:
            case rapidjson::kFalseType:
            case rapidjson::kTrueType:
            case rapidjson::kNumberType:
                return context.Report(
                    Tasks::ReadField,
                    Outcomes::Unsupported,
                    "Unsupported type. ByteStream values cannot be read from "
                    "arrays, objects, nulls, booleans or numbers.");
            default:
                return context.Report(Tasks::ReadField, Outcomes::Unknown, "Unknown json type encountered for ByteStream value.");
            }
        }

        JsonSerializationResult::Result Store(
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            [[maybe_unused]] const Uuid& valueTypeId,
            JsonSerializerContext& context) override
        {
            using JsonSerializationResult::Outcomes;
            using JsonSerializationResult::Tasks;

            AZ_Assert(
                azrtti_typeid<JsonByteStream<Allocator>>() == valueTypeId,
                "Unable to serialize AZStd::vector<AZ::u8> to json because the "
                "provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());

            const auto& valAsByteStream = *static_cast<const JsonByteStream<Allocator>*>(inputValue);
            if (context.ShouldKeepDefaults() || !defaultValue ||
                (valAsByteStream != *static_cast<const JsonByteStream<Allocator>*>(defaultValue)))
            {
                const auto base64ByteStream = AZ::StringFunc::Base64::Encode(valAsByteStream.data(), valAsByteStream.size());
                outputValue.SetString(
                    base64ByteStream.c_str(), static_cast<rapidjson::SizeType>(base64ByteStream.size()), context.GetJsonAllocator());
                return context.Report(Tasks::WriteValue, Outcomes::Success, "ByteStream successfully stored.");
            }

            return context.Report(Tasks::WriteValue, Outcomes::DefaultsUsed, "Default ByteStream used.");
        }
    };
    AZ_TYPE_INFO_TEMPLATE(JsonByteStreamSerializer, JsonByteStreamSerializerId, AZ_TYPE_INFO_CLASS);
} // namespace AZ
