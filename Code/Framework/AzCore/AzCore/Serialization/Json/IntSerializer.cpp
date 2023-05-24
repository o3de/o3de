/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/IntSerializer.h>

#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonStringConversionUtils.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <cerrno>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(BaseJsonIntegerSerializer, SystemAllocator);

    AZ_CLASS_ALLOCATOR_IMPL(JsonCharSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonShortSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonIntSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonLongSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonLongLongSerializer, SystemAllocator);

    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedCharSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedShortSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedIntSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedLongSerializer, SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedLongLongSerializer, SystemAllocator);

    namespace SerializerInternal
    {
        // Below are some additional helper functions which are really only necessary until we can have if constexpr() support from C++17
        // so that our above code will the correct signed or unsigned function calls at compile time instead of branching

        template <typename T, AZStd::enable_if_t<AZStd::is_signed<T>::value, int> = 0>
        static void StoreToValue(rapidjson::Value& outputValue, T inputValue)
        {
            outputValue.SetInt64(inputValue);
        }

        template <typename T, AZStd::enable_if_t<AZStd::is_unsigned<T>::value, int> = 0>
        static void StoreToValue(rapidjson::Value& outputValue, T inputValue)
        {
            outputValue.SetUint64(inputValue);
        }

        template <typename T>
        static JsonSerializationResult::Result LoadInt(T* outputValue, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context, bool isDefaultValue)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(AZStd::is_integral<T>(), "Expected T to be a signed or unsigned type");
            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            if (isDefaultValue)
            {
                *outputValue = {};
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Integer value set to default of zero.");
            }

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                [[fallthrough]];
            case rapidjson::kObjectType:
                [[fallthrough]];
            case rapidjson::kNullType:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Unsupported type. Integers can't be read from arrays, objects or null.");

            case rapidjson::kStringType:
                return TextToValue(outputValue, inputValue.GetString(), context);

            case rapidjson::kFalseType:
                [[fallthrough]];
            case rapidjson::kTrueType:
                *outputValue = inputValue.GetBool() ? 1 : 0;
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success,
                    "Successfully converted boolean to integer value.");

            case rapidjson::kNumberType:
            {
                JSR::ResultCode result(JSR::Tasks::ReadField);
                if (inputValue.IsInt64())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetInt64(), context.GetPath(), context.GetReporter());
                }
                else if (inputValue.IsDouble())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetDouble(), context.GetPath(), context.GetReporter());
                }
                else
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetUint64(), context.GetPath(), context.GetReporter());
                }

                return context.Report(result, result.GetOutcome() == JSR::Outcomes::Success ?
                    "Successfully read integer value from number field." : "Failed to read integer value from number field.");
            }

            default:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown, "Unknown json type encountered for integer.");
            }
        }

        template <typename T>
        static JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            const T inputValAsType = *reinterpret_cast<const T*>(inputValue);
            if (context.ShouldKeepDefaults() || !defaultValue || (inputValAsType != *reinterpret_cast<const T*>(defaultValue)))
            {
                StoreToValue(outputValue, inputValAsType);
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Successfully stored integer value.");
            }
            
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Skipped integer value because default was used.");
        }
    } // namespace SerializerInternal

    auto BaseJsonIntegerSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    JsonSerializationResult::Result JsonCharSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<char>() == outputValueTypeId,
            "Unable to deserialize char to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<char*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonCharSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<char>() == valueTypeId, "Unable to serialize char to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<char>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonShortSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<short>() == outputValueTypeId,
            "Unable to deserialize short to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<short*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonShortSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<short>() == valueTypeId, "Unable to serialize short to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str()); 
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<short>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonIntSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<int>() == outputValueTypeId,
            "Unable to deserialize int to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<int*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonIntSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<int>() == valueTypeId, "Unable to serialize int to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<int>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<long>() == outputValueTypeId,
            "Unable to deserialize long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<long*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonLongSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<long>() == valueTypeId, "Unable to serialize long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<long>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonLongLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<long long>() == outputValueTypeId,
            "Unable to deserialize long long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<long long*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonLongLongSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<long long>() == valueTypeId, "Unable to serialize long long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str()); 
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<long long>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonUnsignedCharSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned char>() == outputValueTypeId,
            "Unable to deserialize unsigned char to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(
            reinterpret_cast<unsigned char*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonUnsignedCharSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned char>() == valueTypeId, "Unable to serialize unsigned char to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str()); 
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned char>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonUnsignedShortSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned short>() == outputValueTypeId,
            "Unable to deserialize unsigned short to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(
            reinterpret_cast<unsigned short*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonUnsignedShortSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned short>() == valueTypeId, "Unable to serialize unsigned short to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned short>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonUnsignedIntSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned int>() == outputValueTypeId,
            "Unable to deserialize unsigned int to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(
            reinterpret_cast<unsigned int*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonUnsignedIntSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned int>() == valueTypeId, "Unable to serialize unsigned int to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned int>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonUnsignedLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned long>() == outputValueTypeId,
            "Unable to deserialize unsigned long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(
            reinterpret_cast<unsigned long*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonUnsignedLongSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned long>() == valueTypeId, "Unable to serialize unsigned long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned long>(outputValue, inputValue, defaultValue, context);
    }

    JsonSerializationResult::Result JsonUnsignedLongLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned long long>() == outputValueTypeId,
            "Unable to deserialize unsigned long long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(
            reinterpret_cast<unsigned long long*>(outputValue), inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonUnsignedLongLongSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        AZ_Assert(azrtti_typeid<unsigned long long>() == valueTypeId, "Unable to serialize unsigned long long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned long long>(outputValue, inputValue, defaultValue, context);
    }
} // namespace AZ
