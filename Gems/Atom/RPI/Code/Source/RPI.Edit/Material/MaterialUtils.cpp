/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPipelineSourceData.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/Serialization/Json/JsonImporter.h>
#include <AzCore/Settings/SettingsRegistry.h>

#include <AzCore/Utils/Utils.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

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

                    uint32_t subId = 0;
                    auto typeId = azrtti_typeid<AttachmentImageAsset>();

                    bool isAttachmentImageAsset = imageFilePath.ends_with(AttachmentImageAsset::Extension);
                    if (!isAttachmentImageAsset)
                    {
                        subId = StreamingImageAsset::GetImageAssetSubId();
                        typeId = azrtti_typeid<StreamingImageAsset>();
                    }

                    Outcome<Data::AssetId> imageAssetId = AssetUtils::MakeAssetId(materialSourceFilePath, imageFilePath, subId, AssetUtils::TraceLevel::None);

                    if (!imageAssetId.IsSuccess())
                    {
                        // When the AssetId cannot be found, we don't want to outright fail, because the runtime has mechanisms for displaying fallback textures which gives the
                        // user a better recovery workflow. On the other hand we can't just provide an empty/invalid Asset<ImageAsset> because that would be interpreted as simply
                        // no value was present and result in using no texture, and this would amount to a silent failure.
                        // So we use a UUID that is clearly invalid, which the runtime and tools will interpret as a missing asset and represent
                        // it as such.
                        static constexpr Uuid InvalidAssetPlaceholderId(Uuid::CreateInvalid());
                        imageAsset = Data::Asset<ImageAsset>{InvalidAssetPlaceholderId, azrtti_typeid<StreamingImageAsset>(), imageFilePath};
                        AZ_Error("MaterialUtils", false, "Material at path %.*s could not resolve image %.*s, using invalid UUID %.*s. "
                            "To resolve this, verify the image exists at the relative path to a scan folder matching this reference. "
                            "Verify a portion of the scan folder is not in the relative path, which is a common cause of this issue.",
                            AZ_STRING_ARG(materialSourceFilePath),
                            AZ_STRING_ARG(imageFilePath),
                            AZ_STRING_ARG(InvalidAssetPlaceholderId.ToFixedString()));
                        return GetImageAssetResult::Missing;
                    }

                    imageAsset = Data::Asset<ImageAsset>{imageAssetId.GetValue(), typeId, imageFilePath};
                    imageAsset.SetAutoLoadBehavior(Data::AssetLoadBehavior::PreLoad);
                    return GetImageAssetResult::Found;
                }
            }

            bool ResolveMaterialPropertyEnumValue(const MaterialPropertyDescriptor* propertyDescriptor, const AZ::Name& enumName, MaterialPropertyValue& outResolvedValue)
            {
                uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                {
                    AZ_Error("MaterialUtils", false, "Enum name \"%s\" can't be found in property \"%s\".", enumName.GetCStr(), propertyDescriptor->GetName().GetCStr());
                    return false;
                }
                outResolvedValue = enumValue;
                return true;
            }

            template<typename SourceDataType>
            AZ::Outcome<SourceDataType> LoadJsonSourceDataWithImports(const AZStd::string& filePath, rapidjson::Document* document, ImportedJsonFiles* importedFiles)
            {
                rapidjson::Document localDocument;

                if (document == nullptr)
                {
                    AZ::Outcome<rapidjson::Document, AZStd::string> loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(filePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                    if (!loadOutcome.IsSuccess())
                    {
                        AZ_Error("MaterialUtils", false, "%s", loadOutcome.GetError().c_str());
                        return AZ::Failure();
                    }

                    localDocument = loadOutcome.TakeValue();
                    document = &localDocument;
                }

                AZ::BaseJsonImporter jsonImporter;
                AZ::JsonImportSettings importSettings;
                importSettings.m_importer = &jsonImporter;
                importSettings.m_loadedJsonPath = filePath;
                AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ResolveImports(document->GetObject(), document->GetAllocator(), importSettings);
                if (result.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
                {
                    AZ_Error("MaterialUtils", false, "%s", result.ToString(filePath).c_str());
                    return AZ::Failure();
                }

                if (importedFiles)
                {
                    *importedFiles = importSettings.m_importer->GetImportedFiles();
                }

                SourceDataType sourceData;

                JsonDeserializerSettings settings;

                JsonReportingHelper reportingHelper;
                reportingHelper.Attach(settings);

                JsonSerialization::Load(sourceData, *document, settings);

                if (reportingHelper.ErrorsReported())
                {
                    return AZ::Failure();
                }
                else
                {
                    return AZ::Success(AZStd::move(sourceData));
                }
            }

            AZ::Outcome<MaterialPipelineSourceData> LoadMaterialPipelineSourceData(const AZStd::string& filePath, rapidjson::Document* document, ImportedJsonFiles* importedFiles)
            {
                return LoadJsonSourceDataWithImports<MaterialPipelineSourceData>(filePath, document, importedFiles);
            }

            AZ::Outcome<MaterialTypeSourceData> LoadMaterialTypeSourceData(const AZStd::string& filePath, rapidjson::Document* document, ImportedJsonFiles* importedFiles)
            {
                AZ::Outcome<MaterialTypeSourceData> outcome = LoadJsonSourceDataWithImports<MaterialTypeSourceData>(filePath, document, importedFiles);
                if (outcome.IsSuccess())
                {
                    outcome.GetValue().UpgradeLegacyFormat();
                    outcome.GetValue().ResolveUvEnums();
                }
                return outcome;
            }

            AZ::Outcome<MaterialSourceData> LoadMaterialSourceData(const AZStd::string& filePath, const rapidjson::Value* document, bool warningsAsErrors)
            {
                AZ::Outcome<rapidjson::Document, AZStd::string> loadOutcome;
                if (document == nullptr)
                {
                    loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(filePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                    if (!loadOutcome.IsSuccess())
                    {
                        AZ_Error("MaterialUtils", false, "%s", loadOutcome.GetError().c_str());
                        return AZ::Failure();
                    }

                    document = &loadOutcome.GetValue();
                }

                MaterialSourceData material;

                JsonDeserializerSettings settings;

                JsonReportingHelper reportingHelper;
                reportingHelper.Attach(settings);

                JsonSerialization::Load(material, *document, settings);
                material.UpgradeLegacyFormat();

                if (reportingHelper.ErrorsReported())
                {
                    return AZ::Failure();
                }
                else if (warningsAsErrors && reportingHelper.WarningsReported())
                {
                    AZ_Error("MaterialUtils", false, "Warnings reported while loading '%s'", filePath.c_str());
                    return AZ::Failure();
                }
                else
                {
                    return AZ::Success(AZStd::move(material));
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

            bool LooksLikeImageFileReference(const MaterialPropertyValue& value)
            {
                // If the source value type is a string, there are two possible property types: Image and Enum. If there is a "." in
                // the string (for the extension) we can assume it's an Image file path.
                return value.Is<AZStd::string>() && AzFramework::StringFunc::Contains(value.GetValue<AZStd::string>(), ".");
            }

            bool IsValidName(AZStd::string_view name)
            {
                // Checks for a c++ style identifier
                return AZStd::regex_match(name.begin(), name.end(), AZStd::regex("^[a-zA-Z_][a-zA-Z0-9_]*$"));
            }

            bool IsValidName(const AZ::Name& name)
            {
                return IsValidName(name.GetStringView());
            }

            bool CheckIsValidName(AZStd::string_view name, [[maybe_unused]] AZStd::string_view nameTypeForDebug)
            {
                if (IsValidName(name))
                {
                    return true;
                }
                else
                {
                    AZ_Error("MaterialUtils", false, "%.*s '%.*s' is not a valid identifier", AZ_STRING_ARG(nameTypeForDebug), AZ_STRING_ARG(name));
                    return false;
                }
            }

            bool CheckIsValidPropertyName(AZStd::string_view name)
            {
                return CheckIsValidName(name, "Property name");
            }

            bool CheckIsValidGroupName(AZStd::string_view name)
            {
                return CheckIsValidName(name, "Group name");
            }

            AZStd::string PredictOriginalMaterialTypeSourcePath(const AZStd::string& materialTypeSourcePath)
            {
                constexpr const char* generatedMaterialTypeSuffix = "_generated.materialtype";
                if (materialTypeSourcePath.ends_with(generatedMaterialTypeSuffix))
                {
                    // Separate the input material type path into a relative path and root folder. This should work for both intermediate,
                    // generated material types and original material types.
                    bool pathFound = false;
                    AZStd::string relativePath;
                    AZStd::string rootFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        pathFound,
                        &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                        materialTypeSourcePath.c_str(),
                        relativePath,
                        rootFolder);

                    if (pathFound)
                    {
                        // Replace the generated suffix if present
                        AZ::StringFunc::Replace(relativePath, generatedMaterialTypeSuffix, ".materialtype");

                        // Search for the original material type file using the stripped generated material type path.
                        pathFound = false;
                        AZ::Data::AssetInfo sourceInfo;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                            pathFound,
                            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                            relativePath.c_str(),
                            sourceInfo,
                            rootFolder);

                        if (pathFound)
                        {
                            const AZ::IO::Path result = AZ::IO::Path(rootFolder) / sourceInfo.m_relativePath;
                            if (!result.empty())
                            {
                                return result.LexicallyNormal().String();
                            }
                        }
                    }
                }

                // Conversion failed. Return the input path.
                return materialTypeSourcePath;
            }

            AZStd::string PredictIntermediateMaterialTypeSourcePath(const AZStd::string& originalMaterialTypeSourcePath)
            {
                // This just normalizes the original path into a relative path that can be easily converted into relative path
                // to the intermediate .materialtype file
                bool pathFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string rootFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    pathFound,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                    originalMaterialTypeSourcePath.c_str(),
                    sourceInfo,
                    rootFolder);

                if (!pathFound)
                {
                    return {};
                }

                IO::Path intermediatePath = sourceInfo.m_relativePath;
                const AZStd::string materialTypeFilename =
                    AZStd::string::format("%.*s_generated.materialtype", AZ_STRING_ARG(intermediatePath.Stem().Native()));
                intermediatePath.ReplaceFilename(materialTypeFilename.c_str());

                AZStd::string intermediatePathString = intermediatePath.Native();
                AZStd::to_lower(intermediatePathString.begin(), intermediatePathString.end());

                IO::Path intermediateMaterialTypePath = Utils::GetProjectPath().c_str();
                intermediateMaterialTypePath /= "Cache";
                intermediateMaterialTypePath /= "Intermediate Assets";
                intermediateMaterialTypePath /= intermediatePathString;
                return intermediateMaterialTypePath.LexicallyNormal().String();
            }

            AZStd::string PredictIntermediateMaterialTypeSourcePath(const AZStd::string& referencingFilePath, const AZStd::string& originalMaterialTypeSourcePath)
            {
                const AZStd::string resolvedPath = AssetUtils::ResolvePathReference(referencingFilePath, originalMaterialTypeSourcePath);
                return PredictIntermediateMaterialTypeSourcePath(resolvedPath);
            }

            AZStd::string GetIntermediateMaterialTypeSourcePath(const AZStd::string& forOriginalMaterialTypeSourcePath)
            {
                const AZStd::string intermediatePathString = PredictIntermediateMaterialTypeSourcePath(forOriginalMaterialTypeSourcePath);
                return IO::LocalFileIO::GetInstance()->Exists(intermediatePathString.c_str()) ? intermediatePathString : AZStd::string{};
            }

            Outcome<Data::AssetId> GetFinalMaterialTypeAssetId(const AZStd::string& referencingFilePath, const AZStd::string& originalMaterialTypeSourcePath)
            {
                const AZStd::string resolvedPath = AssetUtils::ResolvePathReference(referencingFilePath, originalMaterialTypeSourcePath);
                const AZStd::string intermediateMaterialTypePath = GetIntermediateMaterialTypeSourcePath(resolvedPath);
                if (!intermediateMaterialTypePath.empty())
                {
                    return AssetUtils::MakeAssetId(intermediateMaterialTypePath, MaterialTypeSourceData::IntermediateMaterialTypeSubId);
                }
                return AssetUtils::MakeAssetId(resolvedPath, MaterialTypeAsset::SubId);
            }

            AZStd::string GetFinalMaterialTypeSourcePath(const AZStd::string& referencingFilePath, const AZStd::string& originalMaterialTypeSourcePath)
            {
                const AZStd::string resolvedPath = AssetUtils::ResolvePathReference(referencingFilePath, originalMaterialTypeSourcePath);
                const AZStd::string intermediateMaterialTypePath = GetIntermediateMaterialTypeSourcePath(resolvedPath);
                if (intermediateMaterialTypePath.empty())
                {
                    return resolvedPath;
                }
                return intermediateMaterialTypePath;
            }

        }
    }
}
