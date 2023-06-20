/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Render/DiffuseGlobalIlluminationFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        void DiffuseGlobalIlluminationFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DiffuseGlobalIlluminationFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void DiffuseGlobalIlluminationFeatureProcessor::Activate()
        {
            EnableSceneNotification();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::SetQualityLevel(DiffuseGlobalIlluminationQualityLevel qualityLevel)
        {
            if (qualityLevel >= DiffuseGlobalIlluminationQualityLevel::Count)
            {
                AZ_Assert(false, "SetQualityLevel called with invalid quality level [%d]", qualityLevel);
                return;
            }

            m_qualityLevel = qualityLevel;

            UpdatePasses();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline,
                RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                UpdatePasses();
            }
        }

        void DiffuseGlobalIlluminationFeatureProcessor::UpdatePasses()
        {
            float sizeMultiplier = 0.0f;
            switch (m_qualityLevel)
            {
            case DiffuseGlobalIlluminationQualityLevel::Low:
                sizeMultiplier = 0.25f;
                break;
            case DiffuseGlobalIlluminationQualityLevel::Medium:
                sizeMultiplier = 0.5f;
                break;
            case DiffuseGlobalIlluminationQualityLevel::High:
                sizeMultiplier = 1.0f;
                break;
            default:
                AZ_Assert(false, "Unknown DiffuseGlobalIlluminationQualityLevel [%d]", m_qualityLevel);
                break;
            }

            // update the size multiplier on the DiffuseProbeGridDownsamplePass output
            // NOTE: The ownerScene wasn't added to both filters. This is because the passes from the non-owner scene may have invalid SRG values which could lead to
            // GPU error if the scene doesn't have this feature processor enabled.
            // For example, the ASV MultiScene sample may have TDR.
            {
                AZStd::vector<Name> downsamplePassHierarchy = { Name("DiffuseGlobalIlluminationPass"), Name("DiffuseProbeGridDownsamplePass") };
                RPI::PassFilter downsamplePassFilter = RPI::PassFilter::CreateWithPassHierarchy(downsamplePassHierarchy);
                RPI::PassSystemInterface::Get()->ForEachPass(
                    downsamplePassFilter,
                    [sizeMultiplier](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                    {
                        // update the downsample pass size multipliers
                        for (uint32_t attachmentIndex = 0; attachmentIndex < pass->GetOutputCount(); ++attachmentIndex)
                        {
                            RPI::Ptr<RPI::PassAttachment> attachment = pass->GetOutputBinding(attachmentIndex).GetAttachment();
                            if (attachment)
                            {
                                RPI::PassAttachmentSizeMultipliers& sizeMultipliers = attachment->m_sizeMultipliers;

                                sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                                sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                            }
                        }

                        // set the output scale on the PassSrg
                        RPI::FullscreenTrianglePass* downsamplePass = static_cast<RPI::FullscreenTrianglePass*>(pass);
                        RHI::ShaderInputNameIndex outputImageScaleShaderInput = "m_outputImageScale";
                        downsamplePass->GetShaderResourceGroup()->SetConstant(
                            outputImageScaleShaderInput, aznumeric_cast<uint32_t>(1.0f / sizeMultiplier));

                        // update the parent pass IrradianceImage size multiplier
                        RPI::ParentPass* parentPass = pass->GetParent();
                        RPI::Ptr<RPI::PassAttachment> irradianceImageAttachment;
                        for (uint32_t attachmentIndex = 0; attachmentIndex < parentPass->GetInputOutputCount(); ++attachmentIndex)
                        {
                            RPI::Ptr<RPI::PassAttachment> attachment = parentPass->GetInputOutputBinding(attachmentIndex).GetAttachment();
                            if (attachment && attachment->m_name == Name("IrradianceImage"))
                            {
                                irradianceImageAttachment = attachment;
                                break;
                            }
                        }

                        if (irradianceImageAttachment)
                        {
                            RPI::PassAttachmentSizeMultipliers& sizeMultipliers = irradianceImageAttachment->m_sizeMultipliers;
                            sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                            sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                        }

                        // handle all downsample passes
                        return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }

            // update the image scale on the DiffuseComposite pass
            {
                AZStd::vector<Name> compositePassHierarchy = { Name("DiffuseGlobalIlluminationPass"), Name("DiffuseCompositePass") };
                RPI::PassFilter compositePassFilter = RPI::PassFilter::CreateWithPassHierarchy(compositePassHierarchy);
                RPI::PassSystemInterface::Get()->ForEachPass(compositePassFilter, [sizeMultiplier](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                    {
                        RPI::FullscreenTrianglePass* compositePass = static_cast<RPI::FullscreenTrianglePass*>(pass);
                        RHI::ShaderInputNameIndex imageScaleShaderInput = "m_imageScale";
                        compositePass->GetShaderResourceGroup()->SetConstant(imageScaleShaderInput, aznumeric_cast<uint32_t>(1.0f / sizeMultiplier));

                        return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            }
        }
    } // namespace Render
} // namespace AZ
