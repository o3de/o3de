/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/PostProcessing/SMAAFeatureProcessorInterface.h>
#include <PostProcessing/SMAABasePass.h>

namespace AZ
{
    namespace Render
    {
        static const char* const SMAAEdgeDetectionPassTemplateName = "SMAAEdgeDetectionTemplate";

        //! There are three methods for edge detection. The first one uses depth information, second one uses luma information which is calculated
        //! from color value and the third one uses color information. Also, a predication option can be used when using the second and
        //! the third method are used. Detected edges information will be output as an edge texture.
        class SMAAEdgeDetectionPass final
            : public SMAABasePass
        {
        public:
            AZ_RTTI(SMAAEdgeDetectionPass, "{26D07086-9938-4FAB-A212-BB3CB4166641}", SMAABasePass);
            AZ_CLASS_ALLOCATOR(SMAAEdgeDetectionPass, SystemAllocator);
            virtual ~SMAAEdgeDetectionPass() = default;

            //! Creates a SMAAEdgeDetectionPass
            static RPI::Ptr<SMAAEdgeDetectionPass> Create(const RPI::PassDescriptor& descriptor);

            void SetEdgeDetectionMode(SMAAEdgeDetectionMode mode);
            void SetChromaThreshold(float threshold);
            void SetDepthThreshold(float threshold);
            void SetLocalContrastAdaptationFactor(float factor);
            void SetPredicationEnable(bool enable);
            void SetPredicationThreshold(float threshold);
            void SetPredicationScale(float scale);
            void SetPredicationStrength(float strength);

        private:
            SMAAEdgeDetectionPass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides
            void InitializeInternal() override;

            // SMAABasePass functions...
            void UpdateSRG() override;
            void GetCurrentShaderOption(AZ::RPI::ShaderOptionGroup& shaderOption) const override;

            RHI::ShaderInputNameIndex m_renderTargetMetricsShaderInputIndex = "m_renderTargetMetrics";
            RHI::ShaderInputNameIndex m_chromaThresholdShaderInputIndex = "m_chromaThreshold";
            RHI::ShaderInputNameIndex m_depthThresholdShaderInputIndex = "m_depthThreshold";
            RHI::ShaderInputNameIndex m_localContrastAdaptationFactorShaderInputIndex = "m_localContrastAdaptationFactor";
            RHI::ShaderInputNameIndex m_predicationThresholdShaderInputIndex = "m_predicationThreshold";
            RHI::ShaderInputNameIndex m_predicationScaleShaderInputIndex = "m_predicationScale";
            RHI::ShaderInputNameIndex m_predicationStrengthShaderInputIndex = "m_predicationStrength";

            const AZ::Name m_enablePredicationFeatureOptionName;
            const AZ::Name m_edgeDetectionModeOptionName;

            // This is a threshold value for edge detection sensitivity. For details please check comment related to SMAA_THRESHOLD in SMAA.azsli.
            float m_chromaThreshold = 0.1f;
            // This is a threshold value for depth edge detection sensitivity. For details please check comment related to SMAA_DEPTH_THRESHOLD in SMAA.azsli.
            float m_depthThreshold = 0.01f;
            // This is a tweak value for the local contrast adaptation feature. For details please check comment related to SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR in SMAA.azsli.
            float m_localContrastAdaptationFactor = 2.0f;
            // This is a threshold value for predication feature. For details please check comment related to SMAA_PREDICATION_THRESHOLD in SMAA.azsli.
            float m_predicationThreshold = 0.01f;
            // This is a tweak value for predication feature. For details please check comment related to SMAA_PREDICATION_SCALE in SMAA.azsli.
            float m_predicationScale = 2.0f;
            // This is a tweak value for predication feature. For details please check comment related to SMAA_PREDICATION_STRENGTH in SMAA.azsli.
            float m_predicationStrength = 0.4f;
            // This is a edge detection mode variable.
            SMAAEdgeDetectionMode m_edgeDetectionMode = SMAAEdgeDetectionMode::Color;
            // A flag for predication feature. For details please check comment related to SMAA_PREDICATION in SMAA.azsli.
            bool m_predicationEnable = false;
        };
    }   // namespace Render
}   // namespace AZ
