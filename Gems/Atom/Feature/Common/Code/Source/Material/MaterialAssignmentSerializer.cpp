/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/MaterialAssignmentSerializer.h>
#include <Atom/Feature/Material/MaterialAssignment.h>

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>

namespace AZ
{
    namespace Render
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialAssignmentSerializer, AZ::SystemAllocator, 0);

        JsonSerializationResult::Result JsonMaterialAssignmentSerializer::Load(
            void* outputValue,
            [[maybe_unused]] const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
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

            if (inputValue.HasMember("ModelUvOverrides"))
            {
                AZStd::unordered_map<AZStd::string, AZStd::string> uvOverrideMap;
                result.Combine(ContinueLoadingFromJsonObjectField(
                    &uvOverrideMap, azrtti_typeid<decltype(uvOverrideMap)>(), inputValue, "ModelUvOverrides", context));

                materialAssignment->m_matModUvOverrides.clear();
                for (const auto& uvOverride : uvOverrideMap)
                {
                    const AZ::RHI::ShaderSemantic semantic(AZ::RHI::ShaderSemantic::Parse(uvOverride.first));
                    materialAssignment->m_matModUvOverrides[semantic] = AZ::Name(uvOverride.second);
                }
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
                        if (LoadAny<bool>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::u8>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::u16>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::u32>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::u64>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::s8>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::s16>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::s32>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::s64>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<float>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<double>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Vector2>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Vector3>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Vector4>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Color>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZStd::string>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Data::AssetId>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Data::Asset<AZ::Data::AssetData>>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Data::Asset<AZ::RPI::ImageAsset>>(propertyValue, inputPropertyPair.value, context, result) ||
                            LoadAny<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>(propertyValue, inputPropertyPair.value, context, result) ||
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
            rapidjson::Value& outputValue,
            const void* inputValue,
            const void* defaultValue,
            [[maybe_unused]] const Uuid& valueTypeId,
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
                AZ::ScopedContextPath subPathPropertyOverrides(context, "m_matModUvOverrides");
                if (!materialAssignment->m_matModUvOverrides.empty())
                {
                    // Convert the model material UV overrides to a map of strings for simple serialization
                    AZStd::unordered_map<AZStd::string, AZStd::string> uvOverrideMap;
                    AZStd::unordered_map<AZStd::string, AZStd::string> uvOverrideMapDefault;
                    for (const auto& matModUvOverride : materialAssignment->m_matModUvOverrides)
                    {
                        uvOverrideMap[matModUvOverride.first.ToString()] = matModUvOverride.second.GetStringView();
                    }

                    result.Combine(ContinueStoringToJsonObjectField(
                        outputValue, "ModelUvOverrides", &uvOverrideMap, &uvOverrideMapDefault, azrtti_typeid<decltype(uvOverrideMap)>(),
                        context));
                }
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
                            if (StoreAny<bool>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::u8>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::u16>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::u32>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::u64>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::s8>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::s16>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::s32>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::s64>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<float>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<double>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Vector2>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Vector3>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Vector4>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Color>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZStd::string>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Data::AssetId>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Data::Asset<AZ::Data::AssetData>>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Data::Asset<AZ::RPI::ImageAsset>>(propertyValue, outputPropertyValue, context, result) ||
                                StoreAny<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>(propertyValue, outputPropertyValue, context, result) ||
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

        template<typename T>
        bool JsonMaterialAssignmentSerializer::LoadAny(
            AZStd::any& propertyValue,
            const rapidjson::Value& inputPropertyValue,
            AZ::JsonDeserializerContext& context,
            AZ::JsonSerializationResult::ResultCode& result)
        {
            if (inputPropertyValue.IsObject() && inputPropertyValue.HasMember("Value") && inputPropertyValue.HasMember("$type"))
            {
                // Requiring explicit type info to differentiate between colors versus vectors and numeric types
                const AZ::Uuid baseTypeId = azrtti_typeid<T>();
                AZ::Uuid typeId = AZ::Uuid::CreateNull();
                result.Combine(LoadTypeId(typeId, inputPropertyValue, context, &baseTypeId));

                if (typeId == azrtti_typeid<T>())
                {
                    T value = {};
                    result.Combine(ContinueLoadingFromJsonObjectField(&value, azrtti_typeid<T>(), inputPropertyValue, "Value", context));
                    propertyValue = value;
                    return true;
                }
            }
            return false;
        }

        template<typename T>
        bool JsonMaterialAssignmentSerializer::StoreAny(
            const AZStd::any& propertyValue,
            rapidjson::Value& outputPropertyValue,
            AZ::JsonSerializerContext& context,
            AZ::JsonSerializationResult::ResultCode& result)
        {
            if (propertyValue.is<T>())
            {
                outputPropertyValue.SetObject();

                // Storing explicit type info to differentiate between colors versus vectors and numeric types
                rapidjson::Value typeValue;
                result.Combine(StoreTypeId(typeValue, azrtti_typeid<T>(), context));
                outputPropertyValue.AddMember("$type", typeValue, context.GetJsonAllocator());

                const T& value = AZStd::any_cast<T>(propertyValue);
                result.Combine(
                    ContinueStoringToJsonObjectField(outputPropertyValue, "Value", &value, nullptr, azrtti_typeid<T>(), context));
                return true;
            }
            return false;
        }
    } // namespace Render
} // namespace AZ

