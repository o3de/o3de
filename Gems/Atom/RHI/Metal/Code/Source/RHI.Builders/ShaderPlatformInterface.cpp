/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <RHI.Builders/ShaderPlatformInterface.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Reflect/Metal/Base.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Metal
    {
        static const char* MetalShaderPlatformName = "MetalShaderPlatform";
        static const char* MacPlatformShaderHeader = "Builders/ShaderHeaders/Platform/Mac/Metal/PlatformHeader.hlsli";
        static const char* IosPlatformShaderHeader = "Builders/ShaderHeaders/Platform/iOS/Metal/PlatformHeader.hlsli";
        static const char* MacAzslShaderHeader = "Builders/ShaderHeaders/Platform/Mac/Metal/AzslcHeader.azsli";
        static const char* IosAzslShaderHeader = "Builders/ShaderHeaders/Platform/iOS/Metal/AzslcHeader.azsli";

        ShaderPlatformInterface::ShaderPlatformInterface(uint32_t apiUniqueIndex)
            : RHI::ShaderPlatformInterface(apiUniqueIndex)
        {
            // Initialize to nullptr so we can detect whether BuildPipelineLayoutDescriptor
            // was called before CompilePlatformInternal or not.
            // BuildPipelineLayoutDescriptor should be called at least once
            // before CompilePlatformInternal.
            auto it = m_srgLayouts.begin();
            while (it != m_srgLayouts.end())
            {
                *it = nullptr;
            }
        }

        RHI::APIType ShaderPlatformInterface::GetAPIType() const
        {
            return Metal::RHIType;
        }

        AZ::Name ShaderPlatformInterface::GetAPIName() const
        {
            return m_apiName;
        }

        RHI::Ptr <RHI::PipelineLayoutDescriptor> ShaderPlatformInterface::CreatePipelineLayoutDescriptor()
        {
            return AZ::Metal::PipelineLayoutDescriptor::Create();
        }
 
        bool ShaderPlatformInterface::BuildPipelineLayoutDescriptor(
            RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor,
            const ShaderResourceGroupInfoList& srgInfoList,
            const RootConstantsInfo& rootConstantsInfo,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments)
        {
            AZ::Metal::PipelineLayoutDescriptor* metalDescriptor = azrtti_cast<AZ::Metal::PipelineLayoutDescriptor*>(pipelineLayoutDescriptor.get());
            AZ_Assert(metalDescriptor, "PipelineLayoutDescriptor should have been created by now");
            
            const uint32_t groupLayoutCount = static_cast<uint32_t>(srgInfoList.size());
            AZ_Assert(groupLayoutCount <= RHI::Limits::Pipeline::ShaderResourceGroupCountMax, "Exceeded ShaderResourceGroupLayout count limit.");
            
            // Slot to index mapping
            AZ::Metal::SlotToIndexTable slotToIndexTable;
            AZ::Metal::IndexToSlotTable indexToSlotTable;
            slotToIndexTable.fill(static_cast<uint8_t>(RHI::Limits::Pipeline::ShaderResourceGroupCountMax));
            indexToSlotTable.resize(groupLayoutCount);
            m_srgLayouts.clear(); // In case we are building pipeline layout descriptor for multiple shaders.
            m_srgLayouts.resize(groupLayoutCount);

            RHI::ShaderPlatformInterface::ShaderResourceGroupInfoList sortedSrgInfos = srgInfoList;
            //Sort the SRGs to ensure the ones with lowest bindingSlot/SpaceId go at the lowest index in order to honor the frequencyId.
            AZStd::sort(sortedSrgInfos.begin(), sortedSrgInfos.end(),
                [](const RHI::ShaderPlatformInterface::ShaderResourceGroupInfo& first, const RHI::ShaderPlatformInterface::ShaderResourceGroupInfo& second) -> bool
            {
                return first.m_layout->GetBindingSlot() < second.m_layout->GetBindingSlot();
            });
             
            for (uint32_t groupLayoutIndex = 0; groupLayoutIndex < groupLayoutCount; ++groupLayoutIndex)
            {
                const auto& srgInfo = sortedSrgInfos[groupLayoutIndex];
                const RHI::ShaderResourceGroupLayout& groupLayout = *srgInfo.m_layout;
                const uint32_t srgLayoutSlot = groupLayout.GetBindingSlot();
                
                AZ_Assert(srgLayoutSlot <= RHI::Limits::Pipeline::ShaderResourceGroupCountMax, "Cannot exceed the array limit");
                slotToIndexTable[srgLayoutSlot] = groupLayoutIndex;
                indexToSlotTable[groupLayoutIndex] = srgLayoutSlot;
                
                ShaderResourceGroupVisibility srgVisibility;
                for (const auto& resourceBindInfo : srgInfo.m_bindingInfo.m_resourcesRegisterMap)
                {
                    srgVisibility.m_resourcesStageMask.insert({ resourceBindInfo.first, resourceBindInfo.second.m_shaderStageMask });
                }
                srgVisibility.m_constantDataStageMask = srgInfo.m_bindingInfo.m_constantDataBindingInfo.m_shaderStageMask;
                metalDescriptor->AddShaderResourceGroupVisibility(srgVisibility);
                
                //cache the layout in order to fill out unused variables
                m_srgLayouts[groupLayoutIndex] = srgInfo.m_layout;
            }
            
            if (rootConstantsInfo.m_totalSizeInBytes > 0)
            {
                metalDescriptor->SetRootConstantBinding(RootConstantBinding{ rootConstantsInfo.m_registerId, rootConstantsInfo.m_spaceId });
            }
            
            metalDescriptor->SetBindingTables(slotToIndexTable, indexToSlotTable);
            return metalDescriptor->Finalize() == RHI::ResultCode::Success;
        }
    
        RHI::Ptr<RHI::ShaderStageFunction> ShaderPlatformInterface::CreateShaderStageFunction(const StageDescriptor& stageDescriptor)
        {
            RHI::Ptr<ShaderStageFunction> newShaderStageFunction = ShaderStageFunction::Create(RHI::ToRHIShaderStage(stageDescriptor.m_stageType));
            
            const Metal::ShaderSourceCode& sourceCode = stageDescriptor.m_sourceCode;

            //Metal sourceCode is great for debugging but it is not needed as we are also packing the bytecode. This
            //can be removed for more optimized shader assets
            newShaderStageFunction->SetSourceCode(sourceCode);

            const Metal::ShaderByteCode& byteCode = stageDescriptor.m_byteCode;
            const AZStd::string& entryFunctionName = stageDescriptor.m_entryFunctionName;
            newShaderStageFunction->SetByteCode(byteCode);
            newShaderStageFunction->SetEntryFunctionName(entryFunctionName);
            
            newShaderStageFunction->Finalize();
            return newShaderStageFunction;
        }

        bool ShaderPlatformInterface::IsShaderStageForRaster(RHI::ShaderHardwareStage shaderStageType) const
        {
            bool hasRasterProgram = false;

            hasRasterProgram |= shaderStageType == RHI::ShaderHardwareStage::Vertex;
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

        AZStd::string ShaderPlatformInterface::GetAzslCompilerParameters(const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            // Note: all platforms use DirectX packing rules. We enable vk namespace as well to allow
            // for vk syntax to carry  through from dxc to spirv-cross.
            return shaderCompilerArguments.MakeAdditionalAzslcCommandLineString() +
                " --use-spaces --unique-idx --namespace=mt,vk --root-const=128 --pad-root-const";
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
            if(platform.HasTag("mobile"))
            {
                return IosAzslShaderHeader;
            }
            else
            {
                return MacAzslShaderHeader;
            }
        }

       bool ShaderPlatformInterface::CompilePlatformInternal(
           const AssetBuilderSDK::PlatformInfo& platform,
           const AZStd::string& shaderSourcePath,
           const AZStd::string& functionName,
           RHI::ShaderHardwareStage shaderStage,
           const AZStd::string& tempFolderPath,
           StageDescriptor& outputDescriptor,
           const RHI::ShaderCompilerArguments& shaderCompilerArguments) const
        {
            for ([[maybe_unused]] auto srgLayout : m_srgLayouts)
            {
                AZ_Assert(srgLayout != nullptr, "Most likely BuildPipelineLayoutDescriptor() was not called!");
            }

            AZStd::vector<char> shaderSourceCode;
            AZStd::vector<uint8_t> shaderByteCode;

            // Compile HLSL shader to METAL source code
            bool compiledSucessfully = CompileHLSLShader(
                shaderSourcePath,                // shader source filename
                tempFolderPath,                  // AP temp folder for the job
                functionName,                    // name of function that is the entry point
                shaderStage,                     // shader stage (vertex shader, pixel shader, ...)
                shaderCompilerArguments,
                shaderSourceCode,                // cross-compiled shader output
                shaderByteCode,                  // compiled byte code
                platform,                        // target platform
                outputDescriptor.m_byProducts);  // debug objects

            if (!compiledSucessfully)
            {
                AZ_Error(MetalShaderPlatformName, false, "Failed to cross-compile HLSL shader to Metal");
                return false;
            }

            if (shaderSourceCode.size() > 0)
            {
                outputDescriptor.m_stageType = shaderStage;
                outputDescriptor.m_sourceCode = AZStd::move(shaderSourceCode);
                outputDescriptor.m_byteCode = AZStd::move(shaderByteCode);
                outputDescriptor.m_entryFunctionName = AZStd::move(functionName);
            }
            else
            {
                AZ_Error(MetalShaderPlatformName, false, "Compiled shader for %s is invalid", shaderSourcePath.c_str());
                return false;
            }

            return true;
        }

        /* We cross compile to metal SL by going through following transformations
         *  DXC(HLSL)-->SPIR-V Cross(SPIR-V)-->MSL
         */
        bool ShaderPlatformInterface::CompileHLSLShader(
            const AZStd::string& shaderSourceFile,
            const AZStd::string& tempFolder,
            const AZStd::string& entryPoint,
            const RHI::ShaderHardwareStage shaderType,
            const RHI::ShaderCompilerArguments& shaderCompilerArguments,
            AZStd::vector<char>& sourceMetalShader,
            AZStd::vector<uint8_t>& compiledByteCode,
            const AssetBuilderSDK::PlatformInfo& platform,
            ByProducts& byProducts) const
        {
            // Shader compiler executable
            static const char* dxcRelativePath = "Builders/DirectXShaderCompiler/bin/dxc";

            // Output file
            AZStd::string shaderMSLOutputFile = RHI::BuildFileNameWithExtension(shaderSourceFile, tempFolder, "metal");

            // Stage profile name parameter
            const AZStd::string shaderModelVersion = "6_2";
            
            const AZStd::unordered_map<RHI::ShaderHardwareStage, AZStd::string> stageToProfileName =
            {
                {RHI::ShaderHardwareStage::Vertex,   "vs_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Fragment, "ps_" + shaderModelVersion},
                {RHI::ShaderHardwareStage::Compute,  "cs_" + shaderModelVersion}
            };
            auto profileIt = stageToProfileName.find(shaderType);
            if (profileIt == stageToProfileName.end())
            {
                AZ_Error(MetalShaderPlatformName, false, "Unsupported shader stage");
                return false;
            }
            
            // For this approach we will be doing hlsl->spirv(through dxc) and spirv->metalSL(through spirv cross)
            // Output spirv file
            AZStd::string shaderSpirvOutputFile = RHI::BuildFileNameWithExtension(shaderSourceFile, tempFolder, "spirv");
            
            // Compilation parameters
            AZStd::string params = shaderCompilerArguments.MakeAdditionalDxcCommandLineString();
            params += " -spirv"; // Generate SPIRV shader
            
            // Enable half precision types when shader model >= 6.2
            int shaderModelMajor = 0;
            int shaderModelMinor = 0;
            [[maybe_unused]] int numValuesRead = azsscanf(shaderModelVersion.c_str(), "%d_%d", &shaderModelMajor, &shaderModelMinor);
            AZ_Assert(numValuesRead == 2, "Unknown shader model version format");
            if (shaderModelMajor >= 6 && shaderModelMinor >= 2)
            {
                params += " -enable-16bit-types";
            }
            StringFunc::TrimWhiteSpace(params, true, false);

            AZStd::string prependFile;
            if(platform.HasTag("mobile"))
            {
                prependFile = IosPlatformShaderHeader;
            }
            else
            {
                prependFile = MacPlatformShaderHeader;
            }

            RHI::PrependArguments args;
            args.m_sourceFile = shaderSourceFile.c_str();
            args.m_prependFile = prependFile.c_str();
            args.m_destinationFolder = tempFolder.c_str();

            const auto dxcInputFile = RHI::PrependFile(args);
            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                // dump intermediate "true final HLSL" file (shadername.metal.shadersource.prepend)
                byProducts.m_intermediatePaths.insert(dxcInputFile);
            }
            //                                                      1.entry   3.config       5.hlsl-in
            //                                                          |   2.SM  |   4.output   |
            //                                                          |     |   |       |      |
            AZStd::string dxcCommandOptions = AZStd::string::format("-E %s -T %s %s -Fo \"%s\" \"%s\"",
                                                                    entryPoint.c_str(),            // 1
                                                                    profileIt->second.c_str(),     // 2
                                                                    params.c_str(),                // 3
                                                                    shaderSpirvOutputFile.c_str(), // 4
                                                                    dxcInputFile.c_str());         // 5
            
            // Run dxc Compiler
            if (!RHI::ExecuteShaderCompiler(dxcRelativePath, dxcCommandOptions, shaderSourceFile, "DXC"))
            {
                AZ_Error(MetalShaderPlatformName, false, "DXC failed to create the spirv file");
                return false;
            }
            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                byProducts.m_intermediatePaths.insert(shaderSpirvOutputFile);   // the spirv spit by DXC
            }
            
            IO::FileIOStream spirvOutFileStream(shaderSpirvOutputFile.data(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
            
            if (!spirvOutFileStream.IsOpen())
            {
                AZ_Error(MetalShaderPlatformName, false, "Failed because the shader file \"%s\" could not be opened", shaderSpirvOutputFile.data());
                return false;
            }
            if (!spirvOutFileStream.CanRead())
            {
                AZ_Error(MetalShaderPlatformName, false, "Failed because the shader file \"%s\" could not be read", shaderSpirvOutputFile.data());
                spirvOutFileStream.Close();
                return false;
            }
            
            // spirv cross compiler executable
            static const char* spirvCrossRelativePath = "Builders/SPIRVCross/spirv-cross";
           
            AZStd::string spirvCrossCommandOptions = AZStd::string::format("--msl --msl-version 20100 --msl-argument-buffers --msl-decoration-binding --msl-texture-buffer-native --output \"%s\" \"%s\"", shaderMSLOutputFile.c_str(), shaderSpirvOutputFile.c_str());
            
            // Run spirv cross
            if (!RHI::ExecuteShaderCompiler(spirvCrossRelativePath, spirvCrossCommandOptions, shaderSpirvOutputFile, "SpirvCross"))
            {
                AZ_Error(MetalShaderPlatformName, false, "SPIRV-Cross failed to cross compil to metal source.");
                spirvOutFileStream.Close();
                return false;
            }
            spirvOutFileStream.Close();
            
            IO::FileIOStream outFileStream(shaderMSLOutputFile.data(), IO::OpenMode::ModeRead);
            bool finalizeShaderResult = UpdateCompiledShader(outFileStream, MetalShaderPlatformName, shaderMSLOutputFile.data(), sourceMetalShader);
            AZ_Assert(finalizeShaderResult, "Final compiled shader was not created. Check if %s was created", shaderMSLOutputFile.c_str());

            if (BuildHasDebugInfo(shaderCompilerArguments))
            {
                byProducts.m_intermediatePaths.emplace(AZStd::move(shaderMSLOutputFile));   // .msl metal out of sv-cross
            }
            
            bool compileMetalSL = CreateMetalLib(MetalShaderPlatformName, shaderSourceFile, tempFolder, compiledByteCode, sourceMetalShader, platform);
            if (!compileMetalSL)
            {
                AZ_Error(MetalShaderPlatformName, false, "Failed to create bytecode");
                return false;
            }

            return finalizeShaderResult;
        }
        
        bool ShaderPlatformInterface::UpdateCompiledShader(AZ::IO::FileIOStream& fileStream, const char* platformName, const char* fileName, AZStd::vector<char>& compiledShader) const
        {
            if (!fileStream.IsOpen())
            {
                AZ_Error(platformName, false, "Failed because the shader file \"%s\" could not be opened", fileName);
                return false;
            }
            if (!fileStream.CanRead())
            {
                AZ_Error(platformName, false, "Failed because the shader file \"%s\" could not be read", fileName);
                fileStream.Close();
                return false;
            }
            
            compiledShader.resize(fileStream.GetLength() + 1); // +1 to add end of string
            memset(compiledShader.data(), 0, fileStream.GetLength() + 1);
            fileStream.Read(fileStream.GetLength(), compiledShader.data());
            fileStream.Close();
            
            //Ensure that the argument buffer declaration in the shader matches the srg layout
            return AddUnusedResources(compiledShader);
        }
    
        bool ShaderPlatformInterface::CreateMetalLib(const char* platformName,
                                                     const AZStd::string& shaderSourceFile,
                                                     const AZStd::string& tempFolder,
                                                     AZStd::vector<uint8_t>& compiledByteCode,
                                                     AZStd::vector<char>& sourceMetalShader,
                                                     const AssetBuilderSDK::PlatformInfo& platform) const
        {
            AZStd::string inputMetalFile = RHI::BuildFileNameWithExtension(shaderSourceFile, tempFolder, "metal");
            
            AZ::IO::FileIOStream sourceMtlfileStream(inputMetalFile.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);
            if (!sourceMtlfileStream.IsOpen())
            {
                AZ_Error(platformName, false, "Failed because the shader file \"%s\" could not be opened", inputMetalFile.c_str());
                return false;
            }
            
            AZStd::string mtlSource = AZStd::string(sourceMetalShader.begin(), sourceMetalShader.end());
            sourceMtlfileStream.Write(mtlSource.size(), mtlSource.data());
            sourceMtlfileStream.Close();
            
            AZStd::string outputAirFile = RHI::BuildFileNameWithExtension(shaderSourceFile, tempFolder, "air");
            AZStd::string outMetalLibFile = RHI::BuildFileNameWithExtension(shaderSourceFile, tempFolder, "metallib");
            
            //Debug symbols are always enabled at the moment. Need to turn them off for optimized shader assets. 
            AZStd::string shaderDebugInfo = "-gline-tables-only -MO";

            //Apply the correct platform sdk option
            AZStd::string platformSdk = "macosx";
            if (platform.HasTag("mobile"))
            {
                platformSdk = "iphoneos";
            }
            
            //Convert to air file
            AZStd::string mslToAirCommandOptions = AZStd::string::format("-sdk %s metal \"%s\" %s -c -o \"%s\"", platformSdk.c_str(), inputMetalFile.c_str(), shaderDebugInfo.c_str(), outputAirFile.c_str());
            
            if (!RHI::ExecuteShaderCompiler("/usr/bin/xcrun", mslToAirCommandOptions, inputMetalFile, "MslToAir"))
            {
                AZ_Error(MetalShaderPlatformName, false, "Failed to convert to AIR file %s", inputMetalFile.c_str());
                return false;
            }
            
            //convert to metallib
            AZStd::string airToMetalLibCommandOptions = AZStd::string::format("-sdk %s metallib \"%s\" -o \"%s\"", platformSdk.c_str(), outputAirFile.c_str(), outMetalLibFile.c_str());
            
            if (!RHI::ExecuteShaderCompiler("/usr/bin/xcrun", airToMetalLibCommandOptions, outputAirFile, "AirToMetallib"))
            {
                AZ_Error(MetalShaderPlatformName, false, "Failed to convert to metallib file");
                return false;
            }
            
            AZ::IO::FileIOStream fileStream(outMetalLibFile.data(), AZ::IO::OpenMode::ModeRead);
            compiledByteCode.resize(fileStream.GetLength());
            memset(compiledByteCode.data(), 0, fileStream.GetLength() );
            fileStream.Read(fileStream.GetLength(), compiledByteCode.data());
            fileStream.Close();
            
            return true;
        }
        
        bool ShaderPlatformInterface::AddUnusedResources(AZStd::vector<char>& compiledShader) const
        {
            AZStd::string finalMetalSLStr = AZStd::string(compiledShader.begin(), compiledShader.end());
            
            const uint32_t groupLayoutCount = static_cast<uint32_t>(m_srgLayouts.size());
            AZStd::string constantBufferTempStructs = "\n";
            AZStd::string structuredBufferTempStructs = "\n";
            
            for (uint32_t groupLayoutIndex = 0; groupLayoutIndex < groupLayoutCount; ++groupLayoutIndex)
            {
                //const auto& srgInfo = m_srgInfoList[groupLayoutIndex];
                const RHI::ShaderResourceGroupLayout& groupLayout = *m_srgLayouts[groupLayoutIndex];
                
                //Check if an argument buffer declaration exists for this srg layout.
                AZStd::string srgBuffer = AZStd::string::format("spvDescriptorSetBuffer%i", groupLayoutIndex);
                size_t startOfArgBufferPos = finalMetalSLStr.find(srgBuffer);
                if (startOfArgBufferPos == AZStd::string::npos)
                {
                    continue;
                }

                size_t endOfArgBufferPos = finalMetalSLStr.find("}", startOfArgBufferPos);
                AZStd::string fullArgBufferDeclarationStr = finalMetalSLStr.substr(startOfArgBufferPos,endOfArgBufferPos - startOfArgBufferPos + 1);
                
                //Add all the existing or dummy entries into m_argBufferEntries which is a set. The reason for using a set
                //is because we need the entries to be sorted based on the register and we do not want duplicates.
                bool result = AddConstantBufferEntries(groupLayout, constantBufferTempStructs, fullArgBufferDeclarationStr, groupLayoutIndex);
                if(!result)
                {
                     AZ_Error(MetalShaderPlatformName, false, "Failed because adding constant buffer entries within AddUnusedResources failed");
                    return false;
                }
                
                result = AddImageEntries(groupLayout, fullArgBufferDeclarationStr);
                if(!result)
                {
                    AZ_Error(MetalShaderPlatformName, false, "Failed because adding image entries within AddUnusedResources failed");
                    return false;
                }
                
                result = AddSamplerEntries(groupLayout, fullArgBufferDeclarationStr);
                if(!result)
                {
                    AZ_Error(MetalShaderPlatformName, false, "Failed because adding static sampler entries within AddUnusedResources failed");
                    return false;
                }
                
                result = AddBufferEntries(groupLayout, structuredBufferTempStructs, fullArgBufferDeclarationStr, groupLayoutIndex);
                if(!result)
                {
                    AZ_Error(MetalShaderPlatformName, false, "Failed because adding buffer entries within AddUnusedResources failed");
                    return false;
                }
                
                //Create a new spvDescriptorSetBuffer which matches the layout.
                AZStd::string newArgBufferLayoutStr = "\n";
                for (const ArgBufferEntries &entry : m_argBufferEntries )
                {
                    newArgBufferLayoutStr += "    " + entry.first + "\n";
                }
                
                //Replace the existing declaration with the new one just generated.
                //We look for '{' and '}' to find out boundaries of the argument buffer declaration to replace
                size_t startOfArgBufferBracketPos = finalMetalSLStr.find("{", startOfArgBufferPos) + 1;
                size_t endOfArgBufferBracketPos = finalMetalSLStr.find("}", startOfArgBufferBracketPos) - 1;
                finalMetalSLStr.replace(startOfArgBufferBracketPos, endOfArgBufferBracketPos - startOfArgBufferBracketPos, newArgBufferLayoutStr);
                
                m_argBufferEntries.clear();
            }
            
            //Add dummy definitions of constant buffer and structured buffer types to the top of the file
            AZStd::string startOfShaderTag = "using namespace metal;";
            const size_t startOfShaderPos = finalMetalSLStr.find(startOfShaderTag);
            if (startOfShaderPos != AZStd::string::npos)
            {
                finalMetalSLStr.insert(startOfShaderPos + startOfShaderTag.length() + 1, constantBufferTempStructs);
                finalMetalSLStr.insert(startOfShaderPos + startOfShaderTag.length() + 1, structuredBufferTempStructs);
            }
            
            compiledShader = AZStd::vector<char>(finalMetalSLStr.begin(), finalMetalSLStr.end());
            return true;
        }
        
        bool ShaderPlatformInterface::AddConstantBufferEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                                               AZStd::string& constantBufferTempStructs,
                                                               AZStd::string& argBufferStr,
                                                               uint32_t groupLayoutIndex) const
        {
            AZStd::array_view<RHI::ShaderInputConstantDescriptor> shaderInputConstantList = groupLayout.GetShaderInputListForConstants();
            if (shaderInputConstantList.empty())
            {
                return true;
            }
            
            //Only need the information from the first element of the constant buffer.
            const RHI::ShaderInputConstantDescriptor& shaderInputConstant = shaderInputConstantList[0];
            
            uint32_t regId = shaderInputConstant.m_registerId;
            AZStd::string srgResource = AZStd::string::format("id(%i)", regId);
            
            size_t resourceStartPos = argBufferStr.find(srgResource);
            //Check if we need to create a dummy entry
            if (resourceStartPos == AZStd::string::npos)
            {
                uint32_t numElements = groupLayout.GetConstantDataSize()/sizeof(float);
                AZ_Assert(numElements > 0, "There needs to be atleast one element");
                /*
                 * Add dummy declaration of the type. It looks like this
                 *
                 * struct type_DummyStruct"regId"_DescSet"groupLayoutIndex"
                 * {
                 *     float dummyArray["numElements"];
                 * };
                 *
                 */
                constantBufferTempStructs += AZStd::string::format("struct type_DummyStruct%i_DescSet%i\n{\n    float dummyArray[%i];\n};\n", regId, groupLayoutIndex, numElements);
                
                 //Create the final resource entry to be added to the set
                AZStd::string dummyResource = AZStd::string::format("constant type_DummyStruct%i_DescSet%i* dummyConstantBuffer%i [[id(%i)]];", regId, groupLayoutIndex, regId, regId);
                m_argBufferEntries.insert(AZStd::make_pair(dummyResource, regId));
                return true;
            }
            else
            {
                //Constant buffer should always be in the constant address space
                return AddExistingResourceEntry("constant type_ConstantBuffer", resourceStartPos, regId, argBufferStr);
            }
        }
        
        bool ShaderPlatformInterface::AddImageEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                                      AZStd::string& argBufferStr) const
        {
            bool result = true;
            for (const RHI::ShaderInputImageDescriptor& shaderInputImage : groupLayout.GetShaderInputListForImages())
            {
                uint32_t regId = shaderInputImage.m_registerId;
                AZStd::string srgResource = AZStd::string::format("id(%i)", regId);
                
                const size_t resourceStartPos = argBufferStr.find(srgResource);
                //Check if we need to create a dummy entry
                if (resourceStartPos == AZStd::string::npos)
                {
                    AZStd::string textureType;
                    switch(shaderInputImage.m_type)
                    {
                        case RHI::ShaderInputImageType::Image1D:
                        {
                            textureType = "texture1d";
                            break;
                        }
                        case RHI::ShaderInputImageType::Image1DArray:
                        {
                            textureType = "texture1d_array";
                            break;
                        }
                        case RHI::ShaderInputImageType::Image2D:
                        {
                            textureType = "texture2d";
                            break;
                        }
                        case RHI::ShaderInputImageType::Image2DArray:
                        {
                            textureType = "texture2d_array";
                            break;
                        }
                        case RHI::ShaderInputImageType::Image2DMultisample:
                        {
                            textureType = "texture2d_ms";
                            break;
                        }
                        case RHI::ShaderInputImageType::Image3D:
                        {
                            textureType = "texture3d";
                            break;
                        }
                        case RHI::ShaderInputImageType::ImageCube:
                        {
                            textureType = "texturecube";
                            break;
                        }
                        case RHI::ShaderInputImageType::ImageCubeArray:
                        {
                            textureType = "texturecube_array";
                            break;
                        }
                        default:
                        {
                            AZ_Assert(false, "Invalid texture type.");
                        }
                    }
                    
                    //Create the resource entry to be added to the set. Handle arrays by checking the shaderInputImage.m_count
                    AZStd::string dummyResource;
                    if(shaderInputImage.m_count > 1)
                    {
                        dummyResource = AZStd::string::format("const array<%s<float>, %i> dummyImage%i [[id(%i)]];", textureType.c_str(), shaderInputImage.m_count, regId, regId);
                    }
                    else
                    {
                        dummyResource = AZStd::string::format("%s<float> dummyImage%i [[id(%i)]];", textureType.c_str(), regId, regId);
                    }
                    m_argBufferEntries.insert(AZStd::make_pair(dummyResource, regId));
                }
                else
                {
                    bool isAdditionSuccessfull = AddExistingResourceEntry("texture", resourceStartPos, regId, argBufferStr);
                    if(!isAdditionSuccessfull)
                    {
                        //In metal depth textures use keyword depth2d/depth2d_array/depthcube/depthcube_array/depth2d_ms/depth2d_ms_array
                        isAdditionSuccessfull |= AddExistingResourceEntry("depth", resourceStartPos, regId, argBufferStr);
                    }
                    result &= isAdditionSuccessfull;
                }
            }
            return result;
        }
        
        bool ShaderPlatformInterface::ProcessSamplerEntry(uint32_t regId, AZStd::string& argBufferStr, uint32_t samplercount) const
        {
            AZStd::string srgResource = AZStd::string::format("id(%i)", regId);
            
            const size_t resourceStartPos = argBufferStr.find(srgResource);
            //Check if we need to create a dummy entry
            if (resourceStartPos == AZStd::string::npos)
            {
                //Create the resource entry to be added to the set. Handle arrays by checking the samplercount
                AZStd::string dummyResource;
                if(samplercount > 1)
                {
                    dummyResource = AZStd::string::format("const array<sampler, %i> dummySampler%i [[id(%i)]];", samplercount, regId, regId);
                }
                else
                {
                    dummyResource = AZStd::string::format("sampler dummySampler%i [[id(%i)]];", regId, regId);
                }
                m_argBufferEntries.insert(AZStd::make_pair(dummyResource, regId));
                return true;
            }
            else
            {
                return AddExistingResourceEntry("sampler", resourceStartPos, regId, argBufferStr);
            }
        }
        
        bool ShaderPlatformInterface::AddSamplerEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                                        AZStd::string& argBufferStr) const
        {
            bool result = true;
            for (const RHI::ShaderInputStaticSamplerDescriptor& staticSampler : groupLayout.GetStaticSamplers())
            {
                result &= ProcessSamplerEntry(staticSampler.m_registerId, argBufferStr, 0);
            }
            
            for (const RHI::ShaderInputSamplerDescriptor& dynamicSampler : groupLayout.GetShaderInputListForSamplers())
            {
                result &= ProcessSamplerEntry(dynamicSampler.m_registerId, argBufferStr, dynamicSampler.m_count);
            }
            
            return result;
        }
        
        bool ShaderPlatformInterface::AddBufferEntries(const RHI::ShaderResourceGroupLayout& groupLayout,
                                                                 AZStd::string& structuredBufferTempStructs,
                                                                 AZStd::string& argBufferStr,
                                                                 uint32_t groupLayoutIndex) const
        {
            bool result = true;
            for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : groupLayout.GetShaderInputListForBuffers())
            {
                uint32_t regId = shaderInputBuffer.m_registerId;
                AZStd::string srgResource = AZStd::string::format("id(%i)", regId);
                
                size_t resourceStartPos = argBufferStr.find(srgResource);
                //Check if we need to create a dummy entry
                if (resourceStartPos == AZStd::string::npos)
                {
                    uint32_t numElements = shaderInputBuffer.m_strideSize/sizeof(float);
                    AZ_Assert(numElements > 0, "There needs to be atleast one element");
                    /*
                    * Add dummy declaration of the type. It looks like this
                    *
                    * struct DummySRG_"Name"_DescSet"groupLayoutIndex"
                    * {
                    *     float dummyArray["numElements"];
                    * };
                    *
                    * struct type_RWStructuredDummyBuffer"regId"_DescSet"groupLayoutIndex"
                    * {
                    *     DummySRG_"Name"_DescSet"groupLayoutIndex" _m0["shaderInputBuffer.m_count"];
                    * };
                    */
                    structuredBufferTempStructs += AZStd::string::format("struct DummySRG_%s_DescSet%i\n{\n    float dummyArray[%i];\n};\n", shaderInputBuffer.m_name.GetCStr(), groupLayoutIndex, numElements);
                    structuredBufferTempStructs += AZStd::string::format("struct type_RWStructuredDummyBuffer%i_DescSet%i\n{\n    DummySRG_%s_DescSet%i _m0[%i];\n};\n", regId, groupLayoutIndex, shaderInputBuffer.m_name.GetCStr(), groupLayoutIndex, shaderInputBuffer.m_count);
                    
                    //Create the final resource entry to be added to the set
                    AZStd::string dummyResource = AZStd::string::format("device type_RWStructuredDummyBuffer%i_DescSet%i* dummyStructuredBuffer%i [[id(%i)]];", regId, groupLayoutIndex, regId, regId);
                    m_argBufferEntries.insert(AZStd::make_pair(dummyResource, regId));
                }
                else
                {
                    bool entryAddedSuccessfully = false;
                    switch(shaderInputBuffer.m_type)
                    {
                        case RHI::ShaderInputBufferType::Structured:
                        {
                            switch(shaderInputBuffer.m_access)
                            {
                                case  RHI::ShaderInputBufferAccess::Read:
                                {
                                    entryAddedSuccessfully = AddExistingResourceEntry("const device type_StructuredBuffer", resourceStartPos, regId, argBufferStr);
                                    break;
                                }
                                case RHI::ShaderInputBufferAccess::ReadWrite:
                                {
                                    entryAddedSuccessfully = AddExistingResourceEntry("device type_RWStructuredBuffer", resourceStartPos, regId, argBufferStr);
                                    break;
                                }
                            }
                            break;
                        }
                        case RHI::ShaderInputBufferType::Typed:
                        {
                            switch(shaderInputBuffer.m_access)
                            {
                                case RHI::ShaderInputBufferAccess::Read:
                                case RHI::ShaderInputBufferAccess::ReadWrite:
                                {
                                    entryAddedSuccessfully = AddExistingResourceEntry("texture_buffer", resourceStartPos, regId, argBufferStr);
                                    break;
                                }
                            }
                            break;
                        }
                        case RHI::ShaderInputBufferType::Raw:
                        {
                            switch(shaderInputBuffer.m_access)
                            {
                                case  RHI::ShaderInputBufferAccess::Read:
                                {
                                    entryAddedSuccessfully = AddExistingResourceEntry("const device type_ByteAddressBuffer", resourceStartPos, regId, argBufferStr);
                                    break;
                                }
                                case RHI::ShaderInputBufferAccess::ReadWrite:
                                {
                                    entryAddedSuccessfully = AddExistingResourceEntry("device type_RWByteAddressBuffer", resourceStartPos, regId, argBufferStr);
                                    break;
                                }
                            }
                            break;
                        }
                        case RHI::ShaderInputBufferType::Constant:
                        {
                            entryAddedSuccessfully = AddExistingResourceEntry("constant type_ConstantBuffer", resourceStartPos, regId, argBufferStr);
                            break;
                        }
                    }
                    result = result && entryAddedSuccessfully;
                }
            }
            return result;
        }
        
        bool ShaderPlatformInterface::AddExistingResourceEntry(const char* resourceStr,
                                                               size_t resourceStartPos,
                                                               uint32_t regId,
                                                               AZStd::string& argBufferStr) const
        {
            size_t prevEndOfLine = argBufferStr.rfind("\n", resourceStartPos);
            size_t nextEndOfLine = argBufferStr.find("\n", resourceStartPos);
            size_t startOfEntryPos = argBufferStr.find(resourceStr, prevEndOfLine);
            
            //Check to see if a valid entry is found. 
            if(startOfEntryPos == AZStd::string::npos || startOfEntryPos > nextEndOfLine)
            {
                AZ_Error(MetalShaderPlatformName, startOfEntryPos != AZStd::string::npos, "Entry-> %s not found within Descriptor set %s", resourceStr, argBufferStr.c_str());
                return false;
            }
            else
            {
                size_t endOfEntryPos = argBufferStr.find("\n", startOfEntryPos);
                AZ_Assert(endOfEntryPos != AZStd::string::npos, "Resource entry missing");
                
                AZStd::string existingEntry = argBufferStr.substr(prevEndOfLine,endOfEntryPos - prevEndOfLine);
                m_argBufferEntries.insert(AZStd::make_pair(existingEntry, regId));
                return true;
            }
        }
    }
}
