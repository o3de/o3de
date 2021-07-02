/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShaderAssetBuilder.h"

#include <CommonFiles/Preprocessor.h>
#include <CommonFiles/GlobalBuildOptions.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RHI.Reflect/ConstantsLayout.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/JSON/document.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

#include "AzslBuilder.h"
#include "SrgLayoutBuilder.h"
#include "ShaderVariantAssetBuilder.h"
#include "ShaderBuilderUtility.h"
#include "ShaderPlatformInterfaceRequest.h"
#include "AtomShaderConfig.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
namespace AZ
{
    namespace ShaderBuilder
    {
        static constexpr char ShaderAssetBuilderName[] = "ShaderAssetBuilder";
        static constexpr uint32_t ShaderAssetBuildTimestampParam = 0;

        static void AddSrgLayoutJobDependency(AssetBuilderSDK::JobDescriptor& jobDescriptor, const AssetBuilderSDK::PlatformInfo& platformInfo, AZStd::string_view fullFilePath)
        {
            AssetBuilderSDK::SourceFileDependency fileDependency;
            fileDependency.m_sourceFileDependencyPath = fullFilePath;

            AssetBuilderSDK::JobDependency srgLayoutJobDependency;
            srgLayoutJobDependency.m_jobKey = SrgLayoutBuilder::SrgLayoutBuilderJobKey;
            srgLayoutJobDependency.m_platformIdentifier = platformInfo.m_identifier;
            srgLayoutJobDependency.m_sourceFile = fileDependency;
            srgLayoutJobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
            jobDescriptor.m_jobDependencyList.emplace_back(srgLayoutJobDependency);
        }

        void ShaderAssetBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            AZStd::string fullPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true);

            AZ_TracePrintf(ShaderAssetBuilderName, "CreateJobs for Shader \"%s\"\n", fullPath.data());

            // Used to synchronize versions of the ShaderAsset and ShaderVariantTreeAsset, especially during hot-reload.
            // Note it's probably important for this to be set once outside the platform loop so every platform's ShaderAsset
            // has the same value, because later the ShaderVariantTreeAsset job will fetch this value from the local ShaderAsset
            // which could cross platforms (i.e. building an android ShaderVariantTreeAsset on PC would fetch the tiemstamp from
            // the PC's ShaderAsset).
            AZStd::sys_time_t shaderAssetBuildTimestamp = AZStd::GetTimeNowMicroSecond();

