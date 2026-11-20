/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientGlobalIlluminationCubemapPass.h"

#include <Atom/Feature/GradientGlobalIllumination/GradientGlobalIlluminationLogic.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>

namespace AZ::Render
{
    bool GradientGlobalIlluminationCubemapPass::IsGpuComputeSupported()
    {
        auto* imageSystem = RPI::ImageSystemInterface::Get();
        if (!imageSystem)
        {
            return false;
        }

        auto* attachmentPool = imageSystem->GetSystemAttachmentPool().get();
        if (!attachmentPool)
        {
            return false;
        }

        RHI::ImageDescriptor testDesc = RHI::ImageDescriptor::CreateCubemap(RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite, 4, RHI::Format::R16G16B16A16_FLOAT);

        auto testImage = RPI::AttachmentImage::Create(*attachmentPool, testDesc, AZ_NAME_LITERAL("GradientGlobalIllumination_CapProbe"), nullptr, nullptr);

        return testImage != nullptr;
    }

    GradientGlobalIlluminationCubemapPass::GradientGlobalIlluminationCubemapPass(const RPI::PassDescriptor& descriptor)
        : RPI::RenderPass(descriptor)
    {
    }

    void GradientGlobalIlluminationCubemapPass::SetGradientColors(const Color& low, const Color& mid, const Color& high, uint32_t faceSize)
    {
        const bool faceSizeChanged = (faceSize != m_faceSize);

        m_lowColor = low;
        m_midColor = mid;
        m_highColor = high;
        m_faceSize = faceSize;
        m_dirty = true;

        // Rebuild images when the dispatch dimensions change. Initial allocation is handled when the pass is added.
        if (faceSizeChanged && m_diffuseImage)
        {
            QueueForBuildAndInitialization();
        }
    }

    Data::Instance<RPI::AttachmentImage> GradientGlobalIlluminationCubemapPass::GetDiffuseImage() const
    {
        return m_diffuseImage;
    }

    Data::Instance<RPI::AttachmentImage> GradientGlobalIlluminationCubemapPass::GetSpecularImage() const
    {
        return m_specularImage;
    }

    void GradientGlobalIlluminationCubemapPass::SetDetailLayer(const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength)
    {
        m_detailTexture = texture;
        m_detailMapping = mapping;
        m_detailBlend = blend;
        m_detailStrength = strength;
        m_dirty = true;
    }

    void GradientGlobalIlluminationCubemapPass::SetSpecularLayer(const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength)
    {
        m_specularTexture = texture;
        m_specularMapping = mapping;
        m_specularBlend = blend;
        m_specularStrength = strength;
        m_dirty = true;
    }

    void GradientGlobalIlluminationCubemapPass::BuildInternal()
    {
        if (m_buildFailed)
        {
            return;
        }

        const char* shaderPath = "Shaders/GradientGlobalIllumination/GradientGlobalIlluminationCubemap.azshader";
        auto shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(shaderPath);
        if (!shaderAsset.IsReady())
        {
            AZ_Error("GradientGlobalIlluminationCubemapPass", false, "Failed to load shader: %s", shaderPath);
            m_buildFailed = true;
            SetEnabled(false);
            return;
        }

        m_shader = RPI::Shader::FindOrCreate(shaderAsset);
        if (!m_shader)
        {
            AZ_Error("GradientGlobalIlluminationCubemapPass", false, "Failed to create Shader instance from: %s", shaderPath);
            m_buildFailed = true;
            SetEnabled(false);
            return;
        }

        auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
        RHI::PipelineStateDescriptorForDispatch pipelineDesc;
        shaderVariant.ConfigurePipelineState(pipelineDesc);
        m_pipelineState = m_shader->AcquirePipelineState(pipelineDesc);
        if (!m_pipelineState)
        {
            AZ_Error("GradientGlobalIlluminationCubemapPass", false, "Failed to acquire pipeline state for GradientGlobalIlluminationCubemap.");
            m_buildFailed = true;
            SetEnabled(false);
            return;
        }

        auto srgLayout = m_shader->FindShaderResourceGroupLayout(AZ_NAME_LITERAL("PassSrg"));
        if (srgLayout)
        {
            m_passSrg = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), srgLayout->GetName());

