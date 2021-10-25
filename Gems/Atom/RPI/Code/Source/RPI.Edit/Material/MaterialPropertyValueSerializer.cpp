/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyValueSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialPropertySerializer.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AZ
{
    namespace RPI
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialPropertyValueSerializer, SystemAllocator, 0);

        template<typename T>
        JsonSerializationResult::ResultCode JsonMaterialPropertyValueSerializer::LoadVariant(
            MaterialPropertyValue& intoValue,
            const T& defaultValue,
            const rapidjson::Value& inputValue,
            JsonDeserializerContext& context)
        {
            T value = defaultValue;
            JsonSerializationResult::ResultCode result = ContinueLoading(&value, azrtti_typeid<T>(), inputValue, context);
            intoValue = value;
            return result;
        }

        JsonSerializationResult::Result JsonMaterialPropertyValueSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialSourceData::Property>() == outputValueTypeId,
                "Unable to deserialize material property value to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialSourceData::Property* property = reinterpret_cast<MaterialSourceData::Property*>(outputValue);
            AZ_Assert(property, "Output value for JsonMaterialPropertyValueSerializer can't be null.");

            const MaterialTypeSourceData* materialType = context.GetMetadata().Find<MaterialTypeSourceData>();
            if (!materialType)
            {
                AZ_Assert(false, "Material type reference not found");
                return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Catastrophic, "Material type reference not found.");
            }

            const JsonMaterialPropertyValueSerializer::LoadContext* loadContext = context.GetMetadata().Find<JsonMaterialPropertyValueSerializer::LoadContext>();

            // Construct the full property name (groupName.propertyName) by parsing it from the JSON path string.
            size_t startPropertyName = context.GetPath().Get().rfind('/');
            size_t startGroupName = context.GetPath().Get().rfind('/', startPropertyName-1);
            AZStd::string_view groupName = context.GetPath().Get().substr(startGroupName + 1, startPropertyName - startGroupName - 1);
            AZStd::string_view propertyName = context.GetPath().Get().substr(startPropertyName + 1);

            JSR::ResultCode result(JSR::Tasks::ReadField);

            auto propertyDefinition = materialType->FindProperty(groupName, propertyName, loadContext->m_materialTypeVersion);
            if (!propertyDefinition)
            {
                AZStd::string message = AZStd::string::format("Property '%.*s.%.*s' not found in material type.", AZ_STRING_ARG(groupName), AZ_STRING_ARG(propertyName));
                return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, message);
            }
            else
            {
                switch (propertyDefinition->m_dataType)
                {
                case MaterialPropertyDataType::Bool:
                    result.Combine(LoadVariant<bool>(property->m_value, false, inputValue, context));
                    break;
                case MaterialPropertyDataType::Int:
                    result.Combine(LoadVariant<int32_t>(property->m_value, 0, inputValue, context));
                    break;
                case MaterialPropertyDataType::UInt:
                    result.Combine(LoadVariant<uint32_t>(property->m_value, 0u, inputValue, context));
                    break;
                case MaterialPropertyDataType::Float:
                    result.Combine(LoadVariant<float>(property->m_value, 0.0f, inputValue, context));
                    break;
                case MaterialPropertyDataType::Vector2:
                    result.Combine(LoadVariant<Vector2>(property->m_value, Vector2{0.0f, 0.0f}, inputValue, context));
                    break;
                case MaterialPropertyDataType::Vector3:
                    result.Combine(LoadVariant<Vector3>(property->m_value, Vector3{0.0f, 0.0f, 0.0f}, inputValue, context));
                    break;
                case MaterialPropertyDataType::Vector4:
                    result.Combine(LoadVariant<Vector4>(property->m_value, Vector4{0.0f, 0.0f, 0.0f, 0.0f}, inputValue, context));
                    break;
                case MaterialPropertyDataType::Color:
                    result.Combine(LoadVariant<Color>(property->m_value, AZ::Colors::White, inputValue, context));
                    break;
                case MaterialPropertyDataType::Image:
                case MaterialPropertyDataType::Enum:
                    result.Combine(LoadVariant<AZStd::string>(property->m_value, AZStd::string{}, inputValue, context));
                    break;
                default:
                    return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, "Unknown data type");
                }
            }

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully loaded property value.");
            }
            else
            {
                return context.Report(result, "Partially loaded property value.");
            }
        }

        JsonSerializationResult::Result JsonMaterialPropertyValueSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialSourceData::Property>() == valueTypeId,
                "Unable to serialize material property value to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            const MaterialSourceData::Property* property = reinterpret_cast<const MaterialSourceData::Property*>(inputValue);
            AZ_Assert(property, "Input value for JsonMaterialPropertyValueSerializer can't be null.");
            
            JSR::ResultCode result(JSR::Tasks::WriteValue);

            if (property->m_value.Is<bool>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<bool>(), nullptr, azrtti_typeid<bool>(), context));
            }
            else if (property->m_value.Is<int32_t>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<int32_t>(), nullptr, azrtti_typeid<int32_t>(), context));
            }
            else if (property->m_value.Is<uint32_t>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<uint32_t>(), nullptr, azrtti_typeid<uint32_t>(), context));
            }
            else if (property->m_value.Is<float>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<float>(), nullptr, azrtti_typeid<float>(), context));
            }
            else if (property->m_value.Is<Vector2>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<Vector2>(), nullptr, azrtti_typeid<Vector2>(), context));
            }
            else if (property->m_value.Is<Vector3>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<Vector3>(), nullptr, azrtti_typeid<Vector3>(), context));
            }
            else if (property->m_value.Is<Vector4>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<Vector4>(), nullptr, azrtti_typeid<Vector4>(), context));
            }
            else if (property->m_value.Is<Color>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<Color>(), nullptr, azrtti_typeid<Color>(), context));
            }
            else if (property->m_value.Is<AZStd::string>())
            {
                result.Combine(ContinueStoring(outputValue, &property->m_value.GetValue<AZStd::string>(), nullptr, azrtti_typeid<AZStd::string>(), context));
            }

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
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
