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
        // Class for controlling Vignette effect
        class VignettePass final : public RPI::ComputePass
        {
            AZ_RPI_PASS(VignettePass);

        public:
            AZ_RTTI(VignettePass, "{69228453-67F9-473D-ACD6-FA712A23FD23}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(VignettePass, SystemAllocator, 0);

            ~VignettePass() = default;
            static RPI::Ptr<VignettePass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            VignettePass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