            if (!m_passSrg)
            {
                AZ_Error("GradientGlobalIlluminationCubemapPass", false, "Failed to create PassSrg for GradientGlobalIlluminationCubemap.");
                m_buildFailed = true;
                SetEnabled(false);
                return;
            }
        }
        else
        {
            AZ_Error("GradientGlobalIlluminationCubemapPass", false, "Could not find PassSrg layout in GradientGlobalIlluminationCubemap shader.");
            m_buildFailed = true;
            SetEnabled(false);
            return;
        }

        // Reuse adopted images only if both still match the requested face size.
        if (!OutputImagesMatchFaceSize())
        {
            m_diffuseImage = CreateOutputCubemap(AZ_NAME_LITERAL("GradientGlobalIllumination_Diffuse"));
            m_specularImage = CreateOutputCubemap(AZ_NAME_LITERAL("GradientGlobalIllumination_Specular"));
            m_outputsWritten = false;
        }

        if (!m_diffuseImage || !m_specularImage)
        {
            AZ_Error(
                "GradientGlobalIlluminationCubemapPass",
                false,
                "Failed to create AttachmentImage(s) for gradient cubemap. "
                "GPU compute (UAV cubemap) is not supported on this platform.");
            m_buildFailed = true;
            SetEnabled(false);
            return;
        }

        m_sampleAccessScope = AZStd::make_shared<RHI::ScopeProducerFunctionNoData>(
            RHI::ScopeId(AZStd::string::format("%s.SampleAccess", GetPathName().GetCStr())),
            [this](RHI::FrameGraphInterface frameGraph)
            {
                DeclareSampleAccess(frameGraph);
            },
            [](const RHI::FrameGraphCompileContext&)
            {
            },
            [](const RHI::FrameGraphExecuteContext&)
            {
            },
            RHI::HardwareQueueClass::Graphics,
            RPI::Pass::GetDeviceIndex());

        m_dirty = true;
    }

    void GradientGlobalIlluminationCubemapPass::AdoptOutputImages(
        const Data::Instance<RPI::AttachmentImage>& diffuse,
        const Data::Instance<RPI::AttachmentImage>& specular)
    {
        m_diffuseImage = diffuse;
        m_specularImage = specular;

        // The caller supplies initialized output. BuildInternal invalidates it if resizing is needed.
        m_outputsWritten = m_diffuseImage && m_specularImage;
    }

    bool GradientGlobalIlluminationCubemapPass::HasWrittenOutputs() const
    {
        return m_outputsWritten;
    }

    bool GradientGlobalIlluminationCubemapPass::OutputImagesMatchFaceSize() const
    {
        if (!m_diffuseImage || !m_specularImage)
        {
            return false;
        }

        const RHI::ImageDescriptor& descriptor = m_diffuseImage->GetRHIImage()->GetDescriptor();
        return descriptor.m_size.m_width == m_faceSize;
    }

    Data::Instance<RPI::AttachmentImage> GradientGlobalIlluminationCubemapPass::CreateOutputCubemap(const AZ::Name& debugName) const
    {
        auto* imageSystem = RPI::ImageSystemInterface::Get();
        if (!imageSystem)
        {
            return nullptr;
        }
        auto* attachmentPool = imageSystem->GetSystemAttachmentPool().get();
        if (!attachmentPool)
        {
            AZ_Error("GradientGlobalIlluminationCubemapPass", false, "System attachment pool not available.");
            return nullptr;
        }

        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite,
            m_faceSize,
            RHI::Format::R16G16B16A16_FLOAT);

        auto cubemapViewDesc = RHI::ImageViewDescriptor::CreateCubemap();

        return RPI::AttachmentImage::Create(*attachmentPool, imageDesc, debugName, nullptr, &cubemapViewDesc);
    }

    Data::Instance<RPI::Image> GradientGlobalIlluminationCubemapPass::GetOrCreateWhiteFallbackCube()
    {
        if (m_whiteFallbackCube)
        {
            return m_whiteFallbackCube;
        }

        // No system cubemap fallback exists, and the unused cube SRG slot still needs a valid view.
        const RHI::Format format = RHI::Format::R8G8B8A8_UNORM;
        const RHI::Size faceDimensions(1, 1, 1);
        const RHI::DeviceImageSubresourceLayout faceLayout = RHI::GetImageSubresourceLayout(faceDimensions, format);
        const size_t bytesPerFace = faceLayout.m_bytesPerImage;

        AZStd::vector<uint8_t> whiteFace(bytesPerFace, 0xFF);

        Data::Asset<RPI::ImageMipChainAsset> mipChainAsset;
        {
            RPI::ImageMipChainAssetCreator mipCreator;
            mipCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), 1, 6);
            mipCreator.BeginMip(faceLayout);
            for (uint32_t face = 0; face < 6; ++face)
            {
                mipCreator.AddSubImage(whiteFace.data(), bytesPerFace);
            }
            mipCreator.EndMip();
            if (!mipCreator.End(mipChainAsset))
            {
                return nullptr;
            }
        }

        auto* imageSystem = RPI::ImageSystemInterface::Get();
        if (!imageSystem)
        {
            return nullptr;
        }

        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(RHI::ImageBindFlags::ShaderRead, 1, format);

        Data::Asset<RPI::StreamingImageAsset> imageAsset;
        {
            RPI::StreamingImageAssetCreator assetCreator;
            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            assetCreator.SetImageDescriptor(imageDesc);
            assetCreator.SetImageViewDescriptor(RHI::ImageViewDescriptor::CreateCubemap());
            assetCreator.SetFlags(RPI::StreamingImageFlags::NotStreamable);
            assetCreator.SetPoolAssetId(imageSystem->GetSystemStreamingPool()->GetAssetId());
            assetCreator.AddMipChainAsset(*mipChainAsset.Get());
            if (!assetCreator.End(imageAsset))
            {
                return nullptr;
            }
        }

        m_whiteFallbackCube = RPI::StreamingImage::FindOrCreate(imageAsset);
        return m_whiteFallbackCube;
    }

    void GradientGlobalIlluminationCubemapPass::BindTextureLayer(
        const AZ::Name& tex2DName,
        const AZ::Name& texCubeName,
        const AZ::Name& mappingName,
        const AZ::Name& blendName,
        const AZ::Name& strengthName,
        const AZ::Name& enabledName,
        const Data::Instance<RPI::Image>& texture,
        uint8_t mapping,
        uint8_t blend,
        float strength)
    {
        const auto tex2DIdx = m_passSrg->FindShaderInputImageIndex(tex2DName);
        const auto texCubeIdx = m_passSrg->FindShaderInputImageIndex(texCubeName);
        const auto mapIdx = m_passSrg->FindShaderInputConstantIndex(mappingName);
        const auto blendIdx = m_passSrg->FindShaderInputConstantIndex(blendName);
        const auto strIdx = m_passSrg->FindShaderInputConstantIndex(strengthName);
        const auto enIdx = m_passSrg->FindShaderInputConstantIndex(enabledName);

        const bool cubeRequested = mapping == 2;
        bool enabled = false;
        Data::Instance<RPI::Image> tex2D = RPI::ImageSystemInterface::Get()->GetSystemImage(RPI::SystemImage::White);
        Data::Instance<RPI::Image> texCube = GetOrCreateWhiteFallbackCube();
        if (texture && texture->GetImageView())
        {
            const auto& descriptor = texture->GetRHIImage()->GetDescriptor();
            const auto& view = texture->GetImageView()->GetDescriptor();
            if (GradientGlobalIllumination::IsTextureLayerEnabled(descriptor, view, cubeRequested))
            {
                enabled = true;
                if (cubeRequested)
                {
                    texCube = texture;
                }
                else
                {
                    tex2D = texture;
                }
            }
        }

        const bool bound2D = m_passSrg->SetImage(tex2DIdx, tex2D);
        const bool boundCube = texCube && m_passSrg->SetImage(texCubeIdx, texCube);
        enabled = enabled && ((cubeRequested && boundCube) || (!cubeRequested && bound2D));
        m_passSrg->SetConstant(mapIdx, static_cast<uint32_t>(mapping));
        m_passSrg->SetConstant(blendIdx, static_cast<uint32_t>(blend));
        m_passSrg->SetConstant(strIdx, strength);
        m_passSrg->SetConstant(enIdx, static_cast<uint32_t>(enabled));
    }

    void GradientGlobalIlluminationCubemapPass::FrameBeginInternal(FramePrepareParams params)
    {
        // Freeze the write decision before declaring resources or recording commands.
        const bool canWrite = m_pipelineState && m_passSrg && m_diffuseImage && m_specularImage && m_sampleAccessScope;
        m_writeThisFrame = m_dirty && canWrite;

        RPI::RenderPass::FrameBeginInternal(params);

        if (m_writeThisFrame)
        {
            params.m_frameGraphBuilder->ImportScopeProducer(*m_sampleAccessScope);

            m_dirty = false;
            m_outputsWritten = true;
        }
    }

    void GradientGlobalIlluminationCubemapPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);

        // Retain the shader-read state from the preceding update when no write is needed.
        if (!m_writeThisFrame)
        {
            return;
        }

        // Compute addresses the six cube faces through a writable 2D-array view.
        auto declareUav = [&frameGraph](const Data::Instance<RPI::AttachmentImage>& image)
        {
            frameGraph.GetAttachmentDatabase().ImportImage(image->GetAttachmentId(), image->GetRHIImage());

            RHI::ImageScopeAttachmentDescriptor uavDesc;
            uavDesc.m_attachmentId = image->GetAttachmentId();
            uavDesc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 0, 0, 5);
            uavDesc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
            uavDesc.m_loadStoreAction.m_storeAction = RHI::AttachmentStoreAction::Store;

            frameGraph.UseShaderAttachment(uavDesc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
        };

        declareUav(m_diffuseImage);
        declareUav(m_specularImage);
    }

    void GradientGlobalIlluminationCubemapPass::DeclareSampleAccess(RHI::FrameGraphInterface frameGraph)
    {
        // Scene SRG bindings declare no frame-graph reads. This scope orders sampling after the UAV writes.
        auto declareRead = [&frameGraph](const Data::Instance<RPI::AttachmentImage>& image)
        {
            RHI::ImageScopeAttachmentDescriptor readDesc;
            readDesc.m_attachmentId = image->GetAttachmentId();
            readDesc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::Unknown, 0, 0, 0, 5);
            readDesc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::Load;

            frameGraph.UseShaderAttachment(readDesc, RHI::ScopeAttachmentAccess::Read, RHI::ScopeAttachmentStage::AnyGraphics);
        };

        declareRead(m_diffuseImage);
        declareRead(m_specularImage);
    }

    void GradientGlobalIlluminationCubemapPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        if (!m_passSrg)
        {
            return;
        }

        if (m_writeThisFrame)
        {
            const auto lowIdx = m_passSrg->FindShaderInputConstantIndex(AZ_NAME_LITERAL("m_lowColor"));
            const auto midIdx = m_passSrg->FindShaderInputConstantIndex(AZ_NAME_LITERAL("m_midColor"));
            const auto highIdx = m_passSrg->FindShaderInputConstantIndex(AZ_NAME_LITERAL("m_highColor"));
            const auto sizeIdx = m_passSrg->FindShaderInputConstantIndex(AZ_NAME_LITERAL("m_faceSize"));
            const auto diffIdx = m_passSrg->FindShaderInputImageIndex(AZ_NAME_LITERAL("m_outputDiffuse"));
            const auto specIdx = m_passSrg->FindShaderInputImageIndex(AZ_NAME_LITERAL("m_outputSpecular"));

            m_passSrg->SetConstant(lowIdx, AZ::Vector3(m_lowColor.GetR(), m_lowColor.GetG(), m_lowColor.GetB()));
            m_passSrg->SetConstant(midIdx, AZ::Vector3(m_midColor.GetR(), m_midColor.GetG(), m_midColor.GetB()));
            m_passSrg->SetConstant(highIdx, AZ::Vector3(m_highColor.GetR(), m_highColor.GetG(), m_highColor.GetB()));
            m_passSrg->SetConstant(sizeIdx, m_faceSize);

            if (m_diffuseImage)
            {
                if (const RHI::ImageView* uavView = context.GetImageView(m_diffuseImage->GetAttachmentId()))
                {
                    m_passSrg->SetImageView(diffIdx, uavView);
                }
            }
            if (m_specularImage)
            {
                if (const RHI::ImageView* uavView = context.GetImageView(m_specularImage->GetAttachmentId()))
                {
                    m_passSrg->SetImageView(specIdx, uavView);
                }
            }

            BindTextureLayer(
                AZ_NAME_LITERAL("m_detailTex2D"),
                AZ_NAME_LITERAL("m_detailTexCube"),
                AZ_NAME_LITERAL("m_detailMapping"),
                AZ_NAME_LITERAL("m_detailBlend"),
                AZ_NAME_LITERAL("m_detailStrength"),
                AZ_NAME_LITERAL("m_detailEnabled"),
                m_detailTexture,
                m_detailMapping,
                m_detailBlend,
                m_detailStrength);

            BindTextureLayer(
                AZ_NAME_LITERAL("m_specularTex2D"),
                AZ_NAME_LITERAL("m_specularTexCube"),
                AZ_NAME_LITERAL("m_specularMapping"),
                AZ_NAME_LITERAL("m_specularBlend"),
                AZ_NAME_LITERAL("m_specularStrength"),
                AZ_NAME_LITERAL("m_specularEnabled"),
                m_specularTexture,
                m_specularMapping,
                m_specularBlend,
                m_specularStrength);
            m_passSrg->Compile();
        }
    }

    void GradientGlobalIlluminationCubemapPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        if (!m_writeThisFrame || !m_pipelineState || !m_passSrg)
        {
            return;
        }

        const uint32_t groupsX = (m_faceSize + 7) / 8;
        const uint32_t groupsY = (m_faceSize + 7) / 8;
        const uint32_t groupsZ = 6;

        RHI::DispatchDirect directArgs;
        directArgs.m_totalNumberOfThreadsX = groupsX * 8;
        directArgs.m_totalNumberOfThreadsY = groupsY * 8;
        directArgs.m_totalNumberOfThreadsZ = groupsZ;
        directArgs.m_threadsPerGroupX = 8;
        directArgs.m_threadsPerGroupY = 8;
        directArgs.m_threadsPerGroupZ = 1;

        RHI::DispatchItem dispatchItem(RHI::MultiDevice::AllDevices);
        dispatchItem.SetArguments(RHI::DispatchArguments(directArgs));
        dispatchItem.SetPipelineState(m_pipelineState);

        const RHI::ShaderResourceGroup* srgRhi = m_passSrg->GetRHIShaderResourceGroup();
        dispatchItem.SetShaderResourceGroups(AZStd::span<const RHI::ShaderResourceGroup*>(&srgRhi, 1));

        context.GetCommandList()->Submit(dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));

        // Avoid a second dispatch if the scope is split across command lists.
        m_writeThisFrame = false;
    }
} // namespace AZ::Render
