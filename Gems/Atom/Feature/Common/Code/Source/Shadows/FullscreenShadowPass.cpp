/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Shadows/FullscreenShadowPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DevicePipelineState.h>
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
            : Base(descriptor)
            , m_outputName("Output")
            , m_depthInputName("Depth")
        {
        }

        void FullscreenShadowPass::InitializeInternal()
        {
            Base::InitializeInternal();

            AZ::Name msaaOptionName = AZ::Name("MsaaMode::MsaaNone");
            uint16_t numSamples = GetDepthBufferMSAACount();
            if (numSamples > 1)
            {
                msaaOptionName = AZStd::string::format("MsaaMode::Msaa%ux", numSamples);
            }

            UpdateShaderOptions(
                { { AZ::Name("o_msaaMode"), msaaOptionName} }
            );
        }

        void FullscreenShadowPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            SetConstantData();
            Base::CompileResources(context);
        }

        RHI::Size FullscreenShadowPass::GetDepthBufferDimensions()
        {
            RPI::PassAttachmentBinding* outputBinding = RPI::Pass::FindAttachmentBinding(m_outputName);
            auto outputDim = outputBinding->GetAttachment()->m_descriptor.m_image.m_size;
            AZ_Assert(outputDim.m_width > 0 && outputDim.m_height > 0, "Height and width are not valid\n");
            return outputDim;
        }

        uint16_t FullscreenShadowPass::GetDepthBufferMSAACount()
        {
            RPI::PassAttachmentBinding* inputBinding = RPI::Pass::FindAttachmentBinding(m_depthInputName);
            return inputBinding->GetAttachment()->m_descriptor.m_image.m_multisampleState.m_samples;
        }       

        void FullscreenShadowPass::SetConstantData()
        {
            struct alignas(16) ConstantData
            {
                AZStd::array<float, 2> m_screenSize;
                int m_lightIndex;
                int m_filterMode;
                int m_filteringSampleCountMode;
                int m_blendBetweenCascadesEnable;
                int m_receiverShadowPlaneBiasEnable;

            } constantData;

            const RHI::Size resolution = GetDepthBufferDimensions();
            constantData.m_lightIndex = m_lightIndex;
            constantData.m_screenSize = { static_cast<float>(resolution.m_width), static_cast<float>(resolution.m_height) };
            constantData.m_filterMode = static_cast<int>(m_filterMethod);
            constantData.m_filteringSampleCountMode = static_cast<int>(m_filteringSampleCountMode);
            constantData.m_blendBetweenCascadesEnable = m_blendBetweenCascadesEnable ? 1 : 0;
            constantData.m_receiverShadowPlaneBiasEnable = m_receiverShadowPlaneBiasEnable ? 1 : 0;

            [[maybe_unused]] bool setOk = m_shaderResourceGroup->SetConstant(m_constantDataIndex, constantData);
            AZ_Assert(setOk, "FullscreenShadowPass::SetConstantData() - could not set constant data");

        }

    }   // namespace Render
}   // namespace AZ
