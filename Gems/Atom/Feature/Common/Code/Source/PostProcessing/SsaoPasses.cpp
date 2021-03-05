/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/Ssao/SsaoSettings.h>
#include <PostProcessing/FastDepthAwareBlurPasses.h>
#include <PostProcessing/SsaoPasses.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/Feature/PostProcess/Ssao/SsaoConstants.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        // --- SSAO Parent Pass ---

        RPI::Ptr<SsaoParentPass> SsaoParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsaoParentPass> pass = aznew SsaoParentPass(descriptor);
            return AZStd::move(pass);
        }

        SsaoParentPass::SsaoParentPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        { }

        bool SsaoParentPass::IsEnabled() const
        {
            return ParentPass::IsEnabled();
        }

        void SsaoParentPass::OnBuildAttachmentsFinishedInternal()
        {
            ParentPass::OnBuildAttachmentsFinishedInternal();

            m_blurParentPass = FindChildPass(Name("SsaoBlur"))->AsParent();
            AZ_Assert(m_blurParentPass, "[SsaoParentPass] Could not retrieve parent blur pass.");

            m_blurHorizontalPass = azrtti_cast<FastDepthAwareBlurHorPass*>(m_blurParentPass->FindChildPass(Name("HorizontalBlur")).get());
            m_blurVerticalPass = azrtti_cast<FastDepthAwareBlurVerPass*>(m_blurParentPass->FindChildPass(Name("VerticalBlur")).get());
            m_downsamplePass = FindChildPass(Name("DepthDownsample")).get();
            m_upsamplePass = FindChildPass(Name("Upsample")).get();

            AZ_Assert(m_blurHorizontalPass, "[SsaoParentPass] Could not retrieve horizontal blur pass.");
            AZ_Assert(m_blurVerticalPass, "[SsaoParentPass] Could not retrieve vertical blur pass.");
            AZ_Assert(m_downsamplePass, "[SsaoParentPass] Could not retrieve downsample pass.");
            AZ_Assert(m_upsamplePass, "[SsaoParentPass] Could not retrieve upsample pass.");
        }

        void SsaoParentPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    SsaoSettings* ssaoSettings = postProcessSettings->GetSsaoSettings();
                    if (ssaoSettings)
                    {
                        bool ssaoEnabled = ssaoSettings->GetEnabled();
                        bool blurEnabled = ssaoEnabled && ssaoSettings->GetEnableBlur();
                        bool downsampleEnabled = ssaoEnabled && ssaoSettings->GetEnableDownsample();

                        m_blurParentPass->SetEnabled(blurEnabled);
                        if (blurEnabled)
                        {
                            float constFalloff = ssaoSettings->GetBlurConstFalloff();
                            float depthFalloffThreshold = ssaoSettings->GetBlurDepthFalloffThreshold();
                            float depthFalloffStrength = ssaoSettings->GetBlurDepthFalloffStrength();

                            m_blurHorizontalPass->SetConstants(constFalloff, depthFalloffThreshold, depthFalloffStrength);
                            m_blurVerticalPass->SetConstants(constFalloff, depthFalloffThreshold, depthFalloffStrength);
                        }

                        m_downsamplePass->SetEnabled(downsampleEnabled);
                        m_upsamplePass->SetEnabled(downsampleEnabled);
                    }
                }
            }

            ParentPass::FrameBeginInternal(params);
        }

        // --- SSAO Compute Pass ---

        RPI::Ptr<SsaoComputePass> SsaoComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<SsaoComputePass> pass = aznew SsaoComputePass(descriptor);
            return AZStd::move(pass);
        }

        SsaoComputePass::SsaoComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
            if (m_shaderResourceGroup)
            {
                m_constantsIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name("m_constants"));
            }
        }

        void SsaoComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in SsaoCompute.azsl
            struct SsaoConstants
            {
                // The texture dimensions of SSAO output
                AZStd::array<u32, 2> m_outputSize;

                // The size of a pixel relative to screenspace UV
                // Calculated by taking the inverse of the texture dimensions
                AZStd::array<float, 2> m_pixelSize;

                // The size of half a pixel relative to screenspace UV
                AZStd::array<float, 2> m_halfPixelSize;

                // The strength of the SSAO effect
                float m_strength = Ssao::DefaultStrength;

                // The sampling radius calculated in screen UV space 
                float m_samplingRadius = Ssao::DefaultSamplingRadius;

            } ssaoConstants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    SsaoSettings* ssaoSettings = postProcessSettings->GetSsaoSettings();
                    if (ssaoSettings)
                    {
                        if (ssaoSettings->GetEnabled())
                        {
                            ssaoConstants.m_strength = ssaoSettings->GetStrength();
                            ssaoConstants.m_samplingRadius = ssaoSettings->GetSamplingRadius();
                        }
                        else
                        {
                            ssaoConstants.m_strength = 0.0f;
                        }
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "SsaoComputePass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).m_attachment.get();

            AZ_Assert(outputAttachment != nullptr, "SsaoComputePass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            ssaoConstants.m_outputSize[0] = size.m_width;
            ssaoConstants.m_outputSize[1] = size.m_height;

            ssaoConstants.m_pixelSize[0] = 1.0f / float(size.m_width);
            ssaoConstants.m_pixelSize[1] = 1.0f / float(size.m_height);

            ssaoConstants.m_halfPixelSize[0] = 0.5f * ssaoConstants.m_pixelSize[0];
            ssaoConstants.m_halfPixelSize[1] = 0.5f * ssaoConstants.m_pixelSize[1];

            if (m_constantsIndex.IsValid())
            {
                m_shaderResourceGroup->SetConstant(m_constantsIndex, ssaoConstants);
            }

            RPI::ComputePass::FrameBeginInternal(params);
        }

    }   // namespace Render
}   // namespace AZ
