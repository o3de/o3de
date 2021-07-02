/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <regex>

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAssetCreator.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/FileIO.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AzslData.h>
#include <AzslCompiler.h>
#include <CommonFiles/Preprocessor.h>
#include <CommonFiles/GlobalBuildOptions.h>
#include <SrgLayoutBuilder.h>
#include <ShaderPlatformInterfaceRequest.h>
#include <ShaderBuilderUtility.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        AZ::Uuid SrgLayoutBuilder::GetUUID()
        {
            return AZ::Uuid::CreateString("{ABC78905-B3FC-497A-916A-217D1460E52F}");
        }

        void SrgLayoutBuilder::Reflect([[maybe_unused]] AZ::ReflectContext* context)
        {
        }

        SrgLayoutBuilder::SrgLayoutBuilder()
        {
        }

        SrgLayoutBuilder::~SrgLayoutBuilder()
        {
        }

        void SrgLayoutBuilder::Activate()
        {
        }

        void SrgLayoutBuilder::Deactivate()
        {
        }

        void SrgLayoutBuilder::ShutDown()
        {
        }

        RHI::ShaderInputBufferType ToShaderInputBufferType(BufferType bufferType)
        {
            switch (bufferType)
            {
            case BufferType::Buffer:
            case BufferType::RwBuffer:
            case BufferType::RasterizerOrderedBuffer:
                return RHI::ShaderInputBufferType::Typed;
            case BufferType::AppendStructuredBuffer:
            case BufferType::ConsumeStructuredBuffer:
            case BufferType::RasterizerOrderedStructuredBuffer:
            case BufferType::RwStructuredBuffer:
            case BufferType::StructuredBuffer:
                return RHI::ShaderInputBufferType::Structured;
            case BufferType::RasterizerOrderedByteAddressBuffer:
            case BufferType::ByteAddressBuffer:
            case BufferType::RwByteAddressBuffer:
                return RHI::ShaderInputBufferType::Raw;
            case BufferType::RaytracingAccelerationStructure:
                return RHI::ShaderInputBufferType::AccelerationStructure;
            default:
                AZ_Assert(false, "Unhandled BufferType");
                return RHI::ShaderInputBufferType::Unknown;
            }
        }

        RHI::ShaderInputBufferAccess ToShaderInputBufferAccess(BufferType bufferType)
        {
            switch (bufferType)
            {
            case BufferType::Buffer:
            case BufferType::ByteAddressBuffer:
            case BufferType::ConsumeStructuredBuffer:
            case BufferType::StructuredBuffer:
                return RHI::ShaderInputBufferAccess::Read;
            case BufferType::AppendStructuredBuffer:
            case BufferType::RasterizerOrderedStructuredBuffer:
            case BufferType::RasterizerOrderedByteAddressBuffer:
            case BufferType::RasterizerOrderedBuffer:
            case BufferType::RwByteAddressBuffer:
            case BufferType::RwStructuredBuffer:
            case BufferType::RwBuffer:
                return RHI::ShaderInputBufferAccess::ReadWrite;
            default:
                AZ_Assert(false, "Unhandled BufferType");
                return RHI::ShaderInputBufferAccess::Read;
            }
        }

        RHI::ShaderInputImageType ToShaderInputImageType(TextureType textureType)
        {
            switch (textureType)
            {
            case TextureType::Texture1D:                        return RHI::ShaderInputImageType::Image1D;
            case TextureType::Texture1DArray:                   return RHI::ShaderInputImageType::Image1DArray;
            case TextureType::Texture2D:                        return RHI::ShaderInputImageType::Image2D;
            case TextureType::Texture2DArray:                   return RHI::ShaderInputImageType::Image2DArray;
            case TextureType::Texture2DMS:                      return RHI::ShaderInputImageType::Image2DMultisample;
            case TextureType::Texture2DMSArray:                 return RHI::ShaderInputImageType::Image2DMultisampleArray;
            case TextureType::Texture3D:                        return RHI::ShaderInputImageType::Image3D;
            case TextureType::TextureCube:                      return RHI::ShaderInputImageType::ImageCube;
            case TextureType::RwTexture1D:                      return RHI::ShaderInputImageType::Image1D;
            case TextureType::RwTexture1DArray:                 return RHI::ShaderInputImageType::Image1DArray;
            case TextureType::RwTexture2D:                      return RHI::ShaderInputImageType::Image2D;
            case TextureType::RwTexture2DArray:                 return RHI::ShaderInputImageType::Image2DArray;
            case TextureType::RwTexture3D:                      return RHI::ShaderInputImageType::Image3D;
            case TextureType::RasterizerOrderedTexture1D:       return RHI::ShaderInputImageType::Image1D;
            case TextureType::RasterizerOrderedTexture1DArray:  return RHI::ShaderInputImageType::Image1DArray;
            case TextureType::RasterizerOrderedTexture2D:       return RHI::ShaderInputImageType::Image2D;
            case TextureType::RasterizerOrderedTexture2DArray:  return RHI::ShaderInputImageType::Image2DArray;
            case TextureType::RasterizerOrderedTexture3D:       return RHI::ShaderInputImageType::Image3D;
            case TextureType::SubpassInput:                     return RHI::ShaderInputImageType::SubpassInput;
            default:
                AZ_Assert(false, "Unhandled TextureType");
                return RHI::ShaderInputImageType::Unknown;
            }
        }

        RHI::ShaderInputImageAccess ToShaderInputImageAccess(TextureType textureType)
        {
            switch (textureType)
            {
            case TextureType::Texture1D:
            case TextureType::Texture1DArray:
            case TextureType::Texture2D:
            case TextureType::Texture2DArray:
            case TextureType::Texture2DMS:
            case TextureType::Texture2DMSArray:
            case TextureType::Texture3D:
            case TextureType::TextureCube:
                return RHI::ShaderInputImageAccess::Read;
            case TextureType::RwTexture1D:
            case TextureType::RwTexture1DArray:
            case TextureType::RwTexture2D:
            case TextureType::RwTexture2DArray:
            case TextureType::RwTexture3D:
            case TextureType::RasterizerOrderedTexture1D:
            case TextureType::RasterizerOrderedTexture1DArray:
            case TextureType::RasterizerOrderedTexture2D:
            case TextureType::RasterizerOrderedTexture2DArray:
            case TextureType::RasterizerOrderedTexture3D:
                return RHI::ShaderInputImageAccess::ReadWrite;
            default:
                AZ_Assert(false, "Unhandled TextureType");
                return RHI::ShaderInputImageAccess::Read;
            }
        }

        void SrgLayoutBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            AZStd::string fullPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, false);
            AzFramework::StringFunc::Path::Normalize(fullPath);

            AZ_TracePrintf(SrgLayoutBuilderName, "CreateJobs for Srg Layouts \"%s\"\n", fullPath.data());

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor jobDescriptor;
                jobDescriptor.m_priority = 2;
                // [GFX TODO][ATOM-2830] Set 'm_critical' back to 'false' once proper fix for Atom startup issues are in 
                jobDescriptor.m_critical = true;
                jobDescriptor.m_jobKey = SrgLayoutBuilderJobKey;
                jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());

                // Get the platform interfaces to be able to access the prepend file
                AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(info);
                // queue up AzslBuilder dependencies:
                for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
                {
                    const bool isAzsli = AzFramework::StringFunc::Path::IsExtension(fullPath.c_str(), "azsli");
                    if (isAzsli)
                    {
                        auto skipCheck = ShaderBuilderUtility::ShouldSkipFileForSrgProcessing(SrgLayoutBuilderName, fullPath);
                        if (skipCheck != ShaderBuilderUtility::SrgSkipFileResult::ContinueProcess)
                        {
                            continue;
                        }
                    }

                    AddAzslBuilderJobDependency(jobDescriptor, info.m_identifier, shaderPlatformInterface->GetAPIName().GetCStr(), fullPath);
                }
                response.m_createJobOutputs.push_back(jobDescriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void SrgLayoutBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            AZStd::string sourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), sourcePath, false);
            AzFramework::StringFunc::Path::Normalize(sourcePath);

            if (!AzFramework::StringFunc::Path::IsExtension(sourcePath.c_str(), MergedPartialSrgsExtension))  // not .srgi
            {
                auto skipCheck = ShaderBuilderUtility::ShouldSkipFileForSrgProcessing(SrgLayoutBuilderName, sourcePath);
                if (skipCheck != ShaderBuilderUtility::SrgSkipFileResult::ContinueProcess)
                {
                    response.m_resultCode = skipCheck == ShaderBuilderUtility::SrgSkipFileResult::Error ?
                        AssetBuilderSDK::ProcessJobResult_Failed : AssetBuilderSDK::ProcessJobResult_Success;
                    return;
                }
            }

            AZ_TracePrintf(SrgLayoutBuilderName, "Processing Shader Resource Group \"%s\".\n", sourcePath.c_str());

            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            // Note this will update response.m_resultCode
            CreateSRGAsset(sourcePath, request, response, jobCancelListener);
            
            AZ_TracePrintf(SrgLayoutBuilderName, "Finished processing %s\n", sourcePath.c_str());
        }

        void SrgLayoutBuilder::CreateSRGAsset(
            AZStd::string fullSourcePath,
            const AssetBuilderSDK::ProcessJobRequest& request, 
            AssetBuilderSDK::ProcessJobResponse& response, 
            AssetBuilderSDK::JobCancelListener& jobCancelListener)
        {
            // Request the list of valid shader platform interfaces for the target platform.
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
            ShaderPlatformInterfaceRequestBus::BroadcastResult(platformInterfaces, &ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, request.m_platformInfo);
            if (platformInterfaces.empty())
            {
                AZ_Error(SrgLayoutBuilderName, false, "No ShaderPlatformInterfaces found.");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            using SrgDataEntry = AZStd::pair<RHI::ShaderPlatformInterface*, SrgData>;
            // List with all SRGs that need to be processed.
            AZStd::unordered_map<AZStd::string, AZStd::vector<SrgDataEntry>> srgsToProcess;
            // Emit all SRGs per each of the platform interfaces.
            for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
            {
                if (!shaderPlatformInterface)
                {
                    AZ_Error(SrgLayoutBuilderName, false, "ShaderPlatformInterface for [%s] is not registered, can't compile [%s]", request.m_platformInfo.m_identifier.c_str(), request.m_sourceFile.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                SrgDataContainer srgDataContainer;

                auto azslArtifactsOutcome = ShaderBuilderUtility::ObtainBuildArtifactsFromAzslBuilder(SrgLayoutBuilderName, fullSourcePath, shaderPlatformInterface->GetAPIType(), request.m_platformInfo.m_identifier);
                if (!azslArtifactsOutcome.IsSuccess())
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                // create an AzslCompiler instance to use it's json parsing facilities, but we won't call any emit facility. So the compiler actually never runs.
                AzslCompiler azslc(azslArtifactsOutcome.GetValue()[ShaderBuilderUtility::AzslSubProducts::azslin]);  // set the input file for eventual error messages.

                AZ::Outcome<rapidjson::Document, AZStd::string> outcome;
                outcome = JsonSerializationUtils::ReadJsonFile(azslArtifactsOutcome.GetValue()[ShaderBuilderUtility::AzslSubProducts::srg]);
                if (!outcome.IsSuccess())
                {
                    AZ_Error(SrgLayoutBuilderName, false, "%s", outcome.GetError().c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                if (!azslc.ParseSrgPopulateSrgData(outcome.GetValue(), srgDataContainer))
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                for (const SrgData& srgData : srgDataContainer)
                {
                    // Ignore the SRGs included from other files.
                    AZStd::string normalizedContainer = srgData.m_containingFileName;
                    AzFramework::StringFunc::Path::Normalize(normalizedContainer);
                    if (normalizedContainer != fullSourcePath)
                    {
                        AZ_TracePrintf(SrgLayoutBuilderName, "SRG [%s] found in [%s] but is foreign to [%s]. skipped.",
                            srgData.m_name.c_str(), normalizedContainer.c_str(), fullSourcePath.c_str());
                        continue;
                    }
                    AZ_TracePrintf(SrgLayoutBuilderName, "SRG [%s] found in [%s] (native to this file). added.",
                        srgData.m_name.c_str(), normalizedContainer.c_str());
                    srgsToProcess[srgData.m_name].push_back(SrgDataEntry(shaderPlatformInterface, AZStd::move(srgData)));
                }
            }
            
            AZStd::string fileNameOnly;
            AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), fileNameOnly);

            if (srgsToProcess.empty())
            {
                AZ_TracePrintf(SrgLayoutBuilderName, "No ShaderResourceGroups found in '%s'.", fullSourcePath.c_str());
            }

            // Process all SRGs that were emitted.
            for (const auto& entry : srgsToProcess)
            {
                const auto& srgName = entry.first;
                if (jobCancelListener.IsCancelled())
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                AZStd::string fullFileName = fileNameOnly + "_" + srgName;
                AZStd::string shaderResourceGroupAssetPath;
                AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fullFileName.c_str(), shaderResourceGroupAssetPath, true);
                AzFramework::StringFunc::Path::ReplaceExtension(shaderResourceGroupAssetPath, "azsrg");

                RPI::ShaderResourceGroupAssetCreator srgAssetCreator;
                srgAssetCreator.Begin(Uuid::CreateRandom(), Name{ srgName });

                bool success = true;
                // Process the SRG per each shader platform interface.
                for(const SrgDataEntry& srgDataEntry : entry.second)
                {
                    RHI::ShaderPlatformInterface* shaderPlatformInterface = srgDataEntry.first;

                    // The register number only makes sense if the platform uses "spaces",
                    // since the register Id of the resource will not change even if the pipeline layout changes.
                    // We can pass in a default ShaderCompilerArguments because all we care about is whether the shaderPlatformInterface
                    // appends the
                    // "--use-spaces" flag.
                    AZStd::string azslCompilerParameters =
                        shaderPlatformInterface->GetAzslCompilerParameters(RHI::ShaderCompilerArguments{});
                    bool useRegisterId = (AzFramework::StringFunc::Find(azslCompilerParameters, "--use-spaces") != AZStd::string::npos);

                    const SrgData& srgData = srgDataEntry.second;

                    srgAssetCreator.BeginAPI(shaderPlatformInterface->GetAPIType());
                    srgAssetCreator.SetBindingSlot(srgData.m_bindingSlot.m_index);

                    // Samplers
                    for (const SamplerSrgData& samplerData : srgData.m_samplers)
                    {
                        if (samplerData.m_isDynamic)
                        {
                            srgAssetCreator.AddShaderInput({
                                samplerData.m_nameId,
                                samplerData.m_count,
                                useRegisterId ? samplerData.m_registerId : RHI::UndefinedRegisterSlot });
                        }
                        else
                        {
                            srgAssetCreator.AddStaticSampler({
                                samplerData.m_nameId,
                                samplerData.m_descriptor,
                                useRegisterId ? samplerData.m_registerId : RHI::UndefinedRegisterSlot });
                        }
                    }    

                    // Images
                    for (const TextureSrgData& textureData : srgData.m_textures)
                    {
                        const RHI::ShaderInputImageAccess imageAccess =
                            textureData.m_isReadOnlyType ?
                            RHI::ShaderInputImageAccess::Read :
                            RHI::ShaderInputImageAccess::ReadWrite;

                        const RHI::ShaderInputImageType imageType = ToShaderInputImageType(textureData.m_type);

                        if (imageType != RHI::ShaderInputImageType::Unknown)
                        {
                            if (textureData.m_count != aznumeric_cast<uint32_t>(-1))
                            {
                                srgAssetCreator.AddShaderInput({
                                    textureData.m_nameId,
                                    imageAccess,
                                    imageType,
                                    textureData.m_count,
                                    useRegisterId ? textureData.m_registerId : RHI::UndefinedRegisterSlot });
                            }
                            else
                            {
                                // unbounded array
                                srgAssetCreator.AddShaderInput({
                                    textureData.m_nameId,
                                    imageAccess,
                                    imageType,
                                    useRegisterId ? textureData.m_registerId : RHI::UndefinedRegisterSlot });
                            }
                        }
                        else
                        {
                            AZ_Error(SrgLayoutBuilderName, false, "Failed to build Shader Resource Group Asset: Image %s has an unknown type.", textureData.m_nameId.GetCStr());
                            success = false;
                        }
                    }

                    // Buffers
                    {
                        for (const ConstantBufferData& cbData : srgData.m_constantBuffers)
                        {
                            srgAssetCreator.AddShaderInput({
                                cbData.m_nameId,
                                RHI::ShaderInputBufferAccess::Constant,
                                RHI::ShaderInputBufferType::Constant,
                                cbData.m_count,
                                cbData.m_strideSize,
                                useRegisterId ? cbData.m_registerId : RHI::UndefinedRegisterSlot });
                        }

                        for (const BufferSrgData& bufferData : srgData.m_buffers)
                        {
                            const RHI::ShaderInputBufferAccess bufferAccess =
                                bufferData.m_isReadOnlyType ?
                                RHI::ShaderInputBufferAccess::Read :
                                RHI::ShaderInputBufferAccess::ReadWrite;

                            const RHI::ShaderInputBufferType bufferType = ToShaderInputBufferType(bufferData.m_type);

                            if (bufferType != RHI::ShaderInputBufferType::Unknown)
                            {
                                if (bufferData.m_count != aznumeric_cast<uint32_t>(-1))
                                {
                                    srgAssetCreator.AddShaderInput({
                                        bufferData.m_nameId,
                                        bufferAccess,
                                        bufferType,
                                        bufferData.m_count,
                                        bufferData.m_strideSize,
                                        useRegisterId ? bufferData.m_registerId : RHI::UndefinedRegisterSlot });
                                }
                                else
                                {
                                    // unbounded array
                                    srgAssetCreator.AddShaderInput({
                                        bufferData.m_nameId,
                                        bufferAccess,
                                        bufferType,
                                        bufferData.m_strideSize,
                                        useRegisterId ? bufferData.m_registerId : RHI::UndefinedRegisterSlot });
                                }
                            }
                            else
                            {
                                AZ_Error(SrgLayoutBuilderName, false, "Failed to build Shader Resource Group Asset: Buffer %s has un unknown type.", bufferData.m_nameId.GetCStr());
                                success = false;
                            }
                        }
                    }

                    // SRG Constants
                    uint32_t constantDataRegisterId = useRegisterId ? srgData.m_srgConstantDataRegisterId : RHI::UndefinedRegisterSlot;
                    for (const SrgConstantData& srgConstants : srgData.m_srgConstantData)
                    {
                        srgAssetCreator.AddShaderInput({
                            srgConstants.m_nameId,
                            srgConstants.m_constantByteOffset,
                            srgConstants.m_constantByteSize,
                            constantDataRegisterId });
                    }

                    // Shader Variant Key fallback
                    if (srgData.m_fallbackSize > 0)
                    {
                        // Designates this SRG as a ShaderVariantKey fallback
                        srgAssetCreator.SetShaderVariantKeyFallback(srgData.m_fallbackName, srgData.m_fallbackSize);
                    }

                    if (!srgAssetCreator.EndAPI())
                    {
                        AZ_Error(SrgLayoutBuilderName, false, "Failed to End API.");
                        success = false;
                    }

                    if (!success)
                    {
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                        return;
                    }
                }

                Data::Asset<RPI::ShaderResourceGroupAsset> shaderResourceGroupAsset;
                if (!srgAssetCreator.End(shaderResourceGroupAsset))
                {
                    AZ_Error(SrgLayoutBuilderName, false, "Failed to build Shader Resource Group Asset");
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                if (AZ::IO::FileIOBase::GetInstance()->Exists(shaderResourceGroupAssetPath.c_str()))
                {
                    // This would indicate a problem above, making sure each product SRG asset file path is unique.
                    AZ_Error(SrgLayoutBuilderName, false, "Cannot overwrite existing file [%s]. This likely indicates conflicting SRG names.", shaderResourceGroupAssetPath.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                success = AZ::Utils::SaveObjectToFile(shaderResourceGroupAssetPath, AZ::DataStream::ST_JSON, shaderResourceGroupAsset.Get());
                if (!success)
                {
                    AZ_Error(SrgLayoutBuilderName, false, "Failed to save Shader Resource Group Asset to \"%s\"", shaderResourceGroupAssetPath.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                AssetBuilderSDK::JobProduct srgAssetProduct;
                srgAssetProduct.m_productSubID = static_cast<uint32_t>(AZStd::hash<AZStd::string>()(srgName) & 0xFFFFFFFF);
                srgAssetProduct.m_productFileName = shaderResourceGroupAssetPath;
                srgAssetProduct.m_productAssetType = azrtti_typeid<RPI::ShaderResourceGroupAsset>();
                srgAssetProduct.m_dependenciesHandled = true; // This builder has no dependencies to output
                response.m_outputProducts.push_back(srgAssetProduct);

                AZ_TracePrintf(SrgLayoutBuilderName, "Shader Resource Group Asset compiled successfully.\n");
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
    } // namespace ShaderBuilder
} // namespace AZ
