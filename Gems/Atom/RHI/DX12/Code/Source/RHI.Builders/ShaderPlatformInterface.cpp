/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RHI.Builders/ShaderPlatformInterface.h>

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Reflect/DX12/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace DX12
    {
        static const char* DX12ApiName = "dx12";
        static const char* DX12ShaderPlatformName = "DX12ShaderPlatform";
        static const char* PlatformShaderHeader = "Builders/ShaderHeaders/Platform/Windows/DX12/PlatformHeader.hlsli";
        static const char* AzslShaderHeader = "Builders/ShaderHeaders/Platform/Windows/DX12/AzslcHeader.azsli";

        ShaderPlatformInterface::ShaderPlatformInterface(uint32_t apiUniqueIndex)
            : RHI::ShaderPlatformInterface(apiUniqueIndex), m_apiName{ DX12ApiName }
        {
        }

        RHI::APIType ShaderPlatformInterface::GetAPIType() const
        {
            return RHI::APIType{ DX12ApiName };
        }

        AZ::Name ShaderPlatformInterface::GetAPIName() const
        {
            return m_apiName;
        }

        RHI::Ptr<RHI::ShaderStageFunction> ShaderPlatformInterface::CreateShaderStageFunction(const StageDescriptor& stageDescriptor)
        {
            RHI::Ptr<ShaderStageFunction> newShaderStageFunction =  ShaderStageFunction::Create(RHI::ToRHIShaderStage(stageDescriptor.m_stageType));

            const DX12::ShaderByteCode& byteCode = stageDescriptor.m_byteCode;
            const int byteCodeIndex = (stageDescriptor.m_stageType == RHI::ShaderHardwareStage::TessellationEvaluation) ? 1 : 0;
            newShaderStageFunction->SetByteCode(byteCodeIndex, byteCode);

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
            return PipelineLayoutDescriptor::Create();
        }

        bool ShaderPlatformInterface::BuildPipelineLayoutDescriptor(
            RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptorBase,
            const ShaderResourceGroupInfoList& srgInfoList,
            const RootConstantsInfo& rootConstantsInfo,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments)
        {
            PipelineLayoutDescriptor* pipelineLayoutDescriptor = azrtti_cast<PipelineLayoutDescriptor*>(pipelineLayoutDescriptorBase.get());
            AZ_Assert(pipelineLayoutDescriptor, "PipelineLayoutDescriptor should have been created by now");

            for (const ShaderResourceGroupInfo& srgInfo : srgInfoList)
            {
                ShaderResourceGroupVisibility srgVisibility;
                // Copy the resources binding info so we can erase the static samplers 
                // while adding them to the m_staticSamplersShaderStageMask list.
                // Each static sampler has it's own visibility. All other resources share the same visibility mask.
                auto resourcesBindingInfo = srgInfo.m_bindingInfo.m_resourcesRegisterMap;
                for (const RHI::ShaderInputStaticSamplerDescriptor& staticSamplerDescriptor : srgInfo.m_layout->GetStaticSamplers())
                {
                    auto findIt = resourcesBindingInfo.find(staticSamplerDescriptor.m_name);
                    if (findIt != resourcesBindingInfo.end())
                    {
                        // Erase the static sampler from the resource list so we don't use it when calculating
                        // the descriptor table shader stage mask.
                        resourcesBindingInfo.erase(findIt);
                    }
                    else
                    {
                        AZ_Error(DX12ShaderPlatformName, false, "Could not find binding info for static sampler '%s'", staticSamplerDescriptor.m_name.GetCStr());
                        return false;
                    }
                }

                if (shaderCompilerArguments.m_disableOptimizations)
                {
                    // When optimizations are disabled (-Od), all resources declared in the source file are available to all stages
                    // (when enabled only the resources which are referenced in a stage are bound to the stage)
                    srgVisibility.m_descriptorTableShaderStageMask = RHI::ShaderStageMask::All;                    
                }
                else
                {
                    for (const auto& bindInfo : resourcesBindingInfo)
                    {
                        srgVisibility.m_descriptorTableShaderStageMask |= bindInfo.second.m_shaderStageMask;
                    }
                }

                pipelineLayoutDescriptor->AddShaderResourceGroupVisibility(srgVisibility);

                if (rootConstantsInfo.m_totalSizeInBytes > 0)
                {
                    AZ_Assert((rootConstantsInfo.m_totalSizeInBytes % 4) == 0, "Inline constant size is not a multiple of 32 bit");
                    pipelineLayoutDescriptor->SetRootConstantBinding(RootConstantBinding{ rootConstantsInfo.m_totalSizeInBytes / 4, rootConstantsInfo.m_registerId, rootConstantsInfo.m_spaceId });
                }           
            }

            return pipelineLayoutDescriptor->Finalize() == RHI::ResultCode::Success;
        }

        bool ShaderPlatformInterface::CompilePlatformInternal(
            [[maybe_unused]] const AssetBuilderSDK::PlatformInfo& platform,
            const AZStd::string& shaderSourcePath,
            const AZStd::string& functionName,
            RHI::ShaderHardwareStage shaderStage,
            const AZStd::string& tempFolderPath,
            StageDescriptor& outputDescriptor,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            AZStd::vector<uint8_t> shaderByteCode;

            // Compile HLSL shader to byte code
            bool compiledSucessfully = CompileHLSLShader(
                shaderSourcePath,                        // shader source filepath
                tempFolderPath,                          // AP job temp folder
                functionName,                            // name of function that is the entry point
                shaderStage,                             // shader stage (vertex shader, pixel shader, ...)
                shaderCompilerArguments,
                shaderByteCode,                          // compiled shader output
                outputDescriptor.m_byProducts);          // dynamic branch count output & byproduct files

            if (!compiledSucessfully)
            {
                AZ_Error(DX12ShaderPlatformName, false, "Failed to compile HLSL shader");
                return false;
            }

            const char byteCodeHeader[] = { 'D', 'X', 'B', 'C' };
            if (shaderByteCode.size() > sizeof(byteCodeHeader) && memcmp(shaderByteCode.data(), byteCodeHeader, sizeof(byteCodeHeader)) == 0)
            {
                outputDescriptor.m_stageType = shaderStage;
                outputDescriptor.m_byteCode = AZStd::move(shaderByteCode);
            }
            else
            {
                AZ_Error(DX12ShaderPlatformName, false, "Compiled shader for %s is invalid", shaderSourcePath.c_str());
                return false;
            }

            return true;
        }
       
        AZStd::string ShaderPlatformInterface::GetAzslCompilerParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.MakeAdditionalAzslcCommandLineString() +
                " --use-spaces --namespace=dx --root-const=128";
        }

        AZStd::string ShaderPlatformInterface::GetAzslCompilerWarningParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.MakeAdditionalAzslcWarningCommandLineString();
        }

        bool ShaderPlatformInterface::BuildHasDebugInfo(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            return shaderCompilerArguments.m_generateDebugInfo;
        }

        const char* ShaderPlatformInterface::GetAzslHeader(const AssetBuilderSDK::PlatformInfo& platform) const
        {
            AZ_UNUSED(platform);
            return AzslShaderHeader;
        }

        bool ShaderPlatformInterface::CompileHLSLShader(
            const AZStd::string& shaderSourceFile,
            const AZStd::string& tempFolder,
            const AZStd::string& entryPoint,
            const RHI::ShaderHardwareStage shaderStageType,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            AZStd::vector<uint8_t>& compiledShader,
            ByProducts& byProducts) const
        {
            // Shader compiler executable
            static const char* dxcRelativePath = "Builders/DirectXShaderCompiler/dxc.exe";

            // NOTE:
            // Running DX12 on PC with DXIL shaders requires modern GPUs and at least Windows 10 Build 1803 or later for Shader Model 6.2
            // https://github.com/Microsoft/DirectXShaderCompiler/wiki/Running-Shaders

            // -Fo "Output object file"
            AZStd::string shaderOutputFile;
            AzFramework::StringFunc::Path::GetFileName(shaderSourceFile.c_str(), shaderOutputFile);
            AzFramework::StringFunc::Path::Join(tempFolder.c_str(), shaderOutputFile.c_str(), shaderOutputFile);
            AzFramework::StringFunc::Path::ReplaceExtension(shaderOutputFile, "dxil.bin");

            // -Fh "Output header file containing object code", used for counting dynamic branches
            AZStd::string objectCodeOutputFile;
            AzFramework::StringFunc::Path::GetFileName(shaderSourceFile.c_str(), objectCodeOutputFile);
            AzFramework::StringFunc::Path::Join(tempFolder.c_str(), objectCodeOutputFile.c_str(), objectCodeOutputFile);
            AzFramework::StringFunc::Path::ReplaceExtension(objectCodeOutputFile, "dxil.txt");

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
                AZ_Error(DX12ShaderPlatformName, false, "Unsupported shader stage");
                return false;
            }

            // Compilation parameters
            AZStd::string params = shaderCompilerArguments.MakeAdditionalDxcCommandLineString();
            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                params += " -Zi";  // Generate debug information
                params += " -Zss"; // Compute Shader Hash considering source information
            }

            // Enable half precision types when shader model >= 6.2
            int shaderModelMajor = 0;
            int shaderModelMinor = 0;
            [[maybe_unused]] int numValuesRead = azsscanf(shaderModelVersion.c_str(), "%d_%d", &shaderModelMajor, &shaderModelMinor);
            AZ_Assert(numValuesRead == 2, "Unknown shader model version format");
            if (shaderModelMajor >= 6 && shaderModelMinor >= 2)
            {
                params += " -enable-16bit-types";
            }
            AZ::StringFunc::TrimWhiteSpace(params, true, false); // we don't need the extra leading spaces that tend to build up

            unsigned char md5[RHI::Md5NumBytes];
            RHI::PrependArguments args;
            args.m_sourceFile = shaderSourceFile.c_str();
            args.m_prependFile = PlatformShaderHeader;
            args.m_destinationFolder = tempFolder.c_str();
            args.m_digest = &md5;

            const auto dxcInputFile = RHI::PrependFile(args);  // Prepend PAL header & obtain hash
            // -Fd "Write debug information to the given file, or automatically named file in directory when ending in '\\'"
            // If we use the auto-name (hash), there is no way we can retrieve that name apart from listing the directory.
            // Instead, let's just generate that hash ourselves.
            AZStd::string symbolDatabaseFileCliArgument{" "};  // when not debug: still insert a space between 5.dxil and 7.hlsl-in
            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                // prepare .pdb filename:
                AZStd::string md5hex = RHI::ByteToHexString(md5);
                AZStd::string symbolDatabaseFilePath = dxcInputFile.c_str();  // mutate from source
                AZStd::string pdbFileName = md5hex + "-" + profileIt->second;   // concatenate the shader profile to disambiguate vs/ps...
                AzFramework::StringFunc::Path::ReplaceFullName(symbolDatabaseFilePath, pdbFileName.c_str(), "pdb");
                // it is possible that another activated platform/profile, already exported that file. (since it's hashed on the source file)
                // dxc returns an error in such case. we get less surprising effets by just not mentionning an -Fd argument
                if (AZ::IO::SystemFile::Exists(symbolDatabaseFilePath.c_str()))
                {
                    AZ_Warning(DX12ShaderPlatformName, false, "debug symbol file %s already exists -> -Fd argument dropped", symbolDatabaseFilePath.c_str());
                }
                else
                {
                    symbolDatabaseFileCliArgument = " -Fd \"" + symbolDatabaseFilePath + "\" ";  // 6.pdb  hereunder
                    byProducts.m_intermediatePaths.emplace(AZStd::move(symbolDatabaseFilePath));
                }
            }
            const auto dxcEntryPoint = (shaderStageType == RHI::ShaderHardwareStage::RayTracing) ? "" : AZStd::string::format("-E %s", entryPoint.c_str());
            //                                                1.entry   3.config            5.dxil  7.hlsl-in
            //                                                    |   2.SM  |   4.output       | 6.pdb  |
            //                                                    |     |   |       |          |   |    |
            const auto dxcCommandOptions = AZStd::string::format("%s -T %s %s -Fo \"%s\" -Fh \"%s\"%s\"%s\"",
                                                                 dxcEntryPoint.c_str(),                  // 1
                                                                 profileIt->second.c_str(),              // 2
                                                                 params.c_str(),                         // 3
                                                                 shaderOutputFile.c_str(),               // 4
                                                                 objectCodeOutputFile.c_str(),           // 5
                                                                 symbolDatabaseFileCliArgument.c_str(),  // 6
                                                                 dxcInputFile.c_str()                    // 7
                                                                 );

            // Run Shader Compiler
            if (!RHI::ExecuteShaderCompiler(dxcRelativePath, dxcCommandOptions, shaderSourceFile, "DXC"))
            {
                return false;
            }

            auto shaderOutputFileLoadResult = AZ::RHI::LoadFileBytes(shaderOutputFile.c_str());
            if (!shaderOutputFileLoadResult)
            {
                AZ_Error(DX12ShaderPlatformName, false, "%s", shaderOutputFileLoadResult.GetError().c_str());
                return false;
            }

            compiledShader = shaderOutputFileLoadResult.TakeValue();

            // Count the dynamic branches by searching dxc.exe's generated header file.
            // There might be a more ideal way to count the number of dynamic branches, perhaps using DXC libs, but doing it this way is quick and easy to set up.
            auto objectCodeLoadResult = AZ::RHI::LoadFileString(objectCodeOutputFile.c_str());
            if (objectCodeLoadResult)
            {                
                // The regex here is based on dxc source code, which lists terminating instructions as:
                //    case Ret:    return "ret";
                //    case Br:     return "br";
                //    case Switch: return "switch";
                //    case IndirectBr: return "indirectbr";
                //    case Invoke: return "invoke";
                //    case Resume: return "resume";
                //    case Unreachable: return "unreachable";
                // If you have to update this regex, also update UtilsTests RegexCount_DXIL
                byProducts.m_dynamicBranchCount = aznumeric_cast<uint32_t>(AZ::RHI::RegexCount(objectCodeLoadResult.GetValue(), "^ *(br|indirectbr|switch) "));
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
