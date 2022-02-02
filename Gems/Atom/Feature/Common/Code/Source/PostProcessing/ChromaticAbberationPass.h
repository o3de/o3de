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
        class ChromaticAbberationPass final : public RPI::ComputePass
        {
            AZ_RPI_PASS(ChromaticAbberationPass);

        public:
            AZ_RTTI(ChromaticAbberationPass, "{557EF771-7D60-4EF1-BD61-E2446237B85B}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(ChromaticAbberationPass, SystemAllocator, 0);

            ~ChromaticAbberationPass() = default;
            static RPI::Ptr<ChromaticAbberationPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            ChromaticAbberationPass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
