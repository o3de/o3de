/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace RPI
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialPropertyValueSourceDataSerializer, SystemAllocator);

        namespace JSR = JsonSerializationResult;

        template<typename T>
        bool JsonMaterialPropertyValueSourceDataSerializer::LoadAny(
            MaterialPropertyValueSourceData& propertyValue, const T& defaultValue,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            T value = defaultValue;
            JSR::ResultCode result = ContinueLoading(&value, azrtti_typeid<T>(), inputValue, context);

            bool loadSuccess = (result.GetOutcome() == JSR::Outcomes::Success);
            if (loadSuccess)
            {
                propertyValue.m_possibleValues[azrtti_typeid<T>()] = value;
            }
            return loadSuccess;
        }

        JsonSerializationResult::Result JsonMaterialPropertyValueSourceDataSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            AZ_Assert(azrtti_typeid<MaterialPropertyValueSourceData>() == outputValueTypeId,
                "Unable to deserialize material property value to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialPropertyValueSourceData* materialPropertyValue = reinterpret_cast<MaterialPropertyValueSourceData*>(outputValue);
            AZ_Assert(materialPropertyValue, "Output value for MaterialPropertyValueSourceDataSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            // We will attempt to read a value with different data types, most of them will fail but this exercise will
            // report many warnings that are unnecessary. To avoid spamming the Logs with useless errors/warnings We will push
            // a silent reporter and pop it afterwards to report any real errors.
            auto silentReporter = []([[maybe_unused]] AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, [[maybe_unused]] AZStd::string_view path)
            {
                return resultCode;
            };
            context.PushReporter(silentReporter);

            bool atLeastOneSuccess = false;
            // Some types can be serialized into each other, e.g. 42 => true.
            // short-circuiting is forbidden here. (Don't write LoadAny() || LoadAny(), or atLeastOneSuccess || LoadAny())
            atLeastOneSuccess |= LoadAny<bool>(*materialPropertyValue, true, inputValue, context);
            atLeastOneSuccess |= LoadAny<int32_t>(*materialPropertyValue, 0, inputValue, context);
            atLeastOneSuccess |= LoadAny<uint32_t>(*materialPropertyValue, 0u, inputValue, context);
            atLeastOneSuccess |= LoadAny<float>(*materialPropertyValue, 0.0f, inputValue, context);
            atLeastOneSuccess |= LoadAny<AZStd::string>(*materialPropertyValue, AZStd::string(), inputValue, context);
            // Vectors/Colors can only be read from arrays or objects. If none of the basic types (+ string) are successfully loaded, the data should be an array.
            if (!atLeastOneSuccess)
            {
                atLeastOneSuccess |= LoadAny<Vector2>(*materialPropertyValue, Vector2(0.0f), inputValue, context);
                atLeastOneSuccess |= LoadAny<Vector3>(*materialPropertyValue, Vector3(0.0f), inputValue, context);
                atLeastOneSuccess |= LoadAny<Vector4>(*materialPropertyValue, Vector4(0.0f), inputValue, context);
                atLeastOneSuccess |= LoadAny<Color>(*materialPropertyValue, Color(0.0f), inputValue, context);
            }

            context.PopReporter();

            if (atLeastOneSuccess)
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully loaded property value.");
            }
            else
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "No possible supported data type match. See MaterialPropertyDataType");
            }
        }

        JsonSerializationResult::Result JsonMaterialPropertyValueSourceDataSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            AZ_Assert(azrtti_typeid<MaterialPropertyValueSourceData>() == valueTypeId,
                "Unable to serialize functor property value to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            const MaterialPropertyValueSourceData* materialPropertyValue = reinterpret_cast<const MaterialPropertyValueSourceData*>(inputValue);
            AZ_Assert(materialPropertyValue, "Input value for MaterialPropertyValueSourceData can't be null.");

            JSR::ResultCode result(JSR::Tasks::WriteValue);

            MaterialPropertyValue valueToDeserialize;
            // Use the resolved value first. If not applicable, use the first possible source value.
            if (materialPropertyValue->IsResolved())
            {
                valueToDeserialize = materialPropertyValue->m_resolvedValue;
            }
            else
            {
                for (const auto& possibleValue : materialPropertyValue->m_possibleValues)
                {
                    if (possibleValue.second.IsValid())
                    {
                        valueToDeserialize = possibleValue.second;
                    }
                }
            }

            if (valueToDeserialize.Is<bool>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<bool>(), nullptr, azrtti_typeid<bool>(), context));
            }
            else if (valueToDeserialize.Is<int32_t>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<int32_t>(), nullptr, azrtti_typeid<int32_t>(), context));
            }
            else if (valueToDeserialize.Is<uint32_t>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<uint32_t>(), nullptr, azrtti_typeid<uint32_t>(), context));
            }
            else if (valueToDeserialize.Is<float>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<float>(), nullptr, azrtti_typeid<float>(), context));
            }
            else if (valueToDeserialize.Is<Vector2>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<Vector2>(), nullptr, azrtti_typeid<Vector2>(), context));
            }
            else if (valueToDeserialize.Is<Vector3>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<Vector3>(), nullptr, azrtti_typeid<Vector3>(), context));
            }
            else if (valueToDeserialize.Is<Vector4>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<Vector4>(), nullptr, azrtti_typeid<Vector4>(), context));
            }
            else if (valueToDeserialize.Is<Color>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<Color>(), nullptr, azrtti_typeid<Color>(), context));
            }
            else if (valueToDeserialize.Is<AZStd::string>())
            {
                result.Combine(ContinueStoring(outputValue, &valueToDeserialize.GetValue<AZStd::string>(), nullptr, azrtti_typeid<AZStd::string>(), context));
            }
            else
            {
                ContinueStoring(outputValue, 0, nullptr, azrtti_typeid<int32_t>(), context);
                result.Combine(JSR::ResultCode(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed));
            }

            if (result.GetProcessing() == JSR::Processing::Completed)
            {
                return context.Report(result, "Successfully stored property value.");
            }
            else
            {
                return context.Report(result, "Partially stored property value.");
            }
        }
    } // namespace RPI
} // namespace AZ
