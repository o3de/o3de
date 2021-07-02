/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>

#include "AzslData.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        class AzslCompiler;
        struct ShaderFiles;
        struct BindingDependencies;
        struct RootConstantData;

        using ShaderResourceGroupAssets = AZStd::fixed_vector<Data::Asset<RPI::ShaderResourceGroupAsset>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;
        using MapOfStringToStageType = AZStd::unordered_map<AZStd::string, RPI::ShaderStageType>;

        namespace ShaderBuilderUtility
        {
            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile);

            void GetAbsolutePathToAzslFile(const AZStd::string& shaderTemplatePathAndFile, AZStd::string specifiedShaderPathAndName, AZStd::string& absoluteShaderPath);

            //! Opens and read the .shader, returns expanded file paths
            AZStd::shared_ptr<ShaderFiles> PrepareSourceInput(
                const char* builderName,
                const AZStd::string& shaderAssetSourcePath,
                RPI::ShaderSourceData& sourceAsset);

            namespace AzslSubProducts
            {
                AZ_ENUM(SuffixList, azslin, ia, om, srg, options, bindingdep, hlsl);

                using SubId = RPI::ShaderAssetSubId;
                // product sub id enumerators:
                static constexpr SubId SubList[] = {SubId::PostPreprocessingPureAzsl,
                                                    SubId::IaJson,
                                                    SubId::OmJson,
                                                    SubId::SrgJson,
                                                    SubId::OptionsJson,
                                                    SubId::BindingdepJson,
                                                    SubId::GeneratedHlslSource};
                // in the same order, their file name suffix (they replicate what's in AzslcMain.cpp. and hlsl corresponds to what's in AzslBuilder.cpp)
                
                // a type to declare variables holding the full paths of their files
                using Paths = AZStd::fixed_vector<AZStd::string, AZ_ARRAY_SIZE(SubList)>;
            };

            //! [GFX TODO] [ATOM-15472] Deprecated, remove when this ticket is addressed.
            //! Collects and generates the necessary data for compiling a shader.
            //! @azslData must have paths correctly set.
            //! shaderOptionGroupLayout, azslData, srgAssets get the output data.
            AssetBuilderSDK::ProcessJobResultCode PopulateAzslDataFromJsonFiles(
                const char* builderName,
                const AzslSubProducts::Paths& pathOfJsonFiles,
                AzslData& azslData,
                ShaderResourceGroupAssets& srgAssets,
                RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout,
                BindingDependencies& bindingDependencies,
                RootConstantData& rootConstantData
            );

            //! Collects all the JSON files generated during AZSL compilation and loads the data as objects.
            //! @azslData must have paths correctly set.
            //! @azslData, @srgLayoutList, @shaderOptionGroupLayout, @bindingDependencies and @rootConstantData get the output data.
            AssetBuilderSDK::ProcessJobResultCode PopulateAzslDataFromJsonFiles(
                const char* builderName, const AzslSubProducts::Paths& pathOfJsonFiles,
                const bool platformUsesRegisterSpaces, AzslData& azslData,
                RPI::ShaderResourceGroupLayoutList& srgLayoutList, RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout,
                BindingDependencies& bindingDependencies, RootConstantData& rootConstantData);


            RHI::ShaderHardwareStage ToAssetBuilderShaderType(RPI::ShaderStageType stageType);

            //! Must be called before shaderPlatformInterface->CompilePlatformInternal()
            //! This function will prune non entry functions from BindingDependencies and use the
            //! rest of input data to create a pipeline layout descriptor.
            //! The pipeline layout descriptor is returned, but the same data will also be set into the @shaderPlatformInterface
            //! object, which is why it is important to call this method before calling shaderPlatformInterface->CompilePlatformInternal().
            RHI::Ptr<RHI::PipelineLayoutDescriptor> BuildPipelineLayoutDescriptorForApi(
                const char* builderName,
                RHI::ShaderPlatformInterface* shaderPlatformInterface,
                BindingDependencies& bindingDependencies /*inout*/,
                const ShaderResourceGroupAssets& srgAssets,
                const MapOfStringToStageType& shaderEntryPoints,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments,
                const RootConstantData* rootConstantData = nullptr
            );


            //! Must be called before shaderPlatformInterface->CompilePlatformInternal()
            //! This function will prune non entry functions from BindingDependencies and use the
            //! rest of input data to create a pipeline layout descriptor.
            //! The pipeline layout descriptor is returned, but the same data will also be set into the @shaderPlatformInterface
            //! object, which is why it is important to call this method before calling shaderPlatformInterface->CompilePlatformInternal().
            RHI::Ptr<RHI::PipelineLayoutDescriptor> BuildPipelineLayoutDescriptorForApi(
                const char* builderName,
                const RPI::ShaderResourceGroupLayoutList& srgLayoutList,
                const MapOfStringToStageType& shaderEntryPoints,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments,
                const RootConstantData& rootConstantData,
                RHI::ShaderPlatformInterface* shaderPlatformInterface,
                BindingDependencies& bindingDependencies /*inout*/);


            bool CreateShaderInputAndOutputContracts(
                const AzslData& azslData, const MapOfStringToStageType& shaderEntryPoints,
                const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout, const AZStd::string& pathToOmJson,
                const AZStd::string& pathToIaJson, RPI::ShaderInputContract& shaderInputContract,
                RPI::ShaderOutputContract& shaderOutputContract, size_t& colorAttachmentCount);


            //! Returns a list of acceptable default entry point names as a single string for debug messages.
            AZStd::string GetAcceptableDefaultEntryPointNames(const AzslData& shaderData);


            //! Create a file from a string's content.
            //! That file will be named filename.api.azslin
            //! This is meant to be used at this stage:
            //!
            //!     .azsl source -> common header prepend -> preprocess -> azslc -> dxc -> cross
            //!                                                       ^here^
            AZStd::string DumpPreprocessedCode(
                const char* BuilderName,
                const AZStd::string& preprocessedCode,
                const AZStd::string& tempDirPath,
                const AZStd::string& preprocessedFileName,
                const AZStd::string& apiTypeString = "",
                bool add2 = false); // [GFX TODO] Remove add2 when [ATOM-15472]

            //! Create a file from a string's content.
            //! That file will be named filename.api.azsl.prepend
            //! This is meant to be used at this stage:
            //!
            //!     .azsl source -> common header prepend -> preprocess -> azslc -> dxc -> cross
            //!                                         ^here^
            AZStd::string DumpAzslPrependedCode(
                const char* BuilderName,
                const AZStd::string& nonPreprocessedYetAzslSource,
                const AZStd::string& tempDirPath,
                const AZStd::string& stemName,
                const AZStd::string& apiTypeString = "");

            //! "d:/p/f.e" -> "f"
            AZStd::string ExtractStemName(const char* path);

            AZStd::vector<RHI::ShaderPlatformInterface*> DiscoverValidShaderPlatformInterfaces(const AssetBuilderSDK::PlatformInfo& info);
            AZStd::vector<RHI::ShaderPlatformInterface*> DiscoverEnabledShaderPlatformInterfaces(
                const AssetBuilderSDK::PlatformInfo& info, const RPI::ShaderSourceData& shaderSourceData);

            // The idea is that the "Supervariants" json property is optional in .shader files,
            // For cases when it is not specified, this function will return a vector with one item, the default, nameless, supervariant.
            // If "Supervariants" is not empty, then this function will make sure the first supervariant in the list
            // is the default, nameless, supervariant.
            AZStd::vector<RPI::ShaderSourceData::SupervariantInfo> GetSupervariantListFromShaderSourceData(
                const RPI::ShaderSourceData& shaderSourceData);

            void GetDefaultEntryPointsFromFunctionDataList(
                const AZStd::vector<FunctionData> azslFunctionDataList,
                AZStd::unordered_map<AZStd::string, RPI::ShaderStageType>& shaderEntryPoints);

            void LogProfilingData(const char* builderName, AZStd::string_view shaderPath);

            //! Job products sub id generation helper for AzslBuilder
            uint32_t MakeAzslBuildProductSubId(RPI::ShaderAssetSubId subId, RHI::APIType apiType);

            //! Reconstructs the expected output product paths of the AzslBuilder (from the 2 arguments @azslSourceFullPath and @apiType)
            Outcome<AzslSubProducts::Paths> ObtainBuildArtifactsFromAzslBuilder(const char* builderName, const AZStd::string& azslSourceFullPath, RHI::APIType apiType, const AZStd::string& platform);

            //! Returns true if a file should skip processing.
            //! Should be called only for non *.srgi files.
            //! If the file contains "partial ShaderResourceGroup" (validated through a proper regular expression)
            //! then it is skippable. If this same file also defines non-partial SRGs it will be skipped with error.
            //! If the file doesn't contain "ShaderResourceGroup" then it does not define a ShaderResourceGroup
            //! and it is skippable too.
            enum class SrgSkipFileResult { Error, SkipFile, ContinueProcess };
            SrgSkipFileResult ShouldSkipFileForSrgProcessing(const char* builderName, const AZStd::string_view fullPath);
        }  // ShaderBuilderUtility namespace
    } // ShaderBuilder namespace
} // AZ
