/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <Material/MaterialAssignmentSerializer.h>
#include <Atom/Feature/Material/MaterialAssignment.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialAssignmentSerializer, AZ::SystemAllocator, 0);

        JsonSerializationResult::Result JsonMaterialAssignmentSerializer::Load(
            void* outputValue, [[maybe_unused]] const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
            JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<AZ::Render::MaterialAssignment>() == outputValueTypeId,
                "Unable to deserialize MaterialAssignment from json because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            AZ::Render::MaterialAssignment* materialAssignment = reinterpret_cast<AZ::Render::MaterialAssignment*>(outputValue);
            AZ_Assert(materialAssignment, "Output value for JsonMaterialAssignmentSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                result.Combine(ContinueLoadingFromJsonObjectField(
                    &materialAssignment->m_materialAsset, azrtti_typeid<decltype(materialAssignment->m_materialAsset)>(), inputValue,
                    "MaterialAsset", context));
            }

            if (inputValue.HasMember("PropertyOverrides") && inputValue["PropertyOverrides"].IsObject())
            {
                // Attempt to load material property override values for a subset of types
                for (const auto& inputPropertyPair : inputValue["PropertyOverrides"].GetObject())
                {
                    const AZ::Name propertyName(inputPropertyPair.name.GetString());
                    if (!propertyName.IsEmpty())
                    {
                        AZStd::any propertyValue;
                        if (AttemptLoadAny(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Data::Asset<AZ::RPI::ImageAsset>>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(propertyValue, inputPropertyPair.value, context, result))
                        {
                            materialAssignment->m_propertyOverrides[propertyName] = propertyValue;
                        }
                    }
                }
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Succesfully loaded MaterialAssignment information."
                                                                  : "Failed to load MaterialAssignment information.");
        }

        JsonSerializationResult::Result JsonMaterialAssignmentSerializer::Store(
            rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue, [[maybe_unused]] const Uuid& valueTypeId,
            JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<AZ::Render::MaterialAssignment>() == valueTypeId,
                "Unable to Serialize MaterialAssignment because the provided type is %s.", valueTypeId.ToString<AZStd::string>().c_str());

            const AZ::Render::MaterialAssignment* materialAssignment = reinterpret_cast<const AZ::Render::MaterialAssignment*>(inputValue);
            AZ_Assert(materialAssignment, "Input value for JsonMaterialAssignmentSerializer can't be null.");
            const AZ::Render::MaterialAssignment* defaultMaterialAssignmentInstance =
                reinterpret_cast<const AZ::Render::MaterialAssignment*>(defaultValue);

            outputValue.SetObject();

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                AZ::ScopedContextPath subPathMaterialAsset(context, "m_materialAsset");
                const AZ::Data::Asset<RPI::MaterialAsset>* materialAsset = &materialAssignment->m_materialAsset;
                const AZ::Data::Asset<RPI::MaterialAsset>* defaultmaterialAsset =
                    defaultMaterialAssignmentInstance ? &defaultMaterialAssignmentInstance->m_materialAsset : nullptr;

                result.Combine(ContinueStoringToJsonObjectField(
                    outputValue, "MaterialAsset", materialAsset, defaultmaterialAsset,
                    azrtti_typeid<decltype(materialAssignment->m_materialAsset)>(), context));
            }

            {
                AZ::ScopedContextPath subPathPropertyOverrides(context, "m_propertyOverrides");
                if (!materialAssignment->m_propertyOverrides.empty())
                {
                    rapidjson::Value outputPropertyValueContainer;
                    outputPropertyValueContainer.SetObject();

                    // Attempt to extract and store material property override values for a subset of types
                    for (const auto& propertyPair : materialAssignment->m_propertyOverrides)
                    {
                        const AZ::Name& propertyName = propertyPair.first;
                        const AZStd::any& propertyValue = propertyPair.second;
                        if (!propertyName.IsEmpty() && !propertyValue.empty())
                        {
                            rapidjson::Value outputPropertyValue;
                            if (AttemptStoreAny(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Data::Asset<AZ::RPI::ImageAsset>>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(
                                    propertyValue, outputPropertyValue, context, result))
                            {
                                outputPropertyValueContainer.AddMember(
                                    rapidjson::Value::StringRefType(propertyName.GetCStr()), outputPropertyValue,
                                    context.GetJsonAllocator());
                            }
                        }
                    }

                    if (outputPropertyValueContainer.MemberCount() > 0)
                    {
                        outputValue.AddMember("PropertyOverrides", outputPropertyValueContainer, context.GetJsonAllocator());
                    }
                }
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully stored MaterialAssignment information."
                                                                  : "Failed to store MaterialAssignment information.");
        }
    } // namespace Render
} // namespace AZ
