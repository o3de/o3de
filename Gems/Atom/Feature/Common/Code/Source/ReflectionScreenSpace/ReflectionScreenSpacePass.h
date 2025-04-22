/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! Parent pass for screenspace reflections, implements a frame delay to allow time
        //! for the previous frame texture to be populated
        class ReflectionScreenSpacePass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpacePass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpacePass, "{0B27D7F1-F914-4D09-A46D-3E63404771E3}", ParentPass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpacePass, SystemAllocator, 0);
          
            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpacePass> Create(const RPI::PassDescriptor& descriptor);

            void ResetFrameDelay() { m_frameDelayCount = 0; }

        private:
            explicit ReflectionScreenSpacePass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            bool IsEnabled() const override;

            mutable uint32_t m_frameDelayCount = 0;
        };
    }   // namespace RPI
}   // namespace AZ
