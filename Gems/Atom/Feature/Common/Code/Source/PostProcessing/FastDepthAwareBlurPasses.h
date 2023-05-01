/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! Must match the struct in FastDepthAwareBlurHor.azsl and FastDepthAwareBlurVer.azsl
        struct FastDepthAwareBlurPassConstants
        {
            //! The texture dimensions of blur output
            AZStd::array<u32, 2> m_outputSize;

            //! The size of a pixel relative to screenspace UV
            //! Calculated by taking the inverse of the texture dimensions
            AZStd::array<float, 2> m_pixelSize;

            //! The size of half a pixel relative to screenspace UV
            AZStd::array<float, 2> m_halfPixelSize;

            //! How much a value is reduced from pixel to pixel on a perfectly flat surface
            float m_constFalloff = 2.0f / 3.0f;

            //! Threshold used to reduce computed depth difference during blur and thus the depth falloff
            //! Can be thought of as a bias that blurs curved surfaces more like flat surfaces
            //! but generally not needed and can be set to 0.0f
            float m_depthFalloffThreshold = 0.0f;

            //! How much the difference in depth slopes between pixels affects the blur falloff.
            //! The higher this value, the sharper edges will appear
            float m_depthFalloffStrength = 50.0f;

            u32 PADDING[3];

            //! Calculates size constants based on texture size input
            void InitializeFromSize(RHI::Size outputTextureSize);

            //! Sets the constants to be forwarded to the shader
            void SetConstants(float constFalloff, float depthFalloffThreshold, float depthFalloffStrength);
        };

        //! Pass for an optimized horizontal blur with edge detection
        class FastDepthAwareBlurHorPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(FastDepthAwareBlurHorPass);

        public:
            AZ_RTTI(AZ::Render::FastDepthAwareBlurHorPass, "{934F3772-06DA-42E3-A305-2921FFCEDCD4}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(FastDepthAwareBlurHorPass, SystemAllocator);
            virtual ~FastDepthAwareBlurHorPass() = default;

            //! Creates an FastDepthAwareBlurHorPass
            static RPI::Ptr<FastDepthAwareBlurHorPass> Create(const RPI::PassDescriptor& descriptor);

            //! Sets the constants to be forwarded to the shader
            void SetConstants(float constFalloff, float depthFalloffThreshold, float depthFalloffStrength);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            FastDepthAwareBlurHorPass(const RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";

            FastDepthAwareBlurPassConstants m_passConstants;
        };

        //! Pass for an optimized vertical blur with edge detection
        class FastDepthAwareBlurVerPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(FastDepthAwareBlurVerPass);

        public:
            AZ_RTTI(AZ::Render::FastDepthAwareBlurVerPass, "{0DCB71EB-5417-4351-AADE-444DBCDF980E}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(FastDepthAwareBlurVerPass, SystemAllocator);
            virtual ~FastDepthAwareBlurVerPass() = default;

            //! Creates an FastDepthAwareBlurVerPass
            static RPI::Ptr<FastDepthAwareBlurVerPass> Create(const RPI::PassDescriptor& descriptor);

            void SetConstants(float constFalloff, float depthFalloffThreshold, float depthFalloffStrength);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            FastDepthAwareBlurVerPass(const RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";

            FastDepthAwareBlurPassConstants m_passConstants;
        };
    }   // namespace Render
}   // namespace AZ
