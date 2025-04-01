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
        class ChromaticAberrationPass final : public RPI::ComputePass
        {
            AZ_RPI_PASS(ChromaticAberrationPass);

        public:
            AZ_RTTI(ChromaticAberrationPass, "{557EF771-7D60-4EF1-BD61-E2446237B85B}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(ChromaticAberrationPass, SystemAllocator);

            ~ChromaticAberrationPass() = default;
            static RPI::Ptr<ChromaticAberrationPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            ChromaticAberrationPass(const RPI::PassDescriptor& descriptor);

            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        };
    } // namespace Render
} // namespace AZ
