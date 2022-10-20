/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialTypeBuilder.h"
#include <Material/MaterialBuilderUtils.h>

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AssetBuilderSDK/SerializationDependencies.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static constexpr char const MaterialTypeBuilderName[] = "MaterialTypeBuilder";
        }

        const char* MaterialTypeBuilder::JobKey = "Material Type Builder";

        AZStd::string MaterialTypeBuilder::GetBuilderSettingsFingerprint() const
        {
            return AZStd::string::format("[ShouldOutputAllPropertiesMaterial=%d]", ShouldOutputAllPropertiesMaterial());
        }

        void MaterialTypeBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc materialBuilderDescriptor;
            materialBuilderDescriptor.m_name = JobKey;
            materialBuilderDescriptor.m_version = 1; // Split material type builder
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtype", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_busId = azrtti_typeid<MaterialTypeBuilder>();
            materialBuilderDescriptor.m_createJobFunction = AZStd::bind(&MaterialTypeBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            materialBuilderDescriptor.m_processJobFunction = AZStd::bind(&MaterialTypeBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            materialBuilderDescriptor.m_analysisFingerprint = GetBuilderSettingsFingerprint();

            BusConnect(materialBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialBuilderDescriptor);
        }

        MaterialTypeBuilder::~MaterialTypeBuilder()
        {
            BusDisconnect();
        }

        bool MaterialTypeBuilder::ShouldOutputAllPropertiesMaterial() const
        {
            // Enable this setting to generate a default source material file containing an explicit list of all properties and their
            // default values. This is primarily used by artists and developers scraping data from the materials and should only be enabled
            // as needed by those users.
            bool value = false;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Get(value, "/O3DE/Atom/RPI/MaterialTypeBuilder/CreateAllPropertiesMaterial");
            }
            return value;
        }

        void MaterialTypeBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
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

            MaterialUtils::ImportedJsonFiles importedJsonFiles;
            auto materialTypeSourceData = MaterialUtils::LoadMaterialTypeSourceData(fullSourcePath, nullptr, &importedJsonFiles);

            if (!materialTypeSourceData.IsSuccess())
            {
                return;
            }

            for (auto& importedJsonFile : importedJsonFiles)
            {
                AssetBuilderSDK::SourceFileDependency sourceDependency;
                sourceDependency.m_sourceFileDependencyPath = importedJsonFile;
                response.m_sourceFileDependencyList.push_back(sourceDependency);
            }

            for (auto& shader : materialTypeSourceData.GetValue().m_shaderCollection)
            {
                MaterialBuilderUtils::AddPossibleDependencies(request.m_sourceFile,
                    shader.m_shaderFilePath,
                    "Shader Asset",
                    outputJobDescriptor.m_jobDependencyList,
                    response.m_sourceFileDependencyList,
                    false,
                    0);
            }

            auto addFunctorDependencies = [&outputJobDescriptor, &request, &response](const AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>>& functors)
            {
                for (auto& functor : functors)
                {
                    const auto& dependencies = functor->GetActualSourceData()->GetAssetDependencies();

                    for (const MaterialFunctorSourceData::AssetDependency& dependency : dependencies)
                    {
                        MaterialBuilderUtils::AddPossibleDependencies(request.m_sourceFile,
                            dependency.m_sourceFilePath,
                            dependency.m_jobKey.c_str(),
                            outputJobDescriptor.m_jobDependencyList,
                            response.m_sourceFileDependencyList);
                    }
                }
            };

            addFunctorDependencies(materialTypeSourceData.GetValue().m_materialFunctorSourceData);

            materialTypeSourceData.GetValue().EnumeratePropertyGroups([addFunctorDependencies](const MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
                {
                    addFunctorDependencies(propertyGroupStack.back()->GetFunctors());
                    return true;
                });

            materialTypeSourceData.GetValue().EnumerateProperties(
                [&request, &response, &outputJobDescriptor](const MaterialTypeSourceData::PropertyDefinition* property, const MaterialNameContext&)
                {
                    if (property->m_dataType == MaterialPropertyDataType::Image && MaterialUtils::LooksLikeImageFileReference(property->m_value))
                    {
                        MaterialBuilderUtils::AddPossibleImageDependencies(
                            request.m_sourceFile,
                            property->m_value.GetValue<AZStd::string>(),
                            outputJobDescriptor.m_jobDependencyList,
                            response.m_sourceFileDependencyList);
                    }
                    return true;
                });
            
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

        AZ::Data::Asset<MaterialTypeAsset> CreateMaterialTypeAsset(AZStd::string_view materialTypeSourceFilePath, rapidjson::Document& json)
        {
            auto materialType = MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourceFilePath, &json);

            if (!materialType.IsSuccess())
            {
                return  {};
            }

            auto materialTypeAssetOutcome = materialType.GetValue().CreateMaterialTypeAsset(Uuid::CreateRandom(), materialTypeSourceFilePath, true);
            if (!materialTypeAssetOutcome.IsSuccess())
            {
                return  {};
            }

            return materialTypeAssetOutcome.GetValue();
        }
        
        void MaterialTypeBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
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
                AZ_Error(MaterialTypeBuilderName, false, "Failed to load material file: %s", loadOutcome.GetError().c_str());
                return;
            }

            rapidjson::Document& document = loadOutcome.GetValue();

            AZStd::string materialProductPath;
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), fileName);
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, MaterialTypeAsset::Extension);

            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), materialProductPath, true);

            AZ::Data::Asset<MaterialTypeAsset> materialTypeAsset;

            {
                AZ_TraceContext("Product", fileName);
                AZ_TracePrintf("MaterialTypeBuilder", AZStd::string::format("Producing %s...", fileName.c_str()).c_str());

                // Load the material type file and create the MaterialTypeAsset object
                materialTypeAsset = CreateMaterialTypeAsset(fullSourcePath, document);

                if (!materialTypeAsset)
                {
                    // Errors will have been reported above
                    return;
                }

                // [ATOM-13190] Change this back to ST_BINARY. It's ST_XML temporarily for debugging.
                if (!AZ::Utils::SaveObjectToFile(materialProductPath, AZ::DataStream::ST_XML, materialTypeAsset.Get()))
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to save material type to file '%s'!", materialProductPath.c_str());
                    return;
                }

                AssetBuilderSDK::JobProduct jobProduct;
                if (!AssetBuilderSDK::OutputObject(
                    materialTypeAsset.Get(),
                    materialProductPath,
                    azrtti_typeid<RPI::MaterialTypeAsset>(),
                    (u32)MaterialTypeProductSubId::MaterialTypeAsset,
                    jobProduct))
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to output product dependencies.");
                    return;
                }

                response.m_outputProducts.push_back(AZStd::move(jobProduct));
            }

            if(ShouldOutputAllPropertiesMaterial())
            {
                AZStd::string defaultMaterialFileName;
                AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), defaultMaterialFileName);
                defaultMaterialFileName += "_AllProperties.material";

                AZStd::string defaultMaterialFilePath;
                AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), defaultMaterialFileName.c_str(), defaultMaterialFilePath, true);

                AZ_TraceContext("Product", defaultMaterialFileName);
                AZ_TracePrintf("MaterialTypeBuilder", AZStd::string::format("Producing %s...", defaultMaterialFileName.c_str()).c_str());

                MaterialSourceData allPropertyDefaultsMaterial = MaterialSourceData::CreateAllPropertyDefaultsMaterial(materialTypeAsset, request.m_sourceFile);

                if (!JsonUtils::SaveObjectToFile(defaultMaterialFilePath, allPropertyDefaultsMaterial))
                {
                    AZ_Warning(MaterialTypeBuilderName, false, "Failed to save material reference file '%s'!", defaultMaterialFilePath.c_str());
                }
                else
                {
                    AssetBuilderSDK::JobProduct defaultMaterialFileProduct;
                    defaultMaterialFileProduct.m_dependenciesHandled = true; // This product is only for reference, not used at runtime
                    defaultMaterialFileProduct.m_productFileName = defaultMaterialFilePath;
                    defaultMaterialFileProduct.m_productSubID = (u32)MaterialTypeProductSubId::AllPropertiesMaterialSourceFile;
                    response.m_outputProducts.push_back(AZStd::move(defaultMaterialFileProduct));
                }
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialTypeBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }
    } // namespace RPI
} // namespace AZ
