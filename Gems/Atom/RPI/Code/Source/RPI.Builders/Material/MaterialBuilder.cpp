/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialBuilder.h"
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>

#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static constexpr char const MaterialBuilderName[] = "MaterialBuilder";
        }

        const char* MaterialBuilder::JobKey = "Atom Material Builder";

        AZStd::string MaterialBuilder::GetBuilderSettingsFingerprint() const
        {
            return
                AZStd::string::format("[BuildersShouldFinalizeMaterialAssets=%d]", MaterialUtils::BuildersShouldFinalizeMaterialAssets()) +
                AZStd::string::format("[ShouldOutputAllPropertiesMaterial=%d]", ShouldOutputAllPropertiesMaterial());
        }

        void MaterialBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc materialBuilderDescriptor;
            materialBuilderDescriptor.m_name = JobKey;
            materialBuilderDescriptor.m_version = 133; // Preload material dependencies
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.material", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtype", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
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

        bool MaterialBuilder::ShouldOutputAllPropertiesMaterial() const
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

        //! Adds all relevant dependencies for a referenced source file, considering that the path might be relative to the original file location or a full asset path.
        //! This can include both source dependencies and a single job dependency, but will include only source dependencies if the file is not found.
        //! Note the AssetBuilderSDK::JobDependency::m_platformIdentifier will not be set by this function. The calling code must set this value before passing back
        //! to the AssetBuilderSDK::CreateJobsResponse.
        //! @param currentFilePath The path of the .material or .materialtype file being processed
        //! @param referencedParentPath The path to the referenced file as it appears in the current file
        //! @param jobKey The job key for the job that is expected to process the referenced file
        //! @param jobDependencies Dependencies may be added to this list
        //! @param sourceDependencies Dependencies may be added to this list
        //! @param forceOrderOnce If true, any job dependencies will use JobDependencyType::OrderOnce. Use this if the builder will only ever need to get the AssetId
        //!                       of the referenced file but will not need to load the asset.
        void AddPossibleDependencies(
            const AZStd::string& currentFilePath,
            const AZStd::string& referencedParentPath,
            const char* jobKey,
            AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
            AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencies,
            bool forceOrderOnce = false,
            AZStd::optional<AZ::u32> productSubId = AZStd::nullopt)
        {
            bool dependencyFileFound = false;
            
            const bool currentFileIsMaterial = AzFramework::StringFunc::Path::IsExtension(currentFilePath.c_str(), MaterialSourceData::Extension);
            const bool referencedFileIsMaterialType = AzFramework::StringFunc::Path::IsExtension(referencedParentPath.c_str(), MaterialTypeSourceData::Extension);
            const bool shouldFinalizeMaterialAssets = MaterialUtils::BuildersShouldFinalizeMaterialAssets();

            AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDepenencyPaths(currentFilePath, referencedParentPath);
            for (auto& file : possibleDependencies)
            {
                // The first path found is the highest priority, and will have a job dependency, as this is the one
                // the builder will actually use
                if (!dependencyFileFound)
                {
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(dependencyFileFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, file.c_str(), sourceInfo, watchFolder);

                    if (dependencyFileFound)
                    {
                        AssetBuilderSDK::JobDependency jobDependency;
                        jobDependency.m_jobKey = jobKey;
                        jobDependency.m_sourceFile.m_sourceFileDependencyPath = file;
                        jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                        
                        if(productSubId)
                        {
                            jobDependency.m_productSubIds.push_back(productSubId.value());
                        }
                        
                        // If we aren't finalizing material assets, then a normal job dependency isn't needed because the MaterialTypeAsset data won't be used.
                        // However, we do still need at least an OrderOnce dependency to ensure the Asset Processor knows about the material type asset so the builder can get it's AssetId.
                        // This can significantly reduce AP processing time when a material type or its shaders are edited.
                        if (forceOrderOnce || (currentFileIsMaterial && referencedFileIsMaterialType && !shouldFinalizeMaterialAssets))
                        {
                            jobDependency.m_type = AssetBuilderSDK::JobDependencyType::OrderOnce;
                        }

                        jobDependencies.push_back(jobDependency);
                    }
                    else
                    {
                        // The file was not found so we can't add a job dependency. But we add a source dependency instead so if a file
                        // shows up later at this location, it will wake up the builder to try again.

                        AssetBuilderSDK::SourceFileDependency sourceDependency;
                        sourceDependency.m_sourceFileDependencyPath = file;
                        sourceDependencies.push_back(sourceDependency);
                    }
                }
            }
        }

        void AddPossibleImageDependencies(
            const AZStd::string& currentFilePath,
            const AZStd::string& imageFilePath,
            AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
            AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceDependencies)
        {
            if (imageFilePath.empty())
            {
                return;
            }

            AZStd::string ext;
            AzFramework::StringFunc::Path::GetExtension(imageFilePath.c_str(), ext, false);
            AZStd::to_upper(ext.begin(), ext.end());
            AZStd::string jobKey = "Image Compile: " + ext;

            if (ext.empty())
            {
                return;
            }

            bool forceOrderOnce = true;
            AddPossibleDependencies(currentFilePath,
                imageFilePath,
                jobKey.c_str(),
                jobDependencies,
                sourceDependencies,
                forceOrderOnce);
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

            // Load the file so we can detect and report dependencies.
            // If the file is a .materialtype, report dependencies on the .shader files.
            // If the file is a .material, report a dependency on the .materialtype and parent .material file
            {
                AZStd::string fullSourcePath;
                AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullSourcePath, true);

                auto loadOutcome = JsonSerializationUtils::ReadJsonFile(fullSourcePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                if (!loadOutcome.IsSuccess())
                {
                    AZ_Error(MaterialBuilderName, false, "%s", loadOutcome.GetError().c_str());
                    return;
                }

                rapidjson::Document& document = loadOutcome.GetValue();

                const bool isMaterialTypeFile = AzFramework::StringFunc::Path::IsExtension(request.m_sourceFile.c_str(), MaterialTypeSourceData::Extension);
                if (isMaterialTypeFile)
                {
                    MaterialUtils::ImportedJsonFiles importedJsonFiles;
                    auto materialTypeSourceData = MaterialUtils::LoadMaterialTypeSourceData(fullSourcePath, &document, &importedJsonFiles);

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
                        AddPossibleDependencies(request.m_sourceFile,
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
                                AddPossibleDependencies(request.m_sourceFile,
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
                            if (property->m_dataType == MaterialPropertyDataType::Image && MaterialSourceData::LooksLikeImageFileReference(property->m_value))
                            {
                                AddPossibleImageDependencies(
                                    request.m_sourceFile,
                                    property->m_value.GetValue<AZStd::string>(),
                                    outputJobDescriptor.m_jobDependencyList,
                                    response.m_sourceFileDependencyList);
                            }
                            return true;
                        });
                }
                else // it's a .material file
                {
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

                    if (parentMaterialPath.empty())
                    {
                        parentMaterialPath = materialTypePath;
                    }

                    // Register dependency on the parent material source file so we can load it and use it's data to build this variant material.
                    // Note, we don't need a direct dependency on the material type because the parent material will depend on it.
                    AddPossibleDependencies(request.m_sourceFile,
                        parentMaterialPath,
                        JobKey,
                        outputJobDescriptor.m_jobDependencyList,
                        response.m_sourceFileDependencyList,
                        false,
                        0);

                    // Even though above we were able to get away without deserializing the material json, we do need to deserialize here in order
                    // to easily read the property values. Note that with the latest .material file format, it actually wouldn't be too hard to
                    // just read the raw json, it's just a map of property name to property value. But we also are maintaining backward compatible
                    // support for an older file format that nests property values rather than using a flat list. By deserializing we leave it up
                    // to the MaterialSourceData class to provide that backward compatibility (see MaterialSourceData::ConvertToNewDataFormat()).
                    
                    auto materialSourceData = MaterialUtils::LoadMaterialSourceData(fullSourcePath, &materialJson);
                    if (materialSourceData.IsSuccess())
                    {
                        for (auto& [propertyId, propertyValue] : materialSourceData.GetValue().GetPropertyValues())
                        {
                            AZ_UNUSED(propertyId);
                            
                            if (MaterialSourceData::LooksLikeImageFileReference(propertyValue))
                            {
                                AddPossibleImageDependencies(
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
                }
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
        
        AZ::Data::Asset<MaterialAsset> MaterialBuilder::CreateMaterialAsset(AZStd::string_view materialSourceFilePath, const rapidjson::Value& json) const
        {
            auto material = MaterialUtils::LoadMaterialSourceData(materialSourceFilePath, &json, true);

            if (!material.IsSuccess())
            {
                return {};
            }

            MaterialAssetProcessingMode processingMode = MaterialUtils::BuildersShouldFinalizeMaterialAssets() ? MaterialAssetProcessingMode::PreBake : MaterialAssetProcessingMode::DeferredBake;

            auto materialAssetOutcome = material.GetValue().CreateMaterialAsset(Uuid::CreateRandom(), materialSourceFilePath, processingMode, ShouldReportMaterialAssetWarningsAsErrors());
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

            const bool isMaterialTypeFile = AzFramework::StringFunc::Path::IsExtension(request.m_sourceFile.c_str(), MaterialTypeSourceData::Extension);

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
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, isMaterialTypeFile ? MaterialTypeAsset::Extension : MaterialAsset::Extension);

            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), materialProductPath, true);

            if (isMaterialTypeFile)
            {
                AZ::Data::Asset<MaterialTypeAsset> materialTypeAsset;

                {
                    AZ_TraceContext("Product", fileName);
                    AZ_TracePrintf("MaterialBuilder", AZStd::string::format("Producing %s...", fileName.c_str()).c_str());

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
                        AZ_Error(MaterialBuilderName, false, "Failed to save material type to file '%s'!", materialProductPath.c_str());
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
                        AZ_Error(MaterialBuilderName, false, "Failed to output product dependencies.");
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
                    AZ_TracePrintf("MaterialBuilder", AZStd::string::format("Producing %s...", defaultMaterialFileName.c_str()).c_str());

                    MaterialSourceData allPropertyDefaultsMaterial = MaterialSourceData::CreateAllPropertyDefaultsMaterial(materialTypeAsset, request.m_sourceFile);

                    if (!JsonUtils::SaveObjectToFile(defaultMaterialFilePath, allPropertyDefaultsMaterial))
                    {
                        AZ_Warning(MaterialBuilderName, false, "Failed to save material reference file '%s'!", defaultMaterialFilePath.c_str());
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
            else
            {
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
        }

        void MaterialBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }
    } // namespace RPI
} // namespace AZ
