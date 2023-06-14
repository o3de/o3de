/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        //! Child pass for downsampling DepthLinear mips
        class ReflectionScreenSpaceDownsampleDepthLinearChildPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceDownsampleDepthLinearChildPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceDownsampleDepthLinearChildPass, "{3863028B-3CA9-4F45-A7CC-EA2885593F83}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceDownsampleDepthLinearChildPass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearChildPass> Create(const RPI::PassDescriptor& descriptor);

            void SetMipLevel(uint32_t mipLevel) { m_mipLevel = mipLevel; }

        private:
            explicit ReflectionScreenSpaceDownsampleDepthLinearChildPass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            RHI::ShaderInputNameIndex m_mipLevelNameIndex = "m_mipLevel";
            RHI::ShaderInputNameIndex m_halfResolutionNameIndex = "m_halfResolution";
            RHI::ShaderInputNameIndex m_previousMipLevelNameIndex = "m_previousMipLevel";
            RHI::ShaderInputNameIndex m_previousMipWidthNameIndex = "m_previousMipWidth";
            RHI::ShaderInputNameIndex m_previousMipHeightNameIndex = "m_previousMipHeight";

            bool m_updateSrg = false;
            float m_invOutputScale = 1.0f;
            uint32_t m_mipLevel = 0;
            RHI::Size m_imageSize;
            bool m_halfResolution = false;
        };
    }   // namespace RPI
}   // namespace AZ
