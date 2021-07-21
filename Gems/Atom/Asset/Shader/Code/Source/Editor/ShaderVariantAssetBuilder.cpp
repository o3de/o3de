/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#include <AtomCore/Serialization/Json/JsonUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
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

#include "ShaderAssetBuilder.h"
#include "ShaderBuilderUtility.h"
#include "AzslData.h"
#include "AzslCompiler.h"
#include "AzslBuilder.h"
#include <CommonFiles/Preprocessor.h>
#include <CommonFiles/GlobalBuildOptions.h>
#include <ShaderPlatformInterfaceRequest.h>
#include "AtomShaderConfig.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        static constexpr char ShaderVariantAssetBuilderName[] = "ShaderVariantAssetBuilder";

        static constexpr uint32_t ShaderVariantLoadErrorParam = 0;
        static constexpr uint32_t ShaderSourceFilePathJobParam = 2;
        static constexpr uint32_t ShaderVariantJobVariantParam = 3;
        static constexpr uint32_t ShouldExitEarlyFromProcessJobParam = 4;

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
                AzFramework::StringFunc::Path::Normalize(scanFolder);
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
        //! 2- Higher Precedence: <DEVROOT>/<GAME>/ShaderVariants/<Same Scan Folder Subpath as the .shader file>.
        //! The "Higher Precedence" path gives the option to game projects to override what variants to generate. If this
        //!     file exists then the "Lower Precedence" path is disregarded.
        //! A .shader full path is located under an AP scan folder.
        //! Example: "<DEVROOT>/Gems/Atom/Feature/Common/Assets/Materials/Types/StandardPBR_ForwardPass.shader"
        //!     - In this example the Scan Folder is "<DEVROOT>/Gems/Atom/Feature/Common/Assets", while the subfolder is "Materials/Types".
        //! The "Higher Precedence" expected valid location for the .shadervariantlist would be:
        //!     - <DEVROOT>/<GameProject>/ShaderVariants/Materials/Types/StandardPBR_ForwardPass.shadervariantlist.
        //! The "Lower Precedence" valid location would be:
        //!     - <DEVROOT>/Gems/Atom/Feature/Common/Assets/Materials/Types/StandardPBR_ForwardPass.shadervariantlist.
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
            AzFramework::StringFunc::Path::ReplaceExtension(shaderVariantListFileRelativePath, RPI::ShaderVariantListSourceData::Extension);

            const char * gameProjectPath = nullptr;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gameProjectPath, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAbsoluteDevGameFolderPath);

            AZStd::string expectedHigherPrecedenceFileFullPath;
            AzFramework::StringFunc::Path::Join(gameProjectPath, RPI::ShaderVariantTreeAsset::CommonSubFolder, expectedHigherPrecedenceFileFullPath, false /* handle directory overlap? */, false /* be case insensitive? */);
            AzFramework::StringFunc::Path::Join(expectedHigherPrecedenceFileFullPath.c_str(), shaderProductFileRelativePath.c_str(), expectedHigherPrecedenceFileFullPath, false /* handle directory overlap? */, false /* be case insensitive? */);
            AzFramework::StringFunc::Path::ReplaceExtension(expectedHigherPrecedenceFileFullPath, AZ::RPI::ShaderVariantListSourceData::Extension);
            AzFramework::StringFunc::Path::Normalize(expectedHigherPrecedenceFileFullPath);

            AZStd::string normalizedShaderVariantListFileFullPath = shaderVariantListFileFullPath;
            AzFramework::StringFunc::Path::Normalize(normalizedShaderVariantListFileFullPath);

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
            AZStd::string normalizedShaderFileFullPath = shaderFileFullPath;
            AzFramework::StringFunc::Path::Normalize(normalizedShaderFileFullPath);

            AZStd::string normalizedShaderFileFullPathWithoutExtension = normalizedShaderFileFullPath;
            AzFramework::StringFunc::Path::StripExtension(normalizedShaderFileFullPathWithoutExtension);

            AZStd::string normalizedShaderVariantListFileFullPathWithoutExtension = normalizedShaderVariantListFileFullPath;
            AzFramework::StringFunc::Path::StripExtension(normalizedShaderVariantListFileFullPathWithoutExtension);

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
            //In certain circumstances, the capitalization of the drive letter may not match
            const bool caseSensitive = false;
