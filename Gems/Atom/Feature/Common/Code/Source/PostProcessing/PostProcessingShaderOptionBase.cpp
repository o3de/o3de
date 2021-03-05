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

#include <PostProcessing/PostProcessingShaderOptionBase.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        void PostProcessingShaderOptionBase::PreloadShaderVariant(
            const Data::Instance<AZ::RPI::Shader>& shader,
            const RPI::ShaderOptionGroup& shaderOption,
            const RHI::RenderAttachmentConfiguration& renderAttachmentConfiguration,
            const RHI::MultisampleState& multisampleState)
        {
            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            AZ::u64 variationKey = shaderOption.GetShaderVariantId().m_key.to_ullong();

            auto searchResult = shader->FindVariantStableId(shaderOption.GetShaderVariantId());
            auto shaderVariant = shader->GetVariant(searchResult.GetStableId());

            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            pipelineStateDescriptor.m_renderAttachmentConfiguration = renderAttachmentConfiguration;
            pipelineStateDescriptor.m_renderStates.m_multisampleState = multisampleState;

            // No streams required
            RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            inputStreamLayout.Finalize();

            pipelineStateDescriptor.m_inputStreamLayout = inputStreamLayout;

            m_shaderVariantTable[variationKey].m_pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
            m_shaderVariantTable[variationKey].m_isFullyBaked = searchResult.IsFullyBaked();
        }

        void PostProcessingShaderOptionBase::UpdateShaderVariant(const AZ::RPI::ShaderOptionGroup& shaderOption)
        {
            m_currentShaderVariantKeyValue = shaderOption.GetShaderVariantId().m_key.to_ullong();
            auto shaderVariant = GetShaderVariant(m_currentShaderVariantKeyValue);
            AZ_Assert(shaderVariant != nullptr, "Couldn't get a shader variation using the shader variant key[0x%llx].", m_currentShaderVariantKeyValue);

            if (!shaderVariant->m_isFullyBaked)
            {
                m_currentShaderVariantKeyFallbackValue = shaderOption.GetShaderVariantKeyFallbackValue();
            }
        }

        void PostProcessingShaderOptionBase::CompileShaderVariant(Data::Instance<AZ::RPI::ShaderResourceGroup>& shaderResourceGroup)
        {
            // Set the shader variant key falback value to the shader resource group if needed.
            auto shaderVariant = GetShaderVariant(m_currentShaderVariantKeyValue);
            if (shaderVariant != nullptr &&
                !shaderVariant->m_isFullyBaked &&
                shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
            {
                shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_currentShaderVariantKeyFallbackValue);
            }
        }

        const AZ::RHI::PipelineState* PostProcessingShaderOptionBase::GetPipelineStateFromShaderVariant()const
        {
            auto shaderVariant = GetShaderVariant(m_currentShaderVariantKeyValue);
            AZ_Assert(shaderVariant != nullptr, "Couldn't get a shader variation using the shader variant key[0x%llx].", m_currentShaderVariantKeyValue);
            return shaderVariant->m_pipelineState;
        }

        const PostProcessingShaderOptionBase::ShaderVariantInformation* PostProcessingShaderOptionBase::GetShaderVariant(AZ::u64 key) const
        {
            auto result = m_shaderVariantTable.find(key);
            return result != nullptr ? &result->second : nullptr;
        }

    }   // namespace Render
}   // namespace AZ
