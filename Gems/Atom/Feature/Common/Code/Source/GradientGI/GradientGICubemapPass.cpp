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
        m_lowColor  = low;
        m_midColor  = mid;
        m_highColor = high;
        m_exposure  = exposure;
        m_faceSize  = faceSize;
        m_dirty     = true;
    }

    Data::Instance<RPI::AttachmentImage> GradientGICubemapPass::GetCubemapImage() const
    {
        return m_cubemapImage;
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

        // ---- Create persistent output cubemap AttachmentImage ----
        // Bind flags: ShaderRead (SRV for IBL sampling) + ShaderWrite (UAV for compute output).
        // Default view is a cubemap SRV, used when binding to the scene SRG's IBL slots.
        auto* imageSystem = RPI::ImageSystemInterface::Get();
        auto* attachmentPool = imageSystem ? imageSystem->GetSystemAttachmentPool().get() : nullptr;
        if (!attachmentPool)
        {
            AZ_Error("GradientGICubemapPass", false, "System attachment pool not available.");
            SetEnabled(false);
            return;
        }

        RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::CreateCubemap(
            RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::ShaderWrite,
            m_faceSize,
            RHI::Format::R16G16B16A16_FLOAT);

        // Cubemap SRV as default view -- used when the FP binds this image to scene SRG slots.
        auto cubemapViewDesc = RHI::ImageViewDescriptor::CreateCubemap();

        m_cubemapImage = RPI::AttachmentImage::Create(
            *attachmentPool, imageDesc,
            AZ::Name("GradientGI_Cubemap"),
            nullptr,
            &cubemapViewDesc);

        if (!m_cubemapImage)
        {
            AZ_Error("GradientGICubemapPass", false,
                "Failed to create AttachmentImage for gradient cubemap. "
                "GPU compute (UAV cubemap) is not supported on this platform.");
            SetEnabled(false);
            return;
        }

        m_dirty = true;
    }

    // =========================================================================
    // SetupFrameGraphDependencies -- import image and declare UAV scope each frame
    // =========================================================================

    void GradientGICubemapPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
    {
        RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);

        if (!m_cubemapImage)
        {
            return;
        }

        // Import the persistent attachment image into this frame's attachment database.
        frameGraph.GetAttachmentDatabase().ImportImage(
            m_cubemapImage->GetAttachmentId(),
            m_cubemapImage->GetRHIImage());

        // Declare UAV (read/write) access: use a 2DArray view descriptor so the
        // compute shader can address all 6 faces as array slices (required for UAV;
        // cubemap-typed UAVs are not supported in DX12/Vulkan).
        RHI::ImageScopeAttachmentDescriptor uavDesc;
        uavDesc.m_attachmentId = m_cubemapImage->GetAttachmentId();
        uavDesc.m_imageViewDescriptor = RHI::ImageViewDescriptor::Create(
            RHI::Format::Unknown,
            /*mipSliceMin=*/0,
            /*mipSliceMax=*/0,
            /*arraySliceMin=*/0,
            /*arraySliceMax=*/5);
        uavDesc.m_loadStoreAction.m_loadAction  = RHI::AttachmentLoadAction::DontCare;
        uavDesc.m_loadStoreAction.m_storeAction = RHI::AttachmentStoreAction::Store;

        frameGraph.UseShaderAttachment(uavDesc, RHI::ScopeAttachmentAccess::ReadWrite, RHI::ScopeAttachmentStage::ComputeShader);
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
            const auto imgIdx   = m_passSrg->FindShaderInputImageIndex(AZ::Name("m_outputCubemap"));

            m_passSrg->SetConstant(lowIdx,  AZ::Vector3(m_lowColor.GetR(),  m_lowColor.GetG(),  m_lowColor.GetB()));
            m_passSrg->SetConstant(midIdx,  AZ::Vector3(m_midColor.GetR(),  m_midColor.GetG(),  m_midColor.GetB()));
            m_passSrg->SetConstant(highIdx, AZ::Vector3(m_highColor.GetR(), m_highColor.GetG(), m_highColor.GetB()));
            m_passSrg->SetConstant(expIdx,  m_exposure);
            m_passSrg->SetConstant(sizeIdx, m_faceSize);

            // ---- Bind the UAV image view from the frame graph (2DArray, not cubemap) ----
            if (m_cubemapImage)
            {
                const RHI::ImageView* uavView = context.GetImageView(m_cubemapImage->GetAttachmentId());
                if (uavView)
                {
                    m_passSrg->SetImageView(imgIdx, uavView);
                }
            }
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
