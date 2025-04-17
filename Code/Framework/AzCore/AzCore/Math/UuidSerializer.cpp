/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/UuidSerializer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonUuidSerializer, SystemAllocator);

    JsonUuidSerializer::MessageResult::MessageResult(AZStd::string_view message, JsonSerializationResult::ResultCode result)
        : m_message(message)
        , m_result(result)
    {
    }

    JsonUuidSerializer::JsonUuidSerializer()
        : m_zeroUuidString(AZ::Uuid::CreateNull().ToString<AZStd::string>())
        , m_zeroUuidStringNoDashes(AZ::Uuid::CreateNull().ToString<AZStd::string>(true, false))
    {
        m_uuidFormat = AZStd::regex(R"(\{[a-f0-9]{8}-?[a-f0-9]{4}-?[a-f0-9]{4}-?[a-f0-9]{4}-?[a-f0-9]{12}\})",
            AZStd::regex_constants::icase | AZStd::regex_constants::optimize);
    }

    auto JsonUuidSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonUuidSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        MessageResult result = UnreportedLoad(outputValue, outputValueTypeId, inputValue);
        return context.Report(result.m_result, result.m_message);
    }

    JsonUuidSerializer::MessageResult JsonUuidSerializer::UnreportedLoad(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Uuid>() == outputValueTypeId,
            "Unable to deserialize Uuid to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        Uuid* valAsUuid = reinterpret_cast<Uuid*>(outputValue);

        if (IsExplicitDefault(inputValue) || (inputValue == m_zeroUuidString.c_str()) || (inputValue == m_zeroUuidStringNoDashes.c_str()))
        {
            *valAsUuid = AZ::Uuid::CreateNull();
            return MessageResult("Uuid value set to default of null.", JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
        }

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            [[fallthrough]];
        case rapidjson::kObjectType:
            [[fallthrough]];
        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            [[fallthrough]];
        case rapidjson::kNumberType:
            [[fallthrough]];
        case rapidjson::kNullType:
            return MessageResult("Unsupported type. Uuids can only be read from strings.",
                JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported));

        case rapidjson::kStringType:
        {
            AZStd::smatch uuidMatch;
            if (AZStd::regex_search(inputValue.GetString(), inputValue.GetString() + inputValue.GetStringLength(), uuidMatch, m_uuidFormat))
            {
                AZ_Assert(uuidMatch.size() > 0, "Regex found uuid pattern, but zero matches were returned.");
                size_t stringLength = uuidMatch.suffix().first - uuidMatch[0].first;
                AZ::Uuid temp = Uuid::CreateString(uuidMatch[0].first, stringLength);
                if (temp.IsNull())
                {
                    return MessageResult("Failed to create uuid from string.",
                        JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported));
                }
                *valAsUuid = temp;
                return MessageResult("Successfully read uuid.", JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success));
            }
            else
            {
                return MessageResult("No part of the string could be interpreted as a uuid.",
                    JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported));
            }
        }

        default:
            return MessageResult("Unknown json type encountered in Uuid.", JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Unknown));
        }
    }

    JsonSerializationResult::Result JsonUuidSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        MessageResult result = UnreportedStore(outputValue, inputValue, defaultValue, valueTypeId, context);
        return context.Report(result.m_result, result.m_message);
    }

    JsonUuidSerializer::MessageResult JsonUuidSerializer::UnreportedStore(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<Uuid>() == valueTypeId, "Unable to serialize Uuid to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        const Uuid& valAsUuid = *reinterpret_cast<const Uuid*>(inputValue);
        if (context.ShouldKeepDefaults() || !defaultValue || (valAsUuid != *reinterpret_cast<const Uuid*>(defaultValue)))
        {
            AZStd::string valAsString = valAsUuid.ToString<AZStd::string>();
            outputValue.SetString(valAsString.c_str(), aznumeric_caster(valAsString.length()), context.GetJsonAllocator());
            return MessageResult("Uuid successfully stored.", JSR::ResultCode(JSR::Tasks::WriteValue, JSR::Outcomes::Success));
        }
        
        return MessageResult("Default Uuid used.", JSR::ResultCode(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed));
    }
} // namespace AZ
