/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        // Class for controlling MotionBlur effect
        class MotionBlurPass final : public RPI::ComputePass
            , public PostProcessingShaderOptionBase
        {
            AZ_RPI_PASS(MotionBlurPass);

        public:
            AZ_RTTI(MotionBlurPass, "{EA58C10C-F2D9-431B-A4A6-EB63A3118690}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MotionBlurPass, SystemAllocator, 0);

            ~MotionBlurPass() = default;
            static RPI::Ptr<MotionBlurPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
            void SetQualityLevel(uint8_t qualityLevel);

        protected:
            // Behaviour functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            MotionBlurPass(const RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            uint8_t m_qualityLevel = 1;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";

            bool m_needToUpdateShaderVariant = true;

            const AZ::Name m_optionName;
            const AZStd::vector<AZ::Name> m_optionValues;
        };
    } // namespace Render
} // namespace AZ
