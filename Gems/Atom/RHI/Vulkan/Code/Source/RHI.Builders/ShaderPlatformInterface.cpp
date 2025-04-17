/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <Atom/RHI.Edit/Utils.h>

#include <Atom/RHI.Reflect/Vulkan/Base.h>
#include <Atom/RHI.Reflect/Vulkan/ShaderStageFunction.h>
#include <Atom/RHI/RHIUtils.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <RHI.Builders/ShaderPlatformInterface.h>
#include <Vulkan_Traits_Platform.h>

namespace AZ
{
    namespace Vulkan
    {
        [[maybe_unused]] static const char* VulkanShaderPlatformName = "VulkanShaderPlatform";

        RHI::APIType ShaderPlatformInterface::GetAPIType() const
        {
            return Vulkan::RHIType;
        }

        AZ::Name ShaderPlatformInterface::GetAPIName() const
        {
            return m_apiName;
        }

        RHI::Ptr<RHI::ShaderStageFunction> ShaderPlatformInterface::CreateShaderStageFunction(const StageDescriptor& stageDescriptor)
        {
            RHI::Ptr<ShaderStageFunction> newShaderStageFunction = ShaderStageFunction::Create(RHI::ToRHIShaderStage(stageDescriptor.m_stageType));

            const auto& byteCode = stageDescriptor.m_byteCode;
            const AZStd::string& entryFunctionName = stageDescriptor.m_entryFunctionName;
            const int byteCodeIndex = 0;
            newShaderStageFunction->SetByteCode(byteCodeIndex, byteCode, entryFunctionName);

            newShaderStageFunction->Finalize();

            return newShaderStageFunction;
        }

        bool ShaderPlatformInterface::IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const
        {
            bool hasRasterProgram = false;

            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Vertex;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Geometry;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Fragment;

            return hasRasterProgram;
        }

        bool ShaderPlatformInterface::IsShaderStageForCompute(RHI::ShaderHardwareStage shaderStageType) const
        {
            return (shaderStageType == RHI::ShaderHardwareStage::Compute);
        }

        bool ShaderPlatformInterface::IsShaderStageForRayTracing(RHI::ShaderHardwareStage shaderStageType) const
        {
            return (shaderStageType == RHI::ShaderHardwareStage::RayTracing);
        }

        RHI::Ptr<RHI::PipelineLayoutDescriptor> ShaderPlatformInterface::CreatePipelineLayoutDescriptor()
        {
            // Return the base RHI class for the PipelineLayoutDescriptor
            return RHI::PipelineLayoutDescriptor::Create();
        }

        bool ShaderPlatformInterface::BuildPipelineLayoutDescriptor(
            RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor,
            const ShaderResourceGroupInfoList& srgInfoList,
            const RootConstantsInfo& rootConstantsInfo,
            const RHI::ShaderBuildArguments& shaderBuildArguments)
        {
            AZ_UNUSED(srgInfoList);
            AZ_UNUSED(rootConstantsInfo);
            AZ_UNUSED(shaderBuildArguments);

            // Nothing to do, so we just finalize the layout descriptor.
            return pipelineLayoutDescriptor->Finalize() == RHI::ResultCode::Success;
        }

