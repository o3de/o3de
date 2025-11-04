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
        //! Parent pass for all Bloom related passes.
        //! The only difference between this and ParentPass is that this will check for available bloom settings
        //! and disable itself if no settings are found. See the IsEnabled override.
        class BloomParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(BloomParentPass);

        public:
            AZ_RTTI(AZ::Render::BloomParentPass, "{072861A3-A87A-439D-BD8B-D2BDD8D31799}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(BloomParentPass, SystemAllocator);
            virtual ~BloomParentPass() = default;

            //! Creates a BloomParentPass
            static RPI::Ptr<BloomParentPass> Create(const RPI::PassDescriptor& descriptor);

            //! Does a check for available BloomSettings, disable itself when the BloomSetting isn't available.
            bool IsEnabled() const override;

        private:
            BloomParentPass(const RPI::PassDescriptor& descriptor);
        };
    }   // namespace Render
}   // namespace AZ
