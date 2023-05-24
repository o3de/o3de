/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcessing/SMAAFeatureProcessorInterface.h>

namespace AZ
{

    namespace Render
    {
        static const char* const SMAAConvertToPerceptualColorPassTemplateName = "SMAAConvertToPerceptualColorTemplate";

        //! SMAAFeatureProcessor implementation.
        class SMAAFeatureProcessor final
            : public SMAAFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(SMAAFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::SMAAFeatureProcessor, "55E360D5-4810-4932-A782-7EA9104E9374", AZ::Render::SMAAFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            SMAAFeatureProcessor();
            virtual ~SMAAFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket & packet) override;
            void Render(const RenderPacket & packet) override;

            // SMAAFeatureProcessor overrides ...
            void SetEnable(bool enable) override;
            void SetQualityByPreset(SMAAQualityPreset preset) override;
            void SetEdgeDetectionMode(SMAAEdgeDetectionMode mode) override;
            void SetOutputMode(SMAAOutputMode mode) override;

            void SetChromaThreshold(float threshold) override;
            void SetDepthThreshold(float threshold) override;
            void SetLocalContrastAdaptationFactor(float factor) override;
            void SetPredicationEnable(bool enable) override;
            void SetPredicationThreshold(float threshold) override;
            void SetPredicationScale(float scale) override;
            void SetPredicationStrength(float strength) override;

            void SetMaxSearchSteps(int steps) override;
            void SetMaxSearchStepsDiagonal(int steps) override;
            void SetCornerRounding(int cornerRounding) override;
            void SetDiagonalDetectionEnable(bool enable) override;
            void SetCornerDetectionEnable(bool enable) override;

            const SMAAData& GetSettings() const override;
        private:
            SMAAFeatureProcessor(const SMAAFeatureProcessor&) = delete;

            void UpdateConvertToPerceptualPass();
            void UpdateEdgeDetectionPass();
            void UpdateBlendingWeightCalculationPass();
            void UpdateNeighborhoodBlendingPass();

            static constexpr const char* FeatureProcessorName = "SMAAFeatureProcessor";

            SMAAData m_data;

            const AZ::Name m_convertToPerceptualColorPassTemplateNameId;
            const AZ::Name m_edgeDetectioPassTemplateNameId;
            const AZ::Name m_blendingWeightCalculationPassTemplateNameId;
            const AZ::Name m_neighborhoodBlendingPassTemplateNameId;
        };
    } // namespace Render
} // namespace AZ
