/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The color grading pass.
         */
        class HDRColorGradingPass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(HDRColorGradingPass, "{E68E31A1-DB24-4AFF-A029-456A8B74C03C}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(HDRColorGradingPass, SystemAllocator, 0);

            virtual ~HDRColorGradingPass() = default;

            //! Creates a ColorGradingPass
            static RPI::Ptr<HDRColorGradingPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            HDRColorGradingPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            bool IsEnabled() const override;

            const HDRColorGradingSettings* GetHDRColorGradingSettings() const;
            virtual void SetSrgConstants();
        private:
            RHI::ShaderInputNameIndex m_colorAdjustmentWeightIndex = "m_colorAdjustmentWeight";
            RHI::ShaderInputNameIndex m_colorGradingExposureIndex = "m_colorGradingExposure";
            RHI::ShaderInputNameIndex m_colorGradingContrastIndex = "m_colorGradingContrast";
            RHI::ShaderInputNameIndex m_colorGradingPreSaturationIndex = "m_colorGradingPreSaturation";
            RHI::ShaderInputNameIndex m_colorFilterIntensityIndex = "m_colorFilterIntensity";
            RHI::ShaderInputNameIndex m_colorFilterMultiplyIndex = "m_colorFilterMultiply";
            RHI::ShaderInputNameIndex m_colorFilterSwatchIndex = "m_colorFilterSwatch";

            RHI::ShaderInputNameIndex m_whiteBalanceWeightIndex = "m_whiteBalanceWeight";
            RHI::ShaderInputNameIndex m_whiteBalanceKelvinIndex = "m_whiteBalanceKelvin";
            RHI::ShaderInputNameIndex m_whiteBalanceTintIndex = "m_whiteBalanceTint";
            RHI::ShaderInputNameIndex m_whiteBalanceLuminancePreservationIndex = "m_whiteBalanceLuminancePreservation";

            RHI::ShaderInputNameIndex m_splitToneBalanceIndex = "m_splitToneBalance";
            RHI::ShaderInputNameIndex m_splitToneWeightIndex = "m_splitToneWeight";
            RHI::ShaderInputNameIndex m_splitToneShadowsColorIndex = "m_splitToneShadowsColor";
            RHI::ShaderInputNameIndex m_splitToneHighlightsColorIndex = "m_splitToneHighlightsColor";

            RHI::ShaderInputNameIndex m_smhShadowsStartIndex = "m_smhShadowsStart";
            RHI::ShaderInputNameIndex m_smhShadowsEndIndex = "m_smhShadowsEnd";
            RHI::ShaderInputNameIndex m_smhHighlightsStartIndex = "m_smhHighlightsStart";
            RHI::ShaderInputNameIndex m_smhHighlightsEndIndex = "m_smhHighlightsEnd";
            RHI::ShaderInputNameIndex m_smhWeightIndex = "m_smhWeight";
            RHI::ShaderInputNameIndex m_smhShadowsColorIndex = "m_smhShadowsColor";
            RHI::ShaderInputNameIndex m_smhMidtonesColorIndex = "m_smhMidtonesColor";
            RHI::ShaderInputNameIndex m_smhHighlightsColorIndex = "m_smhHighlightsColor";

            RHI::ShaderInputNameIndex m_channelMixingRedIndex = "m_channelMixingRed";
            RHI::ShaderInputNameIndex m_channelMixingGreenIndex = "m_channelMixingGreen";
            RHI::ShaderInputNameIndex m_channelMixingBlueIndex = "m_channelMixingBlue";

            RHI::ShaderInputNameIndex m_finalAdjustmentWeightIndex = "m_finalAdjustmentWeight";
            RHI::ShaderInputNameIndex m_colorGradingPostSaturationIndex = "m_colorGradingPostSaturation";
            RHI::ShaderInputNameIndex m_colorGradingHueShiftIndex = "m_colorGradingHueShift";
        };
    }   // namespace Render
}   // namespace AZ
