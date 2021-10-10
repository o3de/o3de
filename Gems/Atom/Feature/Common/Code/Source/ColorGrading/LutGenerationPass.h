/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PostProcessing/HDRColorGradingPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class LutGenerationPass
            : public AZ::Render::HDRColorGradingPass
        {
        public:
            static const int NumLuts = 3;

            AZ_RTTI(LutGenerationPass, "{C21DABA8-B538-4C80-BA18-5B97CC9259E5}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(LutGenerationPass, SystemAllocator, 0);

            virtual ~LutGenerationPass() = default;

            //! Creates a ColorGradingPass
            static RPI::Ptr<LutGenerationPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            LutGenerationPass(const RPI::PassDescriptor& descriptor);

            void BuildInternal() override;
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;
            bool IsEnabled() const override;
        private:
            // Set viewport scissor based on output LUT resolution
            void SetViewportScissorFromImageSize(const RHI::Size& imageSize);

            DisplayMapperAssetLut m_colorGradingLuts[NumLuts];
            RHI::Size m_colorGradingLutSizes[NumLuts];

            const char* const LutIdentityProductPath[NumLuts] = {
                "lookuptables/lut_identitylinear_16x16x16.azasset",
                "lookuptables/lut_identitylinear_32x32x32.azasset",
                "lookuptables/lut_identitylinear_64x64x64.azasset" };

            RHI::ShaderInputNameIndex m_identityLutIndices[NumLuts] = {
                "m_identityLut16x16x16",
                "m_identityLut32x32x32",
                "m_identityLut64x64x64" };
            RHI::ShaderInputNameIndex m_lutResolutionIndex = "m_lutResolution";
            RHI::ShaderInputNameIndex m_lutShaperTypeIndex = "m_shaperType";
            RHI::ShaderInputNameIndex m_lutShaperBiasIndex = "m_shaperBias";
            RHI::ShaderInputNameIndex m_lutShaperScaleIndex = "m_shaperScale";

            bool m_isInitialized = false;
        };

    } // namespace Render
} // namespace AZ
