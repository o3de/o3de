/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ColorGrading/LutGenerationPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {

        RPI::Ptr<LutGenerationPass> LutGenerationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LutGenerationPass> pass = aznew LutGenerationPass(descriptor);
            return AZStd::move(pass);
        }

         LutGenerationPass::LutGenerationPass(const RPI::PassDescriptor& descriptor)
            : HDRColorGradingPass(descriptor)
        {
        }

        void LutGenerationPass::InitializeInternal()
        {
            HDRColorGradingPass::InitializeInternal();

            m_lutResolutionIndex.Reset();
            m_lutShaperTypeIndex.Reset();
            m_lutShaperScaleIndex.Reset();
        }

        void LutGenerationPass::FrameBeginInternal(FramePrepareParams params)
        {
            const auto* colorGradingSettings = GetHDRColorGradingSettings();
            if (colorGradingSettings)
            {
                m_shaderResourceGroup->SetConstant(m_lutResolutionIndex, colorGradingSettings->GetLutResolution());

                auto shaperParams = AcesDisplayMapperFeatureProcessor::GetShaperParameters(
                    colorGradingSettings->GetShaperPresetType(),
                    colorGradingSettings->GetCustomMinExposure(),
                    colorGradingSettings->GetCustomMaxExposure());
                m_shaderResourceGroup->SetConstant(m_lutShaperTypeIndex, shaperParams.m_type);
                m_shaderResourceGroup->SetConstant(m_lutShaperBiasIndex, shaperParams.m_bias);
                m_shaderResourceGroup->SetConstant(m_lutShaperScaleIndex, shaperParams.m_scale);
            }

            HDRColorGradingPass::FrameBeginInternal(params);
        }

        void LutGenerationPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            const auto* colorGradingSettings = GetHDRColorGradingSettings();
            if (colorGradingSettings)
            {
                uint32_t lutResolution = aznumeric_cast<uint32_t>(colorGradingSettings->GetLutResolution());
                RHI::Size lutSize{ lutResolution * lutResolution, lutResolution, 1 };

                RPI::Ptr<RPI::PassAttachment> attachment = FindOwnedAttachment(Name{ "ColorGradingLut" });
                RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
                imageDescriptor.m_size = lutSize;
                SetViewportScissorFromImageSize(lutSize);
            }
            HDRColorGradingPass::BuildCommandListInternal(context);
        }

        bool LutGenerationPass::IsEnabled() const
        {
            const auto* colorGradingSettings = GetHDRColorGradingSettings();
            return colorGradingSettings ? colorGradingSettings->GetGenerateLut() : false;
        }

        void LutGenerationPass::SetViewportScissorFromImageSize(const RHI::Size& imageSize)
        {
            const RHI::Viewport viewport(0.f, imageSize.m_width * 1.f, 0.f, imageSize.m_height * 1.f);
            const RHI::Scissor scissor(0, 0, imageSize.m_width, imageSize.m_height);
            m_viewportState = viewport;
            m_scissorState = scissor;
        }

    } // namespace Render
} // namespace AZ
