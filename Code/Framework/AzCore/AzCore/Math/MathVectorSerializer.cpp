/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/MathVectorSerializer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/osstring.h>

namespace AZ
{
    namespace JsonMathVectorSerializerInternal
    {
        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result LoadArray(VectorType& output, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");
            
            rapidjson::SizeType arraySize = inputValue.Size();
            if (arraySize < ElementCount)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Not enough numbers in array to load math vector from.");
            }

            AZ::BaseJsonSerializer* floatSerializer = context.GetRegistrationContext()->GetSerializerForType(azrtti_typeid<float>());
            if (!floatSerializer)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "Failed to find the json float serializer.");
            }

            constexpr const char* names[4] = { "0", "1", "2", "3" };
            float values[ElementCount];
            for (int i = 0; i < ElementCount; ++i)
            {
                ScopedContextPath subPath(context, names[i]);
                JSR::Result intermediate = floatSerializer->Load(values + i, azrtti_typeid<float>(), inputValue[i], context);
                if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
                {
                    return intermediate;
                }
            }

            for (int i = 0; i < ElementCount; ++i)
            {
                output.SetElement(i, values[i]);
            }

            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read math vector.");
        }

        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result LoadObject(VectorType& output, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");

            constexpr const char* names[8] = { "X", "x", "Y", "y", "Z", "z", "W", "w" };

            AZ::BaseJsonSerializer* floatSerializer = context.GetRegistrationContext()->GetSerializerForType(azrtti_typeid<float>());
            if (!floatSerializer)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "Failed to find the json float serializer.");
            }

            JSR::ResultCode result(JSR::Tasks::ReadField);
            float values[ElementCount];
            for (int i = 0; i < ElementCount; ++i)
            {
                values[i] = output.GetElement(i);

                size_t nameIndex = i * 2;
                auto iterator = inputValue.FindMember(rapidjson::StringRef(names[nameIndex]));
                if (iterator == inputValue.MemberEnd())
                {
                    nameIndex++;
                    iterator = inputValue.FindMember(rapidjson::StringRef(names[nameIndex]));
                    if (iterator == inputValue.MemberEnd())
                    {
                        // Element not found so leave default value.
                        result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
                        continue;
                    }
                }

                ScopedContextPath subPath(context, names[nameIndex]);
                JSR::Result intermediate = floatSerializer->Load(values + i, azrtti_typeid<float>(), iterator->value, context);
                if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
                {
                    return intermediate;
                }
                else
                {
                    result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success));
                }
            }

            for (int i = 0; i < ElementCount; ++i)
            {
                output.SetElement(i, values[i]);
            }

            return context.Report(result, "Successfully read math vector.");
        }

        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context, bool isExplicitDefault)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");
            
            AZ_Assert(azrtti_typeid<VectorType>() == outputValueTypeId,
                "Unable to deserialize Vector%zu to json because the provided type is %s",
                ElementCount, outputValueTypeId.ToString<OSString>().c_str());
            AZ_UNUSED(outputValueTypeId);
            
            VectorType* vector = reinterpret_cast<VectorType*>(outputValue);
            AZ_Assert(vector, "Output value for JsonVector%iSerializer can't be null.", ElementCount);

            if (isExplicitDefault)
            {
                *vector = VectorType::CreateZero();
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Math vector value set to default of zero.");
            }

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
                return LoadArray<VectorType, ElementCount>(*vector, inputValue, context);
            case rapidjson::kObjectType:
                return LoadObject<VectorType, ElementCount>(*vector, inputValue, context);

            case rapidjson::kStringType:
                [[fallthrough]];
            case rapidjson::kNumberType:
                [[fallthrough]];
            case rapidjson::kNullType:
                [[fallthrough]];
            case rapidjson::kFalseType:
                [[fallthrough]];
            case rapidjson::kTrueType:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                    "Unsupported type. Math vectors can only be read from arrays or objects.");

            default:
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                    "Unknown json type encountered in math vector.");
            }
        }
        
        template<typename VectorType, size_t ElementCount>
        JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(ElementCount >= 2 && ElementCount <= 4, "MathVectorSerializer only support Vector2, Vector3 and Vector4.");
            AZ_Assert(azrtti_typeid<VectorType>() == valueTypeId, "Unable to serialize Vector%i to json because the provided type is %s",
                ElementCount, valueTypeId.ToString<OSString>().c_str());
            AZ_UNUSED(valueTypeId);

            const VectorType* vector = reinterpret_cast<const VectorType*>(inputValue);
            AZ_Assert(vector, "Input value for JsonVector%zuSerializer can't be null.", ElementCount);
            const VectorType* defaultVector = reinterpret_cast<const VectorType*>(defaultValue);

            if (!context.ShouldKeepDefaults() && defaultVector && *vector == *defaultVector)
            {
                return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default math Vector used.");
            }

            outputValue.SetArray();
            for (int i = 0; i < ElementCount; ++i)
            {
                outputValue.PushBack(vector->GetElement(i), context.GetJsonAllocator());
            }

            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Math Vector successfully stored.");
        }
    }


    // BaseJsonVectorSerializer

    AZ_CLASS_ALLOCATOR_IMPL(BaseJsonVectorSerializer, SystemAllocator);

    auto BaseJsonVectorSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }

    
    // Vector2

    AZ_CLASS_ALLOCATOR_IMPL(JsonVector2Serializer, SystemAllocator);

    JsonSerializationResult::Result JsonVector2Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Load<Vector2, 2>(
            outputValue, outputValueTypeId, inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonVector2Serializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Store<Vector2, 2>(outputValue, inputValue, defaultValue, valueTypeId, context);
    }


    // Vector3

    AZ_CLASS_ALLOCATOR_IMPL(JsonVector3Serializer, SystemAllocator);

    JsonSerializationResult::Result JsonVector3Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Load<Vector3, 3>(
            outputValue, outputValueTypeId, inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonVector3Serializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Store<Vector3, 3>(outputValue, inputValue, defaultValue, valueTypeId, context);
    }


    // Vector4

    AZ_CLASS_ALLOCATOR_IMPL(JsonVector4Serializer, SystemAllocator);

    JsonSerializationResult::Result JsonVector4Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Load<Vector4, 4>(
            outputValue, outputValueTypeId, inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonVector4Serializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Store<Vector4, 4>(outputValue, inputValue, defaultValue, valueTypeId, context);
    }

    // Quaternion

    AZ_CLASS_ALLOCATOR_IMPL(JsonQuaternionSerializer, SystemAllocator);

    JsonSerializationResult::Result JsonQuaternionSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        // check for "yaw, pitch, roll" object
        if (inputValue.IsObject())
        {
            if (IsExplicitDefault(inputValue))
            {
                Quaternion* outQuaternion = reinterpret_cast<Quaternion*>(outputValue);
                *outQuaternion = Quaternion::CreateIdentity();
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Using identity quaternion for empty object.");
            }

            AZ::BaseJsonSerializer* floatSerializer = context.GetRegistrationContext()->GetSerializerForType(azrtti_typeid<float>());
            if (!floatSerializer)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "Failed to find the json float serializer.");
            }

            constexpr const char* names[3] = {"yaw", "pitch", "roll"};
            float values[3];
            int i = 0;
            for (auto itr = inputValue.MemberBegin(); itr != inputValue.MemberEnd(); ++i, ++itr)
            {
                ScopedContextPath subPath(context, names[i]);
                JSR::Result intermediate = floatSerializer->Load(values + i, azrtti_typeid<float>(), itr->value, context);
                if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
                {
                    return intermediate;
                }
            }

            auto eulerAnglesDegrees = Vector3::CreateFromFloat3(values);
            reinterpret_cast<Quaternion*>(outputValue)->SetFromEulerDegrees(eulerAnglesDegrees);
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read quaternion.");
        }

        return JsonMathVectorSerializerInternal::Load<Quaternion, 4>(outputValue, outputValueTypeId, inputValue, context, false);
    }

    JsonSerializationResult::Result JsonQuaternionSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        return JsonMathVectorSerializerInternal::Store<Quaternion, 4>(outputValue, inputValue, defaultValue, valueTypeId, context);
    }
}
