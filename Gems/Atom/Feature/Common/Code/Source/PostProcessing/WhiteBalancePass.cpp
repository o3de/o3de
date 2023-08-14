/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/WhiteBalancePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<WhiteBalancePass> WhiteBalancePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<WhiteBalancePass> pass = aznew WhiteBalancePass(descriptor);
            return AZStd::move(pass);
        }

        WhiteBalancePass::WhiteBalancePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool WhiteBalancePass::IsEnabled() const
        {
            if (!ComputePass::IsEnabled())
            {
                return false;
            }
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (!fp)
            {
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const WhiteBalanceSettings* whiteBalanceSettings = postProcessSettings->GetWhiteBalanceSettings();
            if (!whiteBalanceSettings)
            {
                return false;
            }
            return whiteBalanceSettings->GetEnabled();
        }

        void WhiteBalancePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in WhiteBalance.azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                AZStd::array<float, 2> m_outputCenter;
                float m_temperature = WhiteBalance::DefaultTemperature;
                float m_tint = WhiteBalance::DefaultTint;
                AZStd::array<float, 2> m_pad;
            } constants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    WhiteBalanceSettings* whiteBalanceSettings = postProcessSettings->GetWhiteBalanceSettings();
                    if (whiteBalanceSettings)
                    {
                        constants.m_temperature = whiteBalanceSettings->GetTemperature();
                        constants.m_tint = whiteBalanceSettings->GetTint();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "WhiteBalancePass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "WhiteBalancePass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            constants.m_outputSize[0] = size.m_width;
            constants.m_outputSize[1] = size.m_height;
            constants.m_outputCenter[0] = (size.m_width - 1) * 0.5f;
            constants.m_outputCenter[1] = (size.m_height -1) * 0.5f;

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            RPI::ComputePass::FrameBeginInternal(params);
        }
    } // namespace Render
} // namespace AZ
