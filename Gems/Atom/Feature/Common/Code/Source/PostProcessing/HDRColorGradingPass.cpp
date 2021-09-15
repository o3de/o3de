/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <PostProcessing/HDRColorGradingPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

 namespace AZ
{
    namespace Render
    {
        RPI::Ptr<HDRColorGradingPass> HDRColorGradingPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<HDRColorGradingPass> pass = aznew HDRColorGradingPass(descriptor);
            return AZStd::move(pass);
        }
        
        HDRColorGradingPass::HDRColorGradingPass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }
        
        void HDRColorGradingPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_colorGradingExposureIndex.Reset();
            m_colorGradingContrastIndex.Reset();
            m_colorGradingHueShiftIndex.Reset();
            m_colorGradingPreSaturationIndex.Reset();
            m_colorFilterIntensityIndex.Reset();
            m_colorFilterMultiplyIndex.Reset();
            m_whiteBalanceKelvinIndex.Reset();
            m_whiteBalanceTintIndex.Reset();
            m_splitToneBalanceIndex.Reset();
            m_splitToneMixIndex.Reset();
            m_colorGradingPostSaturationIndex.Reset();
            m_smhShadowsStartIndex.Reset();
            m_smhShadowsEndIndex.Reset();
            m_smhHighlightsStartIndex.Reset();
            m_smhHighlightsEndIndex.Reset();
            m_smhMixIndex.Reset();

            m_channelMixingRedIndex.Reset();
            m_channelMixingGreenIndex.Reset();
            m_channelMixingBlueIndex.Reset();

            m_colorFilterSwatchIndex.Reset();
            m_splitToneShadowsColorIndex.Reset();
            m_splitToneHighlightsColorIndex.Reset();
            m_smhShadowsColorIndex.Reset();
            m_smhMidtonesColorIndex.Reset();
            m_smhHighlightsColorIndex.Reset();
        }
        
        void HDRColorGradingPass::FrameBeginInternal(FramePrepareParams params)
        {
            SetSrgConstants();

            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        bool HDRColorGradingPass::IsEnabled() const
        {
            auto* colorGradingSettings = GetHDRColorGradingSettings();
            return colorGradingSettings ? colorGradingSettings->GetEnabled() : false;
        }

        void HDRColorGradingPass::SetSrgConstants()
        {
            const HDRColorGradingSettings* settings = GetHDRColorGradingSettings();
            if (settings)
            {
                m_shaderResourceGroup->SetConstant(m_colorGradingExposureIndex, settings->GetColorGradingExposure());
                m_shaderResourceGroup->SetConstant(m_colorGradingContrastIndex, settings->GetColorGradingContrast());
                m_shaderResourceGroup->SetConstant(m_colorGradingHueShiftIndex, settings->GetColorGradingHueShift());
                m_shaderResourceGroup->SetConstant(m_colorGradingPreSaturationIndex, settings->GetColorGradingPreSaturation());
                m_shaderResourceGroup->SetConstant(m_colorFilterIntensityIndex, settings->GetColorGradingFilterIntensity());
                m_shaderResourceGroup->SetConstant(m_colorFilterMultiplyIndex, settings->GetColorGradingFilterMultiply());
                m_shaderResourceGroup->SetConstant(m_whiteBalanceKelvinIndex, settings->GetWhiteBalanceKelvin());
                m_shaderResourceGroup->SetConstant(m_whiteBalanceTintIndex, settings->GetWhiteBalanceTint());
                m_shaderResourceGroup->SetConstant(m_splitToneBalanceIndex, settings->GetSplitToneBalance());
                m_shaderResourceGroup->SetConstant(m_splitToneMixIndex, settings->GetSplitToneMix());
                m_shaderResourceGroup->SetConstant(m_colorGradingPostSaturationIndex, settings->GetColorGradingPostSaturation());
                m_shaderResourceGroup->SetConstant(m_smhShadowsStartIndex, settings->GetSmhShadowsStart());
                m_shaderResourceGroup->SetConstant(m_smhShadowsEndIndex, settings->GetSmhShadowsEnd());
                m_shaderResourceGroup->SetConstant(m_smhHighlightsStartIndex, settings->GetSmhHighlightsStart());
                m_shaderResourceGroup->SetConstant(m_smhHighlightsEndIndex, settings->GetSmhHighlightsEnd());
                m_shaderResourceGroup->SetConstant(m_smhMixIndex, settings->GetSmhMix());

                m_shaderResourceGroup->SetConstant(m_channelMixingRedIndex, AZ::Vector3(settings->GetChannelMixingRed(), 0.0f, 0.0f));
                m_shaderResourceGroup->SetConstant(m_channelMixingGreenIndex, AZ::Vector3(0.0f, settings->GetChannelMixingGreen(), 0.0f));
                m_shaderResourceGroup->SetConstant(m_channelMixingBlueIndex, AZ::Vector3(0.0f, 0.0f, settings->GetChannelMixingBlue()));

                m_shaderResourceGroup->SetConstant(m_colorFilterSwatchIndex, settings->GetColorFilterSwatch());
                m_shaderResourceGroup->SetConstant(m_splitToneShadowsColorIndex, settings->GetSplitToneShadowsColor());
                m_shaderResourceGroup->SetConstant(m_splitToneHighlightsColorIndex, settings->GetSplitToneHighlightsColor());
                m_shaderResourceGroup->SetConstant(m_smhShadowsColorIndex, settings->GetSmhShadowsColor());
                m_shaderResourceGroup->SetConstant(m_smhMidtonesColorIndex, settings->GetSmhMidtonesColor());
                m_shaderResourceGroup->SetConstant(m_smhHighlightsColorIndex, settings->GetSmhHighlightsColor());
            }
        }

        const AZ::Render::HDRColorGradingSettings* HDRColorGradingPass::GetHDRColorGradingSettings() const
        {
            RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        const HDRColorGradingSettings* colorGradingSettings = postProcessSettings->GetHDRColorGradingSettings();;
                        if (colorGradingSettings != nullptr && colorGradingSettings->GetEnabled())
                        {
                            return postProcessSettings->GetHDRColorGradingSettings();
                        }
                    }
                }
            }
            return nullptr;
        }
    }
}
