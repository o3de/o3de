/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/MotionBlurPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<MotionBlurPass> MotionBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MotionBlurPass> pass = aznew MotionBlurPass(descriptor);
            return AZStd::move(pass);
        }

        MotionBlurPass::MotionBlurPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool MotionBlurPass::IsEnabled() const
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
            const MotionBlurSettings* MotionBlurSettings = postProcessSettings->GetMotionBlurSettings();
            if (!MotionBlurSettings)
            {
                return false;
            }
            return MotionBlurSettings->GetEnabled();
        }

        void MotionBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in MotionBlur.azsl
            struct Constants
            {
                u32 m_sampleNumber = MotionBlur::DefaultSampleNumber;
                float m_strength = MotionBlur::DefaultStrength;
                AZStd::array<u32, 2> m_outputSize;
            } constants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    MotionBlurSettings* MotionBlurSettings = postProcessSettings->GetMotionBlurSettings();
                    if (MotionBlurSettings)
                    {
                        constants.m_sampleNumber = MotionBlurSettings->GetSampleNumber();
                        constants.m_strength = MotionBlurSettings->GetStrength();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "MotionBlurPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "MotionBlurPass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            constants.m_outputSize[0] = size.m_width;
            constants.m_outputSize[1] = size.m_height;

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            RPI::ComputePass::FrameBeginInternal(params);
        }
    } // namespace Render
} // namespace AZ
