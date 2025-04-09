/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/PaniniProjectionPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<PaniniProjectionPass> PaniniProjectionPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<PaniniProjectionPass> pass = aznew PaniniProjectionPass(descriptor);
            return AZStd::move(pass);
        }

        PaniniProjectionPass::PaniniProjectionPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        bool PaniniProjectionPass::IsEnabled() const
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
            const PaniniProjectionSettings* PaniniProjectionSettings = postProcessSettings->GetPaniniProjectionSettings();
            if (!PaniniProjectionSettings)
            {
                return false;
            }
            return PaniniProjectionSettings->GetEnabled();
        }

        void PaniniProjectionPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in PaniniProjection.azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                AZStd::array<float, 2> m_outputCenter;
                float m_depth = PaniniProjection::DefaultDepth;
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
                    PaniniProjectionSettings* PaniniProjectionSettings = postProcessSettings->GetPaniniProjectionSettings();
                    if (PaniniProjectionSettings)
                    {
                        constants.m_depth = PaniniProjectionSettings->GetDepth();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "PaniniProjectionPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "PaniniProjectionPass: Output binding has no attachment!");
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
