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
#include <Atom/RHI/RHIUtils.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
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

            const auto& byteCode = stageDescriptor.m_byteCode;
            const int byteCodeIndex = 0;
            newShaderStageFunction->SetByteCode(byteCodeIndex, byteCode);

            // Read the json data with the specialization constants offsets.
            // If the shader was not compiled with specialization constants this attribute will be empty.
            AZStd::string fileName;
            if (!stageDescriptor.m_extraData.empty())
            {
                auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(stageDescriptor.m_extraData);
                if (!jsonOutcome.IsSuccess())
                {
                    AZ_Error(DX12ShaderPlatformName, false, "%s", jsonOutcome.GetError().c_str());
                    return nullptr;
                }

                const rapidjson::Document& doc = jsonOutcome.GetValue();
                ShaderStageFunction::SpecializationOffsets offsets;
                for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr)
                {
                    if (!AZ::StringFunc::LooksLikeInt(itr->name.GetString()))
                    {
                        AZ_Error(DX12ShaderPlatformName, false, "SpecializationId %s is not an Int", itr->name.GetString());
                        continue;
                    }
                    uint32_t specializationId = static_cast<uint32_t>(AZ::StringFunc::ToInt(itr->name.GetString()));
                    uint32_t offset = itr->value.GetUint();
                    offsets[specializationId] = offset;
                }
                newShaderStageFunction->SetSpecializationOffsets(byteCodeIndex, offsets);
            }
         
            newShaderStageFunction->Finalize();

            return newShaderStageFunction;
        }

        bool ShaderPlatformInterface::IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const
        {
            bool hasRasterProgram = false;

            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Vertex;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Fragment;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Geometry;

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
            const RHI::ShaderBuildArguments& shaderBuildArguments)
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

                const bool dxcDisableOptimizations = RHI::ShaderBuildArguments::HasArgument(shaderBuildArguments.m_dxcArguments, "-Od");
                if (dxcDisableOptimizations)
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

                    srgVisibility.m_descriptorTableShaderStageMask |= srgInfo.m_bindingInfo.m_constantDataBindingInfo.m_shaderStageMask;
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
            const RHI::ShaderBuildArguments& shaderBuildArguments,
            const bool useSpecializationConstants) const
        {
            AZStd::vector<uint8_t> shaderByteCode;
            AZStd::string specializationOffsetsFile;
            // Compile HLSL shader to byte code
            bool compiledSucessfully = CompileHLSLShader(
                shaderSourcePath,                        // shader source filepath
                tempFolderPath,                          // AP job temp folder
                functionName,                            // name of function that is the entry point
                shaderStage,                             // shader stage (vertex shader, pixel shader, ...)
                shaderBuildArguments,
                shaderByteCode,                          // compiled shader output
                outputDescriptor.m_byProducts,           // dynamic branch count output & byproduct files
                specializationOffsetsFile,               // path to the json file with the specialization offsets
                useSpecializationConstants);             // if the shader stage it's using specialization constants

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
                outputDescriptor.m_extraData = AZStd::move(specializationOffsetsFile);
            }
            else
            {
                AZ_Error(DX12ShaderPlatformName, false, "Compiled shader for %s is invalid", shaderSourcePath.c_str());
                return false;
            }

            return true;
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
            const RHI::ShaderBuildArguments& shaderBuildArguments,
            AZStd::vector<uint8_t>& compiledShader,
            ByProducts& byProducts,
            AZStd::string& specializationOffsetsFile,
            const bool useSpecializationConstants) const
        {
            // Shader compiler executable
            const auto dxcRelativePath = RHI::GetDirectXShaderCompilerPath("Builders/DirectXShaderCompiler/dxc.exe");

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
                {RHI::ShaderHardwareStage::RayTracing,             "lib_6_3"}
            };
            auto profileIt = stageToProfileName.find(shaderStageType);
            if (profileIt == stageToProfileName.end())
            {
                AZ_Error(DX12ShaderPlatformName, false, "Unsupported shader stage");
                return false;
            }

            const bool graphicsDevMode = RHI::IsGraphicsDevModeEnabled();

            // Compilation parameters
            auto dxcArguments = shaderBuildArguments.m_dxcArguments;
            if (graphicsDevMode || BuildHasDebugInfo(shaderBuildArguments))
            {
                RHI::ShaderBuildArguments::AppendArguments(dxcArguments, { "-Zi", "-Zss", "-Od" });
            }

            unsigned char sha1[RHI::Sha1NumBytes];
            RHI::PrependArguments args;
            args.m_sourceFile = shaderSourceFile.c_str();
            args.m_prependFile = PlatformShaderHeader;
            args.m_destinationFolder = tempFolder.c_str();
            args.m_digest = &sha1;

            const auto dxcInputFile = RHI::PrependFile(args);  // Prepend PAL header & obtain hash
            // -Fd "Write debug information to the given file, or automatically named file in directory when ending in '\\'"
            // If we use the auto-name (hash), there is no way we can retrieve that name apart from listing the directory.
            // Instead, let's just generate that hash ourselves.
            AZStd::string symbolDatabaseFileCliArgument{" "};  // when not debug: still insert a space between 5.dxil and 7.hlsl-in
            if (graphicsDevMode || shaderBuildArguments.m_generateDebugInfo)
            {
                // prepare .pdb filename:
                AZStd::string sha1hex = RHI::ByteToHexString(sha1);
                AZStd::string symbolDatabaseFilePath = dxcInputFile.c_str();  // mutate from source
                AZStd::string pdbFileName = sha1hex + "-" + profileIt->second; // concatenate the shader profile to disambiguate vs/ps...
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
            const auto params = RHI::ShaderBuildArguments::ListAsString(dxcArguments);
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
            if (!RHI::ExecuteShaderCompiler(dxcRelativePath, dxcCommandOptions, shaderSourceFile, tempFolder, "DXC"))
            {
                return false;
            }

            if (useSpecializationConstants)
            {
                // Need to patch the shader so it can be used with specialization constants.
                const auto dxscRelativePath = RHI::GetDirectXShaderCompilerPath("Builders/DirectXShaderCompiler/dxsc.exe");

                AZStd::string shaderOutputCommon;
                AzFramework::StringFunc::Path::GetFileName(shaderSourceFile.c_str(), shaderOutputCommon);
                AzFramework::StringFunc::Path::Join(tempFolder.c_str(), shaderOutputCommon.c_str(), shaderOutputCommon);

                AZStd::string patchedShaderOutput = shaderOutputCommon;
                AzFramework::StringFunc::Path::ReplaceExtension(patchedShaderOutput, "dxil.patched.bin");
                AZStd::string offsetsOutput = shaderOutputCommon;
                AzFramework::StringFunc::Path::ReplaceExtension(offsetsOutput, "offsets.json");

                const auto dxscCommandOptions = AZStd::string::format(
                    //   1.sentinel    3.offsets_output   
                    //     |    2.output    |   4.dxil-in
                    //     |       |        |      |
                    "-sv=%lu -o=\"%s\" -f=\"%s\" \"%s\"",
                    static_cast<unsigned long>(SCSentinelValue), // 1
                    patchedShaderOutput.c_str(), // 2
                    offsetsOutput.c_str(), // 3
                    shaderOutputFile.c_str() // 4
                );

                if (!RHI::ExecuteShaderCompiler(dxscRelativePath, dxscCommandOptions, shaderSourceFile, tempFolder, "DXSC"))
                {
                    return false;
                }
                shaderOutputFile = patchedShaderOutput;

                specializationOffsetsFile = offsetsOutput;
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

            if (graphicsDevMode || shaderBuildArguments.m_generateDebugInfo)
            {
                byProducts.m_intermediatePaths.emplace(AZStd::move(objectCodeOutputFile));
            }

            return true;
        }
    }
}
