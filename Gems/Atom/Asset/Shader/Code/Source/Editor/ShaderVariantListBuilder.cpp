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
        constexpr uint32_t HashedVariantBatchSize = 30;

        AZ_TYPE_INFO_WITH_NAME_IMPL(HashedVariantListSourceData, "HashedVariantListSourceData", "{D86DA375-DD77-45F9-81D5-2E50C24C8469}");
        AZ_TYPE_INFO_WITH_NAME_IMPL(HashedVariantInfoSourceData, "HashedVariantInfoSourceData", "{6B00EFB2-D02B-4EBD-BA8C-F4C432D234FA}");
        AZ_TYPE_INFO_WITH_NAME_IMPL(ShaderVariantListBuilder, "ShaderVariantListBuilder", "{D7FB0C17-131B-43E8-BCCC-408C1763E538}");

        void HashedVariantListSourceData::Reflect(ReflectContext* context)
        {
            // Serialization Context Reflection
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<HashedVariantInfoSourceData>()
                    ->Version(1)
                    ->Field("VariantInfo", &HashedVariantInfoSourceData::m_variantInfo)
                    ->Field("Hash", &HashedVariantInfoSourceData::m_hash)
                    ;

                serializeContext->Class<HashedVariantListSourceData>()
                    ->Version(1)
                    ->Field("ShaderPath", &HashedVariantListSourceData::m_shaderPath)
                    ->Field("HashedVariants", &HashedVariantListSourceData::m_hashedVariants)
                    ;
            }
        }

        size_t HashedVariantInfoSourceData::HashCombineShaderOptionValues(size_t startingHash, const AZ::RPI::ShaderOptionValuesSourceData& optionValues)
        {
            size_t hash = startingHash;
            AZStd::for_each(optionValues.begin(), optionValues.end(),
                [&](const AZStd::pair<Name, Name>& pair) {
                    AZStd::hash_combine(hash, pair.first.GetHash());
                    AZStd::hash_combine(hash, pair.second.GetHash());
                });
            return hash;
        }

        size_t HashedVariantInfoSourceData::CalculateHash(size_t optionValuesHash, const AZ::RPI::ShaderVariantListSourceData::VariantInfo& variantInfo)
        {
            size_t hash = !optionValuesHash ? HashCombineShaderOptionValues(0, variantInfo.m_options) : optionValuesHash;

            AZStd::hash_combine(hash, variantInfo.m_stableId);
            AZStd::hash_combine(hash, variantInfo.m_enableRegisterAnalysis);
            AZStd::hash_combine(hash, variantInfo.m_asic);

            return hash;
        }

        void HashedVariantInfoSourceData::CalculateHash(size_t optionValuesHash)
        {
            m_hash = CalculateHash(optionValuesHash, m_variantInfo);
        }

        [[maybe_unused]] static constexpr char ShaderVariantListBuilderName[] = "ShaderVariantListBuilder";

        //! Adds source file dependencies for every place a referenced file may appear, and detects if one of
        //! those possible paths resolves to the expected file.
        //! @param variantListFullpath - The full path to the shader variant list file being processed.
        //! @param originalShaderPath - The path to a *.shader file as described inside the shader variant list file.
        //! @param sourceFileDependencies - new source file dependencies will be added to this list
        //! @return absolute path of the shader file, if it exists, otherwise, an empty string.
        static AZStd::string GetSourceShaderAbsolutePath(
            const AZStd::string& variantListFullpath,
            const AZStd::string& originalShaderPath,
            AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies)
        {
            auto& sourceDependency = sourceFileDependencies.emplace_back();
            sourceDependency.m_sourceFileDependencyPath = RPI::AssetUtils::ResolvePathReference(variantListFullpath, originalShaderPath);

            AZ::Data::AssetInfo sourceInfo;
            AZStd::string watchFolder;
            bool found = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                found,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                sourceDependency.m_sourceFileDependencyPath.c_str(),
                sourceInfo,
                watchFolder);

            AZStd::string sourceShaderAbsolutePath;

            if (found)
            {
                AZ::StringFunc::Path::Join(watchFolder.c_str(), sourceInfo.m_relativePath.c_str(), sourceShaderAbsolutePath);
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

            AZ::IO::FixedMaxPath gameProjectPath = AZ::Utils::GetProjectPath();

            auto expectedHigherPrecedenceFileFullPath = (gameProjectPath
                / RPI::ShaderVariantTreeAsset::CommonSubFolder / shaderProductFileRelativePath).LexicallyNormal();
            expectedHigherPrecedenceFileFullPath.ReplaceExtension(RPI::ShaderVariantListSourceData::Extension);

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
                    normalizedShaderFileFullPathWithoutExtension.c_str(), RPI::ShaderVariantListSourceData::Extension,
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


        void ShaderVariantListBuilder::ShutDown()
        {
            // Nothing to do here.
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

            // The idea is to have "layers" of shader variant lists that can be produced by different means.
            // These layers will be merged by the ShaderVariantListBuilder to produce a single "hashedvariantlist" that prunes away
            // all the redundant elements. The main "layer" would be named <ShaderName>.shadervariantlist,
            //  the "sublayers" would be the files named <ShaderName>_<something>.shadervariantlist.
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


        // If successful returns the absolute path of the created HashedVariantList file in the "user/AssetProcessorTemp" folder.
        // If it fails, it returns an empty string
        static AZStd::string SaveHashedVariantListFile(const AssetBuilderSDK::ProcessJobRequest& request
            , const AZStd::string& shaderName
            , const HashedVariantListSourceData& hashedVariantList
            , bool saveAsBinary = true)
        {
            AZ::IO::Path outputFilePath = request.m_tempDirPath;
            outputFilePath /= AZStd::string::format("%s.%s", shaderName.c_str(), HashedVariantListSourceData::Extension);
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

        // If successful returns the absolute path of the created HashedVariantInfo file in the "user/AssetProcessorTemp" folder.
        // If it fails, it returns an empty string
        static AZStd::string SaveHashedVariantBatchFile(const AssetBuilderSDK::ProcessJobRequest& request
            , const AZStd::string& shaderName
            , const HashedVariantListSourceData& hashedVariantBatch
            , bool saveAsBinary = true)
        {
            AZ::IO::Path outputFilePath = request.m_tempDirPath;
            outputFilePath /= AZStd::string::format(
                "%s_%u.%s",
                shaderName.c_str(),
                hashedVariantBatch.m_hashedVariants.front().m_variantInfo.m_stableId / HashedVariantBatchSize,
                HashedVariantInfoSourceData::Extension);
            AZStd::string outputFilePathStr = outputFilePath.String();

            if (saveAsBinary)
            {
                if (!AZ::Utils::SaveObjectToFile(outputFilePathStr, AZ::DataStream::ST_BINARY,
                        &hashedVariantBatch,
                        HashedVariantListSourceData::TYPEINFO_Uuid(),
                        nullptr))
                {
                    AZ_Error(ShaderVariantListBuilderName, false, "Failed to create %s.\n", outputFilePathStr.c_str());
                    return "";
                }
                return outputFilePathStr;
            }

            if (!AZ::RPI::JsonUtils::SaveObjectToFile(outputFilePathStr, hashedVariantBatch))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Failed to create %s.\n", outputFilePathStr.c_str());
                return "";
            }

            return outputFilePathStr;
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

            //ShaderAbsolutePathJobParam
            AZStd::string shaderFullPath;
            if (!jobParameters.contains(ShaderAbsolutePathJobParam))
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Missing job Parameter: ShaderAbsolutePathJobParam");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            shaderFullPath = jobParameters.at(ShaderAbsolutePathJobParam);

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

            auto uniqueStableIdOutcome = RPI::ShaderVariantTreeAssetCreator::ValidateStableIdsAreUnique(shaderVariantList.m_shaderVariants);
            if (!uniqueStableIdOutcome.IsSuccess())
            {
                AZ_Error(ShaderVariantListBuilderName, false, "Variant info validation error: %s", uniqueStableIdOutcome.GetError().c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            // As we calculate hashes for each variant, we are going to calculate the hash
            // of only the optionValues part and this will help us find variants that share the same
            // content but have different StableIds. If two variants with different StableId has the same content this will be an error.
            // Shader variants are divided in batches using their stable id (so the division is stable).
            AZStd::unordered_map<size_t, uint32_t> hashToStableIdMap;
            AZStd::unordered_map<uint32_t, AZStd::vector<HashedVariantInfoSourceData>> hashedVariantBatches;
            HashedVariantListSourceData hashedVariantList;
            hashedVariantList.m_shaderPath = shaderFullPath;
            for (const auto& variantInfo : shaderVariantList.m_shaderVariants)
            {
                size_t optionValuesHash = HashedVariantInfoSourceData::HashCombineShaderOptionValues(0, variantInfo.m_options);
                const auto itor = hashToStableIdMap.find(optionValuesHash);
                if (itor != hashToStableIdMap.end())
                {
                    AZ_Error(ShaderVariantListBuilderName, false, "StableId [%u] has the same option values as StableId[%u].\n", variantInfo.m_stableId, itor->second);
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                hashToStableIdMap.emplace(optionValuesHash, variantInfo.m_stableId);
            
                HashedVariantInfoSourceData hashedVariantInfo;
                hashedVariantInfo.m_variantInfo = variantInfo;
                hashedVariantInfo.CalculateHash(optionValuesHash);
                hashedVariantList.m_hashedVariants.emplace_back(AZStd::move(hashedVariantInfo));

                hashedVariantBatches[variantInfo.m_stableId / HashedVariantBatchSize].push_back(hashedVariantList.m_hashedVariants.back());
            }

            AZ::IO::FixedMaxPath shaderPath(shaderVariantList.m_shaderFilePath);
            AZ::IO::PathView shaderStem = shaderPath.Stem();
            AZStd::string shaderName(shaderStem.Native());

            // Time to generate all Intermediate Assets:
            {
                auto outputFilePath = SaveHashedVariantListFile(request, shaderName, hashedVariantList, false /* as binary */);
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
                response.m_outputProducts.emplace_back(AZStd::move(product));
            }

            // We batch the generation of shader variants because there's some work that is common for every shader variant
            // generation. This way we save some work when building hundreads of variants.
            HashedVariantListSourceData hashedVariantBatch;
            hashedVariantBatch.m_shaderPath = hashedVariantList.m_shaderPath;
            for (auto& batch : hashedVariantBatches)
            {
                hashedVariantBatch.m_hashedVariants = AZStd::move(batch.second);
                auto outputFilePath = SaveHashedVariantBatchFile(request, shaderName, hashedVariantBatch, false /* as binary */);
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
                product.m_productAssetType = azrtti_typeid<>(hashedVariantBatch);
                product.m_productSubID =
                    HashedVariantListSourceData::SubId + batch.first + 1; // 0 is reserved for the full hashed variant list
                response.m_outputProducts.emplace_back(AZStd::move(product));
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

    } // ShaderBuilder
} // AZ
