/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ShaderVariantAssetBuilder.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantTreeAssetCreator.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>

#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RHI.Reflect/ConstantsLayout.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RHI/RHIUtils.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/Process/ProcessCommunicator.h>
#include <AzFramework/Process/ProcessWatcher.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/JSON/document.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/time.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/StringFunc/StringFunc.h>

#include "HashedVariantListSourceData.h"
#include "ShaderAssetBuilder.h"
#include "ShaderBuilderUtility.h"
#include "SrgLayoutUtility.h"
#include "AzslData.h"
#include "AzslCompiler.h"
#include <CommonFiles/Preprocessor.h>
#include <ShaderPlatformInterfaceRequest.h>
#include "ShaderBuildArgumentsManager.h"


namespace AZ
{
    namespace ShaderBuilder
    {
        static constexpr char ShaderVariantAssetBuilderName[] = "ShaderVariantAssetBuilder";


        AZStd::string ShaderVariantAssetBuilder::GetShaderVariantTreeAssetJobKey()
        {
            return AZStd::string::format("%s_varianttree", ShaderVariantAssetBuilderJobKeyPrefix);
        }


        AZStd::string ShaderVariantAssetBuilder::GetShaderVariantAssetJobKey()
        {
            return AZStd::string::format("%s_variantbatch", ShaderVariantAssetBuilderJobKeyPrefix);
        }

        void ShaderVariantAssetBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            // Please see comments in the header file for the core principles of this builder.

