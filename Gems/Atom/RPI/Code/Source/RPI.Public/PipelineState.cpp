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
        {

        }


        PipelineStateForDraw::PipelineStateForDraw(const PipelineStateForDraw& right)
        {
            m_descriptor = right.m_descriptor;
            m_shader = right.m_shader;
            m_pipelineState = right.m_pipelineState;
            m_initDataFromShader = right.m_initDataFromShader;
            m_hasOutputData = right.m_hasOutputData;
            m_dirty = right.m_dirty;
        }

        void PipelineStateForDraw::Init(const Data::Instance<RPI::Shader>& shader, const AZStd::vector<AZStd::pair<Name, Name>>* optionAndValues)
        {
            // Reset some variables
            m_shader = nullptr;
            m_pipelineState = nullptr;

            // Reset some flags
            m_initDataFromShader = false;

            m_dirty = true;
            
            // Get shader variant from the shader
            ShaderVariantStableId shaderVariantStableId = RPI::ShaderAsset::RootShaderVariantStableId;
            if (optionAndValues)
            {
                RPI::ShaderOptionGroup shaderOptionGroup = shader->CreateShaderOptionGroup();
                shaderOptionGroup.SetUnspecifiedToDefaultValues();
                for (auto optionAndValue : *optionAndValues)
                {
                    shaderOptionGroup.SetValue(optionAndValue.first, optionAndValue.second);
                }
                RPI::ShaderVariantSearchResult findVariantResult = shader->FindVariantStableId(shaderOptionGroup.GetShaderVariantId());
                if (!findVariantResult.GetStableId().IsValid())
                {
                    AZ_Error("RPI::PipelineStateForDraw", false, "Failed to find specified shader variant");
                    return;
                }
                shaderVariantStableId = findVariantResult.GetStableId();
            }

            auto shaderVariant = shader->GetVariant(shaderVariantStableId);

            // Fill the descriptor with data from shader variant
            shaderVariant.ConfigurePipelineState(m_descriptor);

            m_initDataFromShader = true;

            // Cache shader so it can be used for create RHI::PipelineState later
            m_shader = shader;
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
                    m_pipelineState = m_shader->AcquirePipelineState(m_descriptor);
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
        
        RHI::PipelineStateDescriptorForDraw& PipelineStateForDraw::Descriptor()
        {
            // Assume the descriptor will be changed if user tries to get reference but not a constant reference
            m_dirty = true;
            return m_descriptor;
        }

        const RHI::PipelineStateDescriptorForDraw& PipelineStateForDraw::ConstDescriptor() const
        {
            return m_descriptor;
        }
    }
}
