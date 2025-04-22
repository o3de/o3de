/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/VignettePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<VignettePass> VignettePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<VignettePass> pass = aznew VignettePass(descriptor);
            return AZStd::move(pass);
        }

        VignettePass::VignettePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool VignettePass::IsEnabled() const
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
            const VignetteSettings* VignetteSettings = postProcessSettings->GetVignetteSettings();
            if (!VignetteSettings)
            {
                return false;
            }
            return VignetteSettings->GetEnabled();
        }

        void VignettePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in Vignette.azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                AZStd::array<float, 2> m_outputCenter;
                float m_strength = Vignette::DefaultIntensity;
                AZStd::array<float, 3> m_pad;
            } constants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    VignetteSettings* VignetteSettings = postProcessSettings->GetVignetteSettings();
                    if (VignetteSettings)
                    {
                        constants.m_strength = VignetteSettings->GetIntensity();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "VignettePass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "VignettePass: Output binding has no attachment!");
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