            // Is this a *.hashedvariantlist? if so We need to create the ShaderVariantTreeAsset
            AZStd::string fileExtension;
            AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), fileExtension, false /*includeDot*/);
            if (fileExtension == HashedVariantListSourceData::Extension)
            {
                CreateShaderVariantTreeJobs(request, response);
                return;
            }
            else if (fileExtension == HashedVariantInfoSourceData::Extension)
            {
                CreateShaderVariantJobs(request, response);
                return;
            }

            AZ_Error(ShaderVariantAssetBuilderName, false, "Unsupported file extension: %s", fileExtension.c_str());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
        }  // CreateJobs


        void ShaderVariantAssetBuilder::CreateShaderVariantTreeJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            AZStd::string variantListRelativePath(request.m_sourceFile.data());
            AZStd::string hashedVariantListFullPath;
            AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), hashedVariantListFullPath, true);

            AZ_TracePrintf(ShaderVariantAssetBuilderName, "CreateShaderVariantTreeJob for Hashed Shader Variant List \"%s\"\n", hashedVariantListFullPath.data());

            HashedVariantListSourceData hashedVariantListDescriptor;
            if (!RPI::JsonUtils::LoadObjectFromFile(hashedVariantListFullPath, hashedVariantListDescriptor, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Assert(false, "Failed to parse Hashed Variant List Descriptor JSON [%s]", hashedVariantListFullPath.c_str());
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                return;
            }

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AZ_TraceContext("For platform", info.m_identifier.data());

                AssetBuilderSDK::JobDescriptor jobDescriptor;

                // The ShaderVariantTreeAsset is high priority, but must be generated after the ShaderAsset 
                jobDescriptor.m_priority = 1;
                jobDescriptor.m_critical = false;

                jobDescriptor.m_jobKey = GetShaderVariantTreeAssetJobKey();
                jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());

                // Declare job dependency on the .azshader, this way the ShaderAsset must be built before
                // the ShaderVariantTreeAsset.
                AssetBuilderSDK::JobDependency jobDependency;
                jobDependency.m_jobKey = ShaderAssetBuilder::ShaderAssetBuilderJobKey;
                jobDependency.m_platformIdentifier = info.m_identifier;
                jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                jobDependency.m_sourceFile.m_sourceFileDependencyPath = hashedVariantListDescriptor.m_shaderPath;
                jobDescriptor.m_jobDependencyList.push_back(jobDependency);

                response.m_createJobOutputs.push_back(jobDescriptor);

            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }


        // For a file with the following name: <shaderName>_<BatchId>.hashedvariantbatch
        // returns the absolute path that looks like: <shaderName>.hashedvariantlist
        static AZStd::string GetHashedVariantListPathFromVariantInfoPath(const AZStd::string& hashedVariantBatchParentPath, const AZStd::string& hashedVariantBatchRelativePath)
        {
            size_t charPos = AZ::StringFunc::Find(hashedVariantBatchRelativePath, "_", 0, true /* reverse*/);
            AZStd::string pathBefore_ = hashedVariantBatchRelativePath.substr(0, charPos);
            return AZStd::string::format(
                "%s%s%s.%s",
                hashedVariantBatchParentPath.c_str(),
                AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING,
                pathBefore_.c_str(),
                HashedVariantListSourceData::Extension);
        }


        void ShaderVariantAssetBuilder::CreateShaderVariantJobs([[maybe_unused]] const AssetBuilderSDK::CreateJobsRequest& request,
                                                               [[maybe_unused]] AssetBuilderSDK::CreateJobsResponse& response) const
        {

            AZStd::string hashedVariantBatchRelativePath(request.m_sourceFile.data());
            AZStd::string hashedVariantBatchFullPath;
            AZ::StringFunc::Path::ConstructFull(
                request.m_watchFolder.data(), request.m_sourceFile.data(), hashedVariantBatchFullPath, true);
            
            AZ_TracePrintf(
                ShaderVariantAssetBuilderName,
                "CreateShaderVariantJobs for Hashed Variant Batch [%s]\n",
                hashedVariantBatchFullPath.data());
            
            HashedVariantListSourceData hashedVariantBatchDescriptor;
            if (!RPI::JsonUtils::LoadObjectFromFile(
                    hashedVariantBatchFullPath, hashedVariantBatchDescriptor, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Assert(false, "Failed to parse Hashed Variant Info Descriptor JSON [%s]", hashedVariantBatchFullPath.c_str());
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                return;
            }

            AZStd::string hashedVariantBatchDescriptorString;
            RPI::JsonUtils::SaveObjectToJsonString(hashedVariantBatchDescriptor, hashedVariantBatchDescriptorString);

            AZStd::string hashedVariantBatchParentPath(request.m_watchFolder.data());
            AZStd::string hashedVariantListFullPath =
                GetHashedVariantListPathFromVariantInfoPath(hashedVariantBatchParentPath, hashedVariantBatchRelativePath);
            
            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AZ_TraceContext("For platform", info.m_identifier.data());
            
                AssetBuilderSDK::JobDescriptor jobDescriptor;
            
                // There can be tens/hundreds of thousands of shader variants. By default each shader will get
                // a root variant that can be used at runtime. In order to prevent the AssetProcessor from
                // being overwhelmed by shader variant compilation We mark all non-root shader variant generation
                // as non critical and very low priority.
                jobDescriptor.m_priority = -5000;
                jobDescriptor.m_critical = false;
            
                jobDescriptor.m_jobKey = GetShaderVariantAssetJobKey();
                jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());

                // Add the content of the hashedVariantBatch file as a parameter to avoid reading it again.
                jobDescriptor.m_jobParameters.emplace(ShaderVariantBatchJobParam, hashedVariantBatchDescriptorString);
            
                // The ShaderVariantAssets should be built AFTER the ShaderVariantTreeAsset.
                // With "OrderOnly" dependency, We make sure ShaderVariantTreeAsset completes before ShaderVariantAsset runs,
                // but don't re-run ShaderVariantAsset just because ShaderVariantTreeAsset ran.
                //
                AssetBuilderSDK::JobDependency variantTreeJobDependency;
                variantTreeJobDependency.m_jobKey = GetShaderVariantTreeAssetJobKey();
                variantTreeJobDependency.m_platformIdentifier = info.m_identifier;
                variantTreeJobDependency.m_sourceFile.m_sourceFileDependencyPath = hashedVariantListFullPath;
                variantTreeJobDependency.m_type = AssetBuilderSDK::JobDependencyType::OrderOnly;
                jobDescriptor.m_jobDependencyList.emplace_back(variantTreeJobDependency);

                // If the *.shader file changes, all the variants need to be rebuilt.
                AssetBuilderSDK::JobDependency shaderAssetJobDependency;
                shaderAssetJobDependency.m_jobKey = ShaderAssetBuilder::ShaderAssetBuilderJobKey;
                shaderAssetJobDependency.m_platformIdentifier = info.m_identifier;
                shaderAssetJobDependency.m_sourceFile.m_sourceFileDependencyPath = hashedVariantBatchDescriptor.m_shaderPath;
                shaderAssetJobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                jobDescriptor.m_jobDependencyList.emplace_back(shaderAssetJobDependency);
            
                response.m_createJobOutputs.push_back(jobDescriptor);
            
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        }


        void ShaderVariantAssetBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            if (request.m_jobDescription.m_jobKey == GetShaderVariantTreeAssetJobKey())
            {
                ProcessShaderVariantTreeJob(request, response);
            }
            else
            {
                ProcessShaderVariantJob(request, response);
            }
        }


        static RPI::Ptr<RPI::ShaderOptionGroupLayout> LoadShaderOptionsGroupLayoutFromShaderAssetBuilder(
            const RHI::ShaderPlatformInterface* shaderPlatformInterface,
            const AssetBuilderSDK::PlatformInfo& platformInfo,
            const AzslCompiler& azslCompiler,
            const AZStd::string& shaderSourceFileFullPath,
            const RPI::SupervariantIndex supervariantIndex,
            bool& useSpecializationConstants)
        {
            auto optionsGroupPathOutcome = ShaderBuilderUtility::ObtainBuildArtifactPathFromShaderAssetBuilder(
                shaderPlatformInterface->GetAPIUniqueIndex(), platformInfo.m_identifier, shaderSourceFileFullPath, supervariantIndex.GetIndex(),
                AZ::RPI::ShaderAssetSubId::OptionsJson);
            if (!optionsGroupPathOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", optionsGroupPathOutcome.GetError().c_str());
                return nullptr;
            }
            auto optionsGroupJsonPath = optionsGroupPathOutcome.TakeValue();
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();
            // The shader options define what options are available, what are the allowed values/range
            // for each option and what is its default value.
            auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(optionsGroupJsonPath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!jsonOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", jsonOutcome.GetError().c_str());
                return nullptr;
            }
            if (!azslCompiler.ParseOptionsPopulateOptionGroupLayout(
                    jsonOutcome.GetValue(), shaderOptionGroupLayout, useSpecializationConstants))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to find a valid list of shader options!");
                return nullptr;
            }

            return shaderOptionGroupLayout;
        }

        static void LoadShaderFunctionsFromShaderAssetBuilder(
            const RHI::ShaderPlatformInterface* shaderPlatformInterface, const AssetBuilderSDK::PlatformInfo& platformInfo,
            const AzslCompiler& azslCompiler, const AZStd::string& shaderSourceFileFullPath,
            const RPI::SupervariantIndex supervariantIndex,
            AzslFunctions& functions)
        {
            auto functionsJsonPathOutcome = ShaderBuilderUtility::ObtainBuildArtifactPathFromShaderAssetBuilder(
                shaderPlatformInterface->GetAPIUniqueIndex(), platformInfo.m_identifier, shaderSourceFileFullPath, supervariantIndex.GetIndex(),
                AZ::RPI::ShaderAssetSubId::IaJson);
            if (!functionsJsonPathOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", functionsJsonPathOutcome.GetError().c_str());
                return;
            }

            auto functionsJsonPath = functionsJsonPathOutcome.TakeValue();
            auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(functionsJsonPath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!jsonOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", jsonOutcome.GetError().c_str());
                return;
            }
            if (!azslCompiler.ParseIaPopulateFunctionData(jsonOutcome.GetValue(), functions))
            {
                functions.clear();
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to find shader functions.");
                return;
            }
        }


        static bool LoadSrgLayoutListFromShaderAssetBuilder(
            const RHI::ShaderPlatformInterface* shaderPlatformInterface,
            const AssetBuilderSDK::PlatformInfo& platformInfo,
            const AzslCompiler& azslCompiler, const AZStd::string& shaderSourceFileFullPath,
            const RPI::SupervariantIndex supervariantIndex,
            RPI::ShaderResourceGroupLayoutList& srgLayoutList,
            RootConstantData& rootConstantData)
        {
            auto srgJsonPathOutcome = ShaderBuilderUtility::ObtainBuildArtifactPathFromShaderAssetBuilder(
                shaderPlatformInterface->GetAPIUniqueIndex(), platformInfo.m_identifier, shaderSourceFileFullPath, supervariantIndex.GetIndex(), AZ::RPI::ShaderAssetSubId::SrgJson);
            if (!srgJsonPathOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", srgJsonPathOutcome.GetError().c_str());
                return false;
            }

            auto srgJsonPath = srgJsonPathOutcome.TakeValue();
            auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(srgJsonPath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!jsonOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", jsonOutcome.GetError().c_str());
                return false;
            }
            SrgDataContainer srgData;
            if (!azslCompiler.ParseSrgPopulateSrgData(jsonOutcome.GetValue(), srgData))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to parse srg data");
                return false;
            }
            // Add all Shader Resource Group Assets that were defined in the shader code to the shader asset
            if (!SrgLayoutUtility::LoadShaderResourceGroupLayouts(ShaderVariantAssetBuilderName, srgData, srgLayoutList))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to load ShaderResourceGroupLayouts");
                return false;
            }
            
            for (auto srgLayout : srgLayoutList)
            {
                if (!srgLayout->Finalize())
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false,
                        "Failed to finalize SrgLayout %s", srgLayout->GetName().GetCStr());
                    return false;
                }
            }
            
            // Access the root constants reflection
            if (!azslCompiler.ParseSrgPopulateRootConstantData(
                    jsonOutcome.GetValue(),
                    rootConstantData)) // consuming data from --srg ("RootConstantBuffer" subjson section)
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to obtain root constant data reflection");
                return false;
            }
            
            return true;
        }


        static bool LoadBindingDependenciesFromShaderAssetBuilder(
            const RHI::ShaderPlatformInterface* shaderPlatformInterface,
            const AssetBuilderSDK::PlatformInfo& platformInfo,
            const AzslCompiler& azslCompiler, const AZStd::string& shaderSourceFileFullPath,
            const RPI::SupervariantIndex supervariantIndex,
            BindingDependencies& bindingDependencies)
        {
            auto bindingsJsonPathOutcome = ShaderBuilderUtility::ObtainBuildArtifactPathFromShaderAssetBuilder(
                shaderPlatformInterface->GetAPIUniqueIndex(), platformInfo.m_identifier, shaderSourceFileFullPath,     supervariantIndex.GetIndex(), AZ::RPI::ShaderAssetSubId::BindingdepJson);
            if (!bindingsJsonPathOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", bindingsJsonPathOutcome.GetError().c_str());
                return false;
            }
    
            auto bindingsJsonPath = bindingsJsonPathOutcome.TakeValue();
            auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(bindingsJsonPath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!jsonOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", jsonOutcome.GetError().c_str());
                return false;
            }
            if (!azslCompiler.ParseBindingdepPopulateBindingDependencies(jsonOutcome.GetValue(), bindingDependencies))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to parse binding dependencies data");
                return false;
            }
        
            return true;
        }


        // Returns the content of the hlsl file for the given supervariant as produced by ShaderAsssetBuilder.
        // In addition to the content it also returns the full path of the hlsl file in @hlslSourcePath.
        static AZStd::string LoadHlslFileFromShaderAssetBuilder(
            const RHI::ShaderPlatformInterface* shaderPlatformInterface, const AssetBuilderSDK::PlatformInfo& platformInfo,
            const AZStd::string& shaderSourceFileFullPath, const RPI::SupervariantIndex supervariantIndex, AZStd::string& hlslSourcePath)
        {
            auto hlslSourcePathOutcome = ShaderBuilderUtility::ObtainBuildArtifactPathFromShaderAssetBuilder(
                shaderPlatformInterface->GetAPIUniqueIndex(), platformInfo.m_identifier, shaderSourceFileFullPath, supervariantIndex.GetIndex(),
                AZ::RPI::ShaderAssetSubId::GeneratedHlslSource);
            if (!hlslSourcePathOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "%s", hlslSourcePathOutcome.GetError().c_str());
                return "";
            }

            hlslSourcePath = hlslSourcePathOutcome.TakeValue();
            Outcome<AZStd::string, AZStd::string> hlslSourceOutcome = Utils::ReadFile(hlslSourcePath, AZ::RPI::JsonUtils::DefaultMaxFileSize);
            if (!hlslSourceOutcome.IsSuccess())
            {
                AZ_Error(
                    ShaderVariantAssetBuilderName, false, "Failed to obtain shader source from %s. [%s]", hlslSourcePath.c_str(),
                    hlslSourceOutcome.TakeError().c_str());
                return "";
            }
            return hlslSourceOutcome.TakeValue();
        }


        void ShaderVariantAssetBuilder::ProcessShaderVariantTreeJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            AZStd::string hashedVariantListFullPath;
            AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), hashedVariantListFullPath, true);

            HashedVariantListSourceData hashedVariantListDescriptor;
            if (!RPI::JsonUtils::LoadObjectFromFile(hashedVariantListFullPath, hashedVariantListDescriptor, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to parse Hashed Variant List Descriptor JSON [%s]", hashedVariantListFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const AZStd::string& shaderSourceFileFullPath = hashedVariantListDescriptor.m_shaderPath;

            AZStd::string shaderName;
            AZ::StringFunc::Path::GetFileName(shaderSourceFileFullPath.c_str(), shaderName);

            auto descriptorParseOutcome = ShaderBuilderUtility::LoadShaderDataJson(shaderSourceFileFullPath);
            if (!descriptorParseOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to parse shader file [%s]", shaderSourceFileFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            RPI::ShaderSourceData shaderSourceDescriptor = descriptorParseOutcome.TakeValue();
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout;

            // Request the list of valid shader platform interfaces for the target platform.
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces =
                ShaderBuilderUtility::DiscoverEnabledShaderPlatformInterfaces(request.m_platformInfo, shaderSourceDescriptor);
            if (platformInterfaces.empty())
            {
                // No work to do. Exit gracefully.
                AZ_TracePrintf(
                    ShaderVariantAssetBuilderName,
                    "No azshadervarianttree is produced on behalf of %s because all valid RHI backends were disabled for this shader.\n",
                    shaderSourceFileFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            // set the input file for eventual error messages, but the compiler won't be called on it.
            AZStd::string azslFullPath;
            ShaderBuilderUtility::GetAbsolutePathToAzslFile(shaderSourceFileFullPath, shaderSourceDescriptor.m_source, azslFullPath);
            AzslCompiler azslc(azslFullPath, request.m_tempDirPath);

            auto supervariantList = ShaderBuilderUtility::GetSupervariantListFromShaderSourceData(shaderSourceDescriptor);

            AZStd::string previousLoopApiName;
            bool usesVariants = false;
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                auto thisLoopApiName = shaderPlatformInterface->GetAPIName().GetStringView();
                for (uint32_t supervariantIndexCounter = 0; supervariantIndexCounter < supervariantList.size(); ++supervariantIndexCounter)
                {
                    RPI::SupervariantIndex supervariantIndex(supervariantIndexCounter);
                    bool usesSpecialization = false;
                    RPI::Ptr<RPI::ShaderOptionGroupLayout> loopLocal_ShaderOptionGroupLayout =
                        LoadShaderOptionsGroupLayoutFromShaderAssetBuilder(
                            shaderPlatformInterface,
                            request.m_platformInfo,
                            azslc,
                            shaderSourceFileFullPath,
                            supervariantIndex,
                            usesSpecialization);
                    if (!loopLocal_ShaderOptionGroupLayout)
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }
                    if (shaderOptionGroupLayout && shaderOptionGroupLayout->GetHash() != loopLocal_ShaderOptionGroupLayout->GetHash())
                    {
                        AZ_Error(
                            ShaderVariantAssetBuilderName,
                            false,
                            "There was a discrepancy in shader options between %s and %s",
                            previousLoopApiName.c_str(),
                            thisLoopApiName.data());
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }

                    // Check if there's a supervariant that needs to generate the variants
                    if (!usesSpecialization || !loopLocal_ShaderOptionGroupLayout->IsFullySpecialized())
                    {
                        usesVariants = true;
                    }
                    shaderOptionGroupLayout = loopLocal_ShaderOptionGroupLayout;
                }
                previousLoopApiName = thisLoopApiName;
            }

            if (!usesVariants)
            {
                // No need to create the variant tree since all supervariants are fully specialized. Exit gracefully.
                AZ_TracePrintf(
                    ShaderVariantAssetBuilderName,
                    "No azshadervarianttree is produced on behalf of %s because all valid RHI backends are using specialization constants for shader options.\n",
                    shaderSourceFileFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            AZStd::vector<RPI::ShaderVariantListSourceData::VariantInfo> variantInfos;
            variantInfos.reserve(hashedVariantListDescriptor.m_hashedVariants.size());
            for (const auto& hashedVariantInfo : hashedVariantListDescriptor.m_hashedVariants)
            {
                variantInfos.push_back(hashedVariantInfo.m_variantInfo);
            }
            
            RPI::ShaderVariantTreeAssetCreator shaderVariantTreeAssetCreator;
            shaderVariantTreeAssetCreator.Begin(Uuid::CreateRandom());
            shaderVariantTreeAssetCreator.SetShaderOptionGroupLayout(*shaderOptionGroupLayout);
            shaderVariantTreeAssetCreator.SetVariantInfos(variantInfos);
            Data::Asset<RPI::ShaderVariantTreeAsset> shaderVariantTreeAsset;
            if (!shaderVariantTreeAssetCreator.End(shaderVariantTreeAsset))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to build Shader Variant Tree Asset");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            AZStd::string filename = AZStd::string::format("%s.%s", shaderName.c_str(), RPI::ShaderVariantTreeAsset::Extension);
            AZStd::string assetPath;
            AZ::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), filename.c_str(), assetPath, true);
            if (!AZ::Utils::SaveObjectToFile(assetPath, AZ::DataStream::ST_BINARY, shaderVariantTreeAsset.Get()))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to save Shader Variant Tree Asset to \"%s\"", assetPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            AssetBuilderSDK::JobProduct assetProduct;
            assetProduct.m_productSubID = RPI::ShaderVariantTreeAsset::ProductSubID;
            assetProduct.m_productFileName = assetPath;
            assetProduct.m_productAssetType = azrtti_typeid<RPI::ShaderVariantTreeAsset>();
            assetProduct.m_dependenciesHandled = true; // This builder has no dependencies to output
            response.m_outputProducts.push_back(assetProduct);

            AZ_TracePrintf(ShaderVariantAssetBuilderName, "Shader Variant Tree Asset [%s] compiled successfully.\n", assetPath.c_str());

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success; 
        }


        void ShaderVariantAssetBuilder::ProcessShaderVariantJob(
            const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            AZStd::string hashedVariantBatchFullPath;
            AZ::StringFunc::Path::ConstructFull(
                request.m_watchFolder.data(), request.m_sourceFile.data(), hashedVariantBatchFullPath, true);


            AZStd::string hashedVariantBatchDescriptorString;
            if (!request.m_jobDescription.m_jobParameters.contains(ShaderVariantBatchJobParam))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Missing job Parameter: ShaderVariantBatchJobParam");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            hashedVariantBatchDescriptorString = request.m_jobDescription.m_jobParameters.at(ShaderVariantBatchJobParam);

            HashedVariantListSourceData hashedVariantBatchDescriptor;
            if (!RPI::JsonUtils::LoadObjectFromJsonString(
                    hashedVariantBatchDescriptorString, hashedVariantBatchDescriptor))
            {
                AZ_Assert(false, "Failed to parse Hashed Variant Batch Descriptor JSON [%s]", hashedVariantBatchFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const AZStd::string& shaderSourceFileFullPath = hashedVariantBatchDescriptor.m_shaderPath;
            AZStd::string shaderFileName;
            AZ::StringFunc::Path::GetFileName(shaderSourceFileFullPath.c_str(), shaderFileName);

            RPI::ShaderSourceData shaderSourceDescriptor;
            AZStd::shared_ptr<ShaderFiles> sources =
                ShaderBuilderUtility::PrepareSourceInput(ShaderVariantAssetBuilderName, shaderSourceFileFullPath, shaderSourceDescriptor);

            // set the input file for eventual error messages, but the compiler won't be called on it.
            AzslCompiler azslc(sources->m_azslSourceFullPath, request.m_tempDirPath);

            // Request the list of valid shader platform interfaces for the target platform.
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces =
                ShaderBuilderUtility::DiscoverEnabledShaderPlatformInterfaces(request.m_platformInfo, shaderSourceDescriptor);
            if (platformInterfaces.empty())
            {
                // No work to do. Exit gracefully.
                AZ_TracePrintf(
                    ShaderVariantAssetBuilderName,
                    "No azshader is produced on behalf of %s because all valid RHI backends were disabled for this shader.\n",
                    shaderSourceFileFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            auto supervariantList = ShaderBuilderUtility::GetSupervariantListFromShaderSourceData(shaderSourceDescriptor);

            ShaderBuildArgumentsManager buildArgsManager;
            buildArgsManager.Init();
            // A job always runs on behalf of an Asset Processing platform (aka PlatformInfo).
            // Let's merge the shader build arguments of the current PlatformInfo with the global
            // set of arguments.
            const auto platformName = ShaderBuilderUtility::GetPlatformNameFromPlatformInfo(request.m_platformInfo);
            buildArgsManager.PushArgumentScope(platformName);

            //! The ShaderOptionGroupLayout is common across all RHIs & Supervariants
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = nullptr;

            // Generate shaders for each of those ShaderPlatformInterfaces.
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                AZStd::string apiName(shaderPlatformInterface->GetAPIName().GetCStr());
                AZ_TraceContext("Platform API", apiName);

                buildArgsManager.PushArgumentScope(apiName);
                buildArgsManager.PushArgumentScope(
                    shaderSourceDescriptor.m_removeBuildArguments,
                    shaderSourceDescriptor.m_addBuildArguments,
                    shaderSourceDescriptor.m_definitions);

                // Loop through all the Supervariants.
                for (uint32_t supervariantIndexCounter = 0;
                    supervariantIndexCounter < supervariantList.size();
                    ++supervariantIndexCounter)
                {
                    const auto& supervariantInfo = supervariantList[supervariantIndexCounter];
                    RPI::SupervariantIndex supervariantIndex(supervariantIndexCounter);

                    // Check if we were canceled before we do any heavy processing of
                    // the shader variant data.
                    if (jobCancelListener.IsCancelled())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                        return;
                    }

                    buildArgsManager.PushArgumentScope(
                        supervariantInfo.m_removeBuildArguments, supervariantInfo.m_addBuildArguments, supervariantInfo.m_definitions);

                    AZStd::string shaderStemNamePrefix = shaderFileName;
                    if (supervariantIndex.GetIndex() > 0)
                    {
                        shaderStemNamePrefix += AZStd::string::format("-%s", supervariantInfo.m_name.GetCStr());
                    }

                    // We need these additional pieces of information To build a shader variant asset:
                    // 1- ShaderOptionsGroupLayout (Need to load it once, because it's the same acrosss all supervariants +  RHIs)
                    // 2- entryFunctions
                    // 3- hlsl code.

                    // 1- ShaderOptionsGroupLayout
                    // The ShaderOptionsGroupLayout is the same for all platforms and supervariants, but the each supervariant
                    // can have the use of specialization constants on or off.
                    bool usesSpecializationConstants = false;
                    shaderOptionGroupLayout = LoadShaderOptionsGroupLayoutFromShaderAssetBuilder(
                        shaderPlatformInterface,
                        request.m_platformInfo,
                        azslc,
                        shaderSourceFileFullPath,
                        supervariantIndex,
                        usesSpecializationConstants);
                    if (!shaderOptionGroupLayout)
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }

                    if (usesSpecializationConstants && shaderOptionGroupLayout->IsFullySpecialized())
                    {
                        // No need to create the shader variants since all supervariants are fully specialized.
                        AZ_TracePrintf(
                            ShaderVariantAssetBuilderName,
                            "No azshaderVariant is produced on behalf of %s, super variant %s, because it's using specialization "
                            "constants "
                            "for shader options.\n",
                            shaderSourceFileFullPath.c_str(),
                            supervariantInfo.m_name.GetCStr());
                        buildArgsManager.PopArgumentScope();
                        continue;
                    }

                    // 2- entryFunctions.
                    AzslFunctions azslFunctions;
                    LoadShaderFunctionsFromShaderAssetBuilder(
                        shaderPlatformInterface, request.m_platformInfo, azslc, shaderSourceFileFullPath, supervariantIndex, azslFunctions);
                    if (azslFunctions.empty())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }
                    MapOfStringToStageType shaderEntryPoints;
                    if (shaderSourceDescriptor.m_programSettings.m_entryPoints.empty())
                    {
                        AZ_Error(ShaderVariantAssetBuilderName, false, "ProgramSettings must specify entry points.");
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }

                    for (const auto& entryPoint : shaderSourceDescriptor.m_programSettings.m_entryPoints)
                    {
                        shaderEntryPoints[entryPoint.m_name] = entryPoint.m_type;
                    }

                    // 3- hlslCode
                    AZStd::string hlslSourcePath;
                    AZStd::string hlslCode = LoadHlslFileFromShaderAssetBuilder(
                        shaderPlatformInterface, request.m_platformInfo, shaderSourceFileFullPath, supervariantIndex, hlslSourcePath);
                    if (hlslCode.empty() || hlslSourcePath.empty())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }

                    //! It is important to keep this refcounted pointer outside of the if block to prevent it from being destroyed.
                    RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor;
                    if (shaderPlatformInterface->VariantCompilationRequiresSrgLayoutData())
                    {
                        RPI::ShaderResourceGroupLayoutList srgLayoutList;
                        RootConstantData rootConstantData;
                        if (!LoadSrgLayoutListFromShaderAssetBuilder(
                                shaderPlatformInterface,
                                request.m_platformInfo,
                                azslc,
                                shaderSourceFileFullPath,
                                supervariantIndex,
                                srgLayoutList,
                                rootConstantData))
                        {
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }

                        BindingDependencies bindingDependencies;
                        if (!LoadBindingDependenciesFromShaderAssetBuilder(
                                shaderPlatformInterface,
                                request.m_platformInfo,
                                azslc,
                                shaderSourceFileFullPath,
                                supervariantIndex,
                                bindingDependencies))
                        {
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }

                        pipelineLayoutDescriptor = ShaderBuilderUtility::BuildPipelineLayoutDescriptorForApi(
                            ShaderVariantAssetBuilderName,
                            srgLayoutList,
                            shaderEntryPoints,
                            buildArgsManager.GetCurrentArguments(),
                            rootConstantData,
                            shaderPlatformInterface,
                            bindingDependencies);
                        if (!pipelineLayoutDescriptor)
                        {
                            AZ_Error(
                                ShaderVariantAssetBuilderName,
                                false,
                                "Failed to build pipeline layout descriptor for api=[%s]",
                                shaderPlatformInterface->GetAPIName().GetCStr());
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                    }

                    // Setup the shader variant creation context:
                    ShaderVariantCreationContext shaderVariantCreationContext = { *shaderPlatformInterface,
                                                                                  request.m_platformInfo,
                                                                                  buildArgsManager.GetCurrentArguments(),
                                                                                  request.m_tempDirPath,
                                                                                  shaderSourceDescriptor,
                                                                                  *shaderOptionGroupLayout.get(),
                                                                                  shaderEntryPoints,
                                                                                  Uuid::CreateRandom(),
                                                                                  shaderStemNamePrefix,
                                                                                  hlslSourcePath,
                                                                                  hlslCode,
                                                                                  usesSpecializationConstants };

                    // Preserve the Temp folder when shaders are compiled with debug symbols
                    // or because the ShaderSourceData has m_keepTempFolder set to true.
                    response.m_keepTempFolder |= shaderVariantCreationContext.m_shaderBuildArguments.m_generateDebugInfo ||
                        shaderSourceDescriptor.m_keepTempFolder || RHI::IsGraphicsDevModeEnabled();

                    for (const HashedVariantInfoSourceData& hashedVariantInfoDescriptor : hashedVariantBatchDescriptor.m_hashedVariants)
                    {
                        const RPI::ShaderVariantListSourceData::VariantInfo& variantInfo = hashedVariantInfoDescriptor.m_variantInfo;

                        AZStd::optional<RHI::ShaderPlatformInterface::ByProducts> outputByproducts;
                        auto shaderVariantAssetOutcome =
                            CreateShaderVariantAsset(variantInfo, shaderVariantCreationContext, outputByproducts);
                        if (!shaderVariantAssetOutcome.IsSuccess())
                        {
                            AZ_Error(ShaderVariantAssetBuilderName, false, "%s\n", shaderVariantAssetOutcome.GetError().c_str());
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                        Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset = shaderVariantAssetOutcome.TakeValue();

                        // Time to save the asset in the tmp folder so it ends up in the Cache folder.
                        const uint32_t productSubID = RPI::ShaderVariantAsset::MakeAssetProductSubId(
                            shaderPlatformInterface->GetAPIUniqueIndex(), supervariantIndex.GetIndex(), shaderVariantAsset->GetStableId());
                        AssetBuilderSDK::JobProduct assetProduct;
                        if (!SerializeOutShaderVariantAsset(
                                shaderVariantAsset,
                                shaderStemNamePrefix,
                                request.m_tempDirPath,
                                *shaderPlatformInterface,
                                productSubID,
                                assetProduct))
                        {
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                        response.m_outputProducts.push_back(assetProduct);

                        if (outputByproducts)
                        {
                            // add byproducts as job output products:
                            uint32_t subProductType = RPI::ShaderVariantAsset::ShaderVariantAssetSubProductType;
                            for (const AZStd::string& byproduct : outputByproducts.value().m_intermediatePaths)
                            {
                                AssetBuilderSDK::JobProduct jobProduct;
                                jobProduct.m_productFileName = byproduct;
                                jobProduct.m_productAssetType = Uuid::CreateName("DebugInfoByProduct-PdbOrDxilTxt");
                                jobProduct.m_productSubID = RPI::ShaderVariantAsset::MakeAssetProductSubId(
                                    shaderPlatformInterface->GetAPIUniqueIndex(),
                                    supervariantIndex.GetIndex(),
                                    shaderVariantAsset->GetStableId(),
                                    subProductType++);
                                response.m_outputProducts.push_back(AZStd::move(jobProduct));
                            }
                        }
                    }

                    buildArgsManager.PopArgumentScope(); // Pop the supervariant build arguments.
                } // End of supervariant for block

                buildArgsManager.PopArgumentScope(); // Pop the .shader build arguments.
                buildArgsManager.PopArgumentScope(); // Pop the RHI build arguments.
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }


        bool ShaderVariantAssetBuilder::SerializeOutShaderVariantAsset(
            const Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset, const AZStd::string& shaderStemNamePrefix,
            const AZStd::string& tempDirPath,
            const RHI::ShaderPlatformInterface& shaderPlatformInterface, const uint32_t productSubID, AssetBuilderSDK::JobProduct& assetProduct)
        {
            AZStd::string filename = AZStd::string::format(
                "%s_%s_%u.%s", shaderStemNamePrefix.c_str(), shaderPlatformInterface.GetAPIName().GetCStr(),
                shaderVariantAsset->GetStableId().GetIndex(), RPI::ShaderVariantAsset::Extension);

            AZStd::string assetPath;
            AZ::StringFunc::Path::ConstructFull(tempDirPath.c_str(), filename.c_str(), assetPath, true);

            if (!AZ::Utils::SaveObjectToFile(assetPath, AZ::DataStream::ST_BINARY, shaderVariantAsset.Get()))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to save Shader Variant Asset to \"%s\"", assetPath.c_str());
                return false;
            }

            assetProduct.m_productSubID = productSubID;
            assetProduct.m_productFileName = assetPath;
            assetProduct.m_productAssetType = azrtti_typeid<RPI::ShaderVariantAsset>();
            assetProduct.m_dependenciesHandled = true; // This builder has no dependencies to output

            AZ_TracePrintf(ShaderVariantAssetBuilderName, "Shader Variant Asset [%s] compiled successfully.\n", assetPath.c_str());
            return true;
        }


        AZ::Outcome<Data::Asset<RPI::ShaderVariantAsset>, AZStd::string> ShaderVariantAssetBuilder::CreateShaderVariantAsset(
            const RPI::ShaderVariantListSourceData::VariantInfo& shaderVariantInfo,
            ShaderVariantCreationContext& creationContext,
            AZStd::optional<RHI::ShaderPlatformInterface::ByProducts>& outputByproducts)
        {
            // Temporary structure used for sorting and caching intermediate results
            struct OptionCache
            {
                AZ::Name m_optionName;
                AZ::Name m_valueName;
                RPI::ShaderOptionIndex m_optionIndex; // Cached m_optionName
                RPI::ShaderOptionValue m_value; // Cached m_valueName
            };
            AZStd::vector<OptionCache> optionList;
            // We can not have more options than the number of options in the layout:
            optionList.reserve(creationContext.m_shaderOptionGroupLayout.GetShaderOptionCount());

            // This loop will validate and cache the indices for each option value:
            for (const auto& shaderOption : shaderVariantInfo.m_options)
            {
                Name optionName{shaderOption.first};
                Name optionValue{shaderOption.second};

                RPI::ShaderOptionIndex optionIndex = creationContext.m_shaderOptionGroupLayout.FindShaderOptionIndex(optionName);
                if (optionIndex.IsNull())
                {
                    return AZ::Failure(AZStd::string::format("Invalid shader option: %s", optionName.GetCStr()));
                }

                const RPI::ShaderOptionDescriptor& option = creationContext.m_shaderOptionGroupLayout.GetShaderOption(optionIndex);
                RPI::ShaderOptionValue value = option.FindValue(optionValue);
                if (value.IsNull())
                {
                    return AZ::Failure(
                        AZStd::string::format("Invalid value (%s) for shader option: %s", optionValue.GetCStr(), optionName.GetCStr()));
                }

                optionList.push_back(OptionCache{optionName, optionValue, optionIndex, value});
            }

            // Create one instance of the shader variant
            RPI::ShaderOptionGroup optionGroup(&creationContext.m_shaderOptionGroupLayout);

            //! Contains the series of #define macro values that define a variant. Can be empty (root variant).
            //! If this string is NOT empty, a new temporary hlsl file will be created that will be the combination
            //! of this string + @m_hlslSourceContent.
            AZStd::string hlslCodeToPrependForVariant;

            // We want to go over all options listed in the variant and set their respective values
            // This loop will populate the optionGroup and m_shaderCodePrefix in order of the option priority
            for (const auto& optionCache : optionList)
            {
                const RPI::ShaderOptionDescriptor& option = creationContext.m_shaderOptionGroupLayout.GetShaderOption(optionCache.m_optionIndex);

                // Assign the option value specified in the variant:
                option.Set(optionGroup, optionCache.m_value);

                // Populate all shader option defines. We have already confirmed they're valid.
                hlslCodeToPrependForVariant += AZStd::string::format(
                    "#define %s_OPTION_DEF %s\n", optionCache.m_optionName.GetCStr(), optionCache.m_valueName.GetCStr());
            }

            AZStd::string variantShaderSourcePath;
            // Check if we need to prepend any code prefix
            if (!hlslCodeToPrependForVariant.empty())
            {
                // Prepend any shader code prefix that we should apply to this variant
                // and save it back to a file.
                AZStd::string variantShaderSourceString(hlslCodeToPrependForVariant);
                variantShaderSourceString += creationContext.m_hlslSourceContent;

                AZStd::string shaderAssetName = AZStd::string::format(
                    "%s_%s_%u.hlsl", creationContext.m_shaderStemNamePrefix.c_str(),
                    creationContext.m_shaderPlatformInterface.GetAPIName().GetCStr(), shaderVariantInfo.m_stableId);
                AZ::StringFunc::Path::Join(
                    creationContext.m_tempDirPath.c_str(), shaderAssetName.c_str(), variantShaderSourcePath, true, true);

                auto outcome = Utils::WriteFile(variantShaderSourceString, variantShaderSourcePath);
                if (!outcome.IsSuccess())
                {
                    return AZ::Failure(AZStd::string::format("Failed to create file %s", variantShaderSourcePath.c_str()));
                }
            }
            else
            {
                variantShaderSourcePath = creationContext.m_hlslSourcePath;
            }

            AZ_TracePrintf(ShaderVariantAssetBuilderName, "Variant StableId: %u", shaderVariantInfo.m_stableId);
            AZ_TracePrintf(ShaderVariantAssetBuilderName, "Variant Shader Options: %s", optionGroup.ToString().c_str());

            const RPI::ShaderVariantStableId shaderVariantStableId{shaderVariantInfo.m_stableId};

            // By this time the optionGroup was populated with all option values for the variant and
            // the m_shaderCodePrefix contains all option related preprocessing macros
            // Let's add the requested variant:
            RPI::ShaderVariantAssetCreator variantCreator;
            RPI::ShaderOptionGroup shaderOptions{&creationContext.m_shaderOptionGroupLayout, optionGroup.GetShaderVariantId()};
            variantCreator.Begin(
                creationContext.m_shaderVariantAssetId, optionGroup.GetShaderVariantId(), shaderVariantStableId,
                shaderOptions.IsFullySpecified());

            const AZStd::unordered_map<AZStd::string, RPI::ShaderStageType>& shaderEntryPoints = creationContext.m_shaderEntryPoints;
            for (const auto& shaderEntryPoint : shaderEntryPoints)
            {
                auto shaderEntryName = shaderEntryPoint.first;
                auto shaderStageType = shaderEntryPoint.second;

                AZ_TracePrintf(ShaderVariantAssetBuilderName, "Entry Point: %s", shaderEntryName.c_str());
                AZ_TracePrintf(ShaderVariantAssetBuilderName, "Begin compiling shader function \"%s\"", shaderEntryName.c_str());

                auto assetBuilderShaderType = ShaderBuilderUtility::ToAssetBuilderShaderType(shaderStageType);

                // Compile HLSL to the platform specific shader.
                RHI::ShaderPlatformInterface::StageDescriptor descriptor;
                bool shaderWasCompiled = creationContext.m_shaderPlatformInterface.CompilePlatformInternal(
                    creationContext.m_platformInfo, variantShaderSourcePath, shaderEntryName, assetBuilderShaderType,
                    creationContext.m_tempDirPath,
                    descriptor,
                    creationContext.m_shaderBuildArguments,
                    creationContext.m_useSpecializationConstants);

                if (!shaderWasCompiled)
                {
                    return AZ::Failure(AZStd::string::format("Could not compile the shader function %s", shaderEntryName.c_str()));
                }
                // bubble up the byproducts to the caller by moving them to the context.
                outputByproducts.emplace(AZStd::move(descriptor.m_byProducts));

                RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction = creationContext.m_shaderPlatformInterface.CreateShaderStageFunction(descriptor);
                variantCreator.SetShaderFunction(ToRHIShaderStage(assetBuilderShaderType), shaderStageFunction);

                if (descriptor.m_byProducts.m_dynamicBranchCount != AZ::RHI::ShaderPlatformInterface::ByProducts::UnknownDynamicBranchCount)
                {
                    AZ_TracePrintf(
                        ShaderVariantAssetBuilderName, "Finished compiling shader function. Number of dynamic branches: %u",
                        descriptor.m_byProducts.m_dynamicBranchCount);
                }
                else
                {
                    AZ_TracePrintf(
                        ShaderVariantAssetBuilderName, "Finished compiling shader function. Number of dynamic branches: unknown");
                }
            }

            if (shaderVariantInfo.m_enableRegisterAnalysis)
            {
                if (creationContext.m_shaderPlatformInterface.GetAPIName().GetStringView() == "vulkan")
                {
                    AZ::IO::FixedMaxPath projectBuildPath = AZ::Utils::GetExecutableDirectory();
                    projectBuildPath = projectBuildPath.RemoveFilename(); // profile
                    projectBuildPath = projectBuildPath.RemoveFilename(); // bin

                    AZ::IO::FixedMaxPath spirvPath(AZStd::string_view(creationContext.m_tempDirPath));
                    spirvPath /= AZ::IO::FixedMaxPathString::format(
                        "%s_vulkan_%u.spirv.bin", creationContext.m_shaderStemNamePrefix.c_str(), shaderVariantInfo.m_stableId);

                    AZStd::string rgaCommand = AZStd::string::format(
                        "-s vk-spv-offline --isa ./disassem_%u.txt --livereg ./livereg_%u.txt --asic %s",
                        shaderVariantInfo.m_stableId,
                        shaderVariantInfo.m_stableId,
                        shaderVariantInfo.m_asic.c_str());

                    AZStd::string RgaPath;
                    if (creationContext.m_platformInfo.m_identifier == "pc")
                    {
                        RgaPath = "\\_deps\\rga-src\\rga.exe";
                    }
                    else
                    {
                        RgaPath = "/_deps/rga-src/rga";
                    }

                    AZStd::string command = AZStd::string::format(
                        "%s%s %s %s", projectBuildPath.c_str(), RgaPath.c_str(), rgaCommand.c_str(), spirvPath.c_str());
                    AZ_TracePrintf(ShaderVariantAssetBuilderName, "Rga command %s\n", command.c_str());

                    AZStd::vector<AZStd::string> fullCommand;
                    fullCommand.push_back(command);
                    AZStd::string failMessage;
                    if (LaunchRadeonGPUAnalyzer(fullCommand, creationContext.m_tempDirPath, failMessage))
                    {
                        // add rga output to the by product list
                        outputByproducts->m_intermediatePaths.insert(AZStd::string::format(
                            "./%s_disassem_%u_frag.txt", shaderVariantInfo.m_asic.c_str(), shaderVariantInfo.m_stableId));
                        outputByproducts->m_intermediatePaths.insert(AZStd::string::format(
                            "./%s_livereg_%u_frag.txt", shaderVariantInfo.m_asic.c_str(), shaderVariantInfo.m_stableId));
                    }
                    else
                    {
                        AZ_Warning(ShaderVariantAssetBuilderName, false, "%s", failMessage.c_str());
                    }
                }
                else
                {
                    AZ_Warning(
                        ShaderVariantAssetBuilderName,
                        false,
                        "Current platform is %s, register analysis is only available on Vulkan for now.",
                        creationContext.m_shaderPlatformInterface.GetAPIName().GetCStr());
                }
            }

            Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset;
            variantCreator.End(shaderVariantAsset);
            return AZ::Success(AZStd::move(shaderVariantAsset));
        }


        bool ShaderVariantAssetBuilder::LaunchRadeonGPUAnalyzer(AZStd::vector<AZStd::string> command, const AZStd::string& workingDirectory, AZStd::string& failMessage)
        {
            AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
            processLaunchInfo.m_commandlineParameters.emplace<AZStd::vector<AZStd::string>>(AZStd::move(command));
            processLaunchInfo.m_workingDirectory = workingDirectory;
            processLaunchInfo.m_showWindow = false;
            AzFramework::ProcessWatcher* watcher =
                AzFramework::ProcessWatcher::LaunchProcess(processLaunchInfo, AzFramework::COMMUNICATOR_TYPE_STDINOUT);
            if (!watcher)
            {
                failMessage = AZStd::string("Rga executable can not be launched");
                return false;
            }

            AZStd::unique_ptr<AzFramework::ProcessWatcher> watcherPtr = AZStd::unique_ptr<AzFramework::ProcessWatcher>(watcher);

            AZStd::string errorMessages;
            AZStd::string outputMessages;
            auto pumpOuputStreams = [&watcherPtr, &errorMessages, &outputMessages]()
            {
                auto communicator = watcherPtr->GetCommunicator();

                // Instead of collecting all the output in a giant string, it would be better to report
                // the chunks of messages as they arrive, but this should be good enough for now.
                if (auto byteCount = communicator->PeekError())
                {
                    AZStd::string chunk;
                    chunk.resize(byteCount);
                    communicator->ReadError(chunk.data(), byteCount);
                    errorMessages += chunk;
                }

                if (auto byteCount = communicator->PeekOutput())
                {
                    AZStd::string chunk;
                    chunk.resize(byteCount);
                    communicator->ReadOutput(chunk.data(), byteCount);
                    outputMessages += chunk;
                }
            };

            uint32_t exitCode = 0;
            bool timedOut = false;

            const AZStd::sys_time_t maxWaitTimeSeconds = 5;
            const AZStd::sys_time_t startTimeSeconds = AZStd::GetTimeNowSecond();

            while (watcherPtr->IsProcessRunning(&exitCode))
            {
                const AZStd::sys_time_t currentTimeSeconds = AZStd::GetTimeNowSecond();
                if (currentTimeSeconds - startTimeSeconds > maxWaitTimeSeconds)
                {
                    timedOut = true;
                    static const uint32_t TimeOutExitCode = 121;
                    exitCode = TimeOutExitCode;
                    watcherPtr->TerminateProcess(TimeOutExitCode);
                    break;
                }
                else
                {
                    pumpOuputStreams();
                }
            }

            AZ_Assert(!watcherPtr->IsProcessRunning(), "Rga execution failed to terminate");

            // Pump one last time to make sure the streams have been flushed
            pumpOuputStreams();

            if (timedOut)
            {
                failMessage = AZStd::string("Rga execution timed out");
                return false;
            }

            if (exitCode != 0)
            {
                failMessage = AZStd::string::format("Rga process failed, exit code %u", exitCode);
                return false;
            }

            if (!errorMessages.empty())
            {
                failMessage = AZStd::string::format("Rga report error message %s", errorMessages.c_str());
                return false;
            }

            if (!outputMessages.empty() && outputMessages.contains("Error"))
            {
                failMessage = AZStd::string::format("Rga report error message %s", outputMessages.c_str());
                return false;
            }

            return true;
        }
    } // ShaderBuilder
} // AZ
