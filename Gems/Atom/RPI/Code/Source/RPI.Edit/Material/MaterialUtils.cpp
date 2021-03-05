/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
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

            template<typename T>
            bool ComparePropertyValues(const AZStd::any& valueA, const AZStd::any& valueB)
            {
                return valueA.is<T>() && valueB.is<T>() && *AZStd::any_cast<T>(&valueA) == *AZStd::any_cast<T>(&valueB);
            }

            bool ArePropertyValuesEqual(const AZStd::any& valueA, const AZStd::any& valueB)
            {
                if (valueA.type() != valueB.type())
                {
                    return false;
                }

                if (ComparePropertyValues<bool>(valueA, valueB) ||
                    ComparePropertyValues<int32_t>(valueA, valueB) ||
                    ComparePropertyValues<uint32_t>(valueA, valueB) ||
                    ComparePropertyValues<float>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Vector2>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Vector3>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Vector4>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Color>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Data::AssetId>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Data::Asset<AZ::RPI::ImageAsset>>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Data::Asset<AZ::RPI::MaterialAsset>>(valueA, valueB) ||
                    ComparePropertyValues<AZ::Data::Asset<AZ::RPI::MaterialTypeAsset>>(valueA, valueB) ||
                    ComparePropertyValues<AZStd::string>(valueA, valueB))
                {
                    return true;
                }

                return false;
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
