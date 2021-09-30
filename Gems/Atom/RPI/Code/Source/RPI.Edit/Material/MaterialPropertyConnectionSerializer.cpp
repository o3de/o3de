/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyConnectionSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace RPI
    {
        namespace JsonMaterialPropertyConnectionSerializerInternal
        {
            namespace Field
            {
                static constexpr const char type[] = "type";
                static constexpr const char name[] = "name";
                static constexpr const char id[] = "id"; // For backward compatibility
                static constexpr const char shaderIndex[] = "shaderIndex";
            }

            static const AZStd::string_view AcceptedFields[] =
            {
                Field::type,
                Field::name,
                Field::id,
                Field::shaderIndex
            };
        }

        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialPropertyConnectionSerializer, SystemAllocator, 0);

        JsonSerializationResult::Result JsonMaterialPropertyConnectionSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;
            using namespace JsonMaterialPropertyConnectionSerializerInternal;

            AZ_Assert(azrtti_typeid<MaterialTypeSourceData::PropertyConnection>() == outputValueTypeId,
                "Unable to deserialize material property connection to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialTypeSourceData::PropertyConnection* propertyConnection = reinterpret_cast<MaterialTypeSourceData::PropertyConnection*>(outputValue);
            AZ_Assert(propertyConnection, "Output value for JsonMaterialPropertyConnectionSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            if (!inputValue.IsObject())
            {
                return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, "Property connection must be a JSON object.");
            }
            
            MaterialUtils::CheckForUnrecognizedJsonFields(AcceptedFields, AZ_ARRAY_SIZE(AcceptedFields), inputValue, context, result);

            result.Combine(ContinueLoadingFromJsonObjectField(&propertyConnection->m_type, azrtti_typeid<MaterialPropertyOutputType>(), inputValue, Field::type, context));
            
            JsonSerializationResult::ResultCode nameResult = ContinueLoadingFromJsonObjectField(&propertyConnection->m_fieldName, azrtti_typeid<AZStd::string>(), inputValue, Field::name, context);
            if (nameResult.GetOutcome() == JsonSerializationResult::Outcomes::DefaultsUsed)
            {
                // This "id" key is for backward compatibility.
                result.Combine(ContinueLoadingFromJsonObjectField(&propertyConnection->m_fieldName, azrtti_typeid<AZStd::string>(), inputValue, Field::id, context));
            }
            else
            {
                result.Combine(nameResult);
            }

            result.Combine(ContinueLoadingFromJsonObjectField(&propertyConnection->m_shaderIndex, azrtti_typeid<int32_t>(), inputValue, Field::shaderIndex, context));

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully loaded property connection.");
            }
            else
            {
                return context.Report(result, "Partially loaded property connection.");
            }
        }
        

        JsonSerializationResult::Result JsonMaterialPropertyConnectionSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;
            using namespace JsonMaterialPropertyConnectionSerializerInternal;

            AZ_Assert(azrtti_typeid<MaterialTypeSourceData::PropertyConnection>() == valueTypeId,
                "Unable to serialize material property connection to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            const MaterialTypeSourceData::PropertyConnection* propertyConnection = reinterpret_cast<const MaterialTypeSourceData::PropertyConnection*>(inputValue);
            AZ_Assert(propertyConnection, "Input value for JsonMaterialPropertyConnectionSerializer can't be null.");
            
            JSR::ResultCode result(JSR::Tasks::WriteValue);

            outputValue.SetObject();

            MaterialTypeSourceData::PropertyConnection defaultConnection;

            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::type, &propertyConnection->m_type, &defaultConnection.m_type, azrtti_typeid<MaterialPropertyOutputType>(), context));
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::name, &propertyConnection->m_fieldName, &defaultConnection.m_fieldName, azrtti_typeid<AZStd::string>(), context));
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::shaderIndex, &propertyConnection->m_shaderIndex, &defaultConnection.m_shaderIndex, azrtti_typeid<int32_t>(), context));

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully stored property connection.");
            }
            else
            {
                return context.Report(result, "Partially stored property connection.");
            }
        }

    } // namespace RPI
} // namespace AZ
