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
        // Parent pass for all AO methods, controls which AO method is used
        class AoParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(AoParentPass);

        public:
            AZ_RTTI(AZ::Render::AoParentPass, "{7ACD89D5-ACFC-4D8F-81EF-88E7EE0E706D}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(AoParentPass, SystemAllocator);
            virtual ~AoParentPass() = default;

            //! Creates an AoParentPass
            static RPI::Ptr<AoParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            AoParentPass(const RPI::PassDescriptor& descriptor);

            // Direct pointers to child passes for enable/disable
            SsaoParentPass* m_ssaoParentPass = nullptr;
            GtaoParentPass* m_gtaoParentPass = nullptr;

            // SSAO is enabled by default, according to the pass file
            Ao::AoMethodType m_currentAoMethod = Ao::AoMethodType::SSAO;
        };
    }   // namespace Render
}   // namespace AZ
