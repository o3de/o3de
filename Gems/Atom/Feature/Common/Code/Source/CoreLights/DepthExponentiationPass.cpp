/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DepthExponentiationPass.h>

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace Render
    {
        DepthExponentiationPass::DepthExponentiationPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
            , m_optionName("o_shadowmapLightType")
            , m_optionValues
            {
                Name("ShadowmapLightType::Directional"),
                Name("ShadowmapLightType::Spot")
            }
        {
        }

        RPI::Ptr<DepthExponentiationPass> DepthExponentiationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew DepthExponentiationPass(descriptor);
        }

        void DepthExponentiationPass::SetShadowmapType(Shadow::ShadowmapType type)
        {
            if (!m_shaderOptionInitialized)
            {
                InitializeShaderOption();
                m_shaderOptionInitialized = true;
            }
            m_shadowmapType = type;
            SetShaderVariantKeyFallbackValue();
        }

        void DepthExponentiationPass::SetShaderVariantKeyFallbackValue()
        {
            const uint32_t typeIndex = aznumeric_cast<uint32_t>(m_shadowmapType);
            RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
            shaderOption.SetValue(m_optionName, m_optionValues[typeIndex]);

            if (!m_shaderVariant[typeIndex].m_isFullyBaked && m_shaderResourceGroup)
            {
                m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(shaderOption.GetShaderVariantKeyFallbackValue());
            }
        }

        Shadow::ShadowmapType DepthExponentiationPass::GetShadowmapType() const
        {
            return m_shadowmapType;
        }

        void DepthExponentiationPass::BuildInternal()
        {
            InitializeShaderOption();
        }

        void DepthExponentiationPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_shaderOptionInitialized)
            {
                InitializeShaderOption();
                m_shaderOptionInitialized = true;
            }

            Base::FrameBeginInternal(params);
        }

        void DepthExponentiationPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderVariantNeedsUpdate)
            {
                SetShadowmapType(m_shadowmapType);
                m_shaderVariantNeedsUpdate = false;
            }

            Base::CompileResources(context);
        }

        void DepthExponentiationPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            const uint32_t typeIndex = aznumeric_cast<uint32_t>(m_shadowmapType);
            m_dispatchItem.SetPipelineState(m_shaderVariant[typeIndex].m_pipelineState);

            Base::BuildCommandListInternal(context);
        }

        void DepthExponentiationPass::InitializeShaderOption()
        {
            AZ_Assert(m_shader != nullptr, "DepthExponentiationPass %s has a null shader when calling FrameBeginInternal.", GetPathName().GetCStr());

            // Caching all pipeline state for each shader variation for performance reason.
            m_shaderVariant.clear();
            for (const Name& valueName : m_optionValues)
            {
                RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_optionName, valueName);

                RPI::ShaderVariant shaderVariant = m_shader->GetVariant(shaderOption.GetShaderVariantId());

                RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shaderOption);

                ShaderVariantInfo variationInfo{!shaderVariant.UseKeyFallback(),
                    m_shader->AcquirePipelineState(pipelineStateDescriptor)
                };
                m_shaderVariant.push_back(AZStd::move(variationInfo));
            }
            m_shaderVariantNeedsUpdate = true;
        }

    } // namespace Rendere
} // namespace AZ
