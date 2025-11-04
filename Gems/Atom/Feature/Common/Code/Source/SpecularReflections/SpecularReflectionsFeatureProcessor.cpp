/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SpecularReflections/SpecularReflectionsFeatureProcessor.h>
#include <Atom/Feature/RayTracing/RayTracingPass.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <ReflectionScreenSpace/ReflectionScreenSpacePass.h>

namespace AZ
{
    namespace Render
    {
        void SpecularReflectionsFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SpecularReflectionsFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void SpecularReflectionsFeatureProcessor::Activate()
        {
            EnableSceneNotification();
        }

        void SpecularReflectionsFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
        }

        void SpecularReflectionsFeatureProcessor::SetSSROptions(const SSROptions& ssrOptions)
        {
            m_ssrOptions = ssrOptions;

            UpdatePasses();
        }

        void SpecularReflectionsFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline,
            RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                UpdatePasses();
            }
        }

        void SpecularReflectionsFeatureProcessor::UpdatePasses()
        {
            // disable raytracing if the platform does not support it
            if (RHI::RHISystemInterface::Get()->GetRayTracingSupport() == RHI::MultiDevice::NoDevices)
            {
                m_ssrOptions.m_reflectionMethod = SSROptions::ReflectionMethod::ScreenSpace;
            }

            // determine size multiplier to pass to the shaders
            float sizeMultiplier = m_ssrOptions.m_halfResolution ? 0.5f : 1.0f;

            // parent SSR pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpacePass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this, sizeMultiplier](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.m_enable);

                        // reset frame delay if screenspace reflections are disabled
                        if (!m_ssrOptions.m_enable)
                        {
                            ReflectionScreenSpacePass* reflectionScreenSpacePass = azrtti_cast<ReflectionScreenSpacePass*>(pass);
                            reflectionScreenSpacePass->ResetFrameDelay();
                        }

                        // size multiplier
                        static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("RayTracingCoordsOutput"), AZ::Name("FallbackColor") };
                        for (const auto& attachmentName : attachmentNames)
                        {
                            RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                            if (attachmentBinding)
                            {
                                RPI::Ptr<RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                if (attachment.get())
                                {
                                    RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                }
                            }
                        }

                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // copy framebuffer pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionCopyFrameBufferPass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.m_enable);
                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // raytracing pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceRayTracingPass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this, sizeMultiplier](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.IsRayTracingEnabled());

                        if (m_ssrOptions.IsRayTracingEnabled())
                        {
                            // options
                            RayTracingPass* rayTracingPass = azrtti_cast<RayTracingPass*>(pass);
                            rayTracingPass->SetMaxRayLength(m_ssrOptions.m_maxRayDistance);

                            Data::Instance<RPI::ShaderResourceGroup> rayTracingPassSrg = rayTracingPass->GetShaderResourceGroup();

                            rayTracingPassSrg->SetConstant(m_invOutputScaleNameIndex, 1.0f / m_ssrOptions.GetOutputScale());
                            rayTracingPassSrg->SetConstant(m_maxRoughnessNameIndex, m_ssrOptions.m_maxRoughness);
                            rayTracingPassSrg->SetConstant(m_reflectionMethodNameIndex, m_ssrOptions.m_reflectionMethod);
                            rayTracingPassSrg->SetConstant(m_rayTraceFallbackSpecularNameIndex, m_ssrOptions.m_rayTraceFallbackSpecular);

                            // size multiplier
                            static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("FallbackAlbedo"), AZ::Name("FallbackPosition"), AZ::Name("FallbackNormal") };
                            for (const auto& attachmentName : attachmentNames)
                            {
                                RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                                if (attachmentBinding)
                                {
                                    RPI::Ptr<RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                    if (attachment.get())
                                    {
                                        RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                        sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                        sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                    }
                                }
                            }
                        }

                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // trace pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceTracePass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [sizeMultiplier](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // size multiplier
                        static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("ScreenSpaceReflectionOutput"), AZ::Name("TraceCoordsOutput") };
                        for (const auto& attachmentName : attachmentNames)
                        {
                            RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                            if (attachmentBinding)
                            {
                                RPI::Ptr<RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                if (attachment.get())
                                {
                                    RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                }
                            }
                        }

                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // downsample depth linear pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceDownsampleDepthLinearPass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [sizeMultiplier](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // size multiplier
                        RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(AZ::Name("DownsampledDepthLinearInputOutput"));

                        if (attachmentBinding)
                        {
                            RPI::Ptr<RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                            if (attachment.get())
                            {
                                RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;
            
                                sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                            }
                        }
            
                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // filter pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceFilterPass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [sizeMultiplier](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // size multiplier
                        static AZStd::vector<AZ::Name> attachmentNames = { AZ::Name("Output") };
                        for (const auto& attachmentName : attachmentNames)
                        {
                            RPI::PassAttachmentBinding* attachmentBinding = pass->FindAttachmentBinding(attachmentName);

                            if (attachmentBinding)
                            {
                                RPI::Ptr<RPI::PassAttachment> attachment = attachmentBinding->GetAttachment();
                                if (attachment.get())
                                {
                                    RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                                }
                            }
                        }

                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // copy history pass
            {
                AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(AZ::Name("ReflectionScreenSpaceCopyHistoryPass"), (AZ::RPI::Scene*) nullptr);
                AZ::RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
                    {
                        // enable/disable
                        pass->SetEnabled(m_ssrOptions.m_temporalFiltering);
                        return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }
        }
    } // namespace Render
} // namespace AZ
