/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPropertyGroupSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace RPI
    {
        namespace JsonMaterialPropertyGroupSerializerInternal
        {
            namespace Field
            {
                static constexpr const char name[] = "name";
                static constexpr const char id[] = "id"; // For backward compatibility
                static constexpr const char displayName[] = "displayName";
                static constexpr const char description[] = "description";
            }

            static const AZStd::string_view AcceptedFields[] =
            {
                Field::name,
                Field::id,
                Field::displayName,
                Field::description
            };
        }

        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialPropertyGroupSerializer, SystemAllocator);

        JsonSerializationResult::Result JsonMaterialPropertyGroupSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;
            using namespace JsonMaterialPropertyGroupSerializerInternal;

            AZ_Assert(azrtti_typeid<MaterialTypeSourceData::GroupDefinition>() == outputValueTypeId,
                "Unable to deserialize material property group to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialTypeSourceData::GroupDefinition* propertyGroup = reinterpret_cast<MaterialTypeSourceData::GroupDefinition*>(outputValue);
            AZ_Assert(propertyGroup, "Output value for JsonMaterialPropertyGroupSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            if (!inputValue.IsObject())
            {
                return context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Unsupported, "Property group must be a JSON object.");
            }
            
            MaterialUtils::CheckForUnrecognizedJsonFields(AcceptedFields, AZ_ARRAY_SIZE(AcceptedFields), inputValue, context, result);
                        
            JsonSerializationResult::ResultCode nameResult = ContinueLoadingFromJsonObjectField(&propertyGroup->m_name, azrtti_typeid<AZStd::string>(), inputValue, Field::name, context);
            if (nameResult.GetOutcome() == JsonSerializationResult::Outcomes::DefaultsUsed)
            {
                // This "id" key is for backward compatibility.
                result.Combine(ContinueLoadingFromJsonObjectField(&propertyGroup->m_name, azrtti_typeid<AZStd::string>(), inputValue, Field::id, context));
            }
            else
            {
                result.Combine(nameResult);
            }
            
            result.Combine(ContinueLoadingFromJsonObjectField(&propertyGroup->m_displayName, azrtti_typeid<AZStd::string>(), inputValue, Field::displayName, context));
            result.Combine(ContinueLoadingFromJsonObjectField(&propertyGroup->m_description, azrtti_typeid<AZStd::string>(), inputValue, Field::description, context));

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully loaded property group.");
            }
            else
            {
                return context.Report(result, "Partially loaded property group.");
            }
        }
        

        JsonSerializationResult::Result JsonMaterialPropertyGroupSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;
            using namespace JsonMaterialPropertyGroupSerializerInternal;

            AZ_Assert(azrtti_typeid<MaterialTypeSourceData::GroupDefinition>() == valueTypeId,
                "Unable to serialize material property group to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            const MaterialTypeSourceData::GroupDefinition* propertyGroup = reinterpret_cast<const MaterialTypeSourceData::GroupDefinition*>(inputValue);
            AZ_Assert(propertyGroup, "Input value for JsonMaterialPropertyGroupSerializer can't be null.");
            
            JSR::ResultCode result(JSR::Tasks::WriteValue);

            outputValue.SetObject();

            AZStd::string defaultEmpty;

            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::name, &propertyGroup->m_name, &defaultEmpty, azrtti_typeid<AZStd::string>(), context));
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::displayName, &propertyGroup->m_displayName, &defaultEmpty, azrtti_typeid<AZStd::string>(), context));
            result.Combine(ContinueStoringToJsonObjectField(outputValue, Field::description, &propertyGroup->m_description, &defaultEmpty, azrtti_typeid<AZStd::string>(), context));

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully stored property group.");
            }
            else
            {
                return context.Report(result, "Partially stored property group.");
            }
        }

    } // namespace RPI
} // namespace AZ
