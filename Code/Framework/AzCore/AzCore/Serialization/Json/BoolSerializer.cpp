/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/BoolSerializer.h>
#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonBoolSerializer, SystemAllocator);

    namespace SerializerInternal
    {
        static JsonSerializationResult::Result TextToValue(bool* outputValue, const char* text, size_t textLength,
            JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            if (text && textLength > 0)
            {
                static constexpr const char trueString[] = "true";
                static constexpr const char falseString[] = "false";
                // remove null terminator for string length counts
                // rapidjson stringlength doesn't include it in length calculations, but sizeof() will
                static constexpr size_t trueStringLength = sizeof(trueString) - 1;
                static constexpr size_t falseStringLength = sizeof(falseString) - 1;

                if ((textLength == trueStringLength) && azstrnicmp(text, trueString, trueStringLength) == 0)
                {
                    *outputValue = true;
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read boolean from string.");
                }
                else if ((textLength == falseStringLength) && azstrnicmp(text, falseString, falseStringLength) == 0)
                {
                    *outputValue = false;
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read boolean from string.");
                }
                else
                {
                    // Intentionally not checking errno here for under or overflow, we only care that the value parsed is not zero.
                    char* parseEnd = nullptr;
                    AZ::s64 num = strtoll(text, &parseEnd, 0);
                    if (parseEnd != text)
                    {
                        *outputValue = (num != 0);
                        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success,
                            "Successfully read boolean from string with number.");
                    }
                }
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "The string didn't contain anything that can be interpreted as a boolean.");
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "The string used to read a boolean from was empty.");
            }
        }
    } // namespace SerializerInternal

    JsonSerializationResult::Result JsonBoolSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<bool>() == outputValueTypeId,
            "Unable to deserialize bool to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

        bool* valAsBool = reinterpret_cast<bool*>(outputValue);

        if (IsExplicitDefault(inputValue))
        {
            *valAsBool = {};
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Boolean value set to default of 'false'.");
        }

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            [[fallthrough]];
        case rapidjson::kObjectType:
            [[fallthrough]];
        case rapidjson::kNullType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Booleans can't be read from arrays, objects or null.");

        case rapidjson::kStringType:
            return SerializerInternal::TextToValue(valAsBool, inputValue.GetString(), inputValue.GetStringLength(), context);

        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            *valAsBool = inputValue.GetBool();
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read boolean.");

        case rapidjson::kNumberType:
            if (inputValue.IsUint64())
            {
                *valAsBool = inputValue.GetUint64() != 0;
            }
            else if (inputValue.IsInt64())
            {
                *valAsBool = inputValue.GetInt64() != 0;
            }
            else if (inputValue.IsDouble())
            {
                double value = inputValue.GetDouble();
                *valAsBool = value != 0.0 && value != -0.0;
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                    "Unsupported json number type encountered for boolean value.");
            }
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read boolean from number.");
        
        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for boolean.");
        }
    }

    JsonSerializationResult::Result JsonBoolSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ_Assert(azrtti_typeid<bool>() == valueTypeId, "Unable to serialize boolean to json because the provided type is %s",
            valueTypeId.ToString<AZ::OSString>().c_str());
        AZ_UNUSED(valueTypeId);

        const bool valAsBool = *reinterpret_cast<const bool*>(inputValue);
        if (context.ShouldKeepDefaults() || !defaultValue || (valAsBool != *reinterpret_cast<const bool*>(defaultValue)))
        {
            outputValue.SetBool(valAsBool);
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Boolean successfully stored");
        }
        
        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default boolean used.");
    }

    auto JsonBoolSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }
} // namespace AZ
