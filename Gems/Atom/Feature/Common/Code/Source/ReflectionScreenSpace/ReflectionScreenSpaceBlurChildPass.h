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
        //! This pass takes performs a separable Gaussian blur of the input reflection image
        //! to the lower mip levels of that image.  The blurred mips are used as roughness levels
        //! when applying reflection data to a surface material.
        class ReflectionScreenSpaceBlurChildPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceBlurChildPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceBlurChildPass, "{238E4D6C-3213-4BA2-8DFE-EAC469346E77}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceBlurChildPass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceBlurChildPass> Create(const RPI::PassDescriptor& descriptor);

            enum PassType
            {
                Vertical,
                Horizontal
            };

            void SetType(PassType passType) { m_passType = passType; }
            void SetMipLevel(uint32_t mipLevel) { m_mipLevel = mipLevel; }

        private:
            explicit ReflectionScreenSpaceBlurChildPass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            RHI::ShaderInputNameIndex m_mipLevelNameIndex = "m_mipLevel";

            bool m_updateSrg = false;
            PassType m_passType;
            uint32_t m_mipLevel = 0;
            RHI::Size m_imageSize;
            float m_invOutputScale = 1.0f;
        };
    }   // namespace RPI
}   // namespace AZ
