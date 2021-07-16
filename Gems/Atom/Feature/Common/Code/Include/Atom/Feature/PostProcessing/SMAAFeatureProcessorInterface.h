/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        //! These are preset enums for SMAA quality settings.
        //! For details please check comment related to SMAA_PRESET_* in SMAA.azsli.
        enum class SMAAQualityPreset : uint32_t
        {
            Low = 0,
            Middle,
            High,
            Ultra,

            Count
        };

        //! These are output modes for neighborhood blending pass in SMAA.
        enum class SMAAOutputMode : uint32_t
        {
            // Apply inverse conversion of provisional tone mapping after blending.
            BlendResultWithProvisionalTonemap = 0,
            // Output blended color directly.
            BlendResult,
            // Directly output a color bound to the InputColorPassThrough texture slot.
            PassThrough,
            // Output edge texture for debug purpose.
            EdgeTexture,
            // Output blend weight texture for debug purpose.
            BlendWeightTexture,

            Count
        };

        //! These are edge detection modes for edge detection pass in SMAA.
        enum class SMAAEdgeDetectionMode : uint32_t
        {
            // Using depth buffer information to detect edge.
            Depth = 0,
            // Using the luminance information calculated from color to detect edge.
            Luma,
            // Using color information directly to detect edge.
            Color,

            Count
        };

        struct SMAAData
        {
            bool m_enable = false;
            SMAAEdgeDetectionMode m_edgeDetectionMode = SMAAEdgeDetectionMode::Color;
            SMAAOutputMode m_outputMode = SMAAOutputMode::BlendResultWithProvisionalTonemap;

            float m_chromaThreshold = 0.1f;
            float m_depthThreshold = 0.01f;
            float m_localContrastAdaptationFactor = 2.0f;
            float m_predicationThreshold = 0.01f;
            float m_predicationScale = 2.0f;
            float m_predicationStrength = 0.4f;

            int m_maxSearchSteps = 32;
            int m_maxSearchStepsDiagonal = 16;
            int m_cornerRounding = 25;

            bool m_predicationEnable = false;
            bool m_enableDiagonalDetection = true;
            bool m_enableCornerDetection = true;
        };

        //! SMAAFeatureProcessorInterface provides an interface to SMAA feature. This is necessary for code outside of
        //! the Atom features gem to communicate with the SMAAFeatureProcessor.
        class SMAAFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SMAAFeatureProcessorInterface, "{7E6A9FB5-E42C-41C3-8F84-40A1D4433A94}");

            //! Enable/disable SMAA feature.
            //! @param enable Flag for SMAA feature.
            virtual void SetEnable(bool enable) = 0;
            //! Sets SMAA quality using preset parameters.
            //! @param preset Preset value for performance and quality of SMAA.
            virtual void SetQualityByPreset(SMAAQualityPreset preset) = 0;
            //! Sets the edge detection mode.
            //! @param mode Edge detection mode.
            virtual void SetEdgeDetectionMode(SMAAEdgeDetectionMode mode) = 0;
            //! Sets the output mode.
            //! @param mode Output mode.
            virtual void SetOutputMode(SMAAOutputMode mode) = 0;

            //! Sets the luma/chroma threshold value which is used by edge detection.
            //! @param threshold Threshold value for edge detection sensitivity. 
            //! For details please check comment related to SMAA_THRESHOLD in SMAA.azsli.
            virtual void SetChromaThreshold(float threshold) = 0;
            //! Sets the depth threshold value which is used by edge detection.
            //! @param threshold Threshold value for depth edge detection sensitivity.
            //! For details please check comment related to SMAA_DEPTH_THRESHOLD in SMAA.azsli.
            virtual void SetDepthThreshold(float threshold) = 0;
            //! Sets the local contrast adaptation factor.
            //! @param factor Tweak value for the local contrast adaptation feature.
            //! For details please check comment related to SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR in SMAA.azsli.
            virtual void SetLocalContrastAdaptationFactor(float factor) = 0;
            //! Enable/disable the predication feature.
            //! @param enable Flag for predication feature.
            //! For details please check comment related to SMAA_PREDICATION in SMAA.azsli.
            virtual void SetPredicationEnable(bool enable) = 0;
            //! Sets the predication threshold value.
            //! @param threshold Threshold value for predication feature.
            //! For details please check comment related to SMAA_PREDICATION_THRESHOLD in SMAA.azsli.
            virtual void SetPredicationThreshold(float threshold) = 0;
            //! Sets the predication scale value.
            //! @param scale Tweak value for predication feature.
            //! For details please check comment related to SMAA_PREDICATION_SCALE in SMAA.azsli.
            virtual void SetPredicationScale(float scale) = 0;
            //! Sets the predication strength value.
            //! @param strength Tweak value for predication feature.
            //! For details please check comment related to SMAA_PREDICATION_STRENGTH in SMAA.azsli.
            virtual void SetPredicationStrength(float strength) = 0;

            //! Sets the search step value in edge search process.
            //! @param steps Number of steps to search edge.
            //! For details please check comment related to SMAA_MAX_SEARCH_STEPS in SMAA.azsli.
            virtual void SetMaxSearchSteps(int steps) = 0;
            //! Sets the search step value in diagonal search process.
            //! @param steps Number of steps to search diagonal edge.
            //! For details please check comment related to SMAA_MAX_SEARCH_STEPS_DIAG in SMAA.azsli.
            virtual void SetMaxSearchStepsDiagonal(int steps) = 0;
            //! Sets the corner rounding value which is used by the sharp geometric feature.
            //! @param cornerRounding Tweak value for the sharp corner feature. 
            //! For details please check comment related to SMAA_CORNER_ROUNDING in SMAA.azsli.
            virtual void SetCornerRounding(int cornerRounding) = 0;
            //! Enable/disable the diagonal edge detection process.
            //! @param enable Flag for diagonal edge detection feature.
            //! For details please check comment related to SMAA_DISABLE_DIAG_DETECTION in SMAA.azsli.
            virtual void SetDiagonalDetectionEnable(bool enable) = 0;
            //! Enable/disable the corner detection which is used by the sharp geometric feature.
            //! @param enable Flag for sharp corner feature.
            //! For details please check comment related to SMAA_DISABLE_CORNER_DETECTION in SMAA.azsli.
            virtual void SetCornerDetectionEnable(bool enable) = 0;

            //! Gets the current settings for SMAA feature.
            //! @return A SMAAData currently used.
            virtual const SMAAData& GetSettings() const = 0;
        };
    } // namespace Render
} // namespace AZ