#else
            //On the other platforms there's no drive letter, so it should be a non-issue.
            const bool caseSensitive = true;
#endif
            if (!StringFunc::Equal(normalizedShaderFileFullPathWithoutExtension.c_str(), normalizedShaderVariantListFileFullPathWithoutExtension.c_str(), caseSensitive))
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
            if (!RPI::JsonUtils::LoadObjectFromFile(variantListFullPath, shaderVariantList))
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
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), variantListFullPath, true);

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

                    // queue up AzslBuilder dependencies:
                    AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(info);
                    for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
                    {
                        AddAzslBuilderJobDependency(jobDescriptor, info.m_identifier, shaderPlatformInterface->GetAPIName().GetCStr(), shaderSourceFileFullPath);
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
            
            AZStd::string foundShaderFile;
            LocateReferencedSourceFile(variantListFullPath, shaderVariantList.m_shaderFilePath, response.m_sourceFileDependencyList, foundShaderFile);

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

                // One job for each variant. Each job will produce one ".azshadervariant".
                for (const AZ::RPI::ShaderVariantListSourceData::VariantInfo& variantInfo : shaderVariantList.m_shaderVariants)
                {
                    AZStd::string variantInfoAsJsonString;
                    const bool convertSuccess = AZ::RPI::JsonUtils::SaveObjectToJsonString(variantInfo, variantInfoAsJsonString);
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

        void ShaderVariantAssetBuilder::ProcessShaderVariantTreeJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AZStd::string variantListFullPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), variantListFullPath, true);

            RPI::ShaderVariantListSourceData shaderVariantListDescriptor;
            if (!RPI::JsonUtils::LoadObjectFromFile(variantListFullPath, shaderVariantListDescriptor))
            {
                AZ_Assert(false, "Failed to parse Shader Variant List Descriptor JSON [%s]", variantListFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const AZStd::string& shaderSourceFileFullPath = request.m_jobDescription.m_jobParameters.at(ShaderSourceFilePathJobParam);

            //For debugging purposes will create a dummy azshadervarianttree file.
            AZStd::string shaderName;
            AzFramework::StringFunc::Path::Split(shaderSourceFileFullPath.c_str(), nullptr /*drive*/, nullptr /*path*/, & shaderName, nullptr /*extension*/);

            RPI::ShaderSourceData shaderSourceDescriptor;
            AZStd::shared_ptr<ShaderFiles> azslSources = ShaderBuilderUtility::PrepareSourceInput(ShaderVariantAssetBuilderName, shaderSourceFileFullPath, shaderSourceDescriptor);
            if (!azslSources)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout;

            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(request.m_platformInfo);
            AzslCompiler azslc(azslSources->m_azslSourceFullPath);  // set the input file for eventual error messages, but the compiler won't be called on it.
            AZStd::string previousLoopApiName;
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                // Null backend is special and does not require any processing.
                if (shaderPlatformInterface->GetAPIUniqueIndex() == static_cast<uint32_t>(AZ::RHI::APIIndex::Null))                
                {
                    continue;
                }

                if (shaderSourceDescriptor.IsRhiBackendDisabled(shaderPlatformInterface->GetAPIName()))
                {
                    // Gracefully do nothing and continue with the next shaderPlatformInterface.
                    AZ_TracePrintf(
                        ShaderVariantAssetBuilderName, "Skipping shader variant tree compilation of [%s] for API [%s]\n",
                        shaderSourceFileFullPath.c_str(),
                        shaderPlatformInterface->GetAPIName().GetCStr());
                    continue;
                }

                auto thisLoopApiName = shaderPlatformInterface->GetAPIName().GetStringView();
                auto azslArtifactsOutcome = ShaderBuilderUtility::ObtainBuildArtifactsFromAzslBuilder(
                    ShaderVariantAssetBuilderName, azslSources->m_azslSourceFullPath, shaderPlatformInterface->GetAPIType(), request.m_platformInfo.m_identifier);
                if (!azslArtifactsOutcome.IsSuccess())
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                RPI::Ptr<RPI::ShaderOptionGroupLayout> loopLocal_ShaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();
                // The shader options define what options are available, what are the allowed values/range
                // for each option and what is its default value.
                auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(azslArtifactsOutcome.GetValue()[ShaderBuilderUtility::AzslSubProducts::options]);
                if (!jsonOutcome.IsSuccess())
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false, "%s", jsonOutcome.GetError().c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                if (!azslc.ParseOptionsPopulateOptionGroupLayout(jsonOutcome.GetValue(), loopLocal_ShaderOptionGroupLayout))
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to find a valid list of shader options!");
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
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), filename.c_str(), assetPath, true);
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

        static AZStd::pair<bool, RHI::ShaderPlatformInterface::ByProducts> CompileShaderVariantForAPI(
            Data::Asset<RPI::ShaderVariantAsset>& shaderVariantAsset,
            RHI::ShaderPlatformInterface* shaderPlatformInterface,
            AzslData& azslData,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout,
            const RPI::ShaderSourceData& shaderSourceDataDescriptor,
            AZStd::sys_time_t shaderAssetBuildTimestamp,
            const RPI::ShaderVariantListSourceData::VariantInfo& variantInfo,
            const ShaderResourceGroupAssets& srgAssets,
            const AssetBuilderSDK::ProcessJobRequest& request,
            BindingDependencies& bindingDependencies,
            const RootConstantData& rootConstantData,
            const AZStd::string& hlslSourcePath,
            const AZStd::string& hlslSourceContent,
            const AZStd::string& pathToOmJson,
            const AZStd::string& pathToIaJson)
        {
            const AZStd::string& tempDirPath = request.m_tempDirPath;
            RHI::ShaderPlatformInterface::ByProducts byproducts;

            // discover entry points
            MapOfStringToStageType shaderEntryPoints;
            if (shaderSourceDataDescriptor.m_programSettings.m_entryPoints.empty())
            {
                AZ_TracePrintf(ShaderVariantAssetBuilderName, "ProgramSettings do not specify entry points, will use GetDefaultEntryPointsFromShader()\n");
                ShaderBuilderUtility::GetDefaultEntryPointsFromFunctionDataList(azslData.m_functions, shaderEntryPoints);
            }
            else
            {
                for (auto& iter : shaderSourceDataDescriptor.m_programSettings.m_entryPoints)
                {
                    shaderEntryPoints[iter.m_name] = iter.m_type;
                }
            }

            if (!ShaderBuilderUtility::BuildPipelineLayoutDescriptorForApi(
                ShaderVariantAssetBuilderName, shaderPlatformInterface, bindingDependencies, srgAssets, shaderEntryPoints, shaderCompilerArguments, &rootConstantData))
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to build pipeline layout descriptor for api=[%s]",
                         shaderPlatformInterface->GetAPIName().GetCStr());
                return { false, byproducts };
            }

            ShaderVariantCreationContext variantCreationContext = { Uuid::CreateRandom(), hlslSourcePath, hlslSourceContent, shaderSourceDataDescriptor,
                tempDirPath, request.m_platformInfo, shaderOptionGroupLayout, shaderEntryPoints, shaderAssetBuildTimestamp };
            AZ::Outcome<Data::Asset<RPI::ShaderVariantAsset>, AZStd::string> outcomeForShaderVariantAsset = ShaderVariantAssetBuilder::CreateShaderVariantAssetForAPI(
                variantInfo,
                variantCreationContext,
                *shaderPlatformInterface,
                azslData,
                shaderCompilerArguments,
                pathToOmJson,
                pathToIaJson);
            if (!outcomeForShaderVariantAsset.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to generate shader variant with StableId [%u] for API [%s]: %s"
                    , variantInfo.m_stableId, shaderPlatformInterface->GetAPIName().GetCStr(), outcomeForShaderVariantAsset.GetError().c_str());
                return { false, byproducts };
            }
            shaderVariantAsset = outcomeForShaderVariantAsset.TakeValue();
            if (variantCreationContext.m_outputByproducts)
            {
                byproducts = *variantCreationContext.m_outputByproducts;
            }
            return { true, byproducts };
        }

        void ShaderVariantAssetBuilder::ProcessShaderVariantJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

            AZStd::string fullPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true);

            const auto& jobParameters = request.m_jobDescription.m_jobParameters;
            const AZStd::string& shaderSourceFileFullPath = jobParameters.at(ShaderSourceFilePathJobParam);
            const AZStd::string& variantJsonString = jobParameters.at(ShaderVariantJobVariantParam);
            RPI::ShaderVariantListSourceData::VariantInfo variantInfo;
            const bool toJsonStringSuccess = AZ::RPI::JsonUtils::LoadObjectFromJsonString(variantJsonString, variantInfo);
            AZ_Assert(toJsonStringSuccess, "Failed to convert json string to VariantInfo");

            auto shaderAssetOutcome = RPI::AssetUtils::LoadAsset<RPI::ShaderAsset>(shaderSourceFileFullPath);
            if (!shaderAssetOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "The shader path [%s] could not be loaded.", shaderSourceFileFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            Data::Asset<RPI::ShaderAsset> shaderAsset = shaderAssetOutcome.TakeValue();

            RPI::ShaderSourceData shaderSourceDescriptor;
            AZStd::shared_ptr<ShaderFiles> sources = ShaderBuilderUtility::PrepareSourceInput(ShaderVariantAssetBuilderName, shaderSourceFileFullPath, shaderSourceDescriptor);

            // Request the list of valid shader platform interfaces for the target platform.
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
            ShaderPlatformInterfaceRequestBus::BroadcastResult(platformInterfaces, &ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, request.m_platformInfo);
            // Generate shaders for each of those ShaderPlatformInterfaces.
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                // Null backend is special and does not require any processing.
                if (shaderPlatformInterface->GetAPIUniqueIndex() == static_cast<uint32_t>(AZ::RHI::APIIndex::Null))
                {
                    continue;
                }

                AZ_TraceContext("ShaderPlatformInterface", shaderPlatformInterface->GetAPIName().GetCStr());

                if (shaderSourceDescriptor.IsRhiBackendDisabled(shaderPlatformInterface->GetAPIName()))
                {
                    // Gracefully do nothing and continue with the next shaderPlatformInterface.
                    AZ_TracePrintf(
                        ShaderVariantAssetBuilderName, "Skipping shader variant compilation of [%s] with StableId [%u] for API [%s]\n",
                        shaderSourceFileFullPath.c_str(), variantInfo.m_stableId, shaderPlatformInterface->GetAPIName().GetCStr());
                    continue;
                }

                if (!shaderPlatformInterface)
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false, "ShaderPlatformInterface for [%s] is not registered, can't compile [%s]", request.m_platformInfo.m_identifier.c_str(), shaderSourceFileFullPath.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                // Check if we were canceled before we do any heavy processing of
                // the shader variant data.
                if (jobCancelListener.IsCancelled())
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                ShaderResourceGroupAssets srgAssets;
                RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout = RPI::ShaderOptionGroupLayout::Create();
                AzslData azslData(sources);
                if (!azslData.m_sources)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                // Load shader reflections
                auto azslArtifactsOutcome = ShaderBuilderUtility::ObtainBuildArtifactsFromAzslBuilder(
                    ShaderVariantAssetBuilderName, azslData.m_sources->m_azslSourceFullPath, shaderPlatformInterface->GetAPIType(), request.m_platformInfo.m_identifier);
                if (!azslArtifactsOutcome.IsSuccess())
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                BindingDependencies bindingDependencies;
                RootConstantData rootConstantData;
                AssetBuilderSDK::ProcessJobResultCode prepareResult = ShaderBuilderUtility::PopulateAzslDataFromJsonFiles(
                    ShaderVariantAssetBuilderName,
                    azslArtifactsOutcome.GetValue(),
                    azslData,
                    srgAssets,
                    shaderOptionGroupLayout,
                    bindingDependencies,
                    rootConstantData);
                if (prepareResult != AssetBuilderSDK::ProcessJobResult_Success)
                {
                    response.m_resultCode = prepareResult;
                    return;
                }

                AZStd::string hlslSourcePath = azslArtifactsOutcome.GetValue()[ShaderBuilderUtility::AzslSubProducts::hlsl];
                Outcome<AZStd::string, AZStd::string> hlslSourceContent = Utils::ReadFile(hlslSourcePath);
                if (!hlslSourceContent.IsSuccess())
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to obtain shader source from %s. [%s]", hlslSourcePath.c_str(), hlslSourceContent.TakeError().c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                GlobalBuildOptions buildOptions = ReadBuildOptions(ShaderVariantAssetBuilderName);

                auto shaderSourceLoadResult = ShaderBuilderUtility::LoadShaderDataJson(shaderSourceFileFullPath);
                if (!shaderSourceLoadResult.IsSuccess())
                {
                    AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to load/parse Shader Descriptor JSON: %s", shaderSourceLoadResult.GetError().c_str());
                    return;
                }

                // The idea of this merge is that we have compiler options coming from 2 source:
                // global options (from project Config/), and .shader options.
                // We define a merge behavior that is: ".shader wins if set"
                RHI::ShaderCompilerArguments mergedArguments = buildOptions.m_compilerArguments;
                mergedArguments.Merge(shaderSourceLoadResult.GetValue().m_compiler);

                Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset;
                auto [success, byproducts] = CompileShaderVariantForAPI(
                    shaderVariantAsset,
                    shaderPlatformInterface,
                    azslData,
                    mergedArguments,
                    *shaderOptionGroupLayout,
                    shaderSourceDescriptor,
                    shaderAsset->GetShaderAssetBuildTimestamp(),
                    variantInfo,
                    srgAssets,
                    request,
                    bindingDependencies,
                    rootConstantData,
                    hlslSourcePath,
                    hlslSourceContent.GetValue(),
                    azslArtifactsOutcome.GetValue()[ShaderBuilderUtility::AzslSubProducts::om],
                    azslArtifactsOutcome.GetValue()[ShaderBuilderUtility::AzslSubProducts::ia]);
                if (!success)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                // Time to save the asset in the cache tmp folder.
                const uint32_t productSubID = RPI::ShaderVariantAsset::MakeAssetProductSubId(shaderPlatformInterface->GetAPIUniqueIndex(), shaderVariantAsset->GetStableId());
                AssetBuilderSDK::JobProduct assetProduct;
                if (!SerializeOutShaderVariantAsset(shaderVariantAsset, shaderSourceFileFullPath, request.m_tempDirPath, *shaderPlatformInterface, productSubID, assetProduct))
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                response.m_outputProducts.push_back(assetProduct);

                // add byproducts as job output products:
                uint32_t subProductType = aznumeric_cast<uint32_t>(RPI::ShaderAssetSubId::GeneratedHlslSource) + 1;
                for (const AZStd::string& byproduct : byproducts.m_intermediatePaths)
                {
                    AssetBuilderSDK::JobProduct jobProduct;
                    jobProduct.m_productFileName = byproduct;
                    jobProduct.m_productAssetType = Uuid::CreateName("DebugInfoByProduct-PdbOrDxilTxt");
                    jobProduct.m_productSubID = RPI::ShaderVariantAsset::MakeAssetProductSubId(
                        shaderPlatformInterface->GetAPIType(), shaderVariantAsset->GetStableId(), subProductType++);
                    response.m_outputProducts.push_back(AZStd::move(jobProduct));
                }
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        static bool CreateShaderVariant(
            ShaderVariantCreationContext& variantCreationContext,
            const AzslData& azslData,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            RHI::ShaderPlatformInterface& shaderPlatformInterface,
            const size_t colorAttachmentCount,
            const RPI::ShaderVariantStableId variantStableId,
            RPI::ShaderVariantAssetCreator& variantCreator)
        {
            bool isVariantValid = true;

            bool hasRasterProgram = false;
            bool hasComputeProgram = false;
            bool hasRayTracingProgram = false;

            const AZStd::unordered_map<AZStd::string, RPI::ShaderStageType>& shaderEntryPoints = variantCreationContext.m_shaderEntryPoints;
            for (const auto& shaderEntryPoint : shaderEntryPoints)
            {
                auto shaderEntryName = shaderEntryPoint.first;
                auto shaderStageType = shaderEntryPoint.second;

                AZ_TracePrintf(ShaderVariantAssetBuilderName, "Entry Point: %s", shaderEntryName.c_str());
                AZ_TracePrintf(ShaderVariantAssetBuilderName, "Begin compiling shader function \"%s\"", shaderEntryName.c_str());

                auto assetBuilderShaderType = ShaderBuilderUtility::ToAssetBuilderShaderType(shaderStageType);

                AZStd::string variantShaderSourcePath;

                // Check if we need to prepend any code prefix
                if (!azslData.m_shaderCodePrefix.empty())
                {
                    // Prepend any shader code prefix that we should apply to this variant
                    // and save it back to a file.
                    AZStd::string variantShaderSourceString(azslData.m_shaderCodePrefix);
                    variantShaderSourceString += variantCreationContext.m_hlslSourceContent;

                    AZStd::string shaderAssetName = AZStd::string::format("%s_%s_%s_%u.hlsl", azslData.m_sources->m_azslFileName.c_str(), shaderEntryName.c_str(),
                        shaderPlatformInterface.GetAPIName().GetCStr(), variantStableId.GetIndex());
                    AzFramework::StringFunc::Path::Join(variantCreationContext.m_tempDirPath.c_str(), shaderAssetName.c_str(), variantShaderSourcePath, true, true);

                    auto outcome = Utils::WriteFile(variantShaderSourceString, variantShaderSourcePath);
                    if (!outcome.IsSuccess())
                    {
                        AZ_Error(ShaderVariantAssetBuilderName, false, "Failed to create file %s", variantShaderSourcePath.c_str());
                        return false;
                    }
                }
                else
                {
                    variantShaderSourcePath = variantCreationContext.m_hlslSourcePath;
                }

                // Compile HLSL to the platform specific shader.
                RHI::ShaderPlatformInterface::StageDescriptor descriptor;
                bool shaderWasCompiled = shaderPlatformInterface.CompilePlatformInternal(
                    variantCreationContext.m_platformInfo,
                    variantShaderSourcePath,
                    shaderEntryName,
                    assetBuilderShaderType,
                    variantCreationContext.m_tempDirPath,
                    descriptor,
                    shaderCompilerArguments);

                if (!shaderWasCompiled)
                {
                    isVariantValid = false;
                    AZ_Error(ShaderVariantAssetBuilderName, false, "Could not compile the shader function %s", shaderEntryName.c_str());
                    continue; // Using continue to report all the errors found
                }
                // bubble up the byproducts to the caller by moving them to the context.
                variantCreationContext.m_outputByproducts.emplace(AZStd::move(descriptor.m_byProducts));

                hasRasterProgram |= shaderPlatformInterface.IsShaderStageForRaster(assetBuilderShaderType);
                hasComputeProgram |= shaderPlatformInterface.IsShaderStageForCompute(assetBuilderShaderType);
                hasRayTracingProgram |= shaderPlatformInterface.IsShaderStageForRayTracing(assetBuilderShaderType);

                RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction = shaderPlatformInterface.CreateShaderStageFunction(descriptor);
                variantCreator.SetShaderFunction(ToRHIShaderStage(assetBuilderShaderType), shaderStageFunction);

                if (descriptor.m_byProducts.m_dynamicBranchCount != AZ::RHI::ShaderPlatformInterface::ByProducts::UnknownDynamicBranchCount)
                {
                    AZ_TracePrintf(ShaderVariantAssetBuilderName, "Finished compiling shader function. Number of dynamic branches: %u", descriptor.m_byProducts.m_dynamicBranchCount);
                }
                else
                {
                    AZ_TracePrintf(ShaderVariantAssetBuilderName, "Finished compiling shader function. Number of dynamic branches: unknown");
                }
            }

            if (hasRasterProgram && hasComputeProgram)
            {
                AZ_Error(ShaderVariantAssetBuilderName, false, "Shader asset descriptor has a program variant that defines both a raster entry point and a compute entry point.");
                isVariantValid = false;
            }

            if (!hasRasterProgram && !hasComputeProgram && !hasRayTracingProgram)
            {
                AZStd::string entryPointNames = ShaderBuilderUtility::GetAcceptableDefaultEntryPointNames(azslData);

                AZ_Error(ShaderVariantAssetBuilderName, false, "Shader asset descriptor has a program variant that does not define any entry points. Either declare entry points in the .shader file, or use one of the available default names (not case-sensitive): [%s]", entryPointNames.data());

                isVariantValid = false;
            }

            if (isVariantValid && hasRasterProgram)
            {
                // Set the various states to what is in the descriptor first as this gives us
                // the baseline. Then whatever is specified in the variant overrides the baseline
                // and that will be used by the runtime.

                RHI::TargetBlendState targetBlendState = variantCreationContext.m_shaderSourceDataDescriptor.m_blendState;
                RHI::DepthStencilState depthStencilState = variantCreationContext.m_shaderSourceDataDescriptor.m_depthStencilState;
                RHI::RasterState rasterState = variantCreationContext.m_shaderSourceDataDescriptor.m_rasterState;

                // [GFX TODO][ATOM-930] - Shader Variant System Phase 2 - Investigation and Prototyping
                // The entire Q3 epic will deal will prototyping different shader variants approaches
                // A lot of code in prototype stage is disabled before a clearer vision for the system
                // is in place, including the unclear way to set states below.
                // RHI::MergeStateInto() needs to be further reviewed because it's not possible to
                // override the base state with a variant state which sets a default value (for example)
                // The states are further duplicated which suggest that they could instead just exist
                // as variants and skip the merge step. This code, if it persists, should move to ShaderDataParser                
                // RHI::MergeStateInto(programVariant.m_blendState, targetBlendState);
                // RHI::MergeStateInto(programVariant.m_depthStencilState.m_depth, depthStencilState.m_depth);
                // RHI::MergeStateInto(programVariant.m_depthStencilState.m_stencil, depthStencilState.m_stencil);
                // RHI::MergeStateInto(programVariant.m_rasterState, rasterState);

                RHI::RenderStates renderStates;
                renderStates.m_rasterState = rasterState;
                renderStates.m_depthStencilState = depthStencilState;
                // [GFX TODO][ATOM-930] We should support unique blend states per RT
                for (size_t i = 0; i < colorAttachmentCount; ++i)
                {
                    renderStates.m_blendState.m_targets[i] = targetBlendState;
                }

                variantCreator.SetRenderStates(renderStates);
            }

            return isVariantValid;
        }


        AZ::Outcome<Data::Asset<RPI::ShaderVariantAsset>, AZStd::string>  ShaderVariantAssetBuilder::CreateShaderVariantAssetForAPI(
            const RPI::ShaderVariantListSourceData::VariantInfo& variantInfo,
            ShaderVariantCreationContext& variantCreationContext,
            RHI::ShaderPlatformInterface& shaderPlatformInterface,
            AzslData& azslData,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            const AZStd::string& pathToOmJson,
            const AZStd::string& pathToIaJson)
        {
            RPI::ShaderInputContract shaderInputContract;
            RPI::ShaderOutputContract shaderOutputContract;
            size_t colorAttachmentCount = 0;
            ShaderBuilderUtility::CreateShaderInputAndOutputContracts(azslData, variantCreationContext.m_shaderEntryPoints, variantCreationContext.m_shaderOptionGroupLayout, pathToOmJson,
                pathToIaJson, shaderInputContract, shaderOutputContract, colorAttachmentCount);

            const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout = variantCreationContext.m_shaderOptionGroupLayout;
            // Temporary structure used for sorting and caching intermediate results
            struct OptionCache
            {
                AZ::Name m_optionName;
                AZ::Name m_valueName;
                RPI::ShaderOptionIndex m_optionIndex; // Cached m_optionName
                RPI::ShaderOptionValue m_value;  // Cached m_valueName
            };
            AZStd::vector<OptionCache> optionList;
            // We can not have more options than the number of options in the layout:
            optionList.reserve(variantCreationContext.m_shaderOptionGroupLayout.GetShaderOptionCount());

            // This loop will validate and cache the indices for each option value:
            for (const auto& shaderOption : variantInfo.m_options)
            {
                Name optionName{ shaderOption.first };
                Name optionValue{ shaderOption.second };

                RPI::ShaderOptionIndex optionIndex = shaderOptionGroupLayout.FindShaderOptionIndex(optionName);
                if (optionIndex.IsNull())
                {
                    return AZ::Failure(AZStd::string::format("Invalid shader option: %s", optionName.GetCStr()));
                }

                const RPI::ShaderOptionDescriptor& option = shaderOptionGroupLayout.GetShaderOption(optionIndex);
                RPI::ShaderOptionValue value = option.FindValue(optionValue);
                if (value.IsNull())
                {
                    return AZ::Failure(AZStd::string::format("Invalid value (%s) for shader option: %s", optionValue.GetCStr(), optionName.GetCStr()));
                }

                optionList.push_back(OptionCache{ optionName, optionValue, optionIndex, value });
            }

            // Create one instance of the shader variant
            RPI::ShaderOptionGroup optionGroup(&shaderOptionGroupLayout);

            // m_shaderCodePrefix contains preprocessing macro defines to switch options on and off in our shader binary
            // Clear it for this variant so we can add each option as we define it
            azslData.m_shaderCodePrefix.clear();

            // We want to go over all options listed in the variant and set their respective values
            // This loop will populate the optionGroup and m_shaderCodePrefix in order of the option priority
            for (const auto& optionCache : optionList)
            {
                const RPI::ShaderOptionDescriptor& option = shaderOptionGroupLayout.GetShaderOption(optionCache.m_optionIndex);

                // Assign the option value specified in the variant:
                option.Set(optionGroup, optionCache.m_value);

                // Populate all shader option defines. We have already confirmed they're valid.
                azslData.m_shaderCodePrefix += AZStd::string::format("#define %s_OPTION_DEF %s\n", optionCache.m_optionName.GetCStr(), optionCache.m_valueName.GetCStr());
            }

            AZ_TracePrintf(ShaderVariantAssetBuilderName, "Variant StableId: %u", variantInfo.m_stableId);
            AZ_TracePrintf(ShaderVariantAssetBuilderName, "Variant Shader Options: %s", optionGroup.ToString().c_str());

            const RPI::ShaderVariantStableId shaderVariantStableId{variantInfo.m_stableId};

            // By this time the optionGroup was populated with all option values for the variant and
            // the m_shaderCodePrefix contains all option related preprocessing macros
            // Let's add the requested variant:
            RPI::ShaderVariantAssetCreator shaderVariantAssetCreator;
            shaderVariantAssetCreator.Begin(variantCreationContext.m_assetId, optionGroup.GetShaderVariantId(), shaderVariantStableId, & shaderOptionGroupLayout);
            shaderVariantAssetCreator.SetShaderAssetBuildTimestamp(variantCreationContext.m_shaderAssetBuildTimestamp);
            shaderVariantAssetCreator.SetInputContract(shaderInputContract);
            shaderVariantAssetCreator.SetOutputContract(shaderOutputContract);

            if (!CreateShaderVariant(
                variantCreationContext,
                azslData,
                shaderCompilerArguments,
                shaderPlatformInterface,
                colorAttachmentCount,
                shaderVariantStableId,
                shaderVariantAssetCreator))
            {
                return AZ::Failure(AZStd::string::format("Failed to create shader variant with StableId=%u", shaderVariantStableId.GetIndex()));
            }

            Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset;
            shaderVariantAssetCreator.End(shaderVariantAsset);
            return AZ::Success(AZStd::move(shaderVariantAsset));
        }

        bool ShaderVariantAssetBuilder::SerializeOutShaderVariantAsset(const Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset, const AZStd::string& shaderSourceFileFullPath, const AZStd::string& tempDirPath,
            const RHI::ShaderPlatformInterface& shaderPlatformInterface, const uint32_t productSubID, AssetBuilderSDK::JobProduct& assetProduct)
        {
            AZStd::string shaderName;
            AzFramework::StringFunc::Path::Split(shaderSourceFileFullPath.c_str(), nullptr /*drive*/, nullptr /*path*/, &shaderName, nullptr /*extension*/);
            AZStd::string filename = AZStd::string::format("%s_%s_%u.%s", shaderName.c_str(), shaderPlatformInterface.GetAPIName().GetCStr(), shaderVariantAsset->GetStableId().GetIndex(), RPI::ShaderVariantAsset::Extension);

            AZStd::string assetPath;
            AzFramework::StringFunc::Path::ConstructFull(tempDirPath.c_str(), filename.c_str(), assetPath, true);

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

    } // ShaderBuilder
} // AZ
