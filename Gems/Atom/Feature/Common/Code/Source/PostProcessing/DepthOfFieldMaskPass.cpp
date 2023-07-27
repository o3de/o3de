/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcessing/DepthOfFieldMaskPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<DepthOfFieldMaskPass> DepthOfFieldMaskPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DepthOfFieldMaskPass> pass = aznew DepthOfFieldMaskPass(descriptor);
            return pass;
        }

        DepthOfFieldMaskPass::DepthOfFieldMaskPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void DepthOfFieldMaskPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_blendFactorIndex.Reset();
            m_inputResolutionInverseIndex.Reset();
            m_radiusMinIndex.Reset();
            m_radiusMaxIndex.Reset();
        }

        void DepthOfFieldMaskPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    DepthOfFieldSettings* dofSettings = postProcessSettings->GetDepthOfFieldSettings();
                    if (dofSettings)
                    {
                        AZ::RHI::Handle<uint32_t> splitSize = dofSettings->GetSplitSizeForPass(GetName());

                        if (splitSize.IsValid())
                        {
                            switch (splitSize.GetIndex())
                            {
                            case 2:
                                SetBlendFactor(dofSettings->m_configurationToViewSRG.m_backBlendFactorDivision2);
                                SetRadiusMinMax(dofSettings->m_minBokehRadiusDivision2, dofSettings->m_maxBokehRadiusDivision2);
                                break;
                            case 4:
                                SetBlendFactor(dofSettings->m_configurationToViewSRG.m_backBlendFactorDivision4);
                                SetRadiusMinMax(dofSettings->m_minBokehRadiusDivision4, dofSettings->m_maxBokehRadiusDivision4);
                                break;
                            case 8:
                                SetBlendFactor(dofSettings->m_configurationToViewSRG.m_backBlendFactorDivision8);
                                SetRadiusMinMax(dofSettings->m_minBokehRadiusDivision8, dofSettings->m_maxBokehRadiusDivision8);
                                break;
                            default:
                                AZ_Assert(false, "DepthOfFieldMaskPass : Failed to get the division number from pass request name for mask.");
                                break;
                            }
                        }
                    }
                }
            }
            RPI::FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void DepthOfFieldMaskPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            // Update resolution size
            const auto attachmentBindings = GetAttachmentBindings();
            const RPI::PassAttachmentBinding& attachmentBinding = attachmentBindings[0];
            if (attachmentBinding.GetAttachment())
            {
                RHI::Size size = attachmentBinding.GetAttachment()->m_descriptor.m_image.m_size;
                m_inputResolutionInverse[0] = 1.0f / size.m_width;
                m_inputResolutionInverse[1] = 1.0f / size.m_height;
            }

            AZ_Assert(m_shaderResourceGroup, "DepthOfFieldMaskPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            m_shaderResourceGroup->SetConstant(m_blendFactorIndex, m_blendFactor);
            m_shaderResourceGroup->SetConstant(m_inputResolutionInverseIndex, m_inputResolutionInverse);
            m_shaderResourceGroup->SetConstant(m_radiusMinIndex, m_radiusMin);
            m_shaderResourceGroup->SetConstant(m_radiusMaxIndex, m_radiusMax);

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void DepthOfFieldMaskPass::SetBlendFactor(const AZStd::array<float, 2>& blendFactor)
        {
            m_blendFactor = blendFactor;
        }

        void DepthOfFieldMaskPass::SetRadiusMinMax(float min, float max)
        {
            m_radiusMin = min;
            m_radiusMax = max;
        }

    }   // namespace Render
}   // namespace AZ
