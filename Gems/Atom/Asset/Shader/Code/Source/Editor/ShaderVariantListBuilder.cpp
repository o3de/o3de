/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Date/DateFormat.h>
#include <AzCore/Utils/Utils.h>

#include <Atom/RPI.Edit/Shader/ShaderVariantTreeAssetCreator.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

#include "HashedVariantListSourceData.h"
#include "ShaderVariantListBuilder.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        AZ_TYPE_INFO_WITH_NAME_IMPL(HashedVariantListSourceData, "HashedVariantListSourceData", "{D86DA375-DD77-45F9-81D5-2E50C24C8469}");
        AZ_TYPE_INFO_WITH_NAME_IMPL(HashedVariantListSourceData::HashedVariantInfo, "HashedVariantInfo", "{6B00EFB2-D02B-4EBD-BA8C-F4C432D234FA}");
        AZ_TYPE_INFO_WITH_NAME_IMPL(ShaderVariantListBuilder, "ShaderVariantListBuilder", "{D7FB0C17-131B-43E8-BCCC-408C1763E538}");

        void HashedVariantListSourceData::Reflect(ReflectContext* context)
        {
            // Serialization Context Reflection
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<HashedVariantInfo>()
                    ->Version(1) // Added Radeon GPU Analyzer
                    ->Field("VariantInfo", &HashedVariantInfo::m_variantInfo)
                    ->Field("Hash", &HashedVariantInfo::m_hash)
                    ->Field("IsNew", &HashedVariantInfo::m_isNew)
                    ;

                serializeContext->Class<HashedVariantListSourceData>()
                    ->Version(1)
                    ->Field("timeStamp", &HashedVariantListSourceData::m_timeStamp)
                    ->Field("Shader", &HashedVariantListSourceData::m_shaderFilePath)
                    ->Field("HashedVariants", &HashedVariantListSourceData::m_hashedVariants)
                    ;
            }
        }

        size_t HashedVariantListSourceData::HashedVariantInfo::HashCombineShaderOptionValues(size_t startingHash, const AZ::RPI::ShaderOptionValuesSourceData& optionValues)
        {
            size_t hash = startingHash;
            AZStd::for_each(optionValues.begin(), optionValues.end(),
                [&](const AZStd::pair<Name, Name>& pair) {
                    AZStd::hash_combine(hash, pair.first.GetHash());
                    AZStd::hash_combine(hash, pair.second.GetHash());
                });
            return hash;
        }

        size_t HashedVariantListSourceData::HashedVariantInfo::CalculateHash(size_t optionValuesHash, const AZ::RPI::ShaderVariantListSourceData::VariantInfo& variantInfo)
        {
            size_t hash = !optionValuesHash ? HashCombineShaderOptionValues(0, variantInfo.m_options) : optionValuesHash;

            AZStd::hash_combine(hash, variantInfo.m_stableId);
            AZStd::hash_combine(hash, variantInfo.m_enableRegisterAnalysis);
            AZStd::hash_combine(hash, variantInfo.m_asic);

            return hash;
        }

        void HashedVariantListSourceData::HashedVariantInfo::CalculateHash(size_t optionValuesHash)
        {
            m_hash = CalculateHash(optionValuesHash, m_variantInfo);
        }

        static constexpr char ShaderVariantListBuilderName[] = "ShaderVariantListBuilder";

        //! Adds source file dependencies for every place a referenced file may appear, and detects if one of
        //! those possible paths resolves to the expected file.
        //! @param variantListFullpath - The full path to the shader variant list file being processed.
        //! @param originalShaderPath - The path to a *.shader file as described inside the shader variant list file.
        //! @param sourceFileDependencies - new source file dependencies will be added to this list
        //! @return absolute path of the shader file, if it exists, otherwise, an empty string.
        static AZStd::string GetSourceShaderAbsolutePath(
            const AZStd::string& variantListFullpath, const AZStd::string& originalShaderPath,
            AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies)
        {
            AZStd::string sourceShaderAbsolutePath;

            AZStd::vector<AZStd::string> possibleDependencies = RPI::AssetUtils::GetPossibleDependencyPaths(variantListFullpath, originalShaderPath);
            for (auto& file : possibleDependencies)
            {
                AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                sourceFileDependency.m_sourceFileDependencyPath = file;
                sourceFileDependencies.push_back(sourceFileDependency);

                if (sourceShaderAbsolutePath.empty())
                {
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    bool found = false;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, file.c_str(), sourceInfo, watchFolder);
                    if (found)
                    {
                        AZ::StringFunc::Path::Join(watchFolder.c_str(), sourceInfo.m_relativePath.c_str(), sourceShaderAbsolutePath);
                    }
                }
            }

            return sourceShaderAbsolutePath;
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
        //!     When @shaderVariantListAbsolutePath is provided by a Gem/Feature instead of the Game Project
        //!     We check if the game project already defined the shader variant list, and if it did it means
        //!     ProcessJob should do no work, but return successfully nonetheless.
        static bool ValidateShaderVariantListFolder(const AZStd::string& shaderVariantListAbsolutePath,
            const AZStd::string& shaderAbsolutePath, bool& shouldExitEarlyFromProcessJob)
        {
            AZStd::string scanFolderFullPath;
            AZStd::string shaderProductFileRelativePath;
            bool success = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success
                , &AzToolsFramework::AssetSystem::AssetSystemRequest::GenerateRelativeSourcePath
                , shaderAbsolutePath, shaderProductFileRelativePath, scanFolderFullPath);
            if (!success)
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Couldn't get the scan folder for shader [%s]", shaderAbsolutePath.c_str());
                return false;
            }
            AZ_Trace(ShaderVariantListBuilderName, "For shader [%s], Scan folder full path [%s], relative file path [%s]", shaderAbsolutePath.c_str(), scanFolderFullPath.c_str(), shaderProductFileRelativePath.c_str());

            AZStd::string shaderVariantListFileRelativePath = shaderProductFileRelativePath;
            AZ::StringFunc::Path::ReplaceExtension(shaderVariantListFileRelativePath, ShaderVariantListBuilder::Extension);// Will be RPI::ShaderVariantListSourceData::Extension);

            AZ::IO::FixedMaxPath gameProjectPath = AZ::Utils::GetProjectPath();

            auto expectedHigherPrecedenceFileFullPath = (gameProjectPath
                / RPI::ShaderVariantTreeAsset::CommonSubFolder / shaderProductFileRelativePath).LexicallyNormal();
            expectedHigherPrecedenceFileFullPath.ReplaceExtension(ShaderVariantListBuilder::Extension);// Will be AZ::RPI::ShaderVariantListSourceData::Extension);

            auto normalizedShaderVariantListFileFullPath = AZ::IO::FixedMaxPath(shaderVariantListAbsolutePath).LexicallyNormal();

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
                AZ_Info(ShaderVariantListBuilderName, "The shadervariantlist [%s] has been overriden by the game project with [%s]",
                    normalizedShaderVariantListFileFullPath.c_str(), expectedHigherPrecedenceFileFullPath.c_str());
                shouldExitEarlyFromProcessJob = true;
                return true;
            }

            // Check the "Lower Precedence" case, .shader path == .shadervariantlist path.
            AZ::IO::Path normalizedShaderFileFullPath = AZ::IO::Path(shaderAbsolutePath).LexicallyNormal();

            auto normalizedShaderFileFullPathWithoutExtension = normalizedShaderFileFullPath;
            normalizedShaderFileFullPathWithoutExtension.ReplaceExtension("");

            auto normalizedShaderVariantListFileFullPathWithoutExtension = normalizedShaderVariantListFileFullPath;
            normalizedShaderVariantListFileFullPathWithoutExtension.ReplaceExtension("");

            if (normalizedShaderFileFullPathWithoutExtension != normalizedShaderVariantListFileFullPathWithoutExtension)
            {
                AZ_Error(ShaderVariantListBuilderName, false, "For shader file at path [%s], the shader variant list [%s] is expected to be located at [%s.%s] or [%s]"
                    , normalizedShaderFileFullPath.c_str(), normalizedShaderVariantListFileFullPath.c_str(),
                    normalizedShaderFileFullPathWithoutExtension.c_str(), ShaderVariantListBuilder::Extension, // Will be RPI::ShaderVariantListSourceData::Extension,
                    expectedHigherPrecedenceFileFullPath.c_str());
                return false;
            }

            shouldExitEarlyFromProcessJob = false;
            return true;
        }

        // Returns true if @filePath is the main shadervariantlist file.
        // The main shadervariantlist file has the same name of the .shader it refers to,
        // except for the fact that it has a different extension.
        static bool IsMainShaderVariantList(const AZStd::string& filePath, const RPI::ShaderVariantListSourceData& shaderVariantList)
        {
            AZ::IO::FixedMaxPath shaderVariantListPath(filePath);
            AZ::IO::FixedMaxPath shaderPath(shaderVariantList.m_shaderFilePath);

            AZ::IO::PathView shaderVariantListStem = shaderVariantListPath.Stem();
            AZ::IO::PathView shaderStem = shaderPath.Stem();

            return shaderVariantListStem == shaderStem;
        }

        void ShaderVariantListBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            AZStd::string variantListFullPath;
            AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), variantListFullPath, true);
            AZ_Trace(ShaderVariantListBuilderName, "CreateJobs for Shader Variant List \"%s\"\n", variantListFullPath.c_str());

            RPI::ShaderVariantListSourceData shaderVariantList;
            if (!RPI::JsonUtils::LoadObjectFromFile(variantListFullPath, shaderVariantList, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Failed to parse Shader Variant List Descriptor JSON from [%s]", variantListFullPath.c_str());
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                return;
            }

            // There can be several <ShaderName>[_<*>].shadervariantlist files, if they share the same <ShaderName> then
            // we will submit a job only on behalf of the file which is named as <ShaderName>.shadervariantlist, later
            // in ProcessJob we'll merge all <ShaderName>[_<*>].shadervariantlist and eventually
            // generate a single Intermediate Source Asset called <ShaderName>.hashedvariantlist.
            if (!IsMainShaderVariantList(variantListFullPath, shaderVariantList))
            {
                // We treat it as a success.
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                return;
            }

            // Let's try to find the absolute path of the source *.shader file, and while at it, setup source dependency on the potential locations
            // of the shader. Sometimes the *.shader will show up in "Cache/Intermediate Asset/".
            const AZStd::string shaderSourceFileAbsolutePath = GetSourceShaderAbsolutePath(variantListFullPath, shaderVariantList.m_shaderFilePath, response.m_sourceFileDependencyList);
            if (shaderSourceFileAbsolutePath.empty())
            {
                // If the absolute path returns empty, it means the *.shader file doesn't exist yet, but may exist in the future.
                // Thanks to source asset dependency, whenever the shader comes up into existence, then CreateJobs for this shadervariantlist
                // will be called again.
                AssetBuilderSDK::JobDescriptor jobDescriptor;

                jobDescriptor.m_priority = -5000;
                jobDescriptor.m_critical = false;
                jobDescriptor.m_jobKey = JobKey;
                jobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);
                jobDescriptor.m_jobParameters.emplace(ShaderVariantLoadErrorParam, AZStd::string("Shader doesn't exist yet"));
                response.m_createJobOutputs.push_back(jobDescriptor);

                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                return;
            }

            // One more thing, need to make sure the shader variant list is located at an appropriate folder.
            bool shouldExitEarlyFromProcessJob = false;
            if (!ValidateShaderVariantListFolder(variantListFullPath, shaderSourceFileAbsolutePath, shouldExitEarlyFromProcessJob))
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                return;
            }


            if (shouldExitEarlyFromProcessJob)
            {
                // Let's create a fake job that will succeed and create nothing.
                // This only happens with shader variant list files provided by the engine,
                // but are overriden by the game project.
                AssetBuilderSDK::JobDescriptor jobDescriptor;
                
                jobDescriptor.m_priority = -5000;
                jobDescriptor.m_critical = false;
                // We are going to create a fake job that we know will fail.
                // We need to use a job key of a real product (In this case we are using the job key for the ShaderVariantTreeAsset).
                // Using a real product job key will guarantee to clear old errors in the future, because a successful product build will
                // replace the lingering error thanks to matching job keys.
                jobDescriptor.m_jobKey = JobKey;
                jobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);
         
                // The value doesn't matter, what matters is the presence of the key which will
                // signal that no assets should be produced on behalf of this shadervariantlist because
                // the game project overrode it.
                jobDescriptor.m_jobParameters.emplace(ShouldExitEarlyFromProcessJobParam, variantListFullPath);
                
                response.m_createJobOutputs.push_back(jobDescriptor);

                response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
                return;
            }

            // TODO. This work will be done as another PR after this work is merged.
            // See if there are other *.shadervariantlist files that are related with this one, and add them
            // as source dependency.

            // This is the real job
            {
                AssetBuilderSDK::JobDescriptor jobDescriptor;
            
                jobDescriptor.m_priority = 1;
                jobDescriptor.m_critical = false;
            
                jobDescriptor.m_jobKey = JobKey;
                jobDescriptor.SetPlatformIdentifier(AssetBuilderSDK::CommonPlatformName);

                jobDescriptor.m_jobParameters.emplace(ShaderVariantListAbsolutePathJobParam, variantListFullPath);
                jobDescriptor.m_jobParameters.emplace(ShaderAbsolutePathJobParam, shaderSourceFileAbsolutePath);
            
                response.m_createJobOutputs.push_back(jobDescriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }  // End of CreateJobs


        // This function is temporarily needed until the AssetSystem APIs provides
        // an API to locate intermediate assets.
        static AZStd::string GetAbsolutePathOfIntermediateAsset(const AZStd::string sourceShaderVariantListAbsolutePath)
        {
            AZ::Data::AssetInfo sourceInfo;
            AZStd::string watchFolder;
            bool found = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(found, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath
                , sourceShaderVariantListAbsolutePath.c_str(), sourceInfo, watchFolder);
            if (!found)
            {
                // It's ok, this could happen when this is the first time processing a shadervariantlist file.
                return AZStd::string();
            }

            AZStd::string relativePath = sourceInfo.m_relativePath;
            AZ::StringFunc::Path::ReplaceExtension(relativePath, HashedVariantListSourceData::Extension);

            AZ::IO::Path gameProjectPath = AZ::Utils::GetProjectPath().c_str();
            // Yes, the path "Cache/Intermediate Assets" is hard coded, but it should be a constant
            // from the Asset System API.
            AZ::IO::Path result = gameProjectPath / AZ::IO::Path("Cache/Intermediate Assets") / relativePath;
            AZStd::string normalizedPath = result.String();
            AZ::StringFunc::Path::Normalize(normalizedPath);
            return normalizedPath;
        }


        // Returns the count of new or modified variants.
        // If there are 3 new variants and 4 modified variants then it will return 7.
        static uint32_t ResolveNewOrModifiedVariants(const HashedVariantListSourceData& prevHashedVariantList
            , HashedVariantListSourceData& hashedVariantList/*in out*/
            , AZStd::chrono::minutes suddenChangeWaitTime)
        {
            long long timeStampDelta = hashedVariantList.m_timeStamp - prevHashedVariantList.m_timeStamp;
            auto deltaSeconds = AZStd::chrono::duration_cast<AZStd::chrono::seconds> (AZStd::chrono::nanoseconds(timeStampDelta));
            bool suddenChange = deltaSeconds < suddenChangeWaitTime;
            if (suddenChange)
            {
                AZ_Trace(ShaderVariantListBuilderName, "A sudden change occured because it's been %llu seconds.\n", deltaSeconds.count());
            }

            // Let's create a dictionary of StableId (key) to HashedVariantInfo (value) from the prevHashedVariantList
            AZStd::unordered_map<uint32_t, const HashedVariantListSourceData::HashedVariantInfo*> prevVariantInfos;
            for (const auto& hashedVariantInfo : prevHashedVariantList.m_hashedVariants)
            {
                prevVariantInfos.emplace(hashedVariantInfo.m_variantInfo.m_stableId, &hashedVariantInfo);
            }

            uint32_t count = 0;
            const auto prevEndItor = prevVariantInfos.end();
            for (auto& hashedVariantInfo : hashedVariantList.m_hashedVariants)
            {
                const auto stableId = hashedVariantInfo.m_variantInfo.m_stableId;
                const auto prevFoundItor = prevVariantInfos.find(stableId);
                if (prevFoundItor == prevEndItor)
                {
                    // Will be generated for the first time.
                    hashedVariantInfo.m_isNew = true;
                    count++;
                    continue;
                }

                const HashedVariantListSourceData::HashedVariantInfo* prevVariantInfo = prevFoundItor->second;
                if (prevVariantInfo->m_hash != hashedVariantInfo.m_hash)
                {
                    // Will be recompiled (or generated for the first time).
                    hashedVariantInfo.m_isNew = true;
                    count++;
                    continue;
                }

                if (suddenChange)
                {
                    // Preserve the previous state.
                    hashedVariantInfo.m_isNew = prevVariantInfo->m_isNew;
                    continue;
                }

                // This variant won't be recompiled.
                hashedVariantInfo.m_isNew = false;

            }
            return count;
        }


        // If successful returns the absolute path of the created HashedVariantList file in the "user/AssetProcessorTemp" folder.
        // If it fails, it returns an empty string
        static AZStd::string SaveHashedVariantListFile(const AssetBuilderSDK::ProcessJobRequest& request
            , const RPI::ShaderVariantListSourceData& shaderVariantList
            , const HashedVariantListSourceData& hashedVariantList
            , bool saveAsBinary = true)
        {
            AZ::IO::FixedMaxPath shaderPath(shaderVariantList.m_shaderFilePath);
            AZ::IO::PathView shaderStem = shaderPath.Stem();
            AZ::IO::Path outputFilePath = request.m_tempDirPath;
            outputFilePath /= AZStd::string::format("%.*s.%s", AZ_STRING_ARG(shaderStem.Native()), HashedVariantListSourceData::Extension);
            AZStd::string outputFilePathStr = outputFilePath.String();

            if (saveAsBinary)
            {
                if (!AZ::Utils::SaveObjectToFile(outputFilePathStr, AZ::DataStream::ST_BINARY
                                                 , &hashedVariantList, HashedVariantListSourceData::TYPEINFO_Uuid(), nullptr))
                {
                    AZ_Error(ShaderVariantListBuilderName, false, "Failed to create %s.\n", outputFilePathStr.c_str());
                    return "";
                }
                return outputFilePathStr;
            }

            if (!AZ::RPI::JsonUtils::SaveObjectToFile(outputFilePathStr, hashedVariantList))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Failed to create %s.\n", outputFilePathStr.c_str());
                return "";
            }

            return outputFilePathStr;
        }


        static void GetBuilderSettingsFromRegistry(bool& enableHashCompare, AZStd::chrono::minutes& suddenChangeWaitTime)
        {
            // The default values.
            bool enableHashCompareInternal = ShaderVariantListBuilder::EnableHashCompareRegistryDefaultValue;
            u64 suddenChangeWaitTimeInternal = ShaderVariantListBuilder::SuddenChangeInMinutesRegistryDefaultValue;

            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Get(enableHashCompareInternal, ShaderVariantListBuilder::EnableHashCompareRegistryKey);
                settingsRegistry->Get(suddenChangeWaitTimeInternal, ShaderVariantListBuilder::SuddenChangeInMinutesRegistryKey);
            }

            enableHashCompare = enableHashCompareInternal;
            suddenChangeWaitTime = AZStd::chrono::minutes(suddenChangeWaitTimeInternal);
        }


        void ShaderVariantListBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            const auto& jobParameters = request.m_jobDescription.m_jobParameters;

            if (jobParameters.contains(ShaderVariantLoadErrorParam))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Error during CreateJobs: %s", jobParameters.at(ShaderVariantLoadErrorParam).c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            
            if (jobParameters.contains(ShouldExitEarlyFromProcessJobParam))
            {
                AZ_Info(ShaderVariantListBuilderName, "Doing nothing on behalf of [%s] because it's been overridden by game project.", jobParameters.at(ShouldExitEarlyFromProcessJobParam).c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            AZStd::string variantListFullPath;
            if (!jobParameters.contains(ShaderVariantListAbsolutePathJobParam))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Missing job Parameter: ShaderVariantListAbsolutePathJobParam");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            variantListFullPath = jobParameters.at(ShaderVariantListAbsolutePathJobParam);

            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            RPI::ShaderVariantListSourceData shaderVariantList;
            if (!RPI::JsonUtils::LoadObjectFromFile(variantListFullPath, shaderVariantList, AZStd::numeric_limits<size_t>::max()))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Failed to parse Shader Variant List Descriptor JSON from [%s]", variantListFullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            auto resultOutcome = RPI::ShaderVariantTreeAssetCreator::ValidateStableIdsAreUnique(shaderVariantList.m_shaderVariants);
            if (!resultOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Variant info validation error: %s", resultOutcome.GetError().c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            bool enableHashCompare = ShaderVariantListBuilder::EnableHashCompareRegistryDefaultValue;
            AZStd::chrono::minutes suddenChangeWaitTime(ShaderVariantListBuilder::SuddenChangeInMinutesRegistryDefaultValue);
            GetBuilderSettingsFromRegistry(enableHashCompare, suddenChangeWaitTime);
            AZ_Warning(ShaderVariantListBuilderName, enableHashCompare, "Hash Compare has been disabled by user from registry key: %s."
                "All variants will be considered new to build.", EnableHashCompareRegistryKey);

            // As we calculate hashes for each variant, we are going to calculate the hash
            // of only the optionValues part and this will help us find variants that share the same
            // content but have different StableIds. If two variants with different StableId has the same content this will be an error.
            AZStd::unordered_map<size_t, uint32_t> hashToStableIdMap;
            HashedVariantListSourceData hashedVariantList;
            hashedVariantList.m_shaderFilePath = shaderVariantList.m_shaderFilePath;
            for (const auto& variantInfo : shaderVariantList.m_shaderVariants)
            {
                size_t optionValuesHash = HashedVariantListSourceData::HashedVariantInfo::HashCombineShaderOptionValues(0, variantInfo.m_options);
                const auto itor = hashToStableIdMap.find(optionValuesHash);
                if (itor != hashToStableIdMap.end())
                {
                    AZ_Error(ShaderVariantListBuilderName, false, "StableId [%u] has the same option values as StableId[%u].\n", variantInfo.m_stableId, itor->second);
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                hashToStableIdMap.emplace(optionValuesHash, variantInfo.m_stableId);
            
                HashedVariantListSourceData::HashedVariantInfo hashedVariantInfo;
                hashedVariantInfo.m_isNew = true;
                hashedVariantInfo.m_variantInfo = variantInfo;
                hashedVariantInfo.CalculateHash(optionValuesHash);
                hashedVariantList.m_hashedVariants.emplace_back(AZStd::move(hashedVariantInfo));
            }

            AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
            hashedVariantList.m_timeStamp = now.time_since_epoch().count();

            if (enableHashCompare)
            {
                // Time to load the previously generated *.hashedvariantlist file and compare hashes for each variantInfo.
                // hashes that match, we'll flag as `hashedVariantInfo.m_isNew = false;` so the ShaderVariantAsset does not rebuild those.
                AZStd::string previousHashedVariantListPath = GetAbsolutePathOfIntermediateAsset(variantListFullPath);
                AZ_Trace(ShaderVariantListBuilderName, "Previous path found at %s\n", previousHashedVariantListPath.c_str());
                HashedVariantListSourceData prevHashedVariantList;
                if (AZ::RPI::JsonUtils::LoadObjectFromFile(previousHashedVariantListPath, prevHashedVariantList, 10 * 1024 * 1024))
                {
                    const auto newVariantCount = ResolveNewOrModifiedVariants(prevHashedVariantList, hashedVariantList, suddenChangeWaitTime);
                    AZ_Info(ShaderVariantListBuilderName, "%u of %zu variants were found to be new.\n", newVariantCount, hashedVariantList.m_hashedVariants.size());
                }
                else
                {
                    AZ_Warning(ShaderVariantListBuilderName, false, "Failed to load previous hashedvariantlist at path: %s. All shader variants will be regenerated", previousHashedVariantListPath.c_str());
                }
            }


            auto outputFilePath = SaveHashedVariantListFile(request, shaderVariantList, hashedVariantList, false /* as binary */);
            if (outputFilePath.empty())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            // Create the intermediate source asset.
            AssetBuilderSDK::JobProduct product;
            product.m_outputFlags = AssetBuilderSDK::ProductOutputFlags::IntermediateAsset;
            product.m_dependenciesHandled = true;
            product.m_productFileName = outputFilePath;
            product.m_productAssetType = azrtti_typeid<>(hashedVariantList);
            product.m_productSubID = HashedVariantListSourceData::SubId;
            response.m_outputProducts.push_back(AZStd::move(product));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

    } // ShaderBuilder
} // AZ
