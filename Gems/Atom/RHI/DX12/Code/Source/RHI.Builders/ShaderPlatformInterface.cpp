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
#include <AzCore/IO/Path/Path.h>                // AZ::IO::MaxPathLength, for the GXDK root buffer
#include <AzCore/PlatformId/PlatformDefaults.h> // AZ::PlatformXbox, for the per-target header choice
#include <AzCore/Utils/Utils.h>                 // AZ::Utils::GetEnv, to locate the GDK console dxc
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace DX12
    {
        static const char* DX12ApiName = "dx12";
        static const char* DX12ShaderPlatformName = "DX12ShaderPlatform";
        // THESE ARE PER-TARGET-PLATFORM, NOT PER-HOST. This builder serves every DX12 asset
        // platform, so "Windows" here is the folder for the pc target, not for the machine running
        // the build.
        //
        // The sub-directory is the PAL-style folder name, which is NOT the asset platform
        // identifier: the "pc" platform's headers live under "Windows". Hence the explicit mapping
        // in GetAzslHeader below rather than composing the path from platform.m_identifier.
        static const char* PlatformShaderHeader = "Builders/ShaderHeaders/Platform/Windows/DX12/PlatformHeader.hlsli";
        static const char* AzslShaderHeader = "Builders/ShaderHeaders/Platform/Windows/DX12/AzslcHeader.azsli";
        static const char* AzslShaderHeaderXbox = "Builders/ShaderHeaders/Platform/Xbox/DX12/AzslcHeader.azsli";

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
            // Mesh and Amplification stages form a mesh-shader raster program (PipelineStateType::Draw).
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Mesh;
            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Amplification;

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
            const AssetBuilderSDK::PlatformInfo& platform,
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
                platform,                                // which asset platform this job is for; selects the compiler
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
            // Previously this discarded @platform and always returned the Windows header, so every
            // DX12 target -- including consoles -- was built against Platform/Windows/DX12, which in
            // turn pulls in Atom/RPI/Platform/Windows/AzslcPlatformHeader.azsli. Adding a
            // Platform/Xbox/DX12 folder therefore had no effect at all: nothing ever asked for it.
            //
            // It matters because that header is where a platform states its shader-visible traits
            // (UNBOUNDED_SIZE, AZ_TRAITS_MATERIALS_USE_SAMPLER_ARRAY, the real/half typedefs).
            // Silently borrowing another platform's is only harmless while the two agree; the moment
            // they diverge the symptom is a shader compile error with no mention of the header.
            //
            // Vulkan solves the same problem by branching on a platform tag (HasTag("mobile")).
            // Tags are the wrong key here: "dx12" is carried by both pc and xbox, so it cannot
            // separate them. The identifier can.
            if (platform.m_identifier == AZ::PlatformXbox)
            {
                return AzslShaderHeaderXbox;
            }

            // Everything else keeps the previous behaviour, deliberately: pc maps to the Windows
            // folder, and any DX12 target added later gets a working default rather than a missing
            // file. A new console should be added as an explicit case above.
            return AzslShaderHeader;
        }

        bool ShaderPlatformInterface::CompileHLSLShader(
            const AssetBuilderSDK::PlatformInfo& platform,
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
            // Shader compiler executable.
            //
            // The xbox platform must use the GDK's console dxc, not the desktop DXC package. The
            // console compiler embeds final, precompiled console ISA into the DXIL container; plain
            // desktop DXIL is accepted by the console driver but forces a runtime recompile of every
            // shader at PSO creation ("XBSC W1003: Runtime Recompilation Required ... No precompiled
            // shader available"), which costs minutes of startup on every single launch. The console
            // compiler precompiles by default -- no extra flag is needed.
            //
            // Resolved through the GXDKLatest environment variable, which the GDK installer sets, so
            // no SDK version is hardcoded here. Failing the job when it is missing is deliberate:
            // silently falling back to desktop DXC would produce a package that boots and renders,
            // making the multi-minute startup indistinguishable from a bug -- exactly the state this
            // branch exists to remove.
            //
            // GATED OFF until root signatures are embedded at build time. The console compiler's
            // precompile step requires a root signature in the DXIL container -- every shader fails
            // with "Precompilation failed - Could not find root signature in dxil container"
            // otherwise, because Atom builds root signatures at RUNTIME (PipelineLayout::Init) and
            // never embeds one. The path to turn this on: generate the textual root signature from
            // this builder's own PipelineLayoutDescriptor, mirroring PipelineLayout::Init's exact
            // traversal (frequency-sorted params; root constants first; per-SRG CBV, resource table,
            // bindless table with offset=0, sampler table; static samplers; mesh drops the IA flag;
            // serialize as rootsig_1_0 to match the runtime's D3D_ROOT_SIGNATURE_VERSION_1), and pass
            // it via -rootsig-define. Until then, xbox uses the desktop DXC like pc: the console
            // driver accepts that DXIL but recompiles every shader at PSO creation (XBSC W1003),
            // which costs minutes of startup per launch.
            constexpr bool consoleShaderPrecompileImplemented = false;
            AZStd::string dxcRelativePath;
            if (consoleShaderPrecompileImplemented && platform.m_identifier == AZ::PlatformXbox)
            {
                char gxdkRoot[AZ::IO::MaxPathLength] = { 0 };
                if (!AZ::Utils::GetEnv(AZStd::span<char>(gxdkRoot), "GXDKLatest") || gxdkRoot[0] == '\0')
                {
                    AZ_Error(
                        DX12ShaderPlatformName, false,
                        "The GXDKLatest environment variable is not set, so the console shader compiler cannot be "
                        "found. Building %s shaders requires the GDK. Install it, or build from an environment where "
                        "GXDKLatest points at <GDK>/<version>/GXDK/.",
                        platform.m_identifier.c_str());
                    return false;
                }
                dxcRelativePath = AZStd::string(gxdkRoot);
                AzFramework::StringFunc::Path::Join(dxcRelativePath.c_str(), "bin/Scarlett/dxc.exe", dxcRelativePath);
            }
            else
            {
                dxcRelativePath = RHI::GetDirectXShaderCompilerPath("Builders/DirectXShaderCompiler/dxc.exe");
            }

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
            // Mesh and Amplification shaders require Shader Model 6.5, so they also use an explicit
            // profile string ("ms_6_5"/"as_6_5") rather than the shared shaderModelVersion constant.
            const AZStd::string shaderModelVersion = "6_2";
            const AZStd::unordered_map<RHI::ShaderHardwareStage, AZStd::string> stageToProfileName =
            {
                {RHI::ShaderHardwareStage::Vertex,                 "vs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Fragment,               "ps_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Compute,                "cs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Geometry,               "gs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Mesh,                   "ms_6_5"},
                {RHI::ShaderHardwareStage::Amplification,          "as_6_5"},
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