            // *** block (remove all this when [ATOM-4225] addressed)
            AZStd::string azslFullPath;
            // Need to get the name of the azsl file from the .shader source asset, to be able to declare a dependency to SRG Layout Job.
            // and the macro options to preprocess.
            auto descriptorParseOutput = ShaderBuilderUtility::LoadShaderDataJson(fullPath);
            if (!descriptorParseOutput.IsSuccess())
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to parse Shader Descriptor JSON: %s", descriptorParseOutput.GetError().c_str());
                return;
            }
            ShaderBuilderUtility::GetAbsolutePathToAzslFile(fullPath, descriptorParseOutput.GetValue().m_source, azslFullPath);
            AZ_Warning(ShaderAssetBuilderName, IO::FileIOBase::GetInstance()->Exists(azslFullPath.c_str()), "Shader program listed as the source entry does not exist: %s.", azslFullPath.c_str());
            GlobalBuildOptions buildOptions = ReadBuildOptions(ShaderAssetBuilderName);
            // *** end block (remove when [Atom-4225])

            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AZ_TraceContext("For platform", platformInfo.m_identifier.data());

                // Get the platform interfaces to be able to access the prepend file
                AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(platformInfo);

                AssetBuilderSDK::JobDescriptor jobDescriptor;
                jobDescriptor.m_priority = 2;
                // [GFX TODO][ATOM-2830] Set 'm_critical' back to 'false' once proper fix for Atom startup issues are in 
                jobDescriptor.m_critical = true;
                jobDescriptor.m_jobKey = ShaderAssetBuilderJobKey;
                jobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // queue up AzslBuilder dependencies:
                for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
                {
                    AddAzslBuilderJobDependency(jobDescriptor, platformInfo.m_identifier, shaderPlatformInterface->GetAPIName().GetCStr(), fullPath);

                    {// *** block (remove all this when [ATOM-4225] addressed)
                        // this is the same code as in AzslBuilder.cpp's CreateJobs. This is not supposed to be repeated here (temporary hack), so not factorized. refer to AzslBuilder.cpp for code comments
                        AZStd::string prependedAzslSourceCode;
                        RHI::PrependArguments args;
                        args.m_sourceFile = fullPath.c_str();
                        args.m_prependFile = shaderPlatformInterface->GetAzslHeader(platformInfo);
                        args.m_addSuffixToFileName = shaderPlatformInterface->GetAPIName().GetCStr();
                        args.m_destinationStringOpt = &prependedAzslSourceCode;

                        if (RHI::PrependFile(args) == fullPath)
                        {
                            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                            return;
                        }
                        AZStd::string originalLocation;
                        AzFramework::StringFunc::Path::GetFullPath(fullPath.c_str(), originalLocation);
                        AZStd::string prependedPath = ShaderBuilderUtility::DumpAzslPrependedCode(
                            ShaderAssetBuilderName, prependedAzslSourceCode, originalLocation, ShaderBuilderUtility::ExtractStemName(fullPath.c_str()), shaderPlatformInterface->GetAPIName().GetStringView());
                        PreprocessorData output;
                        buildOptions.m_compilerArguments.Merge(descriptorParseOutput.GetValue().m_compiler);
                        PreprocessFile(azslFullPath, output, buildOptions.m_preprocessorSettings, true, true);
                        // srg layout builder job dependency on the azsl file itself.
                        // If there's a ShaderResourceGroup defined in this azsl file
                        // then its azsrg asset must be ready before We compile it. The azsrg requirement
                        // is only for diagnostics, because a preprocessed (flattened) azsl file contains
                        // all the SRG data it needs.
                        AddSrgLayoutJobDependency(jobDescriptor, platformInfo, azslFullPath);
                        for (auto included : output.includedPaths)
                        {
                            AddSrgLayoutJobDependency(jobDescriptor, platformInfo, included.c_str());
                        }
                        AZ::IO::SystemFile::Delete(prependedPath.c_str());  // don't let that intermediate file dirty a folder under source version control.
                    }// *** end block remove when [Atom-4225]
                }
                jobDescriptor.m_jobParameters.emplace(ShaderAssetBuildTimestampParam, AZStd::to_string(shaderAssetBuildTimestamp));

                response.m_createJobOutputs.push_back(jobDescriptor);
            }  // for all request.m_enabledPlatforms

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        static AssetBuilderSDK::ProcessJobResultCode CompileForAPI(
            const ShaderBuilderUtility::AzslSubProducts::Paths& pathOfProductFiles,
            RPI::ShaderAssetCreator& shaderAssetCreator,
            RHI::ShaderPlatformInterface* shaderPlatformInterface,
            AssetBuilderSDK::ProcessJobResponse& response,
            AzslData& azslData,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout,
            const RPI::ShaderSourceData& shaderSourceDataDescriptor,
            AZStd::sys_time_t shaderAssetBuildTimestamp,
            const ShaderResourceGroupAssets& srgAssets,
            BindingDependencies& bindingDependencies,
            const RootConstantData& rootConstantData,
            const AssetBuilderSDK::ProcessJobRequest& request)
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            const AZStd::string& tempDirPath = request.m_tempDirPath;

            // discover entry points
            MapOfStringToStageType shaderEntryPoints;
            if (shaderSourceDataDescriptor.m_programSettings.m_entryPoints.empty())
            {
                AZ_TracePrintf(ShaderAssetBuilderName, "ProgramSettings do not specify entry points, will use GetDefaultEntryPointsFromShader()\n");
                ShaderBuilderUtility::GetDefaultEntryPointsFromFunctionDataList(azslData.m_functions, shaderEntryPoints);
            }
            else
            {
                for (auto& iter : shaderSourceDataDescriptor.m_programSettings.m_entryPoints)
                {
                    shaderEntryPoints[iter.m_name] = iter.m_type;
                }
            }

            // Check if we were canceled before we do any heavy processing of
            // the shader data (compiling the shader kernels, processing SRG
            // and pipeline layout data, etc.).
            if (jobCancelListener.IsCancelled())
            {
                return AssetBuilderSDK::ProcessJobResult_Cancelled;
            }

            // Signal the begin of shader data for an RHI API.
            shaderAssetCreator.BeginAPI(shaderPlatformInterface->GetAPIType());
            RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor = ShaderBuilderUtility::BuildPipelineLayoutDescriptorForApi(
                ShaderAssetBuilderName, shaderPlatformInterface, bindingDependencies, srgAssets, shaderEntryPoints, shaderCompilerArguments, &rootConstantData);
            if (!pipelineLayoutDescriptor)
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to build pipeline layout descriptor for api=[%s]",
                         shaderPlatformInterface->GetAPIName().GetCStr());
                AssetBuilderSDK::ProcessJobResult_Failed;
            }
            for (const auto& srgAsset : srgAssets)
            {
                shaderAssetCreator.AddShaderResourceGroupAsset(srgAsset);
            }
            shaderAssetCreator.SetPipelineLayout(pipelineLayoutDescriptor);

            // Generate shader source.
            AZStd::string hlslSourcePath = pathOfProductFiles[ShaderBuilderUtility::AzslSubProducts::hlsl];
            Outcome<AZStd::string, AZStd::string> hlslSourceContent = Utils::ReadFile(hlslSourcePath);
            if (!hlslSourceContent.IsSuccess())
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to obtain shader source from %s. [%s]", hlslSourcePath.c_str(), hlslSourceContent.TakeError().c_str());
                return AssetBuilderSDK::ProcessJobResult_Failed;
            }

            // The root ShaderVariantAsset needs to be created with the known uuid of the source .shader asset because
            // the ShaderAsset owns a Data::Asset<> reference that gets serialized. It must have the correct uuid
            // so the root ShaderVariantAsset is found when the ShaderAsset is deserialized.
            AZStd::string fullSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullSourcePath, true);
            const uint32_t productSubID = RPI::ShaderAsset::MakeAssetProductSubId(
                shaderPlatformInterface->GetAPIUniqueIndex(),
                aznumeric_cast<uint32_t>(RPI::ShaderAssetSubId::RootShaderVariantAsset));
            auto assetIdOutcome = RPI::AssetUtils::MakeAssetId(fullSourcePath, productSubID);
            AZ_Assert(assetIdOutcome.IsSuccess(), "Failed to get AssetId from shader %s", fullSourcePath.c_str());
            const Data::AssetId variantAssetId = assetIdOutcome.TakeValue();

            // We always include the root shader variant (all options are unspecified) in the ShaderAsset itself.
            ShaderVariantCreationContext variantCreationContext = { variantAssetId, hlslSourcePath, hlslSourceContent.GetValue(), shaderSourceDataDescriptor,
                tempDirPath, request.m_platformInfo, *shaderOptionGroupLayout, shaderEntryPoints, shaderAssetBuildTimestamp };
            AZ::Outcome<Data::Asset<RPI::ShaderVariantAsset>, AZStd::string> outcomeForShaderVariantAsset = ShaderVariantAssetBuilder::CreateShaderVariantAssetForAPI(
                RPI::ShaderVariantListSourceData::VariantInfo(),
                variantCreationContext,
                *shaderPlatformInterface,
                azslData,
                shaderCompilerArguments,
                pathOfProductFiles[ShaderBuilderUtility::AzslSubProducts::om],
                pathOfProductFiles[ShaderBuilderUtility::AzslSubProducts::ia]);
            if (!outcomeForShaderVariantAsset.IsSuccess())
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to serialize out the root shader variant for API [%s]: %s"
                    , shaderPlatformInterface->GetAPIName().GetCStr(), outcomeForShaderVariantAsset.GetError().c_str());
                return AssetBuilderSDK::ProcessJobResult_Failed;
            }

            // Time to save, for the given rhi::api, the root shader variant as an asset.
            Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset = outcomeForShaderVariantAsset.TakeValue();

            AssetBuilderSDK::JobProduct variantAssetProduct;
            if (!ShaderVariantAssetBuilder::SerializeOutShaderVariantAsset(shaderVariantAsset, fullSourcePath, tempDirPath,
                *shaderPlatformInterface, productSubID, variantAssetProduct))
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to serialize out the root shader variant for API [%s]"
                    , shaderPlatformInterface->GetAPIName().GetCStr());
                return AssetBuilderSDK::ProcessJobResult_Failed;
            }
            response.m_outputProducts.push_back(variantAssetProduct);

            // add byproducts as job output products:
            if (variantCreationContext.m_outputByproducts)
            {
                uint32_t subProductType = aznumeric_cast<uint32_t>(RPI::ShaderAssetSubId::GeneratedHlslSource) + 1;
                for (const AZStd::string& byproduct : variantCreationContext.m_outputByproducts->m_intermediatePaths)
                {
                    AssetBuilderSDK::JobProduct jobProduct;
                    jobProduct.m_productFileName = byproduct;
                    jobProduct.m_productAssetType = Uuid::CreateName("DebugInfoByProduct-PdbOrDxilTxt");
                    jobProduct.m_productSubID = RPI::ShaderAsset::MakeAssetProductSubId(shaderPlatformInterface->GetAPIUniqueIndex(), subProductType++);
                    response.m_outputProducts.push_back(AZStd::move(jobProduct));
                }
            }

            shaderAssetCreator.SetRootShaderVariantAsset(shaderVariantAsset);

            // Populate the shader asset with all entry stage attributes
            RHI::ShaderStageAttributeMapList attributeMaps;
            attributeMaps.resize(RHI::ShaderStageCount);
            for (const auto& shaderEntry : shaderSourceDataDescriptor.m_programSettings.m_entryPoints)
            {
                auto findId = AZStd::find_if(AZ_BEGIN_END(azslData.m_functions), [&shaderEntry](const auto& func)
                    {
                        return func.m_name == shaderEntry.m_name;
                    });

                if (findId == azslData.m_functions.end())
                {
                    // shaderData.m_functions only contains Vertex, Fragment and Compute entries for now
                    // Tessellation shaders will need to be handled too
                    continue;
                }

                const auto shaderStage = ToRHIShaderStage(ShaderBuilderUtility::ToAssetBuilderShaderType(shaderEntry.m_type));
                for (const auto& attr : findId->attributesList)
                {
                    // Some stages like RHI::ShaderStage::Tessellation are compound and consist of two or more shader entries
                    const Name& attributeName = attr.first;
                    const RHI::ShaderStageAttributeArguments& args = attr.second;
                    const auto stageIndex = static_cast<uint32_t>(shaderStage);
                    AZ_Assert(stageIndex < RHI::ShaderStageCount, "Invalid shader stage specified!");
                    attributeMaps[stageIndex][attributeName] = args;
                }
            }
            shaderAssetCreator.SetShaderStageAttributeMapList(attributeMaps);

            shaderAssetCreator.EndAPI();

            return AssetBuilderSDK::ProcessJobResult_Success;
        }

        static bool SerializeOutShaderAsset(Data::Asset<RPI::ShaderAsset> shaderAsset,
            const AZStd::string& tempDirPath,
            AssetBuilderSDK::ProcessJobResponse& response)
        {
            AZStd::string shaderAssetFileName = AZStd::string::format("%s.%s", shaderAsset->GetName().GetCStr(), RPI::ShaderAsset::Extension);
            AZStd::string shaderAssetOutputPath;
            AzFramework::StringFunc::Path::ConstructFull(tempDirPath.data(), shaderAssetFileName.data(), shaderAssetOutputPath, true);

            if (!Utils::SaveObjectToFile(shaderAssetOutputPath, DataStream::ST_BINARY, shaderAsset.Get()))
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to output Shader Descriptor");
                return false;
            }

            AssetBuilderSDK::JobProduct shaderJobProduct;
            if (!AssetBuilderSDK::OutputObject(shaderAsset.Get(), shaderAssetOutputPath, azrtti_typeid<RPI::ShaderAsset>(),
                aznumeric_cast<uint32_t>(RPI::ShaderAssetSubId::ShaderAsset), shaderJobProduct))
            {
                AZ_Error(ShaderAssetBuilderName, false, "Failed to output product dependencies.");
                return false;
            }
            response.m_outputProducts.push_back(AZStd::move(shaderJobProduct));

            return true;
        }

        void ShaderAssetBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            const AZStd::sys_time_t startTime = AZStd::GetTimeNowTicks();
            AZStd::string fullSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullSourcePath, true);

            RPI::ShaderAssetCreator shaderAssetCreator;
            shaderAssetCreator.Begin(Uuid::CreateRandom());

            // read .shader -> access azsl path -> make absolute
            RPI::ShaderSourceData shaderAssetSource;
            AZStd::shared_ptr<ShaderFiles> inputFiles = ShaderBuilderUtility::PrepareSourceInput(ShaderAssetBuilderName, fullSourcePath, shaderAssetSource);
            if (!inputFiles)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            GlobalBuildOptions buildOptions = ReadBuildOptions(ShaderAssetBuilderName);

            // Save .shader file name
            AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.data(), inputFiles->m_shaderFileName);

            // Request the list of valid shader platform interfaces for the target platform.
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
            ShaderPlatformInterfaceRequestBus::BroadcastResult(platformInterfaces, &ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, request.m_platformInfo);
            // Generate shaders for each of those ShaderPlatformInterfaces.
            uint32_t countOfCompiledPlatformInterfaces = 0;
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                ShaderResourceGroupAssets srgAssets;
                RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();

                if (!shaderPlatformInterface)
                {
                    AZ_Error(ShaderAssetBuilderName, false, "ShaderPlatformInterface for [%s] is not registered, can't compile [%s]", request.m_platformInfo.m_identifier.c_str(), request.m_sourceFile.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                if (shaderAssetSource.IsRhiBackendDisabled(shaderPlatformInterface->GetAPIName()))
                {
                    // Gracefully do nothing and continue with the next shaderPlatformInterface.
                    AZ_TracePrintf(
                        ShaderAssetBuilderName, "Skipping shader compilation [%s] for API [%s]\n", inputFiles->m_shaderFileName.c_str(),
                        shaderPlatformInterface->GetAPIName().GetCStr());
                    continue;
                }

                AZStd::sys_time_t shaderAssetBuildTimestamp = 0;
                auto shaderAssetBuildTimestampIterator = request.m_jobDescription.m_jobParameters.find(ShaderAssetBuildTimestampParam);
                if (shaderAssetBuildTimestampIterator != request.m_jobDescription.m_jobParameters.end())
                {
                    shaderAssetBuildTimestamp = AZStd::stoull(shaderAssetBuildTimestampIterator->second);

                    if (AZStd::to_string(shaderAssetBuildTimestamp) != shaderAssetBuildTimestampIterator->second)
                    {
                        AZ_Assert(false, "Incorrect conversion of ShaderAssetBuildTimestampParam");
                        return;
                    }
                }

                AzslData azslData(inputFiles);

                AZ_TraceContext("Platform API", AZStd::string{ shaderPlatformInterface->GetAPIName().GetStringView() });
                AZ_TracePrintf(ShaderAssetBuilderName, "Preprocessed AZSL File: %s \n", azslData.m_preprocessedFullPath.c_str());

                AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
                if (jobCancelListener.IsCancelled())
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                // obtain the build artifacts from the azsl builder:
                auto azslArtifactsOutcome = ShaderBuilderUtility::ObtainBuildArtifactsFromAzslBuilder(ShaderAssetBuilderName, fullSourcePath, shaderPlatformInterface->GetAPIType(), request.m_platformInfo.m_identifier);
                if (!azslArtifactsOutcome.IsSuccess())
                {
                    // If it failed, it may be because the .shader source file created no products! (this happens when the build options are similar)
                    // (the .azsl file *always* produces AzslBuilder artifacts. The.shader file *sometimes* produces AzslBuilder artifacts, when it has build options that have to be accounted for)
                    // If there are no artifacts from the .shader, then we fall back to the ones from the .azsl:
                    azslArtifactsOutcome = ShaderBuilderUtility::ObtainBuildArtifactsFromAzslBuilder(ShaderAssetBuilderName, inputFiles->m_azslSourceFullPath, shaderPlatformInterface->GetAPIType(), request.m_platformInfo.m_identifier);
                    if (!azslArtifactsOutcome.IsSuccess())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }
                }

                shaderAssetCreator.SetName(Name(azslData.m_sources->m_shaderFileName));
                shaderAssetCreator.SetDrawListName(Name(shaderAssetSource.m_drawListName));

                shaderAssetCreator.SetShaderAssetBuildTimestamp(shaderAssetBuildTimestamp);

                BindingDependencies bindingDependencies;
                RootConstantData rootConstantData;
                AssetBuilderSDK::ProcessJobResultCode azslJsonReadResult = ShaderBuilderUtility::PopulateAzslDataFromJsonFiles(
                    ShaderAssetBuilderName,
                    azslArtifactsOutcome.GetValue(),
                    azslData,
                    srgAssets,
                    shaderOptionGroupLayout,
                    bindingDependencies,
                    rootConstantData);
                if (azslJsonReadResult != AssetBuilderSDK::ProcessJobResult_Success)

                {
                    response.m_resultCode = azslJsonReadResult;
                    return;
                }

                shaderAssetCreator.SetName(Name(azslData.m_sources->m_shaderFileName));
                shaderAssetCreator.SetDrawListName(Name(shaderAssetSource.m_drawListName));
                shaderAssetCreator.SetShaderAssetBuildTimestamp(shaderAssetBuildTimestamp);

                // The ShaderOptionGroupLayout dictates what options/range can be used to create variants
                shaderAssetCreator.SetShaderOptionGroupLayout(shaderOptionGroupLayout);

                AZ_TracePrintf(ShaderAssetBuilderName, "Original AZSL File: %s \n", azslData.m_sources->m_azslSourceFullPath.c_str());

                const uint32_t usedShaderOptionBits = shaderOptionGroupLayout->GetBitSize();
                AZ_TracePrintf(ShaderAssetBuilderName, "Note: This shader uses %u of %u available shader variant key bits. \n", usedShaderOptionBits, RPI::ShaderVariantKeyBitCount);

                // The idea of this merge is that we have compiler options coming from 2 source:
                // global options (from project Config/), and .shader options.
                // We define a merge behavior that is: ".shader wins if set"
                RHI::ShaderCompilerArguments mergedArguments = buildOptions.m_compilerArguments;
                mergedArguments.Merge(shaderAssetSource.m_compiler);

                AssetBuilderSDK::ProcessJobResultCode compileResult =
                    CompileForAPI(
                        azslArtifactsOutcome.GetValue(),
                        shaderAssetCreator,
                        shaderPlatformInterface,
                        response,
                        azslData,
                        mergedArguments,
                        shaderOptionGroupLayout,
                        shaderAssetSource,
                        shaderAssetBuildTimestamp,
                        srgAssets,
                        bindingDependencies,
                        rootConstantData,
                        request);
                if (compileResult != AssetBuilderSDK::ProcessJobResult_Success)
                {
                    response.m_resultCode = compileResult;
                    return;
                }
                ++countOfCompiledPlatformInterfaces;
            } // end for all platforms

            if (!countOfCompiledPlatformInterfaces)
            {
                AZ_TracePrintf(
                    ShaderAssetBuilderName, "No azshader is produced on behalf of %s because all valid RHI backends were disabled for this shader.\n", request.m_sourceFile.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            Data::Asset<RPI::ShaderAsset> shaderAsset;
            if (!shaderAssetCreator.End(shaderAsset))
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            if (!SerializeOutShaderAsset(shaderAsset, request.m_tempDirPath, response))
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            const AZStd::sys_time_t endTime = AZStd::GetTimeNowTicks();
            const AZStd::sys_time_t deltaTime = endTime - startTime;
            const float elapsedTimeSeconds = (float)(deltaTime) / (float)AZStd::GetTimeTicksPerSecond();

            AZ_TracePrintf(ShaderAssetBuilderName, "Finished processing %s in %.2f seconds\n", request.m_sourceFile.c_str(), elapsedTimeSeconds);

            ShaderBuilderUtility::LogProfilingData(ShaderAssetBuilderName, inputFiles->m_azslFileName);
        }

    } // ShaderBuilder
} // AZ
