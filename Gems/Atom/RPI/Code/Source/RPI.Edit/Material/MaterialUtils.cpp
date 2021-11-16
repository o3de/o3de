/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>

#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        namespace MaterialUtils
        {
            GetImageAssetResult GetImageAssetReference(Data::Asset<ImageAsset>& imageAsset, AZStd::string_view materialSourceFilePath, const AZStd::string imageFilePath)
            {
                imageAsset = {};

                if (imageFilePath.empty())
                {
                    // The image value was present but specified an empty string, meaning the texture asset should be explicitly cleared.
                    return GetImageAssetResult::Empty;
                }
                else
                {
                    // We use TraceLevel::None because fallback textures are available and we'll return GetImageAssetResult::Missing below in that case.
                    // Callers of GetImageAssetReference will be responsible for logging warnings or errors as needed.

                    Outcome<Data::AssetId> imageAssetId = AssetUtils::MakeAssetId(materialSourceFilePath, imageFilePath, StreamingImageAsset::GetImageAssetSubId(), AssetUtils::TraceLevel::None);
                    
                    if (!imageAssetId.IsSuccess())
                    {
                        // When the AssetId cannot be found, we don't want to outright fail, because the runtime has mechanisms for displaying fallback textures which gives the
                        // user a better recovery workflow. On the other hand we can't just provide an empty/invalid Asset<ImageAsset> because that would be interpreted as simply
                        // no value was present and result in using no texture, and this would amount to a silent failure.
                        // So we use a randomly generated (well except for the "BADA55E7" bit ;) UUID which the runtime and tools will interpret as a missing asset and represent
                        // it as such.
                        static const Uuid InvalidAssetPlaceholderId = "{BADA55E7-1A1D-4940-B655-9D08679BD62F}";
                        imageAsset = Data::Asset<ImageAsset>{InvalidAssetPlaceholderId, azrtti_typeid<StreamingImageAsset>(), imageFilePath};
                        return GetImageAssetResult::Missing;
                    }
                    
                    imageAsset = Data::Asset<ImageAsset>{imageAssetId.GetValue(), azrtti_typeid<StreamingImageAsset>(), imageFilePath};
                    return GetImageAssetResult::Found;
                }
            }

            bool ResolveMaterialPropertyEnumValue(const MaterialPropertyDescriptor* propertyDescriptor, const AZ::Name& enumName, MaterialPropertyValue& outResolvedValue)
            {
                uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                {
                    AZ_Error("Material", false, "Enum name \"%s\" can't be found in property \"%s\".", enumName.GetCStr(), propertyDescriptor->GetName().GetCStr());
                    return false;
                }
                outResolvedValue = enumValue;
                return true;
            }

            AZ::Outcome<MaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, const rapidjson::Value* document)
            {
                AZ::Outcome<rapidjson::Document, AZStd::string> loadOutcome;
                if (document == nullptr)
                {
                    loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(filePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                    if (!loadOutcome.IsSuccess())
                    {
                        AZ_Error("AZ::RPI::JsonUtils", false, "%s", loadOutcome.GetError().c_str());
                        return AZ::Failure();
                    }

                    document = &loadOutcome.GetValue();
                }

                MaterialTypeSourceData materialType;

                JsonDeserializerSettings settings;

                JsonReportingHelper reportingHelper;
                reportingHelper.Attach(settings);

                // This is required by some custom material serializers to support relative path references.
                JsonFileLoadContext fileLoadContext;
                fileLoadContext.PushFilePath(filePath);
                settings.m_metadata.Add(fileLoadContext);

                JsonSerialization::Load(materialType, *document, settings);
                materialType.ResolveUvEnums();

                if (reportingHelper.ErrorsReported())
                {
                    return AZ::Failure();
                }
                else
                {
                    return AZ::Success(AZStd::move(materialType));
                }
            }

            void CheckForUnrecognizedJsonFields(const AZStd::string_view* acceptedFieldNames, uint32_t acceptedFieldNameCount, const rapidjson::Value& object, JsonDeserializerContext& context, JsonSerializationResult::ResultCode &result)
            {
                for (auto iter = object.MemberBegin(); iter != object.MemberEnd(); ++iter)
                {
                    bool matched = false;

                    for (uint32_t i = 0; i < acceptedFieldNameCount; ++i)
                    {
                        if (iter->name.GetString() == acceptedFieldNames[i])
                        {
                            matched = true;
                            break;
                        }
                    }

                    if (!matched)
                    {
                        ScopedContextPath subPath{context, iter->name.GetString()};
                        result.Combine(context.Report(JsonSerializationResult::Tasks::ReadField, JsonSerializationResult::Outcomes::Skipped, "Skipping unrecognized field"));
                    }
                }
            }
        }
    }
}
