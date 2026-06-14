/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientGICubemapPass.h"

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphExecuteContext.h>
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

namespace AZ::Render
{
    // =========================================================================
    // Platform Support
    // =========================================================================

    /*static*/ bool GradientGICubemapPass::IsGpuComputeSupported()
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

        RHI::ImageDescriptor testDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite,
            /*width=*/4,
            RHI::Format::R16G16B16A16_FLOAT);

        auto testImage = RPI::AttachmentImage::Create(
            *attachmentPool, testDesc, AZ::Name("GradientGI_CapProbe"), nullptr, nullptr);

        return testImage != nullptr;
    }

    // =========================================================================
    // Lifecycle
    // =========================================================================

    GradientGICubemapPass::GradientGICubemapPass(const RPI::PassDescriptor& descriptor)
        : RPI::RenderPass(descriptor)
    {
    }

    // =========================================================================
    // Public API
    // =========================================================================

    void GradientGICubemapPass::SetGradientColors(
        const Color& low, const Color& mid, const Color& high,
        float exposure, uint32_t faceSize)
    {
        const bool faceSizeChanged = (faceSize != m_faceSize);

        m_lowColor  = low;
        m_midColor  = mid;
        m_highColor = high;
        m_exposure  = exposure;
        m_faceSize  = faceSize;
        m_dirty     = true;

        // The output cubemap is allocated at m_faceSize in BuildInternal(). A resolution
        // change therefore requires reallocating it -- otherwise the compute dispatch grid
        // and the image dimensions disagree: down-scaling under-writes each face (stale
        // texels -> discolouration), up-scaling overruns the image bounds (cube-pattern
        // aliasing). Queue a rebuild so BuildInternal() recreates the AttachmentImage at the
        // new size. Guarded on m_cubemapImage so the initial pre-build call is a no-op (the
        // pass system builds it once on add).
        if (faceSizeChanged && m_diffuseImage)
        {
            QueueForBuildAndInitialization();
        }
    }

    Data::Instance<RPI::AttachmentImage> GradientGICubemapPass::GetDiffuseImage() const
    {
        return m_diffuseImage;
    }

    Data::Instance<RPI::AttachmentImage> GradientGICubemapPass::GetSpecularImage() const
    {
        return m_specularImage;
    }

    void GradientGICubemapPass::SetDetailLayer(
        const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength)
    {
        // No reallocation here -- only SRG state changes. The resident texture and constants are
        // (re)bound in CompileResources on the next dirty pass, so this stays cheap.
        m_detailTexture  = texture;
        m_detailMapping  = mapping;
        m_detailBlend    = blend;
        m_detailStrength = strength;
        m_dirty          = true;
    }

    void GradientGICubemapPass::SetSpecularLayer(
        const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength)
    {
        m_specularTexture  = texture;
        m_specularMapping  = mapping;
        m_specularBlend    = blend;
        m_specularStrength = strength;
        m_dirty            = true;
    }

    // =========================================================================
    // BuildInternal -- load shader, create pipeline state, SRG, and output image
    // =========================================================================

    void GradientGICubemapPass::BuildInternal()
    {
        // ---- Load shader asset ----
        const char* shaderPath = "Shaders/GradientGI/GradientGICubemap.azshader";
        auto shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(shaderPath);
        if (!shaderAsset.IsReady())
        {
            AZ_Error("GradientGICubemapPass", false, "Failed to load shader: %s", shaderPath);
            SetEnabled(false);
            return;
        }

        m_shader = RPI::Shader::FindOrCreate(shaderAsset);
        if (!m_shader)
        {
            AZ_Error("GradientGICubemapPass", false, "Failed to create Shader instance from: %s", shaderPath);
            SetEnabled(false);
            return;
        }

        // ---- Build compute pipeline state ----
        auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
        RHI::PipelineStateDescriptorForDispatch pipelineDesc;
        shaderVariant.ConfigurePipelineState(pipelineDesc);
        m_pipelineState = m_shader->AcquirePipelineState(pipelineDesc);
        if (!m_pipelineState)
        {
            AZ_Error("GradientGICubemapPass", false, "Failed to acquire pipeline state for GradientGICubemap.");
            SetEnabled(false);
            return;
        }

        // ---- Create pass SRG (SRG_PerPass = "PassSrg") ----
        auto srgLayout = m_shader->FindShaderResourceGroupLayout(AZ::Name("PassSrg"));
        if (srgLayout)
        {
            m_passSrg = RPI::ShaderResourceGroup::Create(
                m_shader->GetAsset(),
                m_shader->GetSupervariantIndex(),
                srgLayout->GetName());

            AZ_Error("GradientGICubemapPass", m_passSrg, "Failed to create PassSrg for GradientGICubemap.");
        }
        else
        {
            AZ_Error("GradientGICubemapPass", false, "Could not find PassSrg layout in GradientGICubemap shader.");
            SetEnabled(false);
            return;
        }

        // ---- Create persistent output cubemap AttachmentImages (diffuse + specular) ----
        m_diffuseImage  = CreateOutputCubemap("GradientGI_Diffuse");
        m_specularImage = CreateOutputCubemap("GradientGI_Specular");

        if (!m_diffuseImage || !m_specularImage)
        {
            AZ_Error("GradientGICubemapPass", false,
                "Failed to create AttachmentImage(s) for gradient cubemap. "
                "GPU compute (UAV cubemap) is not supported on this platform.");
            SetEnabled(false);
            return;
        }

        m_dirty = true;
    }

    Data::Instance<RPI::AttachmentImage> GradientGICubemapPass::CreateOutputCubemap(const char* debugName) const
    {
        // Bind flags: ShaderRead (SRV for IBL sampling) + ShaderWrite (UAV for compute output).
        // Default view is a cubemap SRV, used when binding to the scene SRG's IBL slots.
        auto* imageSystem = RPI::ImageSystemInterface::Get();
        auto* attachmentPool = imageSystem ? imageSystem->GetSystemAttachmentPool().get() : nullptr;
        if (!attachmentPool)
        {
            AZ_Error("GradientGICubemapPass", false, "System attachment pool not available.");
            return nullptr;
        }

        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite,
            m_faceSize,
            RHI::Format::R16G16B16A16_FLOAT);

        // Cubemap SRV as default view -- used when the FP binds this image to scene SRG slots.
        auto cubemapViewDesc = RHI::ImageViewDescriptor::CreateCubemap();

        return RPI::AttachmentImage::Create(
            *attachmentPool, imageDesc,
            AZ::Name(debugName),
            nullptr,
            &cubemapViewDesc);
    }

    Data::Instance<RPI::Image> GradientGICubemapPass::GetOrCreateWhiteFallbackCube()
    {
        if (m_whiteFallbackCube)
        {
            return m_whiteFallbackCube;
        }

        // Build a 1x1, 6-face opaque-white cubemap. Bound to the cube detail slot whenever it
        // is unused (there is no engine-default cubemap, and SRG slots must stay bound).
        const RHI::Format format = RHI::Format::R8G8B8A8_UNORM;
        const RHI::Size faceDimensions(1, 1, 1);
        const RHI::DeviceImageSubresourceLayout faceLayout = RHI::GetImageSubresourceLayout(faceDimensions, format);
        const size_t bytesPerFace = faceLayout.m_bytesPerImage;

        AZStd::vector<uint8_t> whiteFace(bytesPerFace, 0xFF);

        Data::Asset<RPI::ImageMipChainAsset> mipChainAsset;
        {
            RPI::ImageMipChainAssetCreator mipCreator;
            mipCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), /*mipLevels=*/1, /*arraySize=*/6);
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

    void GradientGICubemapPass::BindTextureLayer(
        const AZ::Name& tex2DName, const AZ::Name& texCubeName,
        const AZ::Name& mappingName, const AZ::Name& blendName,
        const AZ::Name& strengthName, const AZ::Name& enabledName,
        const Data::Instance<RPI::Image>& texture, uint8_t mapping, uint8_t blend, float strength)
    {
        const auto tex2DIdx   = m_passSrg->FindShaderInputImageIndex(tex2DName);
        const auto texCubeIdx = m_passSrg->FindShaderInputImageIndex(texCubeName);
        const auto mapIdx     = m_passSrg->FindShaderInputConstantIndex(mappingName);
        const auto blendIdx   = m_passSrg->FindShaderInputConstantIndex(blendName);
        const auto strIdx     = m_passSrg->FindShaderInputConstantIndex(strengthName);
        const auto enIdx      = m_passSrg->FindShaderInputConstantIndex(enabledName);

        // Detect whether the assigned texture is actually a cubemap.
        bool isCube = false;
        if (texture && texture->GetRHIImage())
        {
            const RHI::ImageDescriptor& d = texture->GetRHIImage()->GetDescriptor();
            isCube = d.m_isCubemap || d.m_arraySize == 6;
        }

        const bool cubeRequested = (mapping == 2); // GradientGITextureMapping::Cube
        const bool useCube       = cubeRequested && isCube;
        // Cube requested but a non-cube texture supplied -> disable rather than bind a 2D image
        // to a cube slot (which would be invalid).
        const bool enabled = (texture != nullptr) && (!cubeRequested || isCube);

        const Data::Instance<RPI::Image>& systemWhite2D =
            RPI::ImageSystemInterface::Get()->GetSystemImage(RPI::SystemImage::White);

        // Both slots must stay bound with valid SRVs; the shader samples only the one matching
        // the active mapping mode.
        Data::Instance<RPI::Image> tex2D   = (enabled && !useCube) ? texture : systemWhite2D;
        Data::Instance<RPI::Image> texCube = useCube ? texture : GetOrCreateWhiteFallbackCube();

        m_passSrg->SetImage(tex2DIdx, tex2D);
        if (texCube)
        {
            m_passSrg->SetImage(texCubeIdx, texCube);
        }
        m_passSrg->SetConstant(mapIdx,   static_cast<uint32_t>(mapping));
        m_passSrg->SetConstant(blendIdx, static_cast<uint32_t>(blend));
        m_passSrg->SetConstant(strIdx,   strength);
        m_passSrg->SetConstant(enIdx,    enabled ? 1u : 0u);
    }

    // =========================================================================
    // SetupFrameGraphDependencies -- import image and declare UAV scope each frame
    // =========================================================================

    void GradientGICubemapPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);

        if (!m_diffuseImage || !m_specularImage)
        {
            return;
        }

        // Import each persistent attachment image and declare UAV (read/write) access using a
        // 2DArray view descriptor so the compute shader can address all 6 faces as array
        // slices (cubemap-typed UAVs are not supported in DX12/Vulkan).
        auto declareUav = [&frameGraph](const Data::Instance<RPI::AttachmentImage>& image)
        {
            frameGraph.GetAttachmentDatabase().ImportImage(
                image->GetAttachmentId(),
                image->GetRHIImage());

            RHI::ImageScopeAttachmentDescriptor uavDesc;
            uavDesc.m_attachmentId = image->GetAttachmentId();
            uavDesc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(
                RHI::Format::Unknown,
                /*mipSliceMin=*/0,
                /*mipSliceMax=*/0,
                /*arraySliceMin=*/0,
                /*arraySliceMax=*/5);
            uavDesc.m_loadStoreAction.m_loadAction  = RHI::AttachmentLoadAction::DontCare;
            uavDesc.m_loadStoreAction.m_storeAction = RHI::AttachmentStoreAction::Store;

            frameGraph.UseShaderAttachment(uavDesc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
        };

        declareUav(m_diffuseImage);
        declareUav(m_specularImage);
    }

    // =========================================================================
    // CompileResources -- bind SRG constants and UAV image view
    // =========================================================================

    void GradientGICubemapPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        if (!m_passSrg)
        {
            return;
        }

        if (m_dirty)
        {
            // ---- Bind gradient color constants ----
            const auto lowIdx   = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_lowColor"));
            const auto midIdx   = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_midColor"));
            const auto highIdx  = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_highColor"));
            const auto expIdx   = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_exposure"));
            const auto sizeIdx  = m_passSrg->FindShaderInputConstantIndex(AZ::Name("m_faceSize"));
            const auto diffIdx  = m_passSrg->FindShaderInputImageIndex(AZ::Name("m_outputDiffuse"));
            const auto specIdx  = m_passSrg->FindShaderInputImageIndex(AZ::Name("m_outputSpecular"));

            m_passSrg->SetConstant(lowIdx,  AZ::Vector3(m_lowColor.GetR(),  m_lowColor.GetG(),  m_lowColor.GetB()));
            m_passSrg->SetConstant(midIdx,  AZ::Vector3(m_midColor.GetR(),  m_midColor.GetG(),  m_midColor.GetB()));
            m_passSrg->SetConstant(highIdx, AZ::Vector3(m_highColor.GetR(), m_highColor.GetG(), m_highColor.GetB()));
            m_passSrg->SetConstant(expIdx,  m_exposure);
            m_passSrg->SetConstant(sizeIdx, m_faceSize);

            // ---- Bind the UAV image views from the frame graph (2DArray, not cubemap) ----
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

            // ---- Texture layers (resident SRVs + parameters) ----
            BindTextureLayer(
                AZ::Name("m_detailTex2D"), AZ::Name("m_detailTexCube"),
                AZ::Name("m_detailMapping"), AZ::Name("m_detailBlend"),
                AZ::Name("m_detailStrength"), AZ::Name("m_detailEnabled"),
                m_detailTexture, m_detailMapping, m_detailBlend, m_detailStrength);

            BindTextureLayer(
                AZ::Name("m_specularTex2D"), AZ::Name("m_specularTexCube"),
                AZ::Name("m_specularMapping"), AZ::Name("m_specularBlend"),
                AZ::Name("m_specularStrength"), AZ::Name("m_specularEnabled"),
                m_specularTexture, m_specularMapping, m_specularBlend, m_specularStrength);
        }

        m_passSrg->Compile();
    }

    // =========================================================================
    // BuildCommandListInternal -- dispatch compute when dirty
    // =========================================================================

    void GradientGICubemapPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
    {
        if (!m_dirty || !m_pipelineState || !m_passSrg)
        {
            return;
        }

        // Compute group counts: shader uses [numthreads(8, 8, 1)] and dispatches
        // over (faceSize x faceSize x 6) total threads.
        const uint32_t groupsX = (m_faceSize + 7) / 8;
        const uint32_t groupsY = (m_faceSize + 7) / 8;
        const uint32_t groupsZ = 6;

        RHI::DispatchDirect directArgs;
        directArgs.m_totalNumberOfThreadsX = groupsX * 8;
        directArgs.m_totalNumberOfThreadsY = groupsY * 8;
        directArgs.m_totalNumberOfThreadsZ = groupsZ;
        directArgs.m_threadsPerGroupX      = 8;
        directArgs.m_threadsPerGroupY      = 8;
        directArgs.m_threadsPerGroupZ      = 1;

        RHI::DispatchItem dispatchItem(RHI::MultiDevice::AllDevices);
        dispatchItem.SetArguments(RHI::DispatchArguments(directArgs));
        dispatchItem.SetPipelineState(m_pipelineState);

        const RHI::ShaderResourceGroup* srgRhi = m_passSrg->GetRHIShaderResourceGroup();
        dispatchItem.SetShaderResourceGroups(AZStd::span<const RHI::ShaderResourceGroup*>(&srgRhi, 1));

        context.GetCommandList()->Submit(
            dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));

        m_dirty = false;
    }

} // namespace AZ::Render
