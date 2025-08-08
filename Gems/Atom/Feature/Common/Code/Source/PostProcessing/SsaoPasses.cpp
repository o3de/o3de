/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/AmbientOcclusion/AoSettings.h>
#include <PostProcessing/FastDepthAwareBlurPasses.h>
#include <PostProcessing/SsaoPasses.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/SsaoConstants.h>
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
            if (!ParentPass::IsEnabled())
            {
                return false;
            }
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (!fp)
            {
                return true;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return true;
            }
            const AoSettings* aoSettings = postProcessSettings->GetAoSettings();
            if (!aoSettings)
            {
                return true;
            }
            return aoSettings->GetEnabled() && aoSettings->GetAoMethod() == Ao::AoMethodType::SSAO;
        }

        void SsaoParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();

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
            AZ::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    AoSettings* aoSettings = postProcessSettings->GetAoSettings();
                    if (aoSettings)
                    {
                        bool ssaoEnabled = aoSettings->GetEnabled();
                        bool blurEnabled = ssaoEnabled && aoSettings->GetEnableBlur();
                        bool downsampleEnabled = ssaoEnabled && aoSettings->GetEnableDownsample();

                        m_blurParentPass->SetEnabled(blurEnabled);
                        if (blurEnabled)
                        {
                            float constFalloff = aoSettings->GetBlurConstFalloff();
                            float depthFalloffThreshold = aoSettings->GetBlurDepthFalloffThreshold();
                            float depthFalloffStrength = aoSettings->GetBlurDepthFalloffStrength();

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
        { }

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
                float m_strength = Ao::DefaultSsaoStrength;

                // The sampling radius calculated in screen UV space
                float m_samplingRadius = Ao::DefaultSsaoSamplingRadius;

            } ssaoConstants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    AoSettings* aoSettings = postProcessSettings->GetAoSettings();
                    if (aoSettings)
                    {
                        if (aoSettings->GetEnabled() && aoSettings->GetAoMethod() == Ao::AoMethodType::SSAO)
                        {
                            ssaoConstants.m_strength = aoSettings->GetSsaoStrength();
                            ssaoConstants.m_samplingRadius = aoSettings->GetSsaoSamplingRadius();
                        }
                        else
                        {
                            ssaoConstants.m_strength = 0.0f;
                        }
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "SsaoComputePass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "SsaoComputePass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            ssaoConstants.m_outputSize[0] = size.m_width;
            ssaoConstants.m_outputSize[1] = size.m_height;

            ssaoConstants.m_pixelSize[0] = 1.0f / float(size.m_width);
            ssaoConstants.m_pixelSize[1] = 1.0f / float(size.m_height);

            ssaoConstants.m_halfPixelSize[0] = 0.5f * ssaoConstants.m_pixelSize[0];
            ssaoConstants.m_halfPixelSize[1] = 0.5f * ssaoConstants.m_pixelSize[1];

            m_shaderResourceGroup->SetConstant(m_constantsIndex, ssaoConstants);

            RPI::ComputePass::FrameBeginInternal(params);
        }

    }   // namespace Render
}   // namespace AZ
