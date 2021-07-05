/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RPI
    {
        namespace MaterialUtils
        {
            Outcome<Data::Asset<ImageAsset>> GetImageAssetReference(AZStd::string_view materialSourceFilePath, const AZStd::string imageFilePath)
            {
                if (imageFilePath.empty())
                {
                    // The image value was present but specified an empty string, meaning the texture asset should be explicitly cleared.
                    return AZ::Success(Data::Asset<ImageAsset>());
                }
                else
                {
                    Outcome<Data::AssetId> imageAssetId = AssetUtils::MakeAssetId(materialSourceFilePath, imageFilePath, StreamingImageAsset::GetImageAssetSubId());
                    if (!imageAssetId.IsSuccess())
                    {
                        return AZ::Failure();
                    }
                    else
                    {
                        Data::Asset<ImageAsset> unloadedImageAssetReference(imageAssetId.GetValue(), azrtti_typeid<StreamingImageAsset>(), imageFilePath);
                        return AZ::Success(unloadedImageAssetReference);
                    }
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
                    loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(filePath);
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
        }
    }
}
