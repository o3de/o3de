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
        // Class for controlling White Balance effect
        class WhiteBalancePass final : public RPI::ComputePass
        {
            AZ_RPI_PASS(WhiteBalancePass);

        public:
            AZ_RTTI(WhiteBalancePass, "{15AAF070-3258-4376-9911-CF4E9C7FAF4A}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(WhiteBalancePass, SystemAllocator, 0);

            ~WhiteBalancePass() = default;
            static RPI::Ptr<WhiteBalancePass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            WhiteBalancePass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
