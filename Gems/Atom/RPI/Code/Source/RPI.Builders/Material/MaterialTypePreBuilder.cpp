/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialTypePreBuilder.h"
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Utils/Utils.h>


namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static constexpr char const MaterialTypePreBuilderName[] = "MaterialTypePreBuilder";
        }

        const char* MaterialTypePreBuilder::JobKey = "Atom Material Type Pre-Builder";

        void MaterialTypePreBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = JobKey;
            builderDescriptor.m_version = 11; // Initial checkin
            // TODO: We won't actually use a different "ex" extension, this is just temporary. We will use the same ".materialtype" extension and just evaluate the content
            // to determine whether this is a stage1 or stage2 material type source file.
            builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.materialtypeex", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            builderDescriptor.m_busId = azrtti_typeid<MaterialTypePreBuilder>();
            builderDescriptor.m_createJobFunction = AZStd::bind(&MaterialTypePreBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(&MaterialTypePreBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);

            LoadMaterialPipelines();
        }

        MaterialTypePreBuilder::~MaterialTypePreBuilder()
        {
            BusDisconnect();
        }

        void MaterialTypePreBuilder::LoadMaterialPipelines()
        {
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                AZStd::vector<AZStd::string> materialPipelineFiles;
                settingsRegistry->GetObject(materialPipelineFiles, "/O3DE/Atom/RPI/MaterialPipelineFiles");

                for (const AZStd::string& file : materialPipelineFiles)
                {
                    auto loadResult = MaterialUtils::LoadMaterialPipelineSourceData(file.c_str());
                    if (loadResult.IsSuccess())
                    {
                        m_materialPipelines.emplace(file, loadResult.TakeValue());
                    }
                    else
                    {
                        AZ_Error(MaterialTypePreBuilderName, false, "Failed to load '%s'.", file.c_str());
                    }
                }
            }
        }

        void MaterialTypePreBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& /*request*/, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            AssetBuilderSDK::JobDescriptor outputJobDescriptor;
            outputJobDescriptor.m_jobKey = JobKey;
            outputJobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);
            response.m_createJobOutputs.push_back(outputJobDescriptor);

            // Note that we don't need to add the .materialtype's materialShaderCode as a source dependency because it's just going to be #included into the final azsl file.
            
            // Add dependencies on each material pipeline, since the output of this builder is a combination of the .materialtype data and the .materialpipeline data.
            for (const auto& [materialPipelineFilePath, materialPipeline] : m_materialPipelines)
            {
                // This comes from a central registry setting, so it must be a full path already.
                response.m_sourceFileDependencyList.push_back({});
                response.m_sourceFileDependencyList.back().m_sourceFileDependencyPath = materialPipelineFilePath;

                for (const MaterialPipelineSourceData::ShaderTemplate& shaderTemplate : materialPipeline.m_shaderTemplates)
                {
                    AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDepenencyPaths(materialPipelineFilePath, shaderTemplate.m_shader);
                    for (const AZStd::string& path : possibleDependencies)
                    {
                        response.m_sourceFileDependencyList.push_back({});
                        response.m_sourceFileDependencyList.back().m_sourceFileDependencyPath = path;
                    }

                    // We don't need to add m_azsli as a dependency because that will be #included into the final azsl file, so the shader asset builder
                    // will account for that dependency.
                }
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void MaterialTypePreBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
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

            AZStd::string materialTypeName;
            AZ::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), materialTypeName);

            AZ::u32 nextProductSubID = 0;

            auto materialTypeLoadResult = MaterialUtils::LoadMaterialTypeSourceData(request.m_fullPath);
            if (!materialTypeLoadResult.IsSuccess())
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to load material type file.");
                return;
            }
            MaterialTypeSourceData materialType = materialTypeLoadResult.TakeValue();
            materialType.m_shaderCollection.clear();

            // Generate the required shaders
            for (const auto& [materialPipelineFilePath, materialPipeline] : m_materialPipelines)
            {
                AZStd::string materialPipelineName;
                AZ::StringFunc::Path::GetFileName(materialPipelineFilePath.c_str(), materialPipelineName);


                // TODO: Eventually we'll use a script and inputs from the material type to decide which shader templates need to be used
                for (const MaterialPipelineSourceData::ShaderTemplate& shaderTemplate : materialPipeline.m_shaderTemplates)
                {
                    AZStd::string shaderName;
                    AZ::StringFunc::Path::GetFileName(shaderTemplate.m_shader.c_str(), shaderName); // This will remove the ".template" extension
                    AZ::StringFunc::Path::ReplaceExtension(shaderName, "");                         // This will remove the ".shader" extension

                    AZStd::string shaderFilePath = AssetUtils::ResolvePathReference(materialPipelineFilePath, shaderTemplate.m_shader);
                    auto shaderFile = AZ::Utils::ReadFile<AZStd::string>(shaderFilePath);
                    if (!shaderFile.IsSuccess())
                    {
                        AZ_Error(MaterialTypePreBuilderName, false, "Failed to load shader file '%s'.", shaderFilePath.c_str());
                        return;
                    }

                    // Intermediate azsl file

                    AZStd::string generatedAzsl = AZStd::string::format(
                        "// This code was generated by %s. Do not modify.\n"
                        "#include <%s>\n",
                        MaterialTypePreBuilderName,
                        shaderTemplate.m_azsli.c_str());

                    if (!materialType.m_materialShaderCode.empty())
                    {
                        generatedAzsl += AZStd::string::format("#include <%s>\n", materialType.m_materialShaderCode.c_str());
                    }

                    generatedAzsl +=
                            " \n"
                            "#if !MaterialFunction_AdjustLocalPosition_DEFINED                                       \n"
                            "    void MaterialFunction_AdjustLocalPosition(inout float3 localPosition) {}            \n"
                            "    #define MaterialFunction_AdjustLocalPosition_DEFINED 1                              \n"
                            "#endif                                                                                  \n"
                            "                                                                                        \n"
                            "#if !MaterialFunction_AdjustWorldPosition_DEFINED                                       \n"
                            "    void MaterialFunction_AdjustWorldPosition(inout float3 worldPosition) {}            \n"
                            "    #define MaterialFunction_AdjustWorldPosition_DEFINED 1                              \n"
                            "#endif                                                                                  \n"
                            "                                                                                        \n"
                            "#if !MaterialFunction_AdjustSurface_DEFINED && MATERIALPIPELINE_SHADER_HAS_PIXEL_STAGE  \n"
                            "    void MaterialFunction_AdjustSurface(inout Surface outSurface) {}                    \n"
                            "    #define MaterialFunction_AdjustSurface_DEFINED 1                                    \n"
                            "#endif                                                                                  \n"
                        ;

                    AZ::IO::Path outputAzslFilePath = request.m_tempDirPath;
                    outputAzslFilePath /= AZStd::string::format("%s_%s_%s.azsl", materialTypeName.c_str(), materialPipelineName.c_str(), shaderName.c_str());

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
                        AZ_Error(MaterialTypePreBuilderName, false, "Failed to write intermediate azsl file.");
                        return;
                    }

                    // Intermediate shader file

                    AZStd::string azslFileReference = AZ::IO::Path{outputAzslFilePath.Filename()}.c_str();
                    AZStd::to_lower(azslFileReference.begin(), azslFileReference.end());
                    AzFramework::StringFunc::Replace(shaderFile.GetValue(), "INSERT_AZSL_HERE", azslFileReference.c_str());

                    AZ::IO::Path outputShaderFilePath = request.m_tempDirPath;
                    outputShaderFilePath /= AZStd::string::format("%s_%s_%s.shader", materialTypeName.c_str(), materialPipelineName.c_str(), shaderName.c_str());

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
                        AZ_Error(MaterialTypePreBuilderName, false, "Failed to write intermediate shader file.");
                        return;
                    }

                    // Add shader to intermediate material type

                    materialType.m_shaderCollection.push_back({});
                    materialType.m_shaderCollection.back().m_shaderFilePath = AZ::IO::Path{outputShaderFilePath.Filename()}.c_str();

                    // TODO: We should probably make the material pipeline file have a field to indicate the name of the corresponding
                    // pass tree template instead of assuming the filenames match.
                    // pass tree template instead of assuming the filenames match.
                    materialType.m_shaderCollection.back().m_renderPipelineName = materialPipelineName;
                }
            }

            AZ::IO::Path outputMaterialTypeFilePath = request.m_tempDirPath;
            outputMaterialTypeFilePath /= AZStd::string::format("%s.materialtype", materialTypeName.c_str());

            if (JsonUtils::SaveObjectToFile(outputMaterialTypeFilePath.String(), materialType))
            {
                AssetBuilderSDK::JobProduct product;
                product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
                product.m_productFileName = outputMaterialTypeFilePath.String();
                product.m_productSubID = nextProductSubID++;
                response.m_outputProducts.push_back(AZStd::move(product));
            }
            else
            {
                AZ_Error(MaterialTypePreBuilderName, false, "Failed to write intermediate material type file.");
                return;
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        void MaterialTypePreBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }
    } // namespace RPI
} // namespace AZ
