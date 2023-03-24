/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialBuilder.h"
#include "MaterialTypeBuilder.h"
#include <Material/MaterialBuilderUtils.h>

#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static constexpr char const MaterialBuilderName[] = "MaterialBuilder";
        }

        const char* MaterialBuilder::JobKey = "Material Builder";

        AZStd::string MaterialBuilder::GetBuilderSettingsFingerprint() const
        {
            return "";
        }

        void MaterialBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc materialBuilderDescriptor;
            materialBuilderDescriptor.m_name = JobKey;
            materialBuilderDescriptor.m_version = 138; // Updated invalid texture UUID + error message
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.material", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_busId = azrtti_typeid<MaterialBuilder>();
            materialBuilderDescriptor.m_createJobFunction = AZStd::bind(&MaterialBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            materialBuilderDescriptor.m_processJobFunction = AZStd::bind(&MaterialBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            materialBuilderDescriptor.m_analysisFingerprint = GetBuilderSettingsFingerprint();

            BusConnect(materialBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialBuilderDescriptor);
        }

        MaterialBuilder::~MaterialBuilder()
        {
            BusDisconnect();
        }

        bool MaterialBuilder::ShouldReportMaterialAssetWarningsAsErrors() const
        {
            bool warningsAsErrors = false;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Get(warningsAsErrors, "/O3DE/Atom/RPI/MaterialBuilder/WarningsAsErrors");
            }
            return warningsAsErrors;
        }

        void MaterialBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            // We'll build up this one JobDescriptor and reuse it to register each of the platforms
            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = JobKey;
            outputJobDescriptor.m_additionalFingerprintInfo = GetBuilderSettingsFingerprint();

            AZStd::string fullSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullSourcePath, true);

            auto loadOutcome = JsonSerializationUtils::ReadJsonFile(fullSourcePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!loadOutcome.IsSuccess())
            {
                AZ_Error(MaterialBuilderName, false, "%s", loadOutcome.GetError().c_str());
                return;
            }

            rapidjson::Document& document = loadOutcome.GetValue();

            // Note we don't use the LoadMaterial() utility function or JsonSerializer here because we don't care about fully
            // processing the material file at this point and reporting on the many things that could go wrong. We just want
            // to report the parent material and material type dependencies. So using rapidjson directly is actually simpler.

            AZStd::string materialTypePath;
            AZStd::string parentMaterialPath;

            auto& materialJson = document;

            const char* const materialTypeField = "materialType";
            const char* const parentMaterialField = "parentMaterial";

            if (materialJson.IsObject() && materialJson.HasMember(materialTypeField) && materialJson[materialTypeField].IsString())
            {
                materialTypePath = materialJson[materialTypeField].GetString();
            }

            if (materialJson.IsObject() && materialJson.HasMember(parentMaterialField) && materialJson[parentMaterialField].IsString())
            {
                parentMaterialPath = materialJson[parentMaterialField].GetString();
            }

            if (!parentMaterialPath.empty())
            {
                // Register dependency on the parent material source file so we can load it and use it's data to build this variant material.
                MaterialBuilderUtils::AddPossibleDependencies(
                    request.m_sourceFile,
                    parentMaterialPath,
                    JobKey,
                    outputJobDescriptor.m_jobDependencyList,
                    response.m_sourceFileDependencyList,
                    false,
                    0);
            }

            // Note that parentMaterialPath may have registered a dependency above, and the parent material reports dependency on the material type as well,
            // so there is a chain that propagates automatically, at least in some cases. However, that isn't sufficient for all cases and a direct dependency
            // on the material type is needed, because ProcessJob loads the parent material and the material type independent of each other. Otherwise, edge
            // cases are possible, where the material type changes in some way that does not impact the parent material asset's final data, yet it does impact
            // the child material. See https://github.com/o3de/o3de/issues/13766
            if (!materialTypePath.empty())
            {
                // If the material uses the "Direct" format, then there will need to be a dependency on that file.
                // If it uses the "Abstract" format, then there will be an intermediate .materialtype and there needs to be a dependency on that file instead.
                // At this point the builder does not know which is the case, without loading the .materialtype file and inspecting its data. The builder
                // avoids that because it could slow things down, and instead just registers both dependencies.

                MaterialBuilderUtils::AddPossibleDependencies(
                    request.m_sourceFile,
                    materialTypePath,
                    MaterialTypeBuilder::FinalStageJobKey,
                    outputJobDescriptor.m_jobDependencyList,
                    response.m_sourceFileDependencyList,
                    false,
                    0);

                const AZStd::string intermediateMaterialTypePath =
                    MaterialUtils::PredictIntermediateMaterialTypeSourcePath(request.m_sourceFile, materialTypePath);
                if (!intermediateMaterialTypePath.empty())
                {
                    MaterialBuilderUtils::AddPossibleDependencies(
                        request.m_sourceFile,
                        intermediateMaterialTypePath,
                        MaterialTypeBuilder::FinalStageJobKey,
                        outputJobDescriptor.m_jobDependencyList,
                        response.m_sourceFileDependencyList,
                        false,
                        0);
                }
            }

            // Even though above we were able to get away without deserializing the material json, we do need to deserialize here in order
            // to easily read the property values. Note that with the latest .material file format, it actually wouldn't be too hard to
            // just read the raw json, it's just a map of property name to property value. But we also are maintaining backward compatible
            // support for an older file format that nests property values rather than using a flat list. By deserializing we leave it up
            // to the MaterialSourceData class to provide that backward compatibility (see MaterialSourceData::UpgradeLegacyFormat()).
                    
            auto materialSourceData = MaterialUtils::LoadMaterialSourceData(fullSourcePath, &materialJson);
            if (materialSourceData.IsSuccess())
            {
                for (auto& [propertyId, propertyValue] : materialSourceData.GetValue().GetPropertyValues())
                {
                    AZ_UNUSED(propertyId);
                            
                    if (MaterialUtils::LooksLikeImageFileReference(propertyValue))
                    {
                        MaterialBuilderUtils::AddPossibleImageDependencies(
                            request.m_sourceFile,
                            propertyValue.GetValue<AZStd::string>(),
                            outputJobDescriptor.m_jobDependencyList,
                            response.m_sourceFileDependencyList);
                    }
                }
                        
            }
            else
            {
                AZ_Warning(MaterialBuilderName, false, "Could not report dependencies for Image properties because the material json couldn't be loaded.");
            }
            
            // Create the output jobs for each platform
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                outputJobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                for (auto& jobDependency : outputJobDescriptor.m_jobDependencyList)
                {
                    jobDependency.m_platformIdentifier = platformInfo.m_identifier;
                }

                response.m_createJobOutputs.push_back(outputJobDescriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        AZ::Data::Asset<MaterialAsset> MaterialBuilder::CreateMaterialAsset(AZStd::string_view materialSourceFilePath, const rapidjson::Value& json) const
        {
            auto material = MaterialUtils::LoadMaterialSourceData(materialSourceFilePath, &json, true);

            if (!material.IsSuccess())
            {
                return {};
            }

            auto materialAssetOutcome = material.GetValue().CreateMaterialAsset(Uuid::CreateRandom(), materialSourceFilePath, ShouldReportMaterialAssetWarningsAsErrors());
            if (!materialAssetOutcome.IsSuccess())
            {
                return {};
            }

            return materialAssetOutcome.GetValue();
        }

        void MaterialBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }
            if (m_isShuttingDown)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            AZStd::string fullSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullSourcePath, true);

            auto loadOutcome = JsonSerializationUtils::ReadJsonFile(fullSourcePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!loadOutcome.IsSuccess())
            {
                AZ_Error(MaterialBuilderName, false, "Failed to load material file: %s", loadOutcome.GetError().c_str());
                return;
            }

            rapidjson::Document& document = loadOutcome.GetValue();

            AZStd::string materialProductPath;
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), fileName);
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, MaterialAsset::Extension);

            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), materialProductPath, true);

            // TODO(MaterialPipeline): Try to improve MaterialBuilder to avoid transient failures when the intermediate .materialtype has not
            // been generated yet, and avoid the annoying failure notifications. We could end ProcessJob with success instead of failure when
            // an intermediate .materialtype is missing, maybe just output a warning instead, and report a source dependency on that file.
            // Then when it appears later, reprocess the job, and then the material asset should show up without every reporting a failure (I think).

            // Load the material file and create the MaterialAsset object
            AZ::Data::Asset<MaterialAsset> materialAsset;
            materialAsset = CreateMaterialAsset(request.m_sourceFile, document);

            if (!materialAsset)
            {
                // Errors will have been reported above
                return;
            }

            // [ATOM-13190] Change this back to ST_BINARY. It's ST_XML temporarily for debugging.
            if (!AZ::Utils::SaveObjectToFile(materialProductPath, AZ::DataStream::ST_XML, materialAsset.Get()))
            {
                AZ_Error(MaterialBuilderName, false, "Failed to save material to file '%s'!", materialProductPath.c_str());
                return;
            }

            AssetBuilderSDK::JobProduct jobProduct;
            if (!AssetBuilderSDK::OutputObject(materialAsset.Get(), materialProductPath, azrtti_typeid<RPI::MaterialAsset>(), 0, jobProduct))
            {
                AZ_Error(MaterialBuilderName, false, "Failed to output product dependencies.");
                return;
            }

            response.m_outputProducts.push_back(AZStd::move(jobProduct));

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }
    } // namespace RPI
} // namespace AZ
