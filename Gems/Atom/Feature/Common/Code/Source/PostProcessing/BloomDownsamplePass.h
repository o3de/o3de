/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>

namespace AZ
{
    namespace Render
    {
        class ShaderResourceGroup;

        class BloomDownsamplePass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(BloomDownsamplePass);

        public:
            AZ_RTTI(BloomDownsamplePass, "{D1CA5F45-70DB-4130-B5FA-147EFB010B1F}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomDownsamplePass, SystemAllocator);
            virtual ~BloomDownsamplePass() = default;

            //! Creates a BloomDownsamplePass
            static RPI::Ptr<BloomDownsamplePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            BloomDownsamplePass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void BuildOutAttachmentBinding();
            AZ::Vector4 CalThresholdConstants();

            // output texture vertical dimension required by compute shader
            AZ::RHI::ShaderInputNameIndex m_sourceImageTexelSizeInputIndex = "m_sourceImageTexelSize";
            AZ::RHI::ShaderInputNameIndex m_thresholdConstantsInputIndex = "m_thresholdConstants";

            float m_threshold = AZ::Render::Bloom::DefaultThreshold;
            float m_knee = AZ::Render::Bloom::DefaultKnee;

            bool m_srgNeedsUpdate = true;
        };
    }   // namespace RPI
}   // namespace AZ
