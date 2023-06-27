/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialTypeBuilder.h"
#include "MaterialPipelineScriptRunner.h"
#include <Material/MaterialBuilderUtils.h>

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptAsset.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static constexpr char const MaterialTypeBuilderName[] = "MaterialTypeBuilder";

            //! Some shaders are used by multiple pipelines, so this name will be used in place of the pipeline name for the final shader filename.
            static constexpr char const PipelineNameForCommonShaders[] = "Common";
        }

        const char* MaterialTypeBuilder::PipelineStageJobKey = "Material Type Builder (Pipeline Stage)";
        const char* MaterialTypeBuilder::FinalStageJobKey = "Material Type Builder (Final Stage)";

        void MaterialTypeBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc materialBuilderDescriptor;
            materialBuilderDescriptor.m_name = "Material Type Builder";
            materialBuilderDescriptor.m_version = 38; // material type indirect references
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtype", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_busId = azrtti_typeid<MaterialTypeBuilder>();
            materialBuilderDescriptor.m_createJobFunction = AZStd::bind(&MaterialTypeBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            materialBuilderDescriptor.m_processJobFunction = AZStd::bind(&MaterialTypeBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            materialBuilderDescriptor.m_analysisFingerprint = m_pipelineStage.GetBuilderSettingsFingerprint() + m_finalStage.GetBuilderSettingsFingerprint();

            BusConnect(materialBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialBuilderDescriptor);
        }

        MaterialTypeBuilder::~MaterialTypeBuilder()
        {
            BusDisconnect();
        }

        AZStd::string MaterialTypeBuilder::PipelineStage::GetBuilderSettingsFingerprint() const
        {
            auto materialPipelinePaths = GetMaterialPipelinePaths();
            AZStd::string materialPipelinePathString;
            AzFramework::StringFunc::Join(materialPipelinePathString, materialPipelinePaths.begin(), materialPipelinePaths.end(), ", ");
            AZStd::string fingerprint = AZStd::string::format("[MaterialPipelineList %s]", materialPipelinePathString.c_str());
            return fingerprint;
        }

        AZStd::string MaterialTypeBuilder::FinalStage::GetBuilderSettingsFingerprint() const
        {
            return AZStd::string::format("[ShouldOutputAllPropertiesMaterial=%d]", ShouldOutputAllPropertiesMaterial());
        }

        bool MaterialTypeBuilder::FinalStage::ShouldOutputAllPropertiesMaterial() const
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
                sourceDependency.m_sourceFileDependencyPath = importedJsonFile.Native();
                response.m_sourceFileDependencyList.push_back(sourceDependency);
            }

            if (materialTypeSourceData.GetValue().GetFormat() == MaterialTypeSourceData::Format::Abstract)
            {
                m_pipelineStage.CreateJobsHelper(request, response, materialTypeSourceData.GetValue());
            }
            else
            {
                m_finalStage.CreateJobsHelper(request, response, materialTypeSourceData.GetValue());
            }
        }

        void MaterialTypeBuilder::PipelineStage::CreateJobsHelper(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response, const MaterialTypeSourceData& materialTypeSourceData) const
        {
            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = MaterialTypeBuilder::PipelineStageJobKey;
            outputJobDescriptor.m_additionalFingerprintInfo = GetBuilderSettingsFingerprint();
            outputJobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);

            auto addPossibleDependencies = [&response](const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath)
            {
                AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDependencyPaths(originatingSourceFilePath, referencedSourceFilePath);
                for (const AZStd::string& path : possibleDependencies)
                {
                    response.m_sourceFileDependencyList.push_back({});
                    response.m_sourceFileDependencyList.back().m_sourceFileDependencyPath = path;
                }
            };

            // Add dependencies for the material type file
            {
                // Even though the materialAzsliFilePath will be #included into the generated .azsl file, which would normally be handled by the final stage builder, we still need
                // a source dependency on this file because PipelineStage::ProcessJobHelper tries to resolve the path and fails if it can't be found.
                addPossibleDependencies(request.m_sourceFile, materialTypeSourceData.m_materialShaderCode);
            }

            // Note we report dependencies based on GetMaterialPipelinePaths() rather than LoadMaterialPipelines(), because dependencies are
            // needed even for pipelines that fail to load, so that the job will re-process when the broken pipeline gets fixed.
            for (AZ::IO::Path materialPipelineFilePath : GetMaterialPipelinePaths())
            {
                // We have to resolve the path to remove any path aliases like @projectroot@ because source dependencies don't support those.
                auto resolvedMaterialPipelineFilePath = AZ::IO::LocalFileIO::GetInstance()->ResolvePath(materialPipelineFilePath);
                if (!resolvedMaterialPipelineFilePath.has_value())
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Could not resolve path '%s'", materialPipelineFilePath.c_str());
                    return;
                }

                response.m_sourceFileDependencyList.push_back({});
                response.m_sourceFileDependencyList.back().m_sourceFileDependencyPath = resolvedMaterialPipelineFilePath.value().Native();
            }

            // Add dependencies for each material pipeline, since the output of this builder is a combination of the .materialtype data and the .materialpipeline data.
            AZStd::map<AZ::IO::Path, MaterialPipelineSourceData> materialPipelines = LoadMaterialPipelines();
            for (const auto& [materialPipelineFilePath, materialPipeline] : materialPipelines)
            {
                for (const MaterialPipelineSourceData::ShaderTemplate& shaderTemplate : materialPipeline.m_shaderTemplates)
                {
                    addPossibleDependencies(materialPipelineFilePath.Native(), shaderTemplate.m_shader);

                    // Even though the AZSLi file will be #included into the generated .azsl file, which would normally be handled by the final stage builder, we still need
                    // a source dependency on this file because PipelineStage::ProcessJobHelper tries to resolve the path and fails if it can't be found.
                    addPossibleDependencies(materialPipelineFilePath.Native(), shaderTemplate.m_azsli);
                }

                if (!materialPipeline.m_pipelineScript.empty())
                {
                    addPossibleDependencies(materialPipelineFilePath.Native(), materialPipeline.m_pipelineScript);
                }
            }

            response.m_createJobOutputs.push_back(outputJobDescriptor);

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void MaterialTypeBuilder::FinalStage::CreateJobsHelper(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response, const MaterialTypeSourceData& materialTypeSourceData) const
        {
            // We'll build up this one JobDescriptor and reuse it to register each of the platforms
            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = FinalStageJobKey;
            outputJobDescriptor.m_additionalFingerprintInfo = GetBuilderSettingsFingerprint();

            for (auto& shader : materialTypeSourceData.m_shaderCollection)
            {
                MaterialBuilderUtils::AddPossibleDependencies(
                    request.m_sourceFile,
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
                        MaterialBuilderUtils::AddPossibleDependencies(
                            request.m_sourceFile,
                            dependency.m_sourceFilePath,
                            dependency.m_jobKey.c_str(),
                            outputJobDescriptor.m_jobDependencyList,
                            response.m_sourceFileDependencyList);
                    }
                }
            };

            addFunctorDependencies(materialTypeSourceData.m_materialFunctorSourceData);

            materialTypeSourceData.EnumeratePropertyGroups([addFunctorDependencies](const MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
                {
                    addFunctorDependencies(propertyGroupStack.back()->GetFunctors());
                    return true;
                });

            materialTypeSourceData.EnumerateProperties(
                [&request, &response, &outputJobDescriptor](const MaterialPropertySourceData* property, const MaterialNameContext&)
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

            for (const auto& pipelinePair : materialTypeSourceData.m_pipelineData)
            {
                for (auto& shader : pipelinePair.second.m_shaderCollection)
                {
                    MaterialBuilderUtils::AddPossibleDependencies(
                        request.m_sourceFile,
                        shader.m_shaderFilePath,
                        "Shader Asset",
                        outputJobDescriptor.m_jobDependencyList,
                        response.m_sourceFileDependencyList,
                        false,
                        0);
                }

                addFunctorDependencies(pipelinePair.second.m_materialFunctorSourceData);
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
                return {};
            }

            auto materialTypeAssetOutcome = materialType.GetValue().CreateMaterialTypeAsset(Uuid::CreateRandom(), materialTypeSourceFilePath, true);
            if (!materialTypeAssetOutcome.IsSuccess())
            {
                return {};
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

            if (request.m_jobDescription.m_jobKey == PipelineStageJobKey)
            {
                m_pipelineStage.ProcessJobHelper(request, response);
            }
            else if (request.m_jobDescription.m_jobKey == FinalStageJobKey)
            {
                m_finalStage.ProcessJobHelper(request, response);
            }
            else
            {
                AZ_Error(MaterialTypeBuilderName, false, "Invalid material type job key.");
            }
        }

        AZStd::vector<AZStd::string> MaterialTypeBuilder::PipelineStage::GetMaterialPipelinePaths() const
        {
            AZStd::vector<AZStd::string> materialPipelines;

            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->GetObject(materialPipelines, "/O3DE/Atom/RPI/MaterialPipelineFiles");
            }

            return materialPipelines;
        }

        AZStd::map<AZ::IO::Path, MaterialPipelineSourceData> MaterialTypeBuilder::PipelineStage::LoadMaterialPipelines() const
        {
            AZStd::map<AZ::IO::Path, MaterialPipelineSourceData> materialPipelines;

            for (const AZStd::string& file : GetMaterialPipelinePaths())
            {
                auto loadResult = MaterialUtils::LoadMaterialPipelineSourceData(file.c_str());
                if (!loadResult.IsSuccess())
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to load '%s'.", file.c_str());
                    continue;
                }

                materialPipelines.emplace(file, loadResult.TakeValue());
            }

            return materialPipelines;
        }

        Name MaterialTypeBuilder::PipelineStage::GetMaterialPipelineName(const AZ::IO::Path& materialPipelineFilePath) const
        {
            return Name{materialPipelineFilePath.Stem().Native()};
        }

        void MaterialTypeBuilder::PipelineStage::ProcessJobHelper(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            const AZStd::string materialTypeName = AZ::IO::Path{request.m_sourceFile}.Stem().Native();

            AZ::u32 nextProductSubID = MaterialTypeSourceData::IntermediateMaterialTypeSubId + 1;

            auto materialTypeLoadResult = MaterialUtils::LoadMaterialTypeSourceData(request.m_fullPath);
            if (!materialTypeLoadResult.IsSuccess())
            {
                AZ_Error(MaterialTypeBuilderName, false, "Failed to load material type file.");
                return;
            }

            MaterialTypeSourceData materialType = materialTypeLoadResult.TakeValue();

            AZStd::map<AZ::IO::Path, MaterialPipelineSourceData> materialPipelines = LoadMaterialPipelines();

            // Some shader templates may be reused by multiple pipelines, so first collect a full picture of all the dependencies
            AZStd::unordered_map<MaterialPipelineSourceData::ShaderTemplate, AZStd::vector<Name/*materialPipielineName*/>> shaderTemplateReferences;
            {
                bool foundProblems = false;

                MaterialPipelineScriptRunner scriptRunner;

                for (const auto& [materialPipelineFilePath, materialPipeline] : materialPipelines)
                {
                    AZ_TraceContext("Material Pipeline", materialPipelineFilePath.c_str());

                    if (!scriptRunner.RunScript(materialPipelineFilePath, materialPipeline, materialType))
                    {
                        // Error messages were be reported by RunScript, no need to report them here.
                        foundProblems = true;
                        continue;
                    }

                    const Name materialPipelineName = GetMaterialPipelineName(materialPipelineFilePath);

                    for (const MaterialPipelineSourceData::ShaderTemplate& shaderTemplate : scriptRunner.GetRelevantShaderTemplates())
                    {
                        AZ_TraceContext("Shader Template", shaderTemplate.m_shader.c_str());

                        // We need to normalize the content of the ShaderTemplate structure since it will be used as the key in the map.
                        // We also check for missing files now, where the original relative path is available for use in the error message.

                        MaterialPipelineSourceData::ShaderTemplate normalizedShaderTemplate = shaderTemplate;

                        auto resolveTemplateFilePathReference = [&foundProblems](const AZ::IO::Path& materialPipelineFilePath, AZStd::string& templateFilePath)
                        {
                            AZStd::string resolvedFilePath = AssetUtils::ResolvePathReference(materialPipelineFilePath.Native(), templateFilePath);

                            if (!AZ::IO::LocalFileIO::GetInstance()->Exists(resolvedFilePath.c_str()))
                            {
                                AZ_Error(MaterialTypeBuilderName, false, "File is missing: '%s', referenced in '%s'", templateFilePath.c_str(), materialPipelineFilePath.Native().c_str());
                                foundProblems = true;
                            }

                            templateFilePath = resolvedFilePath;
                        };

                        resolveTemplateFilePathReference(materialPipelineFilePath, normalizedShaderTemplate.m_shader);
                        resolveTemplateFilePathReference(materialPipelineFilePath, normalizedShaderTemplate.m_azsli);

                        shaderTemplateReferences[normalizedShaderTemplate].push_back(materialPipelineName);
                    }
                }

                if (foundProblems)
                {
                    return;
                }
            }

            // The new material type will no longer be abstract, we remove the reference to the partial
            // material shader code and will replace it below with a concrete shader asset list.
            const AZStd::string materialAzsliFilePath = AssetUtils::ResolvePathReference(request.m_sourceFile, materialType.m_materialShaderCode);
            if (!AZ::IO::LocalFileIO::GetInstance()->Exists(materialAzsliFilePath.c_str()))
            {
                AZ_Error(MaterialTypeBuilderName, false, "File is missing: '%s'", materialType.m_materialShaderCode.c_str());
                return;
            }
            materialType.m_materialShaderCode.clear();
            materialType.m_lightingModel.clear();
            // These should already be clear, but just in case
            materialType.m_shaderCollection.clear(); 
            materialType.m_pipelineData.clear();

            // Generate the required shaders
            for (const auto& [shaderTemplate, materialPipelineList] : shaderTemplateReferences)
            {
                AZ_TraceContext("Shader Template", shaderTemplate.m_shader.c_str());

                AZStd::string materialPipelineIndicator;

                if (materialPipelineList.size() == 1)
                {
                    materialPipelineIndicator = materialPipelineList.begin()->GetCStr();
                }
                else if(materialPipelineList.size() > 1)
                {
                    // Multiple material pipelines reference the same shader, so it should have a generic common name.
                    materialPipelineIndicator = PipelineNameForCommonShaders;
                }
                else
                {
                    // AZ_Assert, not AZ_Error, because it shouldn't be possible to get here since the loop that filled
                    // shaderTemplateReferences should always put at least one pipeline in the list.
                    AZ_Assert(false, "There should be at least one material pipeline referencing the shader");
                    return;
                }

                auto shaderFile = AZ::Utils::ReadFile<AZStd::string>(shaderTemplate.m_shader);
                if (!shaderFile.IsSuccess())
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to load shader template file '%s'. %s", shaderTemplate.m_shader.c_str(), shaderFile.GetError().c_str());
                    return;
                }


                // Intermediate azsl file

                // At this point m_azsli should be absolute due to ResolvePathReference() being called above.
                // It might be better for the include path to be relative to the generated .shader file path in the intermediate cache,
                // so the project could be renamed or moved without having to rebuild the cache. But there's a good chance that moving
                // the project would require a rebuild of the cache anyway.

                AZStd::string generatedAzsl = AZStd::string::format("// This code was generated by %s. Do not modify.\n", MaterialTypeBuilderName);
                generatedAzsl += AZStd::string::format("#define MATERIAL_TYPE_AZSLI_FILE_PATH \"%s\" \n", materialAzsliFilePath.c_str());
                generatedAzsl += AZStd::string::format("#include \"%s\" \n", shaderTemplate.m_azsli.c_str());

                AZ::IO::Path shaderName = shaderTemplate.m_shader;
                shaderName = shaderName.Filename(); // Removes the folder path
                shaderName = shaderName.ReplaceExtension(""); // This will remove the ".template" extension
                shaderName = shaderName.ReplaceExtension(""); // This will remove the ".shader" extension

                AZ::IO::Path outputAzslFilePath = request.m_tempDirPath;
                outputAzslFilePath /= AZStd::string::format("%s_%s_%s.azsl", materialTypeName.c_str(), materialPipelineIndicator.c_str(), shaderName.c_str());

                if (AZ::Utils::WriteFile(generatedAzsl, outputAzslFilePath.c_str()).IsSuccess())
                {
                    AssetBuilderSDK::JobProduct product;
                    product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                    product.m_dependenciesHandled = true;
                    product.m_productFileName = outputAzslFilePath.String();
                    product.m_productSubID = nextProductSubID++;
                    response.m_outputProducts.push_back(AZStd::move(product));
                }
                else
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to write intermediate azsl file '%s'.", outputAzslFilePath.c_str());
                    return;
                }

                // Intermediate shader file

                AZStd::string azslFileReference = AZ::IO::Path{outputAzslFilePath.Filename()}.c_str();
                AZStd::to_lower(azslFileReference.begin(), azslFileReference.end());
                AzFramework::StringFunc::Replace(shaderFile.GetValue(), "TEMPLATE_AZSL_PATH", azslFileReference.c_str());

                AZ::IO::Path outputShaderFilePath = request.m_tempDirPath;
                outputShaderFilePath /= AZStd::string::format("%s_%s_%s.shader", materialTypeName.c_str(), materialPipelineIndicator.c_str(), shaderName.c_str());

                if (AZ::Utils::WriteFile(shaderFile.GetValue(), outputShaderFilePath.c_str()).IsSuccess())
                {
                    AssetBuilderSDK::JobProduct product;
                    product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                    product.m_productFileName = outputShaderFilePath.String();
                    product.m_productSubID = nextProductSubID++;
                    response.m_outputProducts.push_back(AZStd::move(product));
                }
                else
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to write intermediate shader file '%s'.", outputShaderFilePath.c_str());
                    return;
                }

                // Add shader to intermediate material type, for each pipeline.

                for (const Name& materialPipelineName : materialPipelineList)
                {
                    MaterialTypeSourceData::MaterialPipelineState& pipelineData = materialType.m_pipelineData[materialPipelineName];
                    pipelineData.m_shaderCollection.push_back({});
                    pipelineData.m_shaderCollection.back().m_shaderFilePath = AZ::IO::Path{outputShaderFilePath.Filename()}.c_str();
                    pipelineData.m_shaderCollection.back().m_shaderTag = shaderTemplate.m_shaderTag;

                    // Files in the cache, including intermediate files, end up using lower case for all files and folders. We have to match this
                    // in the output .materialtype file, because the asset system's source dependencies are case-sensitive on some platforms.
                    AZStd::to_lower(pipelineData.m_shaderCollection.back().m_shaderFilePath.begin(), pipelineData.m_shaderCollection.back().m_shaderFilePath.end());
                }

                // TODO(MaterialPipeline): We should warn the user if the shader collection has multiple shaders that use the same draw list.
            }

            // Sort the shader file reference just for convenience, for when the user inspects the intermediate .materialtype file
            for (auto& pipelineDataPair : materialType.m_pipelineData)
            {
                AZStd::sort(pipelineDataPair.second.m_shaderCollection.begin(), pipelineDataPair.second.m_shaderCollection.end(),
                    [](const MaterialTypeSourceData::ShaderVariantReferenceData& a, const MaterialTypeSourceData::ShaderVariantReferenceData& b)
                    {
                        return a.m_shaderFilePath < b.m_shaderFilePath;
                    });
            }

            // Add the material pipeline functors
            for (const auto& [materialPipelineFilePath, materialPipeline] : materialPipelines)
            {
                const Name materialPipelineName = GetMaterialPipelineName(materialPipelineFilePath);
                materialType.m_pipelineData[materialPipelineName].m_materialFunctorSourceData = materialPipeline.m_runtimeControls.m_materialFunctorSourceData;
                materialType.m_pipelineData[materialPipelineName].m_pipelinePropertyLayout = materialPipeline.m_runtimeControls.m_materialTypeInternalProperties;
            }

            AZ::IO::Path outputMaterialTypeFilePath = request.m_tempDirPath;
            // The "_generated" postfix is necessary because AP does not allow intermediate file to have the same relative path as a source file.
            outputMaterialTypeFilePath /= AZStd::string::format("%s_generated.materialtype", materialTypeName.c_str());

            AZ_Assert(materialType.GetFormat() != MaterialTypeSourceData::Format::Abstract,
                "The output material type must not use the abstract format, this will likely cause the '%s' job to run in an infinite loop.", PipelineStageJobKey);

            if (JsonUtils::SaveObjectToFile(outputMaterialTypeFilePath.String(), materialType))
            {
                AssetBuilderSDK::JobProduct product;
                product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                product.m_productFileName = outputMaterialTypeFilePath.String();
                product.m_productAssetType = azrtti_typeid<RPI::MaterialTypeSourceData>();
                product.m_productSubID = MaterialTypeSourceData::IntermediateMaterialTypeSubId;
                response.m_outputProducts.push_back(AZStd::move(product));
            }
            else
            {
                AZ_Error(MaterialTypeBuilderName, false, "Failed to write intermediate material type file '%s'.", outputMaterialTypeFilePath.c_str());
                return;
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialTypeBuilder::FinalStage::ProcessJobHelper(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
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

            if (ShouldOutputAllPropertiesMaterial())
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
