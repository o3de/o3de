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
#include <Atom/RPI.Edit/Common/AssetUtils.h>
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
            return AZStd::string::format(
                "[%s %s]", MaterialBuilderName, ShouldReportMaterialAssetWarningsAsErrors() ? "WarningsAsErrorsOn" : "WarningsAsErrorsOff");
        }

        void MaterialBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc materialBuilderDescriptor;
            materialBuilderDescriptor.m_name = JobKey;
            materialBuilderDescriptor.m_version = 140; // Switch from XML to binary assets
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

        void MaterialBuilder::CreateJobs(
            const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
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

            AZStd::string materialSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), materialSourcePath, true);

            // Rather than just reading the JSON document, we read the material source data structure because we need access
            // to material type, parent material, and all of the properties to enumerate images and other dependencies.
            const auto materialSourceDataOutcome = MaterialUtils::LoadMaterialSourceData(materialSourcePath);
            if (!materialSourceDataOutcome)
            {
                AZ_Error(MaterialBuilderName, false, "Failed to load material source data: %s", materialSourcePath.c_str());
                return;
            }

            MaterialBuilderUtils::AddFingerprintForDependency(materialSourcePath, outputJobDescriptor);

            const auto& materialSourceData = materialSourceDataOutcome.GetValue();

            if (!materialSourceData.m_parentMaterial.empty())
            {
                const auto& resolvedParentMaterialPath =
                    AssetUtils::ResolvePathReference(materialSourcePath, materialSourceData.m_parentMaterial);

                // Register dependency on the parent material source file so we can load and use its data to build this material.
                MaterialBuilderUtils::AddPossibleDependencies(
                    materialSourcePath,
                    resolvedParentMaterialPath,
                    response,
                    outputJobDescriptor,
                    JobKey);
            }

            // Note that parentMaterialPath may have registered a dependency above, and the parent material reports dependency on the
            // material type as well, so there is a chain that propagates automatically, at least in some cases. However, that isn't
            // sufficient for all cases and a direct dependency on the material type is needed, because ProcessJob loads the parent material
            // and the material type independent of each other. Otherwise, edge cases are possible, where the material type changes in some
            // way that does not impact the parent material asset's final data, yet it does impact the child material. See
            // https://github.com/o3de/o3de/issues/13766
            if (!materialSourceData.m_materialType.empty())
            {
                // We usually won't load file during CreateJob since we want to keep the function fast. But here we have to load the
                // material type data to find the exact material type format so we could create an accurate source dependency.
                const auto& resolvedMaterialTypePath =
                    AssetUtils::ResolvePathReference(materialSourcePath, materialSourceData.m_materialType);

                const auto& materialTypeSourceDataOutcome = MaterialUtils::LoadMaterialTypeSourceData(resolvedMaterialTypePath);
                if (!materialTypeSourceDataOutcome)
                {
                    AZ_Error(MaterialBuilderName, false, "Failed to load material type source data: %s", resolvedMaterialTypePath.c_str());
                    return;
                }

                const auto& materialTypeSourceData = materialTypeSourceDataOutcome.GetValue();
                const MaterialTypeSourceData::Format materialTypeFormat = materialTypeSourceData.GetFormat();

                // If the material uses the "Direct" format, then there will need to be a dependency on that file. If it uses the "Abstract"
                // format, then there will be an intermediate .materialtype and there needs to be a dependency on that file instead.
                if (materialTypeFormat == MaterialTypeSourceData::Format::Direct)
                {
                    MaterialBuilderUtils::AddPossibleDependencies(
                        materialSourcePath,
                        resolvedMaterialTypePath,
                        response,
                        outputJobDescriptor,
                        MaterialTypeBuilder::FinalStageJobKey);
                }
                else if (materialTypeFormat == MaterialTypeSourceData::Format::Abstract)
                {
                    MaterialBuilderUtils::AddPossibleDependencies(
                        materialSourcePath,
                        resolvedMaterialTypePath,
                        response,
                        outputJobDescriptor,
                        MaterialTypeBuilder::PipelineStageJobKey,
                        AssetBuilderSDK::JobDependencyType::Order,
                        {},
                        AssetBuilderSDK::CommonPlatformName);

                    const AZStd::string intermediateMaterialTypePath =
                        MaterialUtils::PredictIntermediateMaterialTypeSourcePath(materialSourcePath, resolvedMaterialTypePath);
                    if (!intermediateMaterialTypePath.empty())
                    {
                        MaterialBuilderUtils::AddPossibleDependencies(
                            materialSourcePath,
                            intermediateMaterialTypePath,
                            response,
                            outputJobDescriptor,
                            MaterialTypeBuilder::FinalStageJobKey);
                    }
                }
            }

            // Assign dependencies from image properties
            for (auto& [propertyId, propertyValue] : materialSourceData.GetPropertyValues())
            {
                AZ_UNUSED(propertyId);

                if (MaterialUtils::LooksLikeImageFileReference(propertyValue))
                {
                    MaterialBuilderUtils::AddPossibleImageDependencies(
                        materialSourcePath,
                        propertyValue.GetValue<AZStd::string>(),
                        response,
                        outputJobDescriptor);
                }
            }

            // Create the output jobs for each platform
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                outputJobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                for (auto& jobDependency : outputJobDescriptor.m_jobDependencyList)
                {
                    if (jobDependency.m_platformIdentifier.empty())
                    {
                        jobDependency.m_platformIdentifier = platformInfo.m_identifier;
                    }
                }

                response.m_createJobOutputs.push_back(outputJobDescriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void MaterialBuilder::ProcessJob(
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
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

            AZStd::string materialSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(
                request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), materialSourcePath, true);

            const auto materialSourceDataOutcome = MaterialUtils::LoadMaterialSourceData(materialSourcePath);
            if (!materialSourceDataOutcome)
            {
                AZ_Error(MaterialBuilderName, false, "Failed to load material source data: %s", materialSourcePath.c_str());
                return;
            }

            const auto& materialSourceData = materialSourceDataOutcome.GetValue();

            // Load the material file and create the MaterialAsset object
            auto materialAssetOutcome = materialSourceData.CreateMaterialAsset(
                Uuid::CreateRandom(), materialSourcePath, ShouldReportMaterialAssetWarningsAsErrors());
            if (!materialAssetOutcome)
            {
                AZ_Error(MaterialBuilderName, false, "Failed to create material asset from source data: %s", materialSourcePath.c_str());
                return;
            }

            AZ::Data::Asset<MaterialAsset> materialAsset = materialAssetOutcome.GetValue();
            if (!materialAsset)
            {
                // Errors will have been reported above
                return;
            }

            AZStd::string materialProductPath;
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(materialSourcePath.c_str(), fileName);
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, MaterialAsset::Extension);
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), materialProductPath, true);

            if (!AZ::Utils::SaveObjectToFile(materialProductPath, AZ::DataStream::ST_BINARY, materialAsset.Get()))
            {
                AZ_Error(MaterialBuilderName, false, "Failed to save material to file '%s'!", materialProductPath.c_str());
                return;
            }

            AssetBuilderSDK::JobProduct jobProduct;
            if (!AssetBuilderSDK::OutputObject(
                    materialAsset.Get(), materialProductPath, azrtti_typeid<RPI::MaterialAsset>(), 0, jobProduct))
            {
                AZ_Error(MaterialBuilderName, false, "Failed to output product dependencies.");
                return;
            }

            response.m_outputProducts.emplace_back(AZStd::move(jobProduct));

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }
    } // namespace RPI
} // namespace AZ
