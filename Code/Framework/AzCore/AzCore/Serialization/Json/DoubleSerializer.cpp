/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/DoubleSerializer.h>

#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/typetraits/is_floating_point.h>
#include <AzCore/Math/MathUtils.h>
#include <limits>
#include <cerrno>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonDoubleSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonFloatSerializer, SystemAllocator);

    namespace SerializerFloatingPointInternal
    {
        template <typename T>
        static JsonSerializationResult::Result TextToValue(T* outputValue, const char* text, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(AZStd::is_floating_point<T>(), "Expected T to be a floating point type");

            char* parseEnd = nullptr;

            AZ::Locale::ScopedSerializationLocale scopedLocale; // invariant locale for strtod

            errno = 0;
            double parsedDouble = strtod(text, &parseEnd);
            if (errno == ERANGE)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Floating point value it too big or too small to fit in the target.");
            }

            if (parseEnd != text)
            {
                JSR::ResultCode result = JsonNumericCast<T>(*outputValue, parsedDouble, context.GetPath(), context.GetReporter());
                AZStd::string_view message = result.GetOutcome() == JSR::Outcomes::Success ?
                    "Successfully read floating point number from string." : "Failed to read floating point number from string.";
                return context.Report(result, message);
            }
            else
            {
                AZStd::string_view message = (text && text[0]) ?
                    "Unable to read floating point value from provided string." : "String used to read floating point value from was empty.";
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, message);
            }
        }

        template <typename T>
        static JsonSerializationResult::Result Load(
            T* outputValue, const rapidjson::Value& inputValue, JsonDeserializerContext& context, bool isExplicitDefault)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(AZStd::is_floating_point<T>::value, "Expected T to be a floating point type");
            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            if (isExplicitDefault)
            {
                *outputValue = {};
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Floating point value set to default of 0.0.");
            }

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                [[fallthrough]];
            case rapidjson::kObjectType:
                [[fallthrough]];
            case rapidjson::kNullType:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Unsupported type. Floating point values can't be read from arrays, objects or null.");

            case rapidjson::kStringType:
                return TextToValue(outputValue, inputValue.GetString(), context);

            case rapidjson::kFalseType:
                [[fallthrough]];
            case rapidjson::kTrueType:
                *outputValue = inputValue.GetBool() ? 1.0f : 0.0f;
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success,
                    "Successfully converted boolean to floating point value.");

            case rapidjson::kNumberType:
            {
                JSR::ResultCode result(JSR::Tasks::ReadField);
                if (inputValue.IsDouble())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetDouble(), context.GetPath(), context.GetReporter());
                }
                else if (inputValue.IsUint64())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetUint64(), context.GetPath(), context.GetReporter());
                }
                else if (inputValue.IsInt64())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetInt64(), context.GetPath(), context.GetReporter());
                }
                else
                {
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                        "Unsupported json number type encountered for floating point value.");
                }

                return context.Report(result, result.GetOutcome() == JSR::Outcomes::Success ?
                    "Successfully read floating point value from number field." : "Failed to read floating point value from number field.");
            }

            default:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for floating point value.");
            }
        }

        template <typename T>
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            const T inputValAsType = *reinterpret_cast<const T*>(inputValue);
            if (context.ShouldKeepDefaults() || !defaultValue ||
                !AZ::IsClose(inputValAsType, *reinterpret_cast<const T*>(defaultValue), std::numeric_limits<T>::epsilon()))
            {
                outputValue.SetDouble(inputValAsType);
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Floating point number successfully stored.");
            }

            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default floating point number used.");
        }
    } // namespace SerializerFloatingPointInternal

    JsonSerializationResult::Result JsonDoubleSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<double>() == outputValueTypeId,
            "Unable to deserialize double to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerFloatingPointInternal::Load(
            reinterpret_cast<double*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonDoubleSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<double>() == valueTypeId, "Unable to serialize double to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerFloatingPointInternal::Store<double>(outputValue, inputValue, defaultValue, context);
    }

    auto JsonDoubleSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonFloatSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<float>() == outputValueTypeId,
            "Unable to deserialize float to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerFloatingPointInternal::Load(
            reinterpret_cast<float*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonFloatSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<float>() == valueTypeId, "Unable to serialize float to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerFloatingPointInternal::Store<float>(outputValue, inputValue, defaultValue, context);
    }

    auto JsonFloatSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }
} // namespace AZ
