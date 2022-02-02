/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/ChromaticAbberationPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ChromaticAbberationPass> ChromaticAbberationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<ChromaticAbberationPass> pass = aznew ChromaticAbberationPass(descriptor);
            return AZStd::move(pass);
        }

        ChromaticAbberationPass::ChromaticAbberationPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool ChromaticAbberationPass::IsEnabled() const
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
                return true;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return true;
            }
            return true;
        }

        void ChromaticAbberationPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in .azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                AZStd::array<float, 2> m_outputCenter;
                float m_strength;
            } constants{};

            // FUTURE PROOFING WIP
            /* RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    ChromaticAbberationSettings* caSettings = postProcessSettings->GetChromaticAbberationSettings();
                    if (caSettings)
                    {
                        if (caSettings->GetEnabled())
                        {
                            constants.m_strength = caSettings->GetStrength();
                            //etc
                        }
                        else
                        {
                            constants.m_strength = 0.0f;
                            //etc
                        }
                    }
                }
            }*/

            AZ_Assert(GetOutputCount() > 0, "ChromaticAbberationPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).m_attachment.get();

            AZ_Assert(outputAttachment != nullptr, "ChromaticAbberationPass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            constants.m_outputSize[0] = size.m_width;
            constants.m_outputSize[1] = size.m_height;
            constants.m_outputCenter[0] = (size.m_width - 1) * 0.5f;
            constants.m_outputCenter[1] = (size.m_height -1) * 0.5f;
            constants.m_strength = 0.02f;

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            RPI::ComputePass::FrameBeginInternal(params);
        }

    } // namespace Render
} // namespace AZ
