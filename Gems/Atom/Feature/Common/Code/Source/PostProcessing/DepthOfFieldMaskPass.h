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
        //! Pass used to prevent the foreground color to smear into the background blur.
        class DepthOfFieldMaskPass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DepthOfFieldMaskPass);

        public:
            AZ_RTTI(AZ::Render::DepthOfFieldMaskPass, "{37025565-2187-4841-8F17-FE90600BA884}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldMaskPass, SystemAllocator);
            virtual ~DepthOfFieldMaskPass() = default;

            //! Creates a DepthOfFieldMaskPass
            static RPI::Ptr<DepthOfFieldMaskPass> Create(const RPI::PassDescriptor& descriptor);

            // Set pass parameter interfaces...
            void SetBlendFactor(const AZStd::array<float, 2>& blendFactor);
            void SetRadiusMinMax(float min, float max);

        protected:
            // Pass behavior overrides...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            DepthOfFieldMaskPass(const RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            RHI::ShaderInputNameIndex m_blendFactorIndex = "m_blendFactor";
            RHI::ShaderInputNameIndex m_inputResolutionInverseIndex = "m_inputResolutionInverse";
            RHI::ShaderInputNameIndex m_radiusMinIndex = "m_radiusMin";
            RHI::ShaderInputNameIndex m_radiusMaxIndex = "m_radiusMax";

            AZStd::array<float, 2> m_blendFactor = {{0.0f, 0.0f}};
            AZStd::array<float, 2> m_inputResolutionInverse = {{0.0f, 0.0f}};
            float m_radiusMin = 0.0f;
            float m_radiusMax = 0.0f;
            
            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        };
    }   // namespace Render
}   // namespace AZ
