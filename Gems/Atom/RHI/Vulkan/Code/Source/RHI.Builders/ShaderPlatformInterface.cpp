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
        static const char* WindowsPlatformShaderHeader = "Builders/ShaderHeaders/Platform/Windows/Vulkan/PlatformHeader.hlsli";
        static const char* AndroidPlatformShaderHeader = "Builders/ShaderHeaders/Platform/Android/Vulkan/PlatformHeader.hlsli";
        static const char* WindowsAzslShaderHeader = "Builders/ShaderHeaders/Platform/Windows/Vulkan/AzslcHeader.azsli";
        static const char* AndroidAzslShaderHeader = "Builders/ShaderHeaders/Platform/Android/Vulkan/AzslcHeader.azsli";
    
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

            const Vulkan::ShaderByteCode& byteCode = stageDescriptor.m_byteCode;
            const AZStd::string& entryFunctionName = stageDescriptor.m_entryFunctionName;
            const int byteCodeIndex = (stageDescriptor.m_stageType == RHI::ShaderHardwareStage::TessellationEvaluation) ? 1 : 0;
            newShaderStageFunction->SetByteCode(byteCodeIndex, byteCode, entryFunctionName);

            newShaderStageFunction->Finalize();

            return newShaderStageFunction;
        }

        bool ShaderPlatformInterface::IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const
        {
            bool hasRasterProgram = false;

            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Vertex;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Fragment;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::TessellationControl;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::TessellationEvaluation;

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
            const RHI::ShaderCompilerArguments& shaderCompilerArguments)
        {
            AZ_UNUSED(srgInfoList);
            AZ_UNUSED(rootConstantsInfo);
            AZ_UNUSED(shaderCompilerArguments);

            // Nothing to do, so we just finalize the layout descriptor.
            return pipelineLayoutDescriptor->Finalize() == RHI::ResultCode::Success;
        }

        AZStd::string ShaderPlatformInterface::GetAzslCompilerParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            // Note: all platforms use DirectX packing rules.
            return shaderCompilerArguments.MakeAdditionalAzslcCommandLineString() +
                " --use-spaces --unique-idx --namespace=vk --root-const=128";
        }

        AZStd::string ShaderPlatformInterface::GetAzslCompilerWarningParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.MakeAdditionalAzslcWarningCommandLineString();
        }

        bool ShaderPlatformInterface::BuildHasDebugInfo(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.m_dxcGenerateDebugInfo;
        }

        const char* ShaderPlatformInterface::GetAzslHeader(const AssetBuilderSDK::PlatformInfo& platform) const
        {
            if(platform.HasTag("mobile"))
            {
                return AndroidAzslShaderHeader;
            }
            else
            {
                return WindowsAzslShaderHeader;
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
            const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            AZStd::vector<uint8_t> shaderByteCode;

            // Compile HLSL shader to SPIRV byte code
            bool compiledSucessfully = CompileHLSLShader(
                shaderSourcePath,                        // shader source filename
                tempFolderPath,                          // AP temp folder for the job
                functionName,                            // name of function that is the entry point
                shaderAssetType,                         // shader stage (vertex shader, pixel shader, ...)
                shaderCompilerArguments,
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
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            AZStd::vector<uint8_t>& compiledShader,
            const AssetBuilderSDK::PlatformInfo& platform,
            ByProducts& byProducts) const
        {
            // Shader compiler executable
            static const char* dxcRelativePath = AZ_TRAIT_ATOM_SHADERBUILDER_DXC;

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
                {RHI::ShaderHardwareStage::TessellationControl,    "hs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::TessellationEvaluation, "ds_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::RayTracing,             "lib_6_3"}
            };
            auto profileIt = stageToProfileName.find(shaderStageType);
            if (profileIt == stageToProfileName.end())
            {
                AZ_Error(VulkanShaderPlatformName, false, "Unsupported shader stage");
                return false;
            }

            // Compilation parameters
            AZStd::string params = shaderCompilerArguments.MakeAdditionalDxcCommandLineString();
            params += " -spirv"; // Generate SPIRV shader

            switch (shaderStageType)
            {
            case RHI::ShaderHardwareStage::Vertex:
            case RHI::ShaderHardwareStage::Geometry:
            case RHI::ShaderHardwareStage::TessellationEvaluation:
                params += " -fvk-invert-y";
                break;
            case RHI::ShaderHardwareStage::Fragment:
                // Enable the use of subpass input. DXC doesn't compile if a SubpassInput is present
                // when compiling a shader stage that is not the fragment shader (even if it's not being used).
                params += " -DAZ_USE_SUBPASSINPUT";
                params += " -fvk-use-dx-position-w";
                break;
            case RHI::ShaderHardwareStage::TessellationControl:
            case RHI::ShaderHardwareStage::Compute:
            case RHI::ShaderHardwareStage::RayTracing:
                break;
            default:
                AZ_Assert(false, "Invalid Shader stage.");
            }

            // Enable half precision types when shader model >= 6.2
            int shaderModelMajor = 0;
            int shaderModelMinor = 0;
            [[maybe_unused]] int numValuesRead = azsscanf(shaderModelVersion.c_str(), "%d_%d", &shaderModelMajor, &shaderModelMinor);
            AZ_Assert(numValuesRead == 2, "Unknown shader model version format");

            // For mobile, which has 16 bit support in almost all GPUs, we allow 16 bit types in the shader.
            // For PC, 16 bit types will fallback to 32 bit types with a "RelaxedPrecision" decoration. This
            // decoration allows drivers to only compute 16-bits of precision if they want. We don't use "RelaxedPrecision" for
            // mobile because that decoration is not supported by most mobile drivers.
            if (shaderModelMajor >= 6 && shaderModelMinor >= 2 && platform.HasTag("mobile"))
            {
                params += " -enable-16bit-types";
            }

            // Disable image depth hint because it causes some crashes on mobile drivers.
            if (platform.HasTag("mobile"))
            {
                params += " -fvk-disable-depth-hint";
            }

            // Use the same memory layout as DX12, otherwise some offset of constant may get wrong.
            params += " -fvk-use-dx-layout";
            AZ::StringFunc::TrimWhiteSpace(params, true, false);
            AZStd::string prependFile;
            if(platform.HasTag("mobile"))
            {
                prependFile = AndroidPlatformShaderHeader;
            }
            else
            {
                prependFile = WindowsPlatformShaderHeader;
            }

            RHI::PrependArguments args;
            args.m_sourceFile = shaderSourceFile.c_str();
            args.m_prependFile = prependFile.c_str();
            args.m_destinationFolder = tempFolder.c_str();

            const auto dxcInputFile = RHI::PrependFile(args);  // Prepend header
            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                // dump intermediate "true final HLSL" file (shadername.vulkan.shadersource.prepend)
                byProducts.m_intermediatePaths.insert(dxcInputFile);
            }
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
            if (!RHI::ExecuteShaderCompiler(dxcRelativePath, dxcCommandOptions, shaderSourceFile, "DXC"))
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

            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                byProducts.m_intermediatePaths.emplace(AZStd::move(objectCodeOutputFile));
            }

            return true;
        }
    }
}
