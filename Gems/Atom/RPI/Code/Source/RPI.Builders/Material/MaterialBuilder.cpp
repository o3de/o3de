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
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

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

        void MaterialBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc materialBuilderDescriptor;
            materialBuilderDescriptor.m_name = JobKey;
            materialBuilderDescriptor.m_version = 110; // Material version auto update feature
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.material", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtype", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_busId = azrtti_typeid<MaterialBuilder>();
            materialBuilderDescriptor.m_createJobFunction = AZStd::bind(&MaterialBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            materialBuilderDescriptor.m_processJobFunction = AZStd::bind(&MaterialBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(materialBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialBuilderDescriptor);
        }

        MaterialBuilder::~MaterialBuilder()
        {
            BusDisconnect();
        }

        //! Adds all relevant dependencies for a referenced source file, considering that the path might be relative to the original file location or a full asset path.
        //! This will usually include multiple source dependencies and a single job dependency, but will include only source dependencies if the file is not found.
        //! Note the AssetBuilderSDK::JobDependency::m_platformIdentifier will not be set by this function. The calling code must set this value before passing back
        //! to the AssetBuilderSDK::CreateJobsResponse. If isOrderedOnceForMaterialTypes is true and the dependency is a materialtype file, the job dependency type
        //! will be set to JobDependencyType::OrderOnce.
        void AddPossibleDependencies(AZStd::string_view currentFilePath,
            AZStd::string_view referencedParentPath,
            const char* jobKey,
            AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies,
            bool isOrderedOnceForMaterialTypes = false)
        {
            bool dependencyFileFound = false;

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

                        const bool isMaterialTypeFile = AzFramework::StringFunc::Path::IsExtension(file.c_str(), MaterialTypeSourceData::Extension);
                        jobDependency.m_type = (isMaterialTypeFile && isOrderedOnceForMaterialTypes) ? AssetBuilderSDK::JobDependencyType::OrderOnce : AssetBuilderSDK::JobDependencyType::Order;

                        jobDependencies.push_back(jobDependency);
                    }
                }
            }
        }

        template<typename MaterialSourceDataT>
        AZ::Outcome<MaterialSourceDataT> LoadSourceData(const rapidjson::Value& value, const AZStd::string& filePath)
        {
            MaterialSourceDataT material;

            JsonDeserializerSettings settings;

            JsonReportingHelper reportingHelper;
            reportingHelper.Attach(settings);

            // This is required by some custom material serializers to support relative path references.
            JsonFileLoadContext fileLoadContext;
            fileLoadContext.PushFilePath(filePath);
            settings.m_metadata.Add(fileLoadContext);

            JsonSerialization::Load(material, value, settings);

            if (reportingHelper.ErrorsReported())
            {
                return AZ::Failure();
            }
            else if (reportingHelper.WarningsReported())
            {
                AZ_Error(MaterialBuilderName, false, "Warnings reported while loading '%s'", filePath.c_str());
                return AZ::Failure();
            }
            else
            {
                return AZ::Success(AZStd::move(material));
            }
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
                    auto materialTypeSourceData = MaterialUtils::LoadMaterialTypeSourceData(fullSourcePath, &document);

                    if (!materialTypeSourceData.IsSuccess())
                    {
                        return;
                    }

                    for (auto& shader : materialTypeSourceData.GetValue().m_shaderCollection)
                    {
                        AddPossibleDependencies(request.m_sourceFile,
                            shader.m_shaderFilePath,
                            "Shader Asset",
                            outputJobDescriptor.m_jobDependencyList);
                    }

                    for (auto& functor : materialTypeSourceData.GetValue().m_materialFunctorSourceData)
                    {
                        auto dependencies = functor->GetActualSourceData()->GetAssetDependencies();

                        for (const MaterialFunctorSourceData::AssetDependency& dependency : dependencies)
                        {
                            AddPossibleDependencies(request.m_sourceFile,
                                dependency.m_sourceFilePath,
                                dependency.m_jobKey.c_str(),
                                outputJobDescriptor.m_jobDependencyList);
                        }
                    }
                }
                else // it's a .material file
                {
                    // Note we don't use the LoadMaterial() utility function or JsonSerializer here because we don't care about fully
                    // processing the material file at this point and reporting on the many things that could go wrong. We just want
                    // to report the parent material and material type dependencies. So using rapidjson directly is actually simpler.

                    AZStd::string materialTypePath;
                    AZStd::string parentMaterialPath;

                    auto& variantData = document;

                    const char* const materialTypeField = "materialType";
                    const char* const parentMaterialField = "parentMaterial";

                    if (variantData.IsObject() && variantData.HasMember(materialTypeField) && variantData[materialTypeField].IsString())
                    {
                        materialTypePath = variantData[materialTypeField].GetString();
                    }

                    if (variantData.IsObject() && variantData.HasMember(parentMaterialField) && variantData[parentMaterialField].IsString())
                    {
                        parentMaterialPath = variantData[parentMaterialField].GetString();
                    }

                    if (parentMaterialPath.empty())
                    {
                        parentMaterialPath = materialTypePath;
                    }

                    // If includeMaterialPropertyNames is false, then a job dependency is needed so the material builder can validate MaterialAsset properties
                    // against the MaterialTypeAsset at asset build time.
                    // If includeMaterialPropertyNames is true, the material properties will be validated at runtime when the material is loaded, so the job dependency
                    // is needed only for first-time processing to set up the initial MaterialAsset. This speeds up AP processing time when a materialtype file
                    // is edited (e.g. 10s when editing StandardPBR.materialtype on AtomTest project from 45s).
                    bool includeMaterialPropertyNames = true;
                    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                    {
                        settingsRegistry->Get(includeMaterialPropertyNames, "/O3DE/Atom/RPI/MaterialBuilder/IncludeMaterialPropertyNames");
                    }

                    // Register dependency on the parent material source file so we can load it and use it's data to build this variant material.
                    // Note, we don't need a direct dependency on the material type because the parent material will depend on it.
                    AddPossibleDependencies(request.m_sourceFile,
                        parentMaterialPath,
                        JobKey,
                        outputJobDescriptor.m_jobDependencyList,
                        includeMaterialPropertyNames);
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

        AZ::Data::Asset<MaterialTypeAsset> CreateMaterialTypeAsset(AZStd::string_view materialTypeSourceFilePath, const rapidjson::Value& json)
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

        AZ::Data::Asset<MaterialAsset> CreateMaterialAsset(AZStd::string_view materialSourceFilePath, const rapidjson::Value& json)
        {
            auto material = LoadSourceData<MaterialSourceData>(json, materialSourceFilePath);

            if (!material.IsSuccess())
            {
                return {};
            }

            if (MaterialSourceData::ApplyVersionUpdatesResult::Failed == material.GetValue().ApplyVersionUpdates(materialSourceFilePath))
            {
                return {};
            }

            auto materialAssetOutcome = material.GetValue().CreateMaterialAsset(Uuid::CreateRandom(), materialSourceFilePath, true);
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
            AZStd::string fileNameNoExt;
            AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), fileNameNoExt);

            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileNameNoExt.c_str(), materialProductPath, true);
            AzFramework::StringFunc::Path::ReplaceExtension(materialProductPath, isMaterialTypeFile ? MaterialTypeAsset::Extension : MaterialAsset::Extension);

            if (isMaterialTypeFile)
            {
                // Load the material type file and create the MaterialTypeAsset object
                AZ::Data::Asset<MaterialTypeAsset> materialTypeAsset;
                materialTypeAsset = CreateMaterialTypeAsset(request.m_sourceFile, document);

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
                if (!AssetBuilderSDK::OutputObject(materialTypeAsset.Get(), materialProductPath, azrtti_typeid<RPI::MaterialTypeAsset>(), 0, jobProduct))
                {
                    AZ_Error(MaterialBuilderName, false, "Failed to output product dependencies.");
                    return;
                }

                response.m_outputProducts.push_back(AZStd::move(jobProduct));

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
