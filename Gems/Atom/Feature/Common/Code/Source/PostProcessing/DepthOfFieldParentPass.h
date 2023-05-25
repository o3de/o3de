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
        //! Parent pass for all Depth of Field related passes.
        //! The only difference between this and ParentPass is that this will check for available depth of field settings
        //! and disable itself if no settings are found. See the IsEnabled override.
        class DepthOfFieldParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(DepthOfFieldParentPass);

        public:
            AZ_RTTI(AZ::Render::DepthOfFieldParentPass, "{6033066A-CA95-422E-9BF2-8C203171C1A8}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldParentPass, SystemAllocator);
            virtual ~DepthOfFieldParentPass() = default;

            //! Creates a DepthOfFieldParentPass
            static RPI::Ptr<DepthOfFieldParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        private:
            DepthOfFieldParentPass(const RPI::PassDescriptor& descriptor);
        };
    }   // namespace Render
}   // namespace AZ
