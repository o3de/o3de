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
            RHI::ShaderInputNameIndex m_identityLut16x16x16Index = "m_identityLut16x16x16";
            RHI::ShaderInputNameIndex m_identityLut32x32x32Index = "m_identityLut32x32x32";
            RHI::ShaderInputNameIndex m_identityLut64x64x64Index = "m_identityLut64x64x64";

            DisplayMapperAssetLut m_colorGradingLut;

            bool m_isInitialized = false;
        };

    } // namespace Render
} // namespace AZ
