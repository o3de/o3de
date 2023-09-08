/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/ChromaticAberrationPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ChromaticAberrationPass> ChromaticAberrationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ChromaticAberrationPass> pass = aznew ChromaticAberrationPass(descriptor);
            return AZStd::move(pass);
        }

        ChromaticAberrationPass::ChromaticAberrationPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool ChromaticAberrationPass::IsEnabled() const
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
            if (!fp)
            {
                return false;
            }
            const RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const ChromaticAberrationSettings* chromaticAberrationSettings = postProcessSettings->GetChromaticAberrationSettings();
            if (!chromaticAberrationSettings)
            {
                return false;
            }
            return chromaticAberrationSettings->GetEnabled();
        }

        void ChromaticAberrationPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in ChromaticAberration.azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                AZStd::array<float, 2> m_outputCenter;
                float m_strength = ChromaticAberration::DefaultStrength;
                float m_blend = ChromaticAberration::DefaultBlend;
            } constants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    ChromaticAberrationSettings* chromaticAberrationSettings = postProcessSettings->GetChromaticAberrationSettings();
                    if (chromaticAberrationSettings)
                    {
                        constants.m_strength = chromaticAberrationSettings->GetStrength();
                        constants.m_blend = chromaticAberrationSettings->GetBlend();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "ChromaticAberrationPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "ChromaticAberrationPass: Output binding has no attachment!");
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
