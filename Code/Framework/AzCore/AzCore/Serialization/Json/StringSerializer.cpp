/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Serialization/Json/StringSerializer.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace StringSerializerInternal
    {
        template<typename StringType>
        static JsonSerializationResult::Result Load(void* outputValue, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            StringType* valAsString = reinterpret_cast<StringType*>(outputValue);

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                // fallthrough
            case rapidjson::kObjectType:
                // fallthrough
            case rapidjson::kNullType:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Unsupported type. String values can't be read from arrays, objects or null.");

            case rapidjson::kStringType:
                *valAsString = StringType(inputValue.GetString(), inputValue.GetStringLength());
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read string.");

            case rapidjson::kFalseType:
                *valAsString = "False";
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read string from boolean.");
            case rapidjson::kTrueType:
                *valAsString = "True";
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read string from boolean.");

            case rapidjson::kNumberType:
            {
                if (inputValue.IsUint64())
                {
                    AZStd::to_string(*valAsString, inputValue.GetUint64());
                }
                else if (inputValue.IsInt64())
                {
                    AZStd::to_string(*valAsString, inputValue.GetInt64());
                }
                else if (inputValue.IsDouble())
                {
                    AZStd::to_string(*valAsString, inputValue.GetDouble());
                }
                else
                {
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                        "Unsupported json number type encountered for string value.");
                }
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read string from number.");
            }

            default:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for string value.");
            }
        }

        template<typename StringType>
        static JsonSerializationResult::Result StoreWithDefault(rapidjson::Value& outputValue, const void* inputValue,
            const void* defaultValue, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            const StringType& valAsString = *reinterpret_cast<const StringType*>(inputValue);
            if (context.ShouldKeepDefaults() || !defaultValue || (valAsString != *reinterpret_cast<const StringType*>(defaultValue)))
            {
                outputValue.SetString(valAsString.c_str(), aznumeric_caster(valAsString.length()), context.GetJsonAllocator());
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "String successfully stored.");
            }

            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default String used.");
        }
    } // namespace Internal

    AZ_CLASS_ALLOCATOR_IMPL(JsonStringSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonStringSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<AZStd::string>() == outputValueTypeId,
            "Unable to deserialize AZStd::string to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return StringSerializerInternal::Load<AZStd::string>(outputValue, inputValue, context);
    }

    JsonSerializationResult::Result JsonStringSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<AZStd::string>() == valueTypeId, "Unable to serialize AZStd::string to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);

        return StringSerializerInternal::StoreWithDefault<AZStd::string>(outputValue, inputValue, defaultValue, context);
    }


    AZ_CLASS_ALLOCATOR_IMPL(JsonOSStringSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonOSStringSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<OSString>() == outputValueTypeId,
            "Unable to deserialize OSString to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return StringSerializerInternal::Load<OSString>(outputValue, inputValue, context);
    }

    JsonSerializationResult::Result JsonOSStringSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<OSString>() == valueTypeId, "Unable to serialize OSString to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return StringSerializerInternal::StoreWithDefault<OSString>(outputValue, inputValue, defaultValue, context);
    }
} // namespace AZ
