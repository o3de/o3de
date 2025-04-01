/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/DevicePipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/algorithm.h>


namespace AZ
{
    namespace RPI
    {
        Ptr<FullscreenTrianglePass> FullscreenTrianglePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<FullscreenTrianglePass> pass = aznew FullscreenTrianglePass(descriptor);
            return pass;
        }

        FullscreenTrianglePass::FullscreenTrianglePass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
            , m_item(RHI::MultiDevice::AllDevices)
            , m_passDescriptor(descriptor)
        {
            m_defaultShaderAttachmentStage = RHI::ScopeAttachmentStage::FragmentShader;
            LoadShader();
        }

        FullscreenTrianglePass::~FullscreenTrianglePass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        Data::Instance<Shader> FullscreenTrianglePass::GetShader() const
        {
            return m_shader;
        }

        void FullscreenTrianglePass::OnShaderReinitialized(const Shader&)
        {
            LoadShader();
        }

        void FullscreenTrianglePass::OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>&)
        {
            LoadShader();
        }

        void FullscreenTrianglePass::OnShaderVariantReinitialized(const ShaderVariant&)
        {
            LoadShader();
        }

        void FullscreenTrianglePass::UpdateShaderOptionsCommon()
        {
            m_pipelineStateForDraw.UpdateSrgVariantFallback(m_shaderResourceGroup);
            BuildDrawItem();
        }

        void FullscreenTrianglePass::LoadShader()
        {
            AZ_Assert(GetPassState() != PassState::Rendering, "FullscreenTrianglePass - Reloading shader during Rendering phase!");

            // Load FullscreenTrianglePassData
            const FullscreenTrianglePassData* passData = PassUtils::GetPassData<FullscreenTrianglePassData>(m_passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[FullscreenTrianglePass '%s']: Trying to construct without valid FullscreenTrianglePassData!",
                    GetPathName().GetCStr());
                return;
            }

            AZ::Data::AssetId shaderAssetId = passData->m_shaderAsset.m_assetId;
            if (!shaderAssetId.IsValid())
            {
                // This case may happen when PassData comes from a PassRequest defined inside an *.azasset.
                // Unlike the PassBuilder, the AnyAssetBuilder doesn't record the AssetId, so we have to discover the asset id at runtime.
                AZStd::string azshaderPath = passData->m_shaderAsset.m_filePath;
                AZ::StringFunc::Path::ReplaceExtension(azshaderPath, "azshader");
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    shaderAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, azshaderPath.c_str(),
                    azrtti_typeid<ShaderAsset>(), false /*autoRegisterIfNotFound*/);
            }

            // Load Shader
            Data::Asset<ShaderAsset> shaderAsset;
            if (shaderAssetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(shaderAssetId, passData->m_shaderAsset.m_filePath);
            }

            if (!shaderAsset.IsReady())
            {
                AZ_Error("PassSystem", false, "[FullscreenTrianglePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderAsset.m_filePath.data());
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset, GetSuperVariantName());
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[FullscreenTrianglePass '%s']: Failed to create shader instance from asset '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderAsset.m_filePath.data());
                return;
            }

            // Store stencil reference value for the draw call
            m_stencilRef = passData->m_stencilRef;

            m_pipelineStateForDraw.Init(m_shader, m_shader->GetDefaultShaderOptions().GetShaderVariantId());

            UpdateSrgs();

            QueueForInitialization();

            ShaderReloadNotificationBus::Handler::BusDisconnect();
            ShaderReloadNotificationBus::Handler::BusConnect(shaderAsset.GetId());
        }

        void FullscreenTrianglePass::UpdateSrgs()
        {
            if (!m_shader)
            {
                return;
            }

            // Load Pass SRG
            const auto passSrgLayout = m_shader->FindShaderResourceGroupLayout(SrgBindingSlot::Pass);
            if (passSrgLayout)
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), passSrgLayout->GetName());

                [[maybe_unused]] const FullscreenTrianglePassData* passData = PassUtils::GetPassData<FullscreenTrianglePassData>(m_passDescriptor);

                AZ_Assert(m_shaderResourceGroup, "[FullscreenTrianglePass '%s']: Failed to create SRG from shader asset '%s'",
                    GetPathName().GetCStr(),
                    passData->m_shaderAsset.m_filePath.data());

                PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());
            }

            // Load Draw SRG
            // this is necessary since the shader may have options, which require a default draw SRG
            const bool compileDrawSrg = false; // The SRG will be compiled in CompileResources()
            m_drawShaderResourceGroup = m_shader->CreateDefaultDrawSrg(compileDrawSrg);

            // It is valid for there to be no draw srg if there are no shader options, so check to see if it is null.
            if (m_drawShaderResourceGroup)
            {
                m_pipelineStateForDraw.UpdateSrgVariantFallback(m_shaderResourceGroup);
            }
        }

        void FullscreenTrianglePass::BuildDrawItem()
        {
            m_pipelineStateForDraw.SetOutputFromPass(this);

            // No streams required
            RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            inputStreamLayout.Finalize();

            m_pipelineStateForDraw.SetInputStreamLayout(inputStreamLayout);

            // This draw item purposefully does not reference any geometry buffers.
            // Instead it's expected that the extended class uses a vertex shader 
            // that generates a full-screen triangle completely from vertex ids.
            m_geometryView.SetDrawArguments(RHI::DrawLinear(3, 0));

            m_item.SetGeometryView(& m_geometryView);
            m_item.SetPipelineState(m_pipelineStateForDraw.Finalize());
            m_item.SetStencilRef(static_cast<uint8_t>(m_stencilRef));
        }

        void FullscreenTrianglePass::UpdateShaderOptions(const ShaderOptionList& shaderOptions)
        {
            if (m_shader)
            {
                m_pipelineStateForDraw.Init(m_shader, &shaderOptions);
                UpdateShaderOptionsCommon();
            }
        }

        void FullscreenTrianglePass::UpdateShaderOptions(const ShaderVariantId& shaderVariantId)
        {
            if (m_shader)
            {
                m_pipelineStateForDraw.Init(m_shader, shaderVariantId);
                UpdateShaderOptionsCommon();
            }
        }

        void FullscreenTrianglePass::InitializeInternal()
        {
            BuildRenderAttachmentConfiguration();
            if (m_shader->GetSupervariantIndex() != m_shader->GetAsset()->GetSupervariantIndex(GetSuperVariantName()))
            {
                LoadShader();
            }

            RenderPass::InitializeInternal();
            
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->FullscreenTrianglePass::InitializeInternal", this);

            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[FullscreenTrianglePass]: Shader not loaded!");
                return;
            }

            BuildDrawItem();
        }

        void FullscreenTrianglePass::FrameBeginInternal(FramePrepareParams params)
        {
            const PassAttachment* outputAttachment = nullptr;
            
            if (GetOutputCount() > 0)
            {
                outputAttachment = GetOutputBinding(0).GetAttachment().get();
            }
            else if(GetInputOutputCount() > 0)
            {
                outputAttachment = GetInputOutputBinding(0).GetAttachment().get();
            }

            AZ_Assert(outputAttachment != nullptr, "[FullscreenTrianglePass %s] has no valid output or input/output attachments.", GetPathName().GetCStr());

            AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image,
                "[FullscreenTrianglePass %s] output of FullScreenTrianglePass must be an image", GetPathName().GetCStr());

            RHI::Size targetImageSize = outputAttachment->m_descriptor.m_image.m_size;

            m_viewportState.m_maxX = static_cast<float>(targetImageSize.m_width);
            m_viewportState.m_maxY = static_cast<float>(targetImageSize.m_height);

            m_scissorState.m_maxX = static_cast<int32_t>(targetImageSize.m_width);
            m_scissorState.m_maxY = static_cast<int32_t>(targetImageSize.m_height);

            RenderPass::FrameBeginInternal(params);
        }

        // Scope producer functions

        void FullscreenTrianglePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);

            // Update scissor/viewport regions based on the mip level of the render target that is being written into
            uint16_t viewMinMip = RHI::ImageSubresourceRange::HighestSliceIndex;
            for (const PassAttachmentBinding& attachmentBinding : m_attachmentBindings)
            {
                if (attachmentBinding.GetAttachment() != nullptr &&
                    frameGraph.GetAttachmentDatabase().IsAttachmentValid(attachmentBinding.GetAttachment()->GetAttachmentId()) &&
                    attachmentBinding.m_unifiedScopeDesc.GetType() == RHI::AttachmentType::Image &&
                    RHI::CheckBitsAny(attachmentBinding.GetAttachmentAccess(), RHI::ScopeAttachmentAccess::Write) &&
                    attachmentBinding.m_scopeAttachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget)
                {
                    RHI::ImageViewDescriptor viewDesc = attachmentBinding.m_unifiedScopeDesc.GetAsImage().m_imageViewDescriptor;
                    viewMinMip = AZStd::min(viewMinMip, viewDesc.m_mipSliceMin);
                }
            }
            
            if(viewMinMip < RHI::ImageSubresourceRange::HighestSliceIndex)
            {
                uint32_t viewportStateMaxX = static_cast<uint32_t>(m_viewportState.m_maxX);
                uint32_t viewportStateMaxY = static_cast<uint32_t>(m_viewportState.m_maxY);
                m_viewportState.m_maxX = static_cast<float>(viewportStateMaxX >> viewMinMip);
                m_viewportState.m_maxY = static_cast<float>(viewportStateMaxY >> viewMinMip);

                m_scissorState.m_maxX = static_cast<uint32_t>(m_scissorState.m_maxX) >> viewMinMip;
                m_scissorState.m_maxY = static_cast<uint32_t>(m_scissorState.m_maxY) >> viewMinMip;
            }
            
            frameGraph.SetEstimatedItemCount(1);
        }

        void FullscreenTrianglePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
            }

            if (m_drawShaderResourceGroup != nullptr)
            {
                m_drawShaderResourceGroup->Compile();
                BindSrg(m_drawShaderResourceGroup->GetRHIShaderResourceGroup());
            }
        }

        void FullscreenTrianglePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDraw(context);

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            commandList->Submit(m_item.GetDeviceDrawItem(context.GetDeviceIndex()));
        }
        
    }   // namespace RPI
}   // namespace AZ
