/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>

namespace AZ
{
    namespace Render
    {
        // Class for controlling MotionBlur effect
        class MotionBlurPass final : public RPI::ComputePass
        {
            AZ_RPI_PASS(MotionBlurPass);

        public:
            AZ_RTTI(MotionBlurPass, "{EA58C10C-F2D9-431B-A4A6-EB63A3118690}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(MotionBlurPass, SystemAllocator, 0);

            ~MotionBlurPass() = default;
            static RPI::Ptr<MotionBlurPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            MotionBlurPass(const RPI::PassDescriptor& descriptor);
            void InitializeShaderVariant();
            void UpdateCurrentShaderVariant();

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
