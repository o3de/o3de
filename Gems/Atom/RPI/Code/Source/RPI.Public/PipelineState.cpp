/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace RPI
    {
        PipelineStateForDraw::PipelineStateForDraw()
            : m_dirty(false)
            , m_initDataFromShader(false)
            , m_hasOutputData(false)
            , m_isShaderVariantReady(false)
            , m_useRenderStatesOverlay(false)
        {
            m_renderStatesOverlay = RHI::GetInvalidRenderStates();
        }

        PipelineStateForDraw::PipelineStateForDraw(const PipelineStateForDraw& right)
        {
            m_descriptor = right.m_descriptor;
            m_shader = right.m_shader;
            m_pipelineState = right.m_pipelineState;

            m_initDataFromShader = right.m_initDataFromShader;
            m_hasOutputData = right.m_hasOutputData;
            m_dirty = right.m_dirty;
            m_shaderVariantId = right.m_shaderVariantId;
            m_isShaderVariantReady = right.m_isShaderVariantReady;
            m_useRenderStatesOverlay = right.m_useRenderStatesOverlay;

            m_renderStatesOverlay = right.m_renderStatesOverlay;

            if (m_shader)
            {
                ShaderReloadNotificationBus::MultiHandler::BusConnect(m_shader->GetAsset().GetId());
            }
        }

        PipelineStateForDraw::~PipelineStateForDraw()
        {
            Shutdown();
        }

        void PipelineStateForDraw::Init(const Data::Instance<RPI::Shader>& shader, const ShaderOptionList* optionAndValues)
        {
            // Get shader variant from the shader
            ShaderVariantId shaderVariant = {};
            if (optionAndValues)
            {
                RPI::ShaderOptionGroup shaderOptionGroup = shader->CreateShaderOptionGroup();
                shaderOptionGroup.SetUnspecifiedToDefaultValues();
                for (const auto& optionAndValue : *optionAndValues)
                {
                    shaderOptionGroup.SetValue(optionAndValue.first, optionAndValue.second);
                }
                shaderVariant = shaderOptionGroup.GetShaderVariantId();
            }
            Init(shader, shaderVariant);
        }

        void PipelineStateForDraw::Init(const Data::Instance<Shader>& shader, const ShaderVariantId& shaderVariantId)
        {
            // Reset some variables
            m_pipelineState = nullptr;

            // Cache shader so it can be used for create RHI::PipelineState later
            m_shader = shader;

            UpdateShaderVaraintId(shaderVariantId);
            
            // Connect to shader reload notification bus to rebuilt pipeline state when shader or shader variant changed.
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
            ShaderReloadNotificationBus::MultiHandler::BusConnect(shader->GetAsset().GetId());

            m_initDataFromShader = true;
        }

        void PipelineStateForDraw::RefreshShaderVariant()
        {
            auto shaderVariant = m_shader->GetVariant(m_shaderVariantId);
            m_isShaderVariantReady = !shaderVariant.UseKeyFallback();

            auto multisampleState = m_descriptor.m_renderStates.m_multisampleState;

            shaderVariant.ConfigurePipelineState(m_descriptor, m_shaderVariantId);

            // Recover multisampleState if it was set from output data
            if (m_hasOutputData)
            {
                m_descriptor.m_renderStates.m_multisampleState = multisampleState;
            }

            // re-finalize if the pipeline state wasn't finalized.
            if (!m_dirty)
            {
                m_dirty = true;
                Finalize();
            }
        }

        void PipelineStateForDraw::OnShaderReinitialized([[maybe_unused]] const AZ::RPI::Shader& shader)
        {
            RefreshShaderVariant();
        }
        
        void PipelineStateForDraw::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<ShaderAsset>& shaderAsset)
        {
            RefreshShaderVariant();
        }

        void PipelineStateForDraw::OnShaderVariantReinitialized(const ShaderVariant& shaderVariant)
        {
            if(shaderVariant.GetShaderVariantId() == m_shaderVariantId)
            {
                RefreshShaderVariant();
            }
        }
        
        void PipelineStateForDraw::SetOutputFromScene(const Scene* scene, RHI::DrawListTag overrideDrawListTag)
        {
            // Use overrideDrawListTag if it's valid. otherwise get DrawListTag from the shader
            RHI::DrawListTag drawListTag = overrideDrawListTag;
            if (!drawListTag.IsValid())
            {
                drawListTag = m_shader->GetDrawListTag();
            }
            
            // The scene may or may not have the output data for this PipelineState
            // For example, if there is no render pipeline in the scene, or there is no pass in the scene with matching draw list tag
            m_hasOutputData = scene->ConfigurePipelineState(drawListTag, m_descriptor);
            m_dirty = true;
        }

        void PipelineStateForDraw::SetOutputFromPass(RenderPass* renderPass)
        {
            m_hasOutputData = true;
            m_dirty = true;
            m_descriptor.m_renderAttachmentConfiguration = renderPass->GetRenderAttachmentConfiguration();
            m_descriptor.m_renderStates.m_multisampleState = renderPass->GetMultisampleState();
        }

        void PipelineStateForDraw::SetInputStreamLayout(const RHI::InputStreamLayout& inputStreamLayout)
        {
            m_descriptor.m_inputStreamLayout = inputStreamLayout;
        }

        const RHI::PipelineState* PipelineStateForDraw::Finalize()
        {
            if (m_dirty)
            {
                AZ_Assert(m_initDataFromShader, "PipelineStateForDraw::Init() need to be called once before Finalize()");
                m_pipelineState = nullptr;

                if (m_hasOutputData && m_initDataFromShader)
                {
                    RHI::PipelineStateDescriptorForDraw descriptor = m_descriptor;
                    if (m_useRenderStatesOverlay)
                    {
                        RHI::MergeStateInto(m_renderStatesOverlay, descriptor.m_renderStates);
                    }

                    m_pipelineState = m_shader->AcquirePipelineState(descriptor);
                }
                m_dirty = false;
            }
            return m_pipelineState;
        }

        const RHI::PipelineState* PipelineStateForDraw::GetRHIPipelineState() const
        {
            AZ_Assert(false == m_dirty, "The descriptor has been modified and Finalize() need to be called before get a proper PipelineState");

            return m_pipelineState;
        }
                        
        RHI::RenderStates& PipelineStateForDraw::RenderStatesOverlay()
        {
            // Assume the descriptor will be changed if user tries to get reference but not a constant reference
            m_dirty = true;
            m_useRenderStatesOverlay = true;
            return m_renderStatesOverlay;
        }
      
        RHI::InputStreamLayout& PipelineStateForDraw::InputStreamLayout()
        {
            m_dirty = true;
            return m_descriptor.m_inputStreamLayout;
        }

        void PipelineStateForDraw::UpdateShaderVaraintId(const ShaderVariantId& shaderVariantId)
        {
            m_dirty = true;

            // Get shader variant from the shader
            m_shaderVariantId = shaderVariantId;
            ShaderVariant shaderVariant = m_shader->GetVariant(m_shaderVariantId);
            m_isShaderVariantReady = !shaderVariant.UseKeyFallback();

            // Fill the descriptor with data from shader variant
            shaderVariant.ConfigurePipelineState(m_descriptor, m_shaderVariantId);
        }

        const RHI::PipelineStateDescriptorForDraw& PipelineStateForDraw::ConstDescriptor() const
        {
            return m_descriptor;
        }

        const Data::Instance<Shader>& PipelineStateForDraw::GetShader() const
        {
            return m_shader;
        }

        bool PipelineStateForDraw::UpdateSrgVariantFallback(Data::Instance<ShaderResourceGroup>& srg) const
        {
           if (m_isShaderVariantReady)
            {
                return false;
            }

            srg->SetShaderVariantKeyFallbackValue(m_shaderVariantId.m_key);
            return true;
        }

        void PipelineStateForDraw::Shutdown()
        {
            m_descriptor = RHI::PipelineStateDescriptorForDraw{};
            m_shader = nullptr;
            m_pipelineState = nullptr;
            m_shaderVariantId = ShaderVariantId{};
            m_initDataFromShader = false;
            m_hasOutputData = false;
            m_dirty = false;
            m_isShaderVariantReady = false;
                        
            ShaderReloadNotificationBus::MultiHandler::BusDisconnect();
        }

        const ShaderVariantId& PipelineStateForDraw::GetShaderVariantId() const
        {
            return m_shaderVariantId;
        }
    }
}
