/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <DiffuseGlobalIllumination/DiffuseGlobalIlluminationFeatureProcessor.h>

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
                    ->Version(0);
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

        void DiffuseGlobalIlluminationFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            UpdatePasses();
        }

        void DiffuseGlobalIlluminationFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr pipeline)
        {
            UpdatePasses();
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
            AZStd::vector<Name> downsamplePassHierarchy = { Name("DiffuseGlobalIlluminationPass"), Name("DiffuseProbeGridDownsamplePass") };
            RPI::PassHierarchyFilter downsamplePassFilter(downsamplePassHierarchy);
            const AZStd::vector<RPI::Pass*>& downsamplePasses = RPI::PassSystemInterface::Get()->FindPasses(downsamplePassFilter);
            for (RPI::Pass* pass : downsamplePasses)
            {
                for (uint32_t outputIndex = 0; outputIndex < pass->GetOutputCount(); ++outputIndex)
                {
                    RPI::Ptr<RPI::PassAttachment> outputAttachment = pass->GetOutputBinding(outputIndex).m_attachment;
                    RPI::PassAttachmentSizeMultipliers& sizeMultipliers = outputAttachment->m_sizeMultipliers;

                    sizeMultipliers.m_widthMultiplier = sizeMultiplier;
                    sizeMultipliers.m_heightMultiplier = sizeMultiplier;
                }

                // set the output scale on the PassSrg
                RPI::FullscreenTrianglePass* downsamplePass = static_cast<RPI::FullscreenTrianglePass*>(pass);
                auto constantIndex = downsamplePass->GetShaderResourceGroup()->FindShaderInputConstantIndex(Name("m_outputImageScale"));
                downsamplePass->GetShaderResourceGroup()->SetConstant(constantIndex, aznumeric_cast<uint32_t>(1.0f / sizeMultiplier));
            }

            // update the image scale on the DiffuseComposite pass
            AZStd::vector<Name> compositePassHierarchy = { Name("DiffuseGlobalIlluminationPass"), Name("DiffuseCompositePass") };
            RPI::PassHierarchyFilter compositePassFilter(compositePassHierarchy);
            const AZStd::vector<RPI::Pass*>& compositePasses = RPI::PassSystemInterface::Get()->FindPasses(compositePassFilter);
            for (RPI::Pass* pass : compositePasses)
            {
                RPI::FullscreenTrianglePass* compositePass = static_cast<RPI::FullscreenTrianglePass*>(pass);
                auto constantIndex = compositePass->GetShaderResourceGroup()->FindShaderInputConstantIndex(Name("m_imageScale"));
                compositePass->GetShaderResourceGroup()->SetConstant(constantIndex, aznumeric_cast<uint32_t>(1.0f / sizeMultiplier));
            }
        }
    } // namespace Render
} // namespace AZ
