/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#include "AzslData.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        class AzslCompiler;
        struct ShaderFiles;
        struct BindingDependencies;
        struct RootConstantData;

        using MapOfStringToStageType = AZStd::unordered_map<AZStd::string, RPI::ShaderStageType>;

        namespace ShaderBuilderUtility
        {
            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile);

            void GetAbsolutePathToAzslFile(const AZStd::string& shaderSourceFileFullPath, AZStd::string specifiedShaderPathAndName, AZStd::string& absoluteShaderPath);

            //! Opens and read the .shader, returns expanded file paths
            AZStd::shared_ptr<ShaderFiles> PrepareSourceInput(
                const char* builderName,
                const AZStd::string& shaderSourceFileFullPath,
                RPI::ShaderSourceData& sourceAsset);

            namespace AzslSubProducts
            {
                AZ_ENUM(SuffixList, azslin, ia, om, srg, options, bindingdep, hlsl);

                using SubId = RPI::ShaderAssetSubId;
                // product sub id enumerators:
                static constexpr SubId SubList[] = {SubId::FlatAzsl,
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
            AZStd::vector<RHI::ShaderPlatformInterface*> DiscoverEnabledShaderPlatformInterfaces(
                const AssetBuilderSDK::PlatformInfo& info, const RPI::ShaderSourceData& shaderSourceData);

            // The idea is that the "Supervariants" json property is optional in .shader files,
            // For cases when it is not specified, this function will return a vector with one item, the default, nameless, supervariant.
            // If "Supervariants" is not empty, then this function will make sure the first supervariant in the list
            // is the default, nameless, supervariant.
            AZStd::vector<RPI::ShaderSourceData::SupervariantInfo> GetSupervariantListFromShaderSourceData(
                const RPI::ShaderSourceData& shaderSourceData);

            void LogProfilingData(const char* builderName, AZStd::string_view shaderPath);

            //! Returns the asset path of a product artifact produced by ShaderAssetBuilder.
            Outcome<AZStd::string, AZStd::string> ObtainBuildArtifactPathFromShaderAssetBuilder(
                const uint32_t rhiUniqueIndex, const AZStd::string& platformIdentifier, const AZStd::string& shaderJsonPath,
                const uint32_t supervariantIndex, RPI::ShaderAssetSubId shaderAssetSubId);


            class IncludedFilesParser
            {
            public:
                IncludedFilesParser();
                ~IncludedFilesParser() = default;

                //! This static function was made public for testability purposes only.
                //! Parses the string @haystack, looking for "#include file" lines with a regular expression.
                //! Returns the list of relative paths as included by the file.
                //! REMARK: The algorithm may over prescribe what files to include because it doesn't discern between comments, etc.
                //!         Also, a #include line may be protected by #ifdef macros but this algorithm doesn't care.
                //! Over prescribing is not a real problem, albeit potential waste in processing. Under prescribing would be a real problem.
                AZStd::vector<AZStd::string> ParseStringAndGetIncludedFiles(AZStd::string_view haystack) const;

                //! This static function was made public for testability purposes only.
                //! Opens the file @sourceFilePath, loads the content into a string and returns ParseStringAndGetIncludedFiles(content)
                AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> ParseFileAndGetIncludedFiles(AZStd::string_view sourceFilePath) const;

            private:
                AZStd::regex m_includeRegex;
            };

        }  // ShaderBuilderUtility namespace
    } // ShaderBuilder namespace
} // AZ
