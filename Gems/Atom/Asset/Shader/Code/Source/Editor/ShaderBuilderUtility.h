/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>


namespace AZ
{
    namespace ShaderBuilder
    {
        class AzslCompiler;
        struct ShaderFiles;
        struct AzslData;
        struct BindingDependencies;
        struct RootConstantData;

        using ShaderResourceGroupAssets = AZStd::fixed_vector<Data::Asset<RPI::ShaderResourceGroupAsset>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;
        using MapOfStringToStageType = AZStd::unordered_map<AZStd::string, RPI::ShaderStageType>;

        namespace ShaderBuilderUtility
        {
            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile);

            void GetAbsolutePathToAzslFile(const AZStd::string& shaderTemplatePathAndFile, AZStd::string specifiedShaderPathAndName, AZStd::string& absoluteShaderPath);

            uint32_t MakeDebugByproductSubId(RHI::APIType apiType, const AZStd::string& productFileName);

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
                static constexpr SubId SubList[] = { SubId::PostPreprocessingPureAzsl, SubId::IaJson, SubId::OmJson, SubId::SrgJson, SubId::OptionsJson, SubId::BindingdepJson, SubId::GeneratedSource };
                // in the same order, their file name suffix (they replicate what's in AzslcMain.cpp. and hlsl corresponds to what's in AzslBuilder.cpp)
                
                // a type to declare variables holding the full paths of their files
                using Paths = AZStd::fixed_vector<AZStd::string, AZ_ARRAY_SIZE(SubList)>;
            };

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

            RHI::ShaderHardwareStage ToAssetBuilderShaderType(RPI::ShaderStageType stageType);

            //! Must be called before shaderPlatformInterface->CompilePlatformInternal()
            //! This function will prune non entry functions from BindingDependencies and use the
            //! rest of input data to create a pipeline layout descriptor.
            //! The pipeline layout descriptor is returned, but the same data will also be set into the @shaderPlatformInterface
            //! object, which is why it is important to call this method before calling shaderPlatformInterface->CompilePlatformInternal().
            RHI::Ptr<RHI::PipelineLayoutDescriptor> BuildPipelineLayoutDescriptorForApi(
                const char* BuilderName,
                RHI::ShaderPlatformInterface* shaderPlatformInterface,
                BindingDependencies& bindingDependencies /*inout*/,
                const ShaderResourceGroupAssets& srgAssets,
                const MapOfStringToStageType& shaderEntryPoints,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments,
                const RootConstantData* rootConstantData = nullptr
            );

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
                const AZStd::string& apiTypeString = "");

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
