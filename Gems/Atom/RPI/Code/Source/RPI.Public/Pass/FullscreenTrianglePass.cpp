/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RPI.Reflect/Pass/FullscreenTrianglePassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineState.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

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
            , m_passDescriptor(descriptor)
        {
            LoadShader();
        }

        FullscreenTrianglePass::~FullscreenTrianglePass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        void FullscreenTrianglePass::OnShaderReinitialized(const Shader&)
        {
            LoadShader();
        }

        void FullscreenTrianglePass::OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>&)
        {
            LoadShader();
        }

        void FullscreenTrianglePass::OnShaderVariantReinitialized(const Shader&, const ShaderVariantId&, ShaderVariantStableId)
        {
            LoadShader();
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

            // Load Shader
            Data::Asset<ShaderAsset> shaderAsset;
            if (passData->m_shaderAsset.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(passData->m_shaderAsset.m_assetId, passData->m_shaderAsset.m_filePath);
            }

            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Error("PassSystem", false, "[FullscreenTrianglePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderAsset.m_filePath.data());
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset);
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[FullscreenTrianglePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderAsset.m_filePath.data());
                return;
            }

            // Load Pass SRG
            const Data::Asset<ShaderResourceGroupAsset>& passSrgAsset = m_shader->FindShaderResourceGroupAsset(Name{ "PassSrg" });
            if (passSrgAsset)
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(passSrgAsset);

                AZ_Assert(m_shaderResourceGroup, "[FullscreenTrianglePass '%s']: Failed to create SRG from shader asset '%s'",
                    GetPathName().GetCStr(),
                    passData->m_shaderAsset.m_filePath.data());

                PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());
            }

            // Load Draw SRG
            // this is necessary since the shader may have options, which require a default draw SRG
            const Data::Asset<ShaderResourceGroupAsset>& drawSrgAsset = m_shader->FindShaderResourceGroupAsset(Name{ "DrawSrg" });
            if (drawSrgAsset)
            {
                m_drawShaderResourceGroup = ShaderResourceGroup::Create(drawSrgAsset);
            }

            // Store stencil reference value for the draw call
            m_stencilRef = passData->m_stencilRef;

            QueueForInitialization();

            ShaderReloadNotificationBus::Handler::BusDisconnect();
            ShaderReloadNotificationBus::Handler::BusConnect(shaderAsset.GetId());
        }

        void FullscreenTrianglePass::InitializeInternal()
        {
            RenderPass::InitializeInternal();

            // This draw item purposefully does not reference any geometry buffers.
            // Instead it's expected that the extended class uses a vertex shader 
            // that generates a full-screen triangle completely from vertex ids.
            RHI::DrawLinear draw = RHI::DrawLinear();
            draw.m_vertexCount = 3;

            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;

            // [GFX TODO][ATOM-872] The pass should be able to drive the shader variant
            // This is a pattern that should be established somewhere.
            auto shaderVariant = m_shader->GetVariant(m_shaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            pipelineStateDescriptor.m_renderAttachmentConfiguration = GetRenderAttachmentConfiguration();
            pipelineStateDescriptor.m_renderStates.m_multisampleState = GetMultisampleState();

            // No streams required
            RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            inputStreamLayout.Finalize();

            pipelineStateDescriptor.m_inputStreamLayout = inputStreamLayout;

            m_item.m_arguments = RHI::DrawArguments(draw);
            m_item.m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
            m_item.m_stencilRef = m_stencilRef;
        }

        void FullscreenTrianglePass::FrameBeginInternal(FramePrepareParams params)
        {
            const PassAttachment* outputAttachment = nullptr;
            
            if (GetOutputCount() > 0)
            {
                outputAttachment = GetOutputBinding(0).m_attachment.get();
            }
            else if(GetInputOutputCount() > 0)
            {
                outputAttachment = GetInputOutputBinding(0).m_attachment.get();
            }

            AZ_Assert(outputAttachment != nullptr, "[FullscreenTrianglePass %s] has no valid output or input/output attachments.", GetPathName().GetCStr());

            AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image,
                "[FullscreenTrianglePass %s] output of FullScreenTrianglePass must be an image", GetPathName().GetCStr());

            RHI::Size targetImageSize = outputAttachment->m_descriptor.m_image.m_size;

            // Base viewport and scissor off of target attachment
            m_viewportState = RHI::Viewport(0, static_cast<float>(targetImageSize.m_width), 0, static_cast<float>(targetImageSize.m_height));
            m_scissorState = RHI::Scissor(0, 0, targetImageSize.m_width, targetImageSize.m_height);

            RenderPass::FrameBeginInternal(params);
        }

        // Scope producer functions

        void FullscreenTrianglePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph);
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

            SetSrgsForDraw(commandList);

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            commandList->Submit(m_item);
        }
        
    }   // namespace RPI
}   // namespace AZ