        const char* ShaderPlatformInterface::GetAzslHeader([[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform) const
        {
            if (platform.HasTag("mobile"))
            {
                return AZ_TRAIT_ATOM_MOBILE_AZSL_SHADER_HEADER;
            }
            else
            {
                return AZ_TRAIT_ATOM_AZSL_SHADER_HEADER;
            }
        }

        // Takes in HLSL source file path and then compiles the HLSL to bytecode and
        // appends it to the AZ::Vulkan::ShaderStageDescriptor inside the provided outputAsset.
        bool ShaderPlatformInterface::CompilePlatformInternal(
            const AssetBuilderSDK::PlatformInfo& platform,
            const AZStd::string& shaderSourcePath,
            const AZStd::string& functionName,
            RHI::ShaderHardwareStage shaderAssetType,
            const AZStd::string& tempFolderPath,
            StageDescriptor& outputDescriptor,
            const RHI::ShaderBuildArguments& shaderBuildArguments,
            [[maybe_unused]] const bool useSpecializationConstants) const
        {
            AZStd::vector<uint8_t> shaderByteCode;

            // Compile HLSL shader to SPIRV byte code
            bool compiledSucessfully = CompileHLSLShader(
                shaderSourcePath,                        // shader source filename
                tempFolderPath,                          // AP temp folder for the job
                functionName,                            // name of function that is the entry point
                shaderAssetType,                         // shader stage (vertex shader, pixel shader, ...)
                shaderBuildArguments,
                shaderByteCode,                          // compiled shader output
                platform,                                // target platform
                outputDescriptor.m_byProducts);          // dynamic branch count output & debug dumps

            if (!compiledSucessfully)
            {
                AZ_Error(VulkanShaderPlatformName, false, "Failed to cross-compile HLSL shader to SPIRV");
                return false;
            }

            if (shaderByteCode.size() > 0)
            {
                AZ_Assert(!functionName.empty(), "There is no entry function name.");
                outputDescriptor.m_stageType = shaderAssetType;
                outputDescriptor.m_byteCode = AZStd::move(shaderByteCode);
                outputDescriptor.m_entryFunctionName = AZStd::move(functionName);
            }
            else
            {
                AZ_Error(VulkanShaderPlatformName, false, "Compiled shader for %s is invalid", shaderSourcePath.c_str());
                return false;
            }

            return true;
        }

        bool ShaderPlatformInterface::CompileHLSLShader(
            const AZStd::string& shaderSourceFile,
            const AZStd::string& tempFolder,
            const AZStd::string& entryPoint,
            const RHI::ShaderHardwareStage shaderStageType,
            const RHI::ShaderBuildArguments& shaderBuildArguments,
            AZStd::vector<uint8_t>& compiledShader,
            [[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform,
            ByProducts& byProducts) const
        {
            // Shader compiler executable
            const auto dxcRelativePath = RHI::GetDirectXShaderCompilerPath(AZ_TRAIT_ATOM_SHADERBUILDER_DXC);

            // -Fo "Output file"
            AZStd::string shaderOutputFile;
            AzFramework::StringFunc::Path::GetFileName(shaderSourceFile.c_str(), shaderOutputFile);
            AzFramework::StringFunc::Path::Join(tempFolder.c_str(), shaderOutputFile.c_str(), shaderOutputFile);
            AzFramework::StringFunc::Path::ReplaceExtension(shaderOutputFile, "spirv.bin");

            // -Fh "Output header file containing object code", used for counting dynamic branches
            AZStd::string objectCodeOutputFile;
            AzFramework::StringFunc::Path::GetFileName(shaderSourceFile.c_str(), objectCodeOutputFile);
            AzFramework::StringFunc::Path::Join(tempFolder.c_str(), objectCodeOutputFile.c_str(), objectCodeOutputFile);
            AzFramework::StringFunc::Path::ReplaceExtension(objectCodeOutputFile, "spirv.txt");

            // Stage profile name parameter
            // Note: RayTracing shaders must be compiled with version 6_3, while the rest of the stages
            // are compiled with version 6_2, so RayTracing cannot share the version constant.
            const AZStd::string shaderModelVersion = "6_2";
            const AZStd::unordered_map<RHI::ShaderHardwareStage, AZStd::string> stageToProfileName =
            {
                {RHI::ShaderHardwareStage::Vertex,                 "vs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Fragment,               "ps_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Compute,                "cs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Geometry,               "gs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::RayTracing,             "lib_6_3"}
            };
            auto profileIt = stageToProfileName.find(shaderStageType);
            if (profileIt == stageToProfileName.end())
            {
                AZ_Error(VulkanShaderPlatformName, false, "Unsupported shader stage");
                return false;
            }

            // Make a copy of compilation parameters for DXC. We'll need
            // to do per shader stage customization of arguments.
            // NOTE: The current architecture of the ShaderBuildArgumentsManager API
            // would allow to easily customize build arguments per shader stage.
            // At the moment it is an overkill to enable such customization.
            // If, in the future, the need arises across other RHIs and platforms
            // We can revisit these hard coded parameters.
            auto dxcArguments = shaderBuildArguments.m_dxcArguments;

            //Add debug symbols within spirv
            const bool graphicsDevMode = RHI::IsGraphicsDevModeEnabled();
            if (graphicsDevMode || BuildHasDebugInfo(shaderBuildArguments))
            {
                // Remark: Ideally we'd use "-fspv-debug=vulkan-with-source", but
                // - DXC 1.6.2106 crashes with this error when compiling large shaders (small shaders works fine).
                //     dxc failed : unknown SPIR-V debug info control parameter: vulkan-with-source
                // - DXC 1.7.2212.1 crashes with the following error when compiling large shaders:
                //     fatal error: generated SPIR-V is invalid: ID '2123[%2123]' has not been defined
                //        %2122 = OpExtInst %void %2 DebugTypeFunction %uint_3 %void %2123 %1415
                // 
                // There are already several bug reports like this one: https://github.com/microsoft/DirectXShaderCompiler/issues/4767
                RHI::ShaderBuildArguments::AppendArguments(dxcArguments, { "-fspv-debug=line" });
            }

            switch (shaderStageType)
            {
            case RHI::ShaderHardwareStage::Vertex:
            case RHI::ShaderHardwareStage::Geometry:
                RHI::ShaderBuildArguments::AppendArguments(dxcArguments, { "-fvk-invert-y" });
                break;
            case RHI::ShaderHardwareStage::Fragment:
                RHI::ShaderBuildArguments::AppendArguments(dxcArguments, { "-fvk-use-dx-position-w"});
                break;
            case RHI::ShaderHardwareStage::Compute:
            case RHI::ShaderHardwareStage::RayTracing:
                break;
            default:
                AZ_Assert(false, "Invalid Shader stage.");
            }

            AZStd::string prependFile;
            if (platform.HasTag("mobile"))
            {
                prependFile = AZ_TRAIT_ATOM_MOBILE_AZSL_PLATFORM_HEADER;
            }
            else
            {
                prependFile = AZ_TRAIT_ATOM_AZSL_PLATFORM_HEADER;
            }

            RHI::PrependArguments args;
            args.m_sourceFile = shaderSourceFile.c_str();
            args.m_prependFile = prependFile.c_str();
            args.m_destinationFolder = tempFolder.c_str();

            const auto dxcInputFile = RHI::PrependFile(args);  // Prepend header

            if (graphicsDevMode || BuildHasDebugInfo(shaderBuildArguments))
            {
                // dump intermediate "true final HLSL" file (shadername.vulkan.shadersource.prepend)
                byProducts.m_intermediatePaths.insert(dxcInputFile);
            }

            const auto params = RHI::ShaderBuildArguments::ListAsString(dxcArguments);
            const auto dxcEntryPoint = (shaderStageType == RHI::ShaderHardwareStage::RayTracing) ? "" : AZStd::string::format("-E %s", entryPoint.c_str());
            //                                                1.entry   3.config           5.dxil  6.hlsl-in
            //                                                    |   2.SM  |   4.output       |      |
            //                                                    |     |   |       |          |      |
            const auto dxcCommandOptions = AZStd::string::format("%s -T %s %s -Fo \"%s\" -Fh \"%s\" \"%s\"",
                                                                 dxcEntryPoint.c_str(),                  // 1
                                                                 profileIt->second.c_str(),              // 2
                                                                 params.c_str(),                         // 3
                                                                 shaderOutputFile.c_str(),               // 4
                                                                 objectCodeOutputFile.c_str(),           // 5
                                                                 dxcInputFile.c_str());                  // 6
            // note: unlike DX12, the -Fd switch fails with -spirv. waiting for an answer on https://github.com/microsoft/DirectXShaderCompiler/issues/3111
            //       therefore, the debug data is probably embedded in the spirv blob.

            // Run Shader Compiler
            if (!RHI::ExecuteShaderCompiler(dxcRelativePath, dxcCommandOptions, shaderSourceFile, tempFolder, "DXC"))
            {
                return false;
            }

            auto shaderOutputFileLoadResult = AZ::RHI::LoadFileBytes(shaderOutputFile.c_str());
            if (!shaderOutputFileLoadResult)
            {
                AZ_Error(VulkanShaderPlatformName, false, "%s", shaderOutputFileLoadResult.GetError().c_str());
                return false;
            }

            compiledShader = shaderOutputFileLoadResult.TakeValue();

            // Count the dynamic branches by searching dxc.exe's generated header file.
            // There might be a more ideal way to count the number of dynamic branches, perhaps using DXC libs, but doing it this way is quick and easy to set up.
            auto objectCodeLoadResult = AZ::RHI::LoadFileString(objectCodeOutputFile.c_str());
            if (objectCodeLoadResult)
            {
                // The regex here is based on https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_control_flow, which lists terminating instructions as:
                //    OpBranch
                //    OpBranchConditional
                //    OpSwitch
                //    OpReturn
                //    OpReturnValue
                // If you have to update this regex, also update UtilsTests RegexCount_SPIRV
                byProducts.m_dynamicBranchCount = aznumeric_cast<uint32_t>(AZ::RHI::RegexCount(objectCodeLoadResult.GetValue(), "^ *(OpBranch|OpBranchConditional|OpSwitch) "));
            }
            else
            {
                byProducts.m_dynamicBranchCount = ByProducts::UnknownDynamicBranchCount;
            }

            if (graphicsDevMode || BuildHasDebugInfo(shaderBuildArguments))
            {
                byProducts.m_intermediatePaths.emplace(AZStd::move(objectCodeOutputFile));
            }

            return true;
        }
    }
}
