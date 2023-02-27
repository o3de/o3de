/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        //! Pass used to apply Bokeh Depth of Field blur onto a lighting buffer.
        class DepthOfFieldBokehBlurPass final
            : public RPI::FullscreenTrianglePass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(DepthOfFieldBokehBlurPass);

        public:
            AZ_RTTI(AZ::Render::DepthOfFieldBokehBlurPass, "{B6C292B1-0360-4472-9955-E74CBD5EFC25}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldBokehBlurPass, SystemAllocator);
            virtual ~DepthOfFieldBokehBlurPass() = default;

            //! Creates a DepthOfFieldBokehBlurPass
            static RPI::Ptr<DepthOfFieldBokehBlurPass> Create(const RPI::PassDescriptor& descriptor);

            // Set pass parameter interfaces...
            void SetRadiusMinMax(float min, float max);
            void UpdateSampleTexcoords(uint32_t divisionNumber, float viewAspectRatio);

        protected:
            // Behaviour functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            DepthOfFieldBokehBlurPass(const RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            // SRG binding indices...
            RHI::ShaderInputNameIndex m_sampleNumberIndex = "m_sampleNumber";
            RHI::ShaderInputNameIndex m_radiusMinIndex = "m_radiusMin";
            RHI::ShaderInputNameIndex m_radiusMaxIndex = "m_radiusMax";
            RHI::ShaderInputNameIndex m_sampleTexcoordsRadiusIndex = "m_sampleTexcoordsRadius";

            // maximum number of samples.
            // Sampled in the order of 6, 12, 18, 24 from the center to the periphery.
            // The maximum number od samplingd at this time is 6 + 12 + 18 + 24.
            static constexpr int SampleMax = 60;
            uint32_t m_sampleNumber = 6;
            float m_radiusMin = 0.0f;
            float m_radiusMax = 0.0f;
            AZStd::array<AZStd::array<float, 4>, SampleMax> m_sampleTexcoords;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            bool m_needToUpdateShaderVariant = true;

            const AZ::Name m_optionName;
            const AZStd::vector<AZ::Name> m_optionValues;
        };
    }   // namespace Render
}   // namespace AZ
