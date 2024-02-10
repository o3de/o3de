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

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Debug/TraceContext.h>

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
            materialBuilderDescriptor.m_version = 50; // Using ordered map for shader templates to fix unstable assets sub IDs
            materialBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtype", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            materialBuilderDescriptor.m_busId = azrtti_typeid<MaterialTypeBuilder>();
            materialBuilderDescriptor.m_createJobFunction = AZStd::bind(&MaterialTypeBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            materialBuilderDescriptor.m_processJobFunction = AZStd::bind(&MaterialTypeBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            materialBuilderDescriptor.m_analysisFingerprint += m_pipelineStage.GetBuilderSettingsFingerprint();
            materialBuilderDescriptor.m_analysisFingerprint += m_finalStage.GetBuilderSettingsFingerprint();

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
            return AZStd::string::format("[MaterialPipelineList %s]", materialPipelinePathString.c_str());
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

            AZStd::string materialTypeSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(
                request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), materialTypeSourcePath, true);

            // List of JSON file paths imported as part of the material type source data. These will be added as source dependencies as well
            // as used to update the fingerprint for the job.
            MaterialUtils::ImportedJsonFiles importedJsonFiles;

            auto materialTypeSourceDataOutcome =
                MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourcePath, nullptr, &importedJsonFiles);
            if (!materialTypeSourceDataOutcome)
            {
                AZ_Error(MaterialTypeBuilderName, false, "Failed to load material type source data: %s", materialTypeSourcePath.c_str());
                return;
            }

            auto materialTypeSourceData = materialTypeSourceDataOutcome.TakeValue();
            switch (materialTypeSourceData.GetFormat())
            {
            case MaterialTypeSourceData::Format::Abstract:
                m_pipelineStage.CreateJobsHelper(request, response, materialTypeSourcePath, materialTypeSourceData);
                break;
            case MaterialTypeSourceData::Format::Direct:
                m_finalStage.CreateJobsHelper(request, response, materialTypeSourcePath, materialTypeSourceData);
                break;
            default:
                AZ_Error(MaterialTypeBuilderName, false, "Failed to create job for material type source data with invalid format: %s", materialTypeSourcePath.c_str());
                return;
            }

            // Registering source dependencies for imported JSON files.
            for (const auto& importedJsonFile : importedJsonFiles)
            {
                AssetBuilderSDK::SourceFileDependency sourceDependency;
                sourceDependency.m_sourceFileDependencyPath = importedJsonFile.Native();
                response.m_sourceFileDependencyList.emplace_back(AZStd::move(sourceDependency));

                // Updating fingerprint to account for imported JSON dependencies.
                for (auto& outputJobDescriptor : response.m_createJobOutputs)
                {
                    MaterialBuilderUtils::AddFingerprintForDependency(importedJsonFile.Native(), outputJobDescriptor);
                }
            }
        }

        void MaterialTypeBuilder::PipelineStage::CreateJobsHelper(
            [[maybe_unused]] const AssetBuilderSDK::CreateJobsRequest& request,
            AssetBuilderSDK::CreateJobsResponse& response,
            const AZStd::string& materialTypeSourcePath,
            const MaterialTypeSourceData& materialTypeSourceData) const
        {
            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = PipelineStageJobKey;
            outputJobDescriptor.m_additionalFingerprintInfo = GetBuilderSettingsFingerprint();
            outputJobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);

            MaterialBuilderUtils::AddFingerprintForDependency(materialTypeSourcePath, outputJobDescriptor);

            auto addPossibleDependencies = [&response, &outputJobDescriptor](const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath)
            {
                auto& sourceDependency = response.m_sourceFileDependencyList.emplace_back();
                sourceDependency.m_sourceFileDependencyPath =
                    RPI::AssetUtils::ResolvePathReference(originatingSourceFilePath, referencedSourceFilePath);
                MaterialBuilderUtils::AddFingerprintForDependency(sourceDependency.m_sourceFileDependencyPath, outputJobDescriptor);
            };

            // Add dependencies for the material type file
            // Even though the materialAzsliFilePath will be #included into the generated .azsl file, which would normally be handled by the
            // final stage builder, we still need a source dependency on this file because PipelineStage::ProcessJobHelper tries to resolve
            // the path and fails if it can't be found.
            addPossibleDependencies(materialTypeSourcePath, materialTypeSourceData.m_materialShaderCode);

            // Note we report dependencies based on GetMaterialPipelinePaths() rather than LoadMaterialPipelines(), because dependencies are
            // needed even for pipelines that fail to load, so that the job will re-process when the broken pipeline gets fixed.
            for (const auto& materialPipelineFilePath : GetMaterialPipelinePaths())
            {
                addPossibleDependencies(materialTypeSourcePath, materialPipelineFilePath);
            }

            // Add dependencies for each material pipeline, since the output of this builder is a combination of the .materialtype data and the .materialpipeline data.
            for (const auto& [materialPipelineFilePath, materialPipeline] : LoadMaterialPipelines())
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

        void MaterialTypeBuilder::FinalStage::CreateJobsHelper(
            const AssetBuilderSDK::CreateJobsRequest& request,
            AssetBuilderSDK::CreateJobsResponse& response,
            const AZStd::string& materialTypeSourcePath,
            const MaterialTypeSourceData& materialTypeSourceData) const
        {
            // We'll build up this one JobDescriptor and reuse it to register each of the platforms
            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = FinalStageJobKey;
            outputJobDescriptor.m_additionalFingerprintInfo = GetBuilderSettingsFingerprint();

            MaterialBuilderUtils::AddFingerprintForDependency(materialTypeSourcePath, outputJobDescriptor);

            for (const auto& shader : materialTypeSourceData.GetShaderReferences())
            {
                MaterialBuilderUtils::AddJobDependency(
                    outputJobDescriptor, AssetUtils::ResolvePathReference(materialTypeSourcePath, shader.m_shaderFilePath), "Shader Asset");
            }

            auto addFunctorDependencies = [&outputJobDescriptor, &materialTypeSourcePath](
                                              const AZStd::vector<Ptr<MaterialFunctorSourceDataHolder>>& functors)
            {
                for (const auto& functor : functors)
                {
                    for (const MaterialFunctorSourceData::AssetDependency& dependency :
                         functor->GetActualSourceData()->GetAssetDependencies())
                    {
                        MaterialBuilderUtils::AddJobDependency(
                            outputJobDescriptor,
                            AssetUtils::ResolvePathReference(materialTypeSourcePath, dependency.m_sourceFilePath),
                            dependency.m_jobKey);
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
                [&outputJobDescriptor, &materialTypeSourcePath](const MaterialPropertySourceData* property, const MaterialNameContext&)
                {
                    if (property->m_dataType == MaterialPropertyDataType::Image &&
                        MaterialUtils::LooksLikeImageFileReference(property->m_value))
                    {
                        MaterialBuilderUtils::AddPossibleImageDependencies(
                            materialTypeSourcePath, property->m_value.GetValue<AZStd::string>(), outputJobDescriptor);
                    }
                    return true;
                });

            for (const auto& pipelinePair : materialTypeSourceData.m_pipelineData)
            {
                addFunctorDependencies(pipelinePair.second.m_materialFunctorSourceData);
            }

            // Duplicating output job descriptors for each platform
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

            AZStd::string materialTypeSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(
                request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), materialTypeSourcePath, true);

            auto materialTypeSourceDataOutcome = MaterialUtils::LoadMaterialTypeSourceData(materialTypeSourcePath, nullptr, nullptr);
            if (!materialTypeSourceDataOutcome)
            {
                AZ_Error(MaterialTypeBuilderName, false, "Failed to load material type source data: %s", materialTypeSourcePath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            auto materialTypeSourceData = materialTypeSourceDataOutcome.TakeValue();
            switch (materialTypeSourceData.GetFormat())
            {
            case MaterialTypeSourceData::Format::Abstract:
                m_pipelineStage.ProcessJobHelper(request, response, materialTypeSourcePath, materialTypeSourceData);
                break;
            case MaterialTypeSourceData::Format::Direct:
                m_finalStage.ProcessJobHelper(request, response, materialTypeSourcePath, materialTypeSourceData);
                break;
            default:
                AZ_Error(MaterialTypeBuilderName, false, "Failed to process job for material type source data with invalid format: %s", materialTypeSourcePath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
        }

        AZStd::set<AZStd::string> MaterialTypeBuilder::PipelineStage::GetMaterialPipelinePaths() const
        {
            AZStd::set<AZStd::string> combinedMaterialPipelines;

            auto ResolvePathAndAddToReturnValue = [&](const AZStd::string& path)
            {
                AZ::IO::FixedMaxPath pathWithoutAlias;
                AZ::IO::FileIOBase::GetInstance()->ResolvePath(pathWithoutAlias, AZ::IO::PathView{ path });
                combinedMaterialPipelines.insert(pathWithoutAlias.StringAsPosix());
            };

            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZStd::vector<AZStd::string> defaultMaterialPipelines;
                settingsRegistry->GetObject(defaultMaterialPipelines, "/O3DE/Atom/RPI/MaterialPipelineFiles");
                AZStd::for_each(defaultMaterialPipelines.begin(), defaultMaterialPipelines.end(), ResolvePathAndAddToReturnValue);

                AZStd::map<AZStd::string, AZStd::vector<AZStd::string>> gemMaterialPipelines;
                settingsRegistry->GetObject(gemMaterialPipelines, "/O3DE/Atom/RPI/MaterialPipelineFilesByGem");
                for (const auto& [_ /*gemName*/, gemMaterialPipelinePaths] : gemMaterialPipelines)
                {
                    AZStd::for_each(gemMaterialPipelinePaths.begin(), gemMaterialPipelinePaths.end(), ResolvePathAndAddToReturnValue);
                }
            }

            return combinedMaterialPipelines;
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

        void MaterialTypeBuilder::PipelineStage::ProcessJobHelper(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response,
            const AZStd::string& materialTypeSourcePath,
            MaterialTypeSourceData& materialTypeSourceData) const
        {
            AZ::u32 nextProductSubID = MaterialTypeSourceData::IntermediateMaterialTypeSubId + 1;

            const AZStd::string materialTypeName = AZ::IO::Path{ materialTypeSourcePath }.Stem().Native();

            const AZStd::map<AZ::IO::Path, MaterialPipelineSourceData> materialPipelines = LoadMaterialPipelines();

            // A list of pointers to lists
            // Each leaf element is a line that will be included in the object SRG of every shader
            // This allows Material Pipelines to add members to object SRGs, for example a texture space shading
            // pipeline can add a texture index to the Object SRG so the object can refer to it's lighting texture.
            AZStd::vector<const AZStd::vector<AZStd::string>*> objectSrgAdditionsFromMaterialPipelines;

            // Some shader templates may be reused by multiple pipelines, so first collect a full picture of all the dependencies
            AZStd::map<MaterialPipelineSourceData::ShaderTemplate, AZStd::vector<Name/*materialPipielineName*/>> shaderTemplateReferences;
            {
                bool foundProblems = false;

                MaterialPipelineScriptRunner scriptRunner;

                for (const auto& [materialPipelineFilePath, materialPipeline] : materialPipelines)
                {
                    AZ_TraceContext("Material Pipeline", materialPipelineFilePath.c_str());

                    if (!scriptRunner.RunScript(materialPipelineFilePath, materialPipeline, materialTypeSourceData))
                    {
                        // Error messages were be reported by RunScript, no need to report them here.
                        foundProblems = true;
                        continue;
                    }

                    const Name materialPipelineName = GetMaterialPipelineName(materialPipelineFilePath);

                    const MaterialPipelineScriptRunner::ShaderTemplatesList& shaderTemplateList = scriptRunner.GetRelevantShaderTemplates();

                    for (const MaterialPipelineSourceData::ShaderTemplate& shaderTemplate : shaderTemplateList)
                    {
                        AZ_TraceContext("Shader Template", shaderTemplate.m_shader.c_str());

                        // We need to normalize the content of the ShaderTemplate structure since it will be used as the key in the map.
                        // We also check for missing files now, where the original relative path is available for use in the error message.

                        MaterialPipelineSourceData::ShaderTemplate normalizedShaderTemplate = shaderTemplate;

                        auto resolveTemplateFilePathReference = [&foundProblems](const AZ::IO::Path& materialPipelineFilePath, AZStd::string& templateFilePath)
                        {
                            const AZStd::string resolvedFilePath =
                                AssetUtils::ResolvePathReference(materialPipelineFilePath.Native(), templateFilePath);

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

                    // Only include Object SRGs from material pipelines which have shader templates relevant to this material
                    // This avoids adding extra SRG members in materials for MaterialPipelines it won't be rendered in
                    if (!shaderTemplateList.empty())
                    {
                        objectSrgAdditionsFromMaterialPipelines.push_back(&materialPipeline.m_objectSrgAdditions);
                    }
                }

                if (foundProblems)
                {
                    return;
                }
            }

            // The new material type will no longer be abstract, we remove the reference to the partial
            // material shader code and will replace it below with a concrete shader asset list.
            const AZ::IO::FixedMaxPath materialAzsliFilePath(
                AssetUtils::ResolvePathReference(materialTypeSourcePath, materialTypeSourceData.m_materialShaderCode));
            if (!AZ::IO::LocalFileIO::GetInstance()->Exists(materialAzsliFilePath.c_str()))
            {
                AZ_Error(MaterialTypeBuilderName, false, "File is missing: '%s'", materialTypeSourceData.m_materialShaderCode.c_str());
                return;
            }

            materialTypeSourceData.m_materialShaderCode.clear();
            materialTypeSourceData.m_lightingModel.clear();
            // These should already be clear, but just in case
            materialTypeSourceData.m_shaderCollection.clear(); 
            materialTypeSourceData.m_pipelineData.clear();

            u32 commonCounter = 0;

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
                    // The common name is appended with an incrementing value to avoid naming conflicts. Naming conflicts
                    // happen if Pipeline A and Pipeline B include shader X and Pipeline C and Pipeline D include shader Y,
                    // and X and Y have the same name (for example depth.shader.template).
                    materialPipelineIndicator = AZStd::string::format("%s_%u", PipelineNameForCommonShaders, commonCounter++);
                }
                else
                {
                    // AZ_Assert, not AZ_Error, because it shouldn't be possible to get here since the loop that filled
                    // shaderTemplateReferences should always put at least one pipeline in the list.
                    AZ_Assert(false, "There should be at least one material pipeline referencing the shader");
                    return;
                }

                AZ::RPI::ShaderSourceData shaderSourceData;
                if (!AZ::RPI::JsonUtils::LoadObjectFromFile(shaderTemplate.m_shader, shaderSourceData))
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to load shader template file '%s'.", shaderTemplate.m_shader.c_str());
                    return;
                }

                // Intermediate azsl file
                AZStd::string generatedAzsl;
                generatedAzsl += AZStd::string::format("// This code was generated by %s. Do not modify.\n", MaterialTypeBuilderName);

                // Generate the #define that will include new object srg members that were specified in the material pipelines 
                generatedAzsl += AZStd::string::format("#define MATERIAL_PIPELINE_OBJECT_SRG_MEMBERS   \\\n");

                for (const AZStd::vector<AZStd::string>* perMaterialPipelineAdditions : objectSrgAdditionsFromMaterialPipelines)
                {
                    for (const AZStd::string& objectSrgAddition : *perMaterialPipelineAdditions)
                    {
                        generatedAzsl += AZStd::string::format("%s;   \\\n", objectSrgAddition.c_str());
                    }
                }

                generatedAzsl += AZStd::string::format("\n");

                // At this point m_azsli should be absolute due to ResolvePathReference() being called above.
                // It might be better for the include path to be relative to the generated .shader file path in the intermediate cache,
                // so the project could be renamed or moved without having to rebuild the cache. But there's a good chance that moving
                // the project would require a rebuild of the cache anyway.
                const AZ::IO::PathView materialAzsliFilePathView{ materialAzsliFilePath };
                generatedAzsl += AZStd::string::format("#define MATERIAL_TYPE_AZSLI_FILE_PATH \"%s\" \n", materialAzsliFilePathView.StringAsPosix().c_str());
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
                    response.m_outputProducts.emplace_back(AZStd::move(product));
                }
                else
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to write intermediate azsl file '%s'.", outputAzslFilePath.c_str());
                    return;
                }

                // Intermediate shader file

                AZStd::string azslFileReference = AZ::IO::Path{ outputAzslFilePath.Filename() }.AsPosix();
                AZStd::to_lower(azslFileReference.begin(), azslFileReference.end());
                shaderSourceData.m_source = azslFileReference.c_str();

                AZ::IO::Path outputShaderFilePath = request.m_tempDirPath;
                outputShaderFilePath /= AZStd::string::format("%s_%s_%s.shader", materialTypeName.c_str(), materialPipelineIndicator.c_str(), shaderName.c_str());

                if (AZ::RPI::JsonUtils::SaveObjectToFile(outputShaderFilePath.c_str(), shaderSourceData))
                {
                    AssetBuilderSDK::JobProduct product;
                    product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                    product.m_dependenciesHandled = true;
                    product.m_productFileName = outputShaderFilePath.String();
                    product.m_productSubID = nextProductSubID++;
                    response.m_outputProducts.emplace_back(AZStd::move(product));
                }
                else
                {
                    AZ_Error(MaterialTypeBuilderName, false, "Failed to write intermediate shader file '%s'.", outputShaderFilePath.c_str());
                    return;
                }

                // Add shader to intermediate material type, for each pipeline.

                for (const Name& materialPipelineName : materialPipelineList)
                {
                    MaterialTypeSourceData::MaterialPipelineState& pipelineData = materialTypeSourceData.m_pipelineData[materialPipelineName];

                    MaterialTypeSourceData::ShaderVariantReferenceData shaderVariantReferenceData;
                    shaderVariantReferenceData.m_shaderFilePath = AZ::IO::Path{ outputShaderFilePath.Filename() }.c_str();
                    shaderVariantReferenceData.m_shaderTag = shaderTemplate.m_shaderTag;

                    // Files in the cache, including intermediate files, end up using lower case for all files and folders. We have to match this
                    // in the output .materialtype file, because the asset system's source dependencies are case-sensitive on some platforms.
                    AZStd::to_lower(shaderVariantReferenceData.m_shaderFilePath.begin(), shaderVariantReferenceData.m_shaderFilePath.end());
                    pipelineData.m_shaderCollection.emplace_back(AZStd::move(shaderVariantReferenceData));
                }

                // TODO(MaterialPipeline): We should warn the user if the shader collection has multiple shaders that use the same draw list.
            }

            // Sort the shader file reference just for convenience, for when the user inspects the intermediate .materialtype file
            for (auto& pipelineDataPair : materialTypeSourceData.m_pipelineData)
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
                MaterialTypeSourceData::MaterialPipelineState& pipelineData = materialTypeSourceData.m_pipelineData[materialPipelineName];
                pipelineData.m_materialFunctorSourceData = materialPipeline.m_runtimeControls.m_materialFunctorSourceData;
                pipelineData.m_pipelinePropertyLayout = materialPipeline.m_runtimeControls.m_materialTypeInternalProperties;
            }

            // Convert all texture references to aliases in case there are any paths relative to the original, abstract material type. If
            // these paths remain relative to the original material type then they cannot be resolved and will not load with the final
            // material type. That will fail the build.
            materialTypeSourceData.EnumerateProperties(
                [&materialTypeSourcePath](const MaterialPropertySourceData* property, const MaterialNameContext&)
                {
                    if (property->m_dataType == MaterialPropertyDataType::Image &&
                        MaterialUtils::LooksLikeImageFileReference(property->m_value))
                    {
                        if (auto fileIOBase = AZ::IO::FileIOBase::GetInstance())
                        {
                            const AZStd::string originalPath = property->m_value.GetValue<AZStd::string>();
                            const AZStd::string absolutePath = RPI::AssetUtils::ResolvePathReference(materialTypeSourcePath, originalPath);
                            if (const auto aliasedPathResult = fileIOBase->ConvertToAlias(AZ::IO::PathView{ absolutePath }))
                            {
                                // Using const_cast because EnumerateProperties is currently only implemented as const.
                                const_cast<MaterialPropertySourceData*>(property)->m_value = aliasedPathResult->LexicallyNormal().String();
                            }
                        }
                    }
                    return true;
                });

            // The "_generated" postfix is necessary because AP does not allow intermediate file to have the same relative path as a source
            // file.
            AZ::IO::Path outputMaterialTypeFilePath = request.m_tempDirPath;
            outputMaterialTypeFilePath /= AZStd::string::format("%s_generated.materialtype", materialTypeName.c_str());

            AZ_Assert(materialTypeSourceData.GetFormat() != MaterialTypeSourceData::Format::Abstract,
                "The output material type must not use the abstract format, this will likely cause the '%s' job to run in an infinite loop.", PipelineStageJobKey);

            if (JsonUtils::SaveObjectToFile(outputMaterialTypeFilePath.String(), materialTypeSourceData))
            {
                AssetBuilderSDK::JobProduct product;
                product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                product.m_dependenciesHandled = true;
                product.m_productFileName = outputMaterialTypeFilePath.String();
                product.m_productAssetType = azrtti_typeid<RPI::MaterialTypeSourceData>();
                product.m_productSubID = MaterialTypeSourceData::IntermediateMaterialTypeSubId;
                response.m_outputProducts.emplace_back(AZStd::move(product));
            }
            else
            {
                AZ_Error(MaterialTypeBuilderName, false, "Failed to write intermediate material type file '%s'.", outputMaterialTypeFilePath.c_str());
                return;
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialTypeBuilder::FinalStage::ProcessJobHelper(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response,
            const AZStd::string& materialTypeSourcePath,
            const MaterialTypeSourceData& materialTypeSourceData) const
        {
            AZStd::string materialProductPath;
            AZStd::string fileName;
            AzFramework::StringFunc::Path::GetFileName(materialTypeSourcePath.c_str(), fileName);
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, MaterialTypeAsset::Extension);
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), materialProductPath, true);

            AZ::Data::Asset<MaterialTypeAsset> materialTypeAsset;

            {
                AZ_TraceContext("Product", fileName);
                AZ_TracePrintf(MaterialTypeBuilderName, AZStd::string::format("Producing %s...", fileName.c_str()).c_str());

                // Load the material type file and create the MaterialTypeAsset object
                auto materialTypeAssetOutcome =
                    materialTypeSourceData.CreateMaterialTypeAsset(Uuid::CreateRandom(), materialTypeSourcePath, true);
                if (!materialTypeAssetOutcome.IsSuccess())
                {
                    // Errors will have been reported above
                    return;
                }

                materialTypeAsset = materialTypeAssetOutcome.GetValue();
                if (!materialTypeAsset)
                {
                    // Errors will have been reported above
                    return;
                }

                if (!AZ::Utils::SaveObjectToFile(materialProductPath, AZ::DataStream::ST_BINARY, materialTypeAsset.Get()))
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

                response.m_outputProducts.emplace_back(AZStd::move(jobProduct));
            }

            if (ShouldOutputAllPropertiesMaterial())
            {
                AZStd::string defaultMaterialFileName;
                AzFramework::StringFunc::Path::GetFileName(materialTypeSourcePath.c_str(), defaultMaterialFileName);
                defaultMaterialFileName += "_AllProperties.json";

                AZStd::string defaultMaterialFilePath;
                AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), defaultMaterialFileName.c_str(), defaultMaterialFilePath, true);

                AZ_TraceContext("Product", defaultMaterialFileName);
                AZ_TracePrintf(MaterialTypeBuilderName, AZStd::string::format("Producing %s...", defaultMaterialFileName.c_str()).c_str());

                MaterialSourceData allPropertyDefaultsMaterial = MaterialSourceData::CreateAllPropertyDefaultsMaterial(materialTypeAsset, materialTypeSourcePath);

                if (!JsonUtils::SaveObjectToFile(defaultMaterialFilePath, allPropertyDefaultsMaterial))
                {
                    AZ_Warning(MaterialTypeBuilderName, false, "Failed to save material reference file '%s'!", defaultMaterialFilePath.c_str());
                }
                else
                {
                    AssetBuilderSDK::JobProduct defaultMaterialFileProduct;
                    defaultMaterialFileProduct.m_dependenciesHandled = true; // This product is only for reference, not used at runtime
                    defaultMaterialFileProduct.m_productFileName = defaultMaterialFilePath;
                    defaultMaterialFileProduct.m_productAssetType = AZ::Uuid::CreateString("{FE8E7122-9E96-44F0-A4E4-F134DD9804E2}"); // Need a unique asset type for this raw JSON file
                    defaultMaterialFileProduct.m_productSubID = (u32)MaterialTypeProductSubId::AllPropertiesMaterialSourceFile;
                    response.m_outputProducts.emplace_back(AZStd::move(defaultMaterialFileProduct));
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
