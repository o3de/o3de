/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceDataSerializer.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSerializer.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    namespace RPI
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonMaterialSourceDataSerializer, SystemAllocator, 0);
        
        JsonSerializationResult::Result JsonMaterialSourceDataSerializer::Load(void* outputValue, const Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, JsonDeserializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialSourceData>() == outputValueTypeId,
                "Unable to deserialize material to json because the provided type is %s",
                outputValueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(outputValueTypeId);

            MaterialSourceData* materialSourceData = reinterpret_cast<MaterialSourceData*>(outputValue);
            AZ_Assert(materialSourceData, "Output value for JsonMaterialSourceDataSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);

            if (!inputValue.IsObject())
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported, "Material data must be a JSON object");
            }

            result.Combine(ContinueLoadingFromJsonObjectField(&materialSourceData->m_description, azrtti_typeid<AZStd::string>(), inputValue, "description", context));
            result.Combine(ContinueLoadingFromJsonObjectField(&materialSourceData->m_parentMaterial, azrtti_typeid<AZStd::string>(), inputValue, "parentMaterial", context));
            result.Combine(ContinueLoadingFromJsonObjectField(&materialSourceData->m_materialType, azrtti_typeid<AZStd::string>(), inputValue, "materialType", context));
            result.Combine(ContinueLoadingFromJsonObjectField(&materialSourceData->m_materialTypeVersion, azrtti_typeid<uint32_t>(), inputValue, "materialTypeVersion", context));

            if (materialSourceData->m_materialType.empty())
            {
                return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "Required field 'materialType' is missing or invalid");
            }

            JsonFileLoadContext* jsonFileLoadContext = context.GetMetadata().Find<JsonFileLoadContext>();

            if (!jsonFileLoadContext)
            {
                // Go ahead and create a JsonFileLoadContext because we'll need to use it below when loading the material type
                context.GetMetadata().Add(JsonFileLoadContext{});
                jsonFileLoadContext = context.GetMetadata().Find<JsonFileLoadContext>();
            }

            // Load the material type file because we need the property type information in order to know how to read the property values
            MaterialTypeSourceData materialTypeData;
            {
                AZStd::string materialTypePath = AssetUtils::ResolvePathReference(jsonFileLoadContext->GetFilePath(), materialSourceData->m_materialType);

                auto materialTypeJson = JsonSerializationUtils::ReadJsonFile(materialTypePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                if (!materialTypeJson.IsSuccess())
                {
                    AZStd::string failureMessage;
                    failureMessage = AZStd::string::format("Failed to load material-type file '%s': %s", materialTypePath.c_str(), materialTypeJson.GetError().c_str());
                    ScopedContextPath subPath{context, "materialType"};
                    return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, failureMessage);
                }
                else
                {
                    // Since we're about to load a different file the JsonFileLoadContext needs to be changed to reflect the file that's being loaded.
                    jsonFileLoadContext->PushFilePath(materialTypePath);

                    // We also need a special reporting function for the material type, to note the fact that the issue is in the material type not this file.
                    auto reportingPrev = context.GetReporter();
                    context.PushReporter([materialTypePath, reportingPrev](AZStd::string_view message, JSR::ResultCode result, AZStd::string_view path) -> JSR::ResultCode
                    {
                        AZStd::string materialTypeFilename;
                        if (!AzFramework::StringFunc::Path::GetFullFileName(materialTypePath.c_str(), materialTypeFilename))
                        {
                            materialTypeFilename = materialTypePath;
                        }

                        AZStd::string newPath = AZStd::string::format("[%.*s]%.*s", AZ_STRING_ARG(materialTypeFilename), AZ_STRING_ARG(path));
                        return reportingPrev(message, result, newPath);
                    });

                    JsonDeserializerSettings settings;
                    settings.m_metadata = context.GetMetadata();
                    settings.m_reporting = context.GetReporter();
                    settings.m_registrationContext = context.GetRegistrationContext();
                    settings.m_serializeContext = context.GetSerializeContext();
                    settings.m_clearContainers = context.ShouldClearContainers();

                    JsonSerializationResult::ResultCode materialTypeLoadResult = JsonSerialization::Load(materialTypeData, materialTypeJson.GetValue(), settings);
                    materialTypeData.ResolveUvEnums();

                    // Restore prior configuration
                    context.PopReporter();
                    jsonFileLoadContext->PopFilePath();

                    // Even though results from the material type file is a separate JSON serialization, we combine the results to make sure
                    // any issues are bubbled up. I'm not sure if this is the most desirable approach, but better to over-report issues than
                    // under-report them.
                    result.Combine(materialTypeLoadResult);
                }
            }

            context.GetMetadata().Add(AZStd::move(materialTypeData));

            JsonMaterialPropertyValueSerializer::LoadContext materialPropertyValueLoadContext;
            materialPropertyValueLoadContext.m_materialTypeVersion = materialSourceData->m_materialTypeVersion;
            context.GetMetadata().Add(materialPropertyValueLoadContext);

            result.Combine(ContinueLoadingFromJsonObjectField(&materialSourceData->m_properties, azrtti_typeid<MaterialSourceData::PropertyGroupMap>(), inputValue, "properties", context));

            if (result.GetProcessing() == JsonSerializationResult::Processing::Completed)
            {
                return context.Report(result, "Successfully loaded material.");
            }
            else
            {
                return context.Report(result, "Partially loaded material.");
            }
        }


        JsonSerializationResult::Result JsonMaterialSourceDataSerializer::Store(rapidjson::Value& outputValue, const void* inputValue,
            [[maybe_unused]] const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
        {
            namespace JSR = JsonSerializationResult;

            AZ_Assert(azrtti_typeid<MaterialSourceData>() == valueTypeId,
                "Unable to serialize material to json because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());
            AZ_UNUSED(valueTypeId);

            const MaterialSourceData* materialSourceData = reinterpret_cast<const MaterialSourceData*>(inputValue);
            AZ_Assert(materialSourceData, "Input value for JsonMaterialSourceDataSerializer can't be null.");

            JSR::ResultCode resultCode(JSR::Tasks::ReadField);
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "description",  &materialSourceData->m_description, nullptr, azrtti_typeid<AZStd::string>(), context));
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "parentMaterial", &materialSourceData->m_parentMaterial, nullptr, azrtti_typeid<AZStd::string>(), context));
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "materialType", &materialSourceData->m_materialType, nullptr, azrtti_typeid<AZStd::string>(), context));
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "materialTypeVersion", &materialSourceData->m_materialTypeVersion, nullptr, azrtti_typeid<uint32_t>(), context));
            resultCode.Combine(ContinueStoringToJsonObjectField(outputValue, "properties", &materialSourceData->m_properties, nullptr, azrtti_typeid<MaterialSourceData::PropertyGroupMap>(), context));

            return context.Report(resultCode, "Processed material.");
        }
    } // namespace RPI
} // namespace AZ
