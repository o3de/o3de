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
#include <PostProcessing/GtaoPasses.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/Feature/PostProcess/AmbientOcclusion/GtaoConstants.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        // --- GTAO Parent Pass ---

        RPI::Ptr<GtaoParentPass> GtaoParentPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<GtaoParentPass> pass = aznew GtaoParentPass(descriptor);
            return AZStd::move(pass);
        }

        GtaoParentPass::GtaoParentPass(const RPI::PassDescriptor& descriptor)
            : RPI::ParentPass(descriptor)
        { }

        bool GtaoParentPass::IsEnabled() const
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
            return aoSettings->GetEnabled() && aoSettings->GetAoMethod() == Ao::AoMethodType::GTAO;
        }

        void GtaoParentPass::InitializeInternal()
        {
            ParentPass::InitializeInternal();

            m_blurParentPass = FindChildPass(Name("GtaoBlur"))->AsParent();
            AZ_Assert(m_blurParentPass, "[GtaoParentPass] Could not retrieve parent blur pass.");

            m_blurHorizontalPass = azrtti_cast<FastDepthAwareBlurHorPass*>(m_blurParentPass->FindChildPass(Name("HorizontalBlur")).get());
            m_blurVerticalPass = azrtti_cast<FastDepthAwareBlurVerPass*>(m_blurParentPass->FindChildPass(Name("VerticalBlur")).get());
            m_downsamplePass = FindChildPass(Name("DepthDownsample")).get();
            m_upsamplePass = FindChildPass(Name("Upsample")).get();

            AZ_Assert(m_blurHorizontalPass, "[GtaoParentPass] Could not retrieve horizontal blur pass.");
            AZ_Assert(m_blurVerticalPass, "[GtaoParentPass] Could not retrieve vertical blur pass.");
            AZ_Assert(m_downsamplePass, "[GtaoParentPass] Could not retrieve downsample pass.");
            AZ_Assert(m_upsamplePass, "[GtaoParentPass] Could not retrieve upsample pass.");
        }

        void GtaoParentPass::FrameBeginInternal(FramePrepareParams params)
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
                        bool gtaoEnabled = aoSettings->GetEnabled();
                        bool blurEnabled = gtaoEnabled && aoSettings->GetEnableBlur();
                        bool downsampleEnabled = gtaoEnabled && aoSettings->GetEnableDownsample();

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

        // --- GTAO Compute Pass ---

        RPI::Ptr<GtaoComputePass> GtaoComputePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<GtaoComputePass> pass = aznew GtaoComputePass(descriptor);
            return AZStd::move(pass);
        }

        GtaoComputePass::GtaoComputePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        { }

        void GtaoComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in GtaoCompute.azsl
            struct GtaoConstants
            {
                // The texture dimensions of GTAO output
                AZStd::array<u32, 2> m_outputSize;

                // The size of a pixel relative to screenspace UV
                // Calculated by taking the inverse of the texture dimensions
                AZStd::array<float, 2> m_pixelSize;

                // The size of half a pixel relative to screenspace UV
                AZStd::array<float, 2> m_halfPixelSize;

                // The strength of the GTAO effect
                float m_strength = Gtao::DefaultGtaoStrength;

                // Power of the GTAO effect
                float m_power = Gtao::DefaultGtaoPower;

                // World radius of effect
                float m_worldRadius = Gtao::DefaultGtaoWorldRadius;

                // Max pixel depth where AO effect is still calculated
                float m_maxDepth = Gtao::DefaultGtaoMaxDepth;

                // A heuristic to bias occlusion for thin or thick objects
                float m_thicknessBlend = Gtao::DefaultGtaoThicknessBlend;

                // FOV correction factor for world radius
                float m_fovScale;

            } gtaoConstants{};

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
                        if (aoSettings->GetEnabled() && aoSettings->GetAoMethod() == Ao::AoMethodType::GTAO)
                        {
                            gtaoConstants.m_strength = aoSettings->GetGtaoStrength();
                            gtaoConstants.m_power = aoSettings->GetGtaoPower();
                            gtaoConstants.m_worldRadius = aoSettings->GetGtaoWorldRadius();
                            gtaoConstants.m_maxDepth = aoSettings->GetGtaoMaxDepth();
                            gtaoConstants.m_thicknessBlend = aoSettings->GetGtaoThicknessBlend();
                        }
                        else
                        {
                            gtaoConstants.m_strength = 0.0f;
                        }
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "GtaoComputePass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "GtaoComputePass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            // Element 0, 0 of the projection matrix is equal to `1/tan(fovX/2)`
            float invTanHalfFovX = view->GetViewToClipMatrix().GetElement(0, 0);
            gtaoConstants.m_fovScale = invTanHalfFovX * float(size.m_height);

            gtaoConstants.m_outputSize[0] = size.m_width;
            gtaoConstants.m_outputSize[1] = size.m_height;

            gtaoConstants.m_pixelSize[0] = 1.0f / float(size.m_width);
            gtaoConstants.m_pixelSize[1] = 1.0f / float(size.m_height);

            gtaoConstants.m_halfPixelSize[0] = 0.5f * gtaoConstants.m_pixelSize[0];
            gtaoConstants.m_halfPixelSize[1] = 0.5f * gtaoConstants.m_pixelSize[1];

            m_shaderResourceGroup->SetConstant(m_constantsIndex, gtaoConstants);

            RPI::ComputePass::FrameBeginInternal(params);
        }

    }   // namespace Render
}   // namespace AZ
