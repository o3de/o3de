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

#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
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

        //! Adds source file dependencies for every place a referenced file may appear, and detects if one of
        //! those possible paths resolves to the expected file.
        //! @param currentFilePath - the full path to the file being processed
        //! @param referencedParentPath - the path to a reference file, which may be relative to the @currentFilePath, or may be a full asset path.
        //! @param sourceFileDependencies - new source file dependencies will be added to this list
        //! @param foundSourceFile - if one of the source file dependencies is found, the highest priority one will be indicated here, otherwise this will be empty.
        //! @return true if the referenced file was found and @foundSourceFile was set
        bool LocateReferencedSourceFile(
            AZStd::string_view currentFilePath, AZStd::string_view referencedParentPath,
            AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies,
            AZStd::string& foundSourceFile)
        {
            foundSourceFile.clear();

            bool found = false;

            AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDepenencyPaths(currentFilePath, referencedParentPath);
            for (auto& file : possibleDependencies)
            {
                AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                sourceFileDependency.m_sourceFileDependencyPath = file;
                sourceFileDependencies.push_back(sourceFileDependency);

                if (!found)
                {
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, file.c_str(), sourceInfo, watchFolder);

                    if (found)
                    {
                        foundSourceFile = file;
                    }
                }
            }

            return found;
        }


        //! Returns true if @sourceFileFullPath starts with a valid asset processor scan folder, false otherwise.
        //! In case of true, it splits @sourceFileFullPath into @scanFolderFullPath and @filePathFromScanFolder.
        //! @sourceFileFullPath The full path to a source asset file.
        //! @scanFolderFullPath [out] Gets the full path of the scan folder where the source file is located.
        //! @filePathFromScanFolder [out] Get the file path relative to  @scanFolderFullPath.
        static bool SplitSourceAssetPathIntoScanFolderFullPathAndRelativeFilePath(const AZStd::string& sourceFileFullPath, AZStd::string& scanFolderFullPath, AZStd::string& filePathFromScanFolder)
        {
            AZStd::vector<AZStd::string> scanFolders;
            bool success = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetSafeFolders, scanFolders);
            if (!success)
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Couldn't get the scan folders");
                return false;
            }

            for (AZStd::string scanFolder : scanFolders)
            {
                AZ::StringFunc::Path::Normalize(scanFolder);
                if (!AZ::StringFunc::StartsWith(sourceFileFullPath, scanFolder))
                {
                    continue;
                }
                const size_t scanFolderSize = scanFolder.size();
                const size_t sourcePathSize = sourceFileFullPath.size();
                scanFolderFullPath = scanFolder;
                filePathFromScanFolder = sourceFileFullPath.substr(scanFolderSize + 1, sourcePathSize - scanFolderSize - 1);
                return true;
            }

            return false;
        }

        //! Validates if a given .shadervariantlist file is located at the correct path for a given .shader full path.
        //! There are two valid paths:
        //! 1- Lower Precedence: The same folder where the .shader file is located.
        //! 2- Higher Precedence: <project-path>/ShaderVariants/<Same Scan Folder Subpath as the .shader file>.
        //! The "Higher Precedence" path gives the option to game projects to override what variants to generate. If this
        //!     file exists then the "Lower Precedence" path is disregarded.
        //! A .shader full path is located under an AP scan folder.
        //! Example: "@gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR_ForwardPass.shader"
        //!     - In this example the Scan Folder is "<atom-gem-path>/Feature/Common/Assets", while the subfolder is "Materials/Types".
        //! The "Higher Precedence" expected valid location for the .shadervariantlist would be:
        //!     - <GameProject>/ShaderVariants/Materials/Types/StandardPBR_ForwardPass.shadervariantlist.
        //! The "Lower Precedence" valid location would be:
        //!     - @gemroot:Atom_Feature_Common@/Assets/Materials/Types/StandardPBR_ForwardPass.shadervariantlist.
        //! @shouldExitEarlyFromProcessJob [out] Set to true if ProcessJob should do no work but return successfully.
        //!     Set to false if ProcessJob should do work and create assets.
        //!     When @shaderVariantListFileFullPath is provided by a Gem/Feature instead of the Game Project
        //!     We check if the game project already defined the shader variant list, and if it did it means
        //!     ProcessJob should do no work, but return successfully nonetheless.
        static bool ValidateShaderVariantListLocation(const AZStd::string& shaderVariantListFileFullPath,
            const AZStd::string& shaderFileFullPath, bool& shouldExitEarlyFromProcessJob)
        {
            AZStd::string scanFolderFullPath;
            AZStd::string shaderProductFileRelativePath;
            if (!SplitSourceAssetPathIntoScanFolderFullPathAndRelativeFilePath(shaderFileFullPath, scanFolderFullPath, shaderProductFileRelativePath))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Couldn't get the scan folder for shader [%s]", shaderFileFullPath.c_str());
                return false;
            }
            AZ_TracePrintf(ShaderVariantAssetBuilderName, "For shader [%s], Scan folder full path [%s], relative file path [%s]", shaderFileFullPath.c_str(), scanFolderFullPath.c_str(), shaderProductFileRelativePath.c_str());

            AZStd::string shaderVariantListFileRelativePath = shaderProductFileRelativePath;
            AZ::StringFunc::Path::ReplaceExtension(shaderVariantListFileRelativePath, RPI::ShaderVariantListSourceData::Extension);

            AZ::IO::FixedMaxPath gameProjectPath = AZ::Utils::GetProjectPath();

            auto expectedHigherPrecedenceFileFullPath = (gameProjectPath
                / RPI::ShaderVariantTreeAsset::CommonSubFolder / shaderProductFileRelativePath).LexicallyNormal();
            expectedHigherPrecedenceFileFullPath.ReplaceExtension(AZ::RPI::ShaderVariantListSourceData::Extension);

            auto normalizedShaderVariantListFileFullPath = AZ::IO::FixedMaxPath(shaderVariantListFileFullPath).LexicallyNormal();

            if (expectedHigherPrecedenceFileFullPath == normalizedShaderVariantListFileFullPath)
            {
                // Whenever the Game Project declares a *.shadervariantlist file we always do work.
                shouldExitEarlyFromProcessJob = false;
                return true;
            }

            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;
            bool foundHigherPrecedenceAsset = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundHigherPrecedenceAsset
                , &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath
                , expectedHigherPrecedenceFileFullPath.c_str(), assetInfo, watchFolder);
            if (foundHigherPrecedenceAsset)
            {
                AZ_TracePrintf(ShaderVariantAssetBuilderName, "The shadervariantlist [%s] has been overriden by the game project with [%s]",
                    normalizedShaderVariantListFileFullPath.c_str(), expectedHigherPrecedenceFileFullPath.c_str());
                shouldExitEarlyFromProcessJob = true;
                return true;
            }

            // Check the "Lower Precedence" case, .shader path == .shadervariantlist path.
            AZ::IO::Path normalizedShaderFileFullPath = AZ::IO::Path(shaderFileFullPath).LexicallyNormal();

            auto normalizedShaderFileFullPathWithoutExtension = normalizedShaderFileFullPath;
            normalizedShaderFileFullPathWithoutExtension.ReplaceExtension("");

            auto normalizedShaderVariantListFileFullPathWithoutExtension = normalizedShaderVariantListFileFullPath;
            normalizedShaderVariantListFileFullPathWithoutExtension.ReplaceExtension("");

            if (normalizedShaderFileFullPathWithoutExtension != normalizedShaderVariantListFileFullPathWithoutExtension)
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "For shader file at path [%s], the shader variant list [%s] is expected to be located at [%s.%s] or [%s]"
                    , normalizedShaderFileFullPath.c_str(), normalizedShaderVariantListFileFullPath.c_str(),
                    normalizedShaderFileFullPathWithoutExtension.c_str(), RPI::ShaderVariantListSourceData::Extension,
                    expectedHigherPrecedenceFileFullPath.c_str());
                return false;
            }

            shouldExitEarlyFromProcessJob = false;
            return true;
        }

        // We treat some issues as warnings and return "Success" from CreateJobs allows us to report the dependency.
        // If/when a valid dependency file appears, that will trigger the ShaderVariantAssetBuilder to run again.
        // Since CreateJobs will pass, we forward this message to ProcessJob which will report it as an error.
        struct LoadResult
        {
            enum class Code
            {
                Error,
                DeferredError,
                Success
            };

            Code m_code;
            AZStd::string m_deferredMessage; // Only used when m_code == DeferredError
        };

        static LoadResult LoadShaderVariantList(const AZStd::string& variantListFullPath, RPI::ShaderVariantListSourceData& shaderVariantList, AZStd::string& shaderSourceFileFullPath,
            bool& shouldExitEarlyFromProcessJob)
        {
            // Need to get the name of the shader file from the template so that we can preprocess the shader data and setup
            // source file dependencies.
            if (!RPI::JsonUtils::LoadObjectFromFile(variantListFullPath, shaderVariantList, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to parse Shader Variant List Descriptor JSON from [%s]", variantListFullPath.c_str());
                return LoadResult{LoadResult::Code::Error};
            }

            const AZStd::string resolvedShaderPath = AZ::RPI::AssetUtils::ResolvePathReference(variantListFullPath, shaderVariantList.m_shaderFilePath);
            if (!AZ::IO::LocalFileIO::GetInstance()->Exists(resolvedShaderPath.c_str()))
            {
                return LoadResult{LoadResult::Code::DeferredError, AZStd::string::format("The shader path [%s] was not found.", resolvedShaderPath.c_str())};
            }

            shaderSourceFileFullPath = resolvedShaderPath;

            if (!ValidateShaderVariantListLocation(variantListFullPath, shaderSourceFileFullPath, shouldExitEarlyFromProcessJob))
            {
                return LoadResult{LoadResult::Code::Error};
            }

            if (shouldExitEarlyFromProcessJob)
            {
                return LoadResult{LoadResult::Code::Success};
            }

            auto resultOutcome = RPI::ShaderVariantTreeAssetCreator::ValidateStableIdsAreUnique(shaderVariantList.m_shaderVariants);
            if (!resultOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Variant info validation error: %s", resultOutcome.GetError().c_str());
                return LoadResult{LoadResult::Code::Error};
            }

            if (!IO::FileIOBase::GetInstance()->Exists(shaderSourceFileFullPath.c_str()))
            {
                return LoadResult{LoadResult::Code::DeferredError, AZStd::string::format("ShaderSourceData file does not exist: %s.", shaderSourceFileFullPath.c_str())};
            }

            return LoadResult{LoadResult::Code::Success};
        } // LoadShaderVariantListAndAzslSource

        void ShaderVariantAssetBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            AZStd::string variantListFullPath;
            AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), variantListFullPath, true);

            AZ_TracePrintf(ShaderVariantAssetBuilderName, "CreateJobs for Shader Variant List \"%s\"\n", variantListFullPath.data());

            RPI::ShaderVariantListSourceData shaderVariantList;
            AZStd::string shaderSourceFileFullPath;
            bool shouldExitEarlyFromProcessJob = false;
            const LoadResult loadResult = LoadShaderVariantList(variantListFullPath, shaderVariantList, shaderSourceFileFullPath, shouldExitEarlyFromProcessJob);

            if (loadResult.m_code == LoadResult::Code::Error)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                return;
            }
            
            AZStd::string foundShaderFile;
            LocateReferencedSourceFile(variantListFullPath, shaderVariantList.m_shaderFilePath, response.m_sourceFileDependencyList, foundShaderFile);

            if (loadResult.m_code == LoadResult::Code::DeferredError || shouldExitEarlyFromProcessJob)
            {
                for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
                {
                    // Let's create fake jobs that will fail ProcessJob, but are useful to establish dependency on the shader file.
                    AssetBuilderSDK::JobDescriptor jobDescriptor;

                    jobDescriptor.m_priority = -5000;
                    jobDescriptor.m_critical = false;
                    jobDescriptor.m_jobKey = ShaderVariantAssetBuilderJobKey;
                    jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());
                    
                    if (!foundShaderFile.empty())
                    {
                        AssetBuilderSDK::JobDependency jobDependency;
                        jobDependency.m_jobKey = ShaderAssetBuilder::ShaderAssetBuilderJobKey;
                        jobDependency.m_platformIdentifier = info.m_identifier;
                        jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                        jobDependency.m_sourceFile.m_sourceFileDependencyPath = foundShaderFile;
                        jobDescriptor.m_jobDependencyList.push_back(jobDependency);
                    }

                    if (loadResult.m_code == LoadResult::Code::DeferredError)
                    {
                        jobDescriptor.m_jobParameters.emplace(ShaderVariantLoadErrorParam, loadResult.m_deferredMessage);
                    }

                    if (shouldExitEarlyFromProcessJob)
                    {
                        // The value doesn't matter, what matters is the presence of the key which will
                        // signal that no assets should be produced on behalf of this shadervariantlist because
                        // the game project overrode it.
                        jobDescriptor.m_jobParameters.emplace(ShouldExitEarlyFromProcessJobParam, variantListFullPath);
                    }

                    response.m_createJobOutputs.push_back(jobDescriptor);
                }
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                return;
            }
            
            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AZ_TraceContext("For platform", info.m_identifier.data());

                // First job is for the ShaderVariantTreeAsset.
                {
                    AssetBuilderSDK::JobDescriptor jobDescriptor;
                
                    // The ShaderVariantTreeAsset is high priority, but must be generated after the ShaderAsset 
                    jobDescriptor.m_priority = 1;
                    jobDescriptor.m_critical = false;
                
                    jobDescriptor.m_jobKey = GetShaderVariantTreeAssetJobKey();
                    jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());

                    if (!foundShaderFile.empty())
                    {
                        AssetBuilderSDK::JobDependency jobDependency;
                        jobDependency.m_jobKey = ShaderAssetBuilder::ShaderAssetBuilderJobKey;
                        jobDependency.m_platformIdentifier = info.m_identifier;
                        jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                        jobDependency.m_sourceFile.m_sourceFileDependencyPath = foundShaderFile;
                        jobDescriptor.m_jobDependencyList.push_back(jobDependency);
                    }
                
                    jobDescriptor.m_jobParameters.emplace(ShaderSourceFilePathJobParam, shaderSourceFileFullPath);
                
                    response.m_createJobOutputs.push_back(jobDescriptor);
                }

                // One job for each variant. Each job will produce one ".azshadervariant" per RHI per supervariant.
                for (const AZ::RPI::ShaderVariantListSourceData::VariantInfo& variantInfo : shaderVariantList.m_shaderVariants)
                {
                    AZStd::string variantInfoAsJsonString;
                    [[maybe_unused]] const bool convertSuccess = AZ::RPI::JsonUtils::SaveObjectToJsonString(variantInfo, variantInfoAsJsonString);
                    AZ_Assert(convertSuccess, "Failed to convert VariantInfo to json string");

                    AssetBuilderSDK::JobDescriptor jobDescriptor;

                    // There can be tens/hundreds of thousands of shader variants. By default each shader will get
                    // a root variant that can be used at runtime. In order to prevent the AssetProcessor from
                    // being overtaken by shader variant compilation We mark all non-root shader variant generation
                    // as non critical and very low priority.
                    jobDescriptor.m_priority = -5000;
                    jobDescriptor.m_critical = false;

                    jobDescriptor.m_jobKey = GetShaderVariantAssetJobKey(RPI::ShaderVariantStableId{variantInfo.m_stableId});
                    jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());

                    // The ShaderVariantAssets are job dependent on the ShaderVariantTreeAsset.
                    AssetBuilderSDK::SourceFileDependency fileDependency;
                    fileDependency.m_sourceFileDependencyPath = variantListFullPath;
                    AssetBuilderSDK::JobDependency variantTreeJobDependency;
                    variantTreeJobDependency.m_jobKey = GetShaderVariantTreeAssetJobKey();
                    variantTreeJobDependency.m_platformIdentifier = info.m_identifier;
                    variantTreeJobDependency.m_sourceFile = fileDependency;
                    variantTreeJobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                    jobDescriptor.m_jobDependencyList.emplace_back(variantTreeJobDependency);

                    jobDescriptor.m_jobParameters.emplace(ShaderVariantJobVariantParam, variantInfoAsJsonString);
                    jobDescriptor.m_jobParameters.emplace(ShaderSourceFilePathJobParam, shaderSourceFileFullPath);

                    response.m_createJobOutputs.push_back(jobDescriptor);
                }

            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }  // CreateJobs

        void ShaderVariantAssetBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            const auto& jobParameters = request.m_jobDescription.m_jobParameters;

            if (jobParameters.contains(ShaderVariantLoadErrorParam))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Error during CreateJobs: %s", jobParameters.at(ShaderVariantLoadErrorParam).c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            
            if (jobParameters.contains(ShouldExitEarlyFromProcessJobParam))
            {
                AZ_TracePrintf(ShaderVariantAssetBuilderName, "Doing nothing on behalf of [%s] because it's been overridden by game project.", jobParameters.at(ShouldExitEarlyFromProcessJobParam).c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

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
            const RPI::SupervariantIndex supervariantIndex)
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
            if (!azslCompiler.ParseOptionsPopulateOptionGroupLayout(jsonOutcome.GetValue(), shaderOptionGroupLayout))
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
            const bool platformUsesRegisterSpaces,
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
            if (!SrgLayoutUtility::LoadShaderResourceGroupLayouts(ShaderVariantAssetBuilderName, srgData, platformUsesRegisterSpaces, srgLayoutList))
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
            AZStd::string variantListFullPath;
            AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), variantListFullPath, true);

            RPI::ShaderVariantListSourceData shaderVariantListDescriptor;
            if (!RPI::JsonUtils::LoadObjectFromFile(variantListFullPath, shaderVariantListDescriptor, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Assert(false, "Failed to parse Shader Variant List Descriptor JSON [%s]", variantListFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const AZStd::string& shaderSourceFileFullPath = request.m_jobDescription.m_jobParameters.at(ShaderSourceFilePathJobParam);

            //For debugging purposes will create a dummy azshadervarianttree file.
            AZStd::string shaderName;
            AZ::StringFunc::Path::GetFileName(shaderSourceFileFullPath.c_str(), shaderName);

            // No error checking because the same calls were already executed during CreateJobs()
            auto descriptorParseOutcome = ShaderBuilderUtility::LoadShaderDataJson(shaderSourceFileFullPath);
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
            AzslCompiler azslc(azslFullPath);

            AZStd::string previousLoopApiName;
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                auto thisLoopApiName = shaderPlatformInterface->GetAPIName().GetStringView();
                RPI::Ptr<RPI::ShaderOptionGroupLayout> loopLocal_ShaderOptionGroupLayout =
                    LoadShaderOptionsGroupLayoutFromShaderAssetBuilder(
                        shaderPlatformInterface, request.m_platformInfo, azslc, shaderSourceFileFullPath, RPI::DefaultSupervariantIndex);
                if (!loopLocal_ShaderOptionGroupLayout)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                if (shaderOptionGroupLayout && shaderOptionGroupLayout->GetHash() != loopLocal_ShaderOptionGroupLayout->GetHash())
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false, "There was a discrepancy in shader options between %s and %s", previousLoopApiName.c_str(), thisLoopApiName.data());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                shaderOptionGroupLayout = loopLocal_ShaderOptionGroupLayout;
                previousLoopApiName = thisLoopApiName;
            }

            RPI::ShaderVariantTreeAssetCreator shaderVariantTreeAssetCreator;
            shaderVariantTreeAssetCreator.Begin(Uuid::CreateRandom());
            shaderVariantTreeAssetCreator.SetShaderOptionGroupLayout(*shaderOptionGroupLayout);
            shaderVariantTreeAssetCreator.SetVariantInfos(shaderVariantListDescriptor.m_shaderVariants);
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

        void ShaderVariantAssetBuilder::ProcessShaderVariantJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            AZStd::string fullPath;
            AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true);

            const auto& jobParameters = request.m_jobDescription.m_jobParameters;
            const AZStd::string& shaderSourceFileFullPath = jobParameters.at(ShaderSourceFilePathJobParam);
            auto descriptorParseOutcome = ShaderBuilderUtility::LoadShaderDataJson(shaderSourceFileFullPath);
            RPI::ShaderSourceData shaderSourceData = descriptorParseOutcome.TakeValue();
            AZStd::string shaderFileName;
            AZ::StringFunc::Path::GetFileName(shaderSourceFileFullPath.c_str(), shaderFileName);

            const AZStd::string& variantJsonString = jobParameters.at(ShaderVariantJobVariantParam);
            RPI::ShaderVariantListSourceData::VariantInfo variantInfo;
            [[maybe_unused]] const bool fromJsonStringSuccess = AZ::RPI::JsonUtils::LoadObjectFromJsonString(variantJsonString, variantInfo);
            AZ_Assert(fromJsonStringSuccess, "Failed to convert json string to VariantInfo");

            RPI::ShaderSourceData shaderSourceDescriptor;
            AZStd::shared_ptr<ShaderFiles> sources = ShaderBuilderUtility::PrepareSourceInput(ShaderVariantAssetBuilderName, shaderSourceFileFullPath, shaderSourceDescriptor);

            // set the input file for eventual error messages, but the compiler won't be called on it.
            AzslCompiler azslc(sources->m_azslSourceFullPath);

            // Request the list of valid shader platform interfaces for the target platform.
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces =
                ShaderBuilderUtility::DiscoverEnabledShaderPlatformInterfaces(request.m_platformInfo, shaderSourceDescriptor);
            if (platformInterfaces.empty())
            {
                // No work to do. Exit gracefully.
                AZ_TracePrintf(ShaderVariantAssetBuilderName,
                    "No azshader is produced on behalf of %s because all valid RHI backends were disabled for this shader.\n",
                    shaderSourceFileFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }
            
            const AZ::u64 shaderVariantAssetBuildTimestamp = AZStd::GetTimeUTCMilliSecond();

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
                buildArgsManager.PushArgumentScope(shaderSourceData.m_removeBuildArguments, shaderSourceData.m_addBuildArguments, shaderSourceData.m_definitions);

                // Loop through all the Supervariants.
                uint32_t supervariantIndexCounter = 0;
                for (const auto& supervariantInfo : supervariantList)
                {
                    RPI::SupervariantIndex supervariantIndex(supervariantIndexCounter);

                    // Check if we were canceled before we do any heavy processing of
                    // the shader variant data.
                    if (jobCancelListener.IsCancelled())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                        return;
                    }

                    buildArgsManager.PushArgumentScope(supervariantInfo.m_removeBuildArguments, supervariantInfo.m_addBuildArguments, supervariantInfo.m_definitions);

                    AZStd::string shaderStemNamePrefix = shaderFileName;
                    if (supervariantIndex.GetIndex() > 0)
                    {
                        shaderStemNamePrefix += supervariantInfo.m_name.GetStringView();
                    }

                    // We need these additional pieces of information To build a shader variant asset:
                    // 1- ShaderOptionsGroupLayout (Need to load it once, because it's the same acrosss all supervariants +  RHIs)
                    // 2- entryFunctions
                    // 3- hlsl code.

                    // 1- ShaderOptionsGroupLayout
                    if (!shaderOptionGroupLayout)
                    {
                        shaderOptionGroupLayout =
                            LoadShaderOptionsGroupLayoutFromShaderAssetBuilder(
                                shaderPlatformInterface, request.m_platformInfo, azslc, shaderSourceFileFullPath, supervariantIndex);
                        if (!shaderOptionGroupLayout)
                        {
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                    }

                    // 2- entryFunctions.
                    AzslFunctions azslFunctions;
                    LoadShaderFunctionsFromShaderAssetBuilder(
                        shaderPlatformInterface, request.m_platformInfo, azslc, shaderSourceFileFullPath, supervariantIndex,  azslFunctions);
                    if (azslFunctions.empty())
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }
                    MapOfStringToStageType shaderEntryPoints;
                    if (shaderSourceDescriptor.m_programSettings.m_entryPoints.empty())
                    {
                        AZ_Error(ShaderVariantAssetBuilderName, false,  "ProgramSettings must specify entry points.");
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
                        const auto& azslcArguments = buildArgsManager.GetCurrentArguments().m_azslcArguments;
                        const bool platformUsesRegisterSpaces = RHI::ShaderBuildArguments::HasArgument(azslcArguments, "--use-spaces");
                    
                        RPI::ShaderResourceGroupLayoutList srgLayoutList;
                        RootConstantData rootConstantData;
                        if (!LoadSrgLayoutListFromShaderAssetBuilder(
                            shaderPlatformInterface, request.m_platformInfo, azslc, shaderSourceFileFullPath, supervariantIndex,
                            platformUsesRegisterSpaces,
                            srgLayoutList,
                            rootConstantData))
                        {
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                        
                        BindingDependencies bindingDependencies;
                        if (!LoadBindingDependenciesFromShaderAssetBuilder(
                            shaderPlatformInterface, request.m_platformInfo, azslc, shaderSourceFileFullPath, supervariantIndex,
                            bindingDependencies))
                        {
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                        
                        pipelineLayoutDescriptor =
                            ShaderBuilderUtility::BuildPipelineLayoutDescriptorForApi(
                                ShaderVariantAssetBuilderName, srgLayoutList, shaderEntryPoints, buildArgsManager.GetCurrentArguments(), rootConstantData,
                                shaderPlatformInterface, bindingDependencies);
                        if (!pipelineLayoutDescriptor)
                        {
                            AZ_Error(
                                ShaderVariantAssetBuilderName, false, "Failed to build pipeline layout descriptor for api=[%s]",
                                shaderPlatformInterface->GetAPIName().GetCStr());
                            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                            return;
                        }
                    }

                    // Setup the shader variant creation context:
                    ShaderVariantCreationContext shaderVariantCreationContext =
                    {
                        *shaderPlatformInterface, request.m_platformInfo, buildArgsManager.GetCurrentArguments(), request.m_tempDirPath,
                        shaderVariantAssetBuildTimestamp,
                        shaderSourceDescriptor,
                        *shaderOptionGroupLayout.get(),
                        shaderEntryPoints,
                        Uuid::CreateRandom(),
                        shaderStemNamePrefix,
                        hlslSourcePath, hlslCode
                    };

                    AZStd::optional<RHI::ShaderPlatformInterface::ByProducts> outputByproducts;
                    auto shaderVariantAssetOutcome = CreateShaderVariantAsset(variantInfo, shaderVariantCreationContext, outputByproducts);
                    if (!shaderVariantAssetOutcome.IsSuccess())
                    {
                        AZ_Error(ShaderVariantAssetBuilderName, false, "%s\n", shaderVariantAssetOutcome.GetError().c_str());
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }
                    Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset = shaderVariantAssetOutcome.TakeValue();


                    // Time to save the asset in the tmp folder so it ends up in the Cache folder.
                    const uint32_t productSubID = RPI::ShaderVariantAsset::MakeAssetProductSubId(
                        shaderPlatformInterface->GetAPIUniqueIndex(), supervariantIndex.GetIndex(),
                        shaderVariantAsset->GetStableId());
                    AssetBuilderSDK::JobProduct assetProduct;
                    if (!SerializeOutShaderVariantAsset(shaderVariantAsset, shaderStemNamePrefix,
                            request.m_tempDirPath, *shaderPlatformInterface, productSubID,
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
                                shaderPlatformInterface->GetAPIUniqueIndex(), supervariantIndex.GetIndex(), shaderVariantAsset->GetStableId(),
                                subProductType++);
                            response.m_outputProducts.push_back(AZStd::move(jobProduct));
                        }
                    }
                    buildArgsManager.PopArgumentScope(); // Pop the supervariant build arguments.
                    supervariantIndexCounter++;
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
            variantCreator.SetBuildTimestamp(creationContext.m_assetBuildTimestamp);

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
                    creationContext.m_tempDirPath, descriptor, creationContext.m_shaderBuildArguments);

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
