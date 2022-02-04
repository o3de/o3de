/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Shadows/FullscreenShadowPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<FullscreenShadowPass> FullscreenShadowPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<FullscreenShadowPass> pass = aznew FullscreenShadowPass(descriptor);
            return pass;
        }

        FullscreenShadowPass::FullscreenShadowPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void FullscreenShadowPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            SetConstantData();
            FullscreenTrianglePass::CompileResources(context);
        }

        RPI::PassAttachmentBinding FullscreenShadowPass::GetPassAttachmentBinding(AZ::Name name)
        {
            for (unsigned int i = 0; i < GetInputCount(); ++i)
            {
                auto b = GetInputBinding(i);
                if (b.m_name == name)
                    return b;
            }

            for (unsigned int i = 0; i < GetInputOutputCount(); ++i)
            {
                auto b = GetInputOutputBinding(i);
                if (b.m_name == name)
                    return b;
            }

            for (unsigned int i = 0; i < GetOutputCount(); ++i)
            {
                auto b = GetOutputBinding(i);
                if (b.m_name == name)
                    return b;
            }
            return {};
        }

        AZ::RHI::Size FullscreenShadowPass::GetDepthBufferDimensions()
        {
            auto outputBinding = GetPassAttachmentBinding(AZ::Name("Output"));
            auto outputDim = outputBinding.m_attachment->m_descriptor.m_image.m_size;
            AZ_Assert(outputDim.m_width > 0 && outputDim.m_height > 0, "Height and width are not valid\n");
            return outputDim;
        }

        //void FullscreenShadowPass::ChooseShaderVariant()
        //{
        //    const AZ::RPI::ShaderVariant& shaderVariant = CreateShaderVariant();
        //    CreatePipelineStateFromShaderVariant(shaderVariant);
        //}

        //AZ::RPI::ShaderOptionGroup FullscreenShadowPass::CreateShaderOptionGroup()
        //{
        //    RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
        //    shaderOptionGroup.SetUnspecifiedToDefaultValues();
        //    return shaderOptionGroup;
        //}

        //void FullscreenShadowPass::CreatePipelineStateFromShaderVariant(const RPI::ShaderVariant& shaderVariant)
        //{
        //    AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
        //    shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
        //    m_msaaPipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
        //    AZ_Error("FullscreenShadowPass", m_msaaPipelineState, "Failed to acquire pipeline state for shader");
        //}

        //const AZ::RPI::ShaderVariant& FullscreenShadowPass::CreateShaderVariant()
        //{
        //    RPI::ShaderOptionGroup shaderOptionGroup = CreateShaderOptionGroup();
        //    const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(shaderOptionGroup.GetShaderVariantId());

        //    //Set the fallbackkey
        //    //if (m_drawSrg)
        //    //{
        //    //    m_drawSrg->SetShaderVariantKeyFallbackValue(shaderOptionGroup.GetShaderVariantKeyFallbackValue());
        //    //}
        //    return shaderVariant;
        //}

        void FullscreenShadowPass::SetConstantData()
        {
            struct ConstantData
            {
                AZStd::array<float, 2> m_screenSize;
            } constantData;

            const RHI::Size resolution = GetDepthBufferDimensions();
            constantData.m_screenSize = { static_cast<float>(resolution.m_width), static_cast<float>(resolution.m_height) };

            [[maybe_unused]] bool setOk = m_shaderResourceGroup->SetConstant(m_constantDataIndex, constantData);
            AZ_Assert(setOk, "FullscreenShadowPass::SetConstantData() - could not set constant data");
        }

        //void FullscreenShadowPass::BuildInternal()
        //{
        //    FullscreenTrianglePass::BuildInternal();
        //    m_shader;
        //}

        //void FullscreenShadowPass::OnShaderReloaded()
        //{
        //    //LoadShader();
        //    //AZ_Assert(GetPassState() != RPI::PassState::Rendering, "FullscreenShadowPass: Trying to reload shader during rendering");
        //    //if (GetPassState() == RPI::PassState::Idle)
        //    //{
        //    //}
        //}

        //void FullscreenShadowPass::OnShaderReinitialized(const AZ::RPI::Shader&)
        //{
        //    OnShaderReloaded();
        //}

        //void FullscreenShadowPass::OnShaderAssetReinitialized(const Data::Asset<AZ::RPI::ShaderAsset>&)
        //{
        //    OnShaderReloaded();
        //}

        //void FullscreenShadowPass::OnShaderVariantReinitialized(const AZ::RPI::ShaderVariant&)
        //{
        //    OnShaderReloaded();
        //}

    }   // namespace Render
}   // namespace AZ
