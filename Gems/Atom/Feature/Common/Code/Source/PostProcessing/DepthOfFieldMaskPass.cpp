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

        void DepthOfFieldMaskPass::Init()
        {
            FullscreenTrianglePass::Init();

            m_blendFactorIndex.Reset();
            m_inputResolutionInverseIndex.Reset();
            m_radiusMinIndex.Reset();
            m_radiusMaxIndex.Reset();
        }

        void DepthOfFieldMaskPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
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
            const RPI::PassAttachmentBinding& attachmentBinding = GetAttachmentBindings()[0];
            if (attachmentBinding.m_attachment)
            {
                RHI::Size size = attachmentBinding.m_attachment->m_descriptor.m_image.m_size;
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
