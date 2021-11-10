/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        //! A class for DiffuseProbeGridDownsample to allow for disabling
        class DiffuseProbeGridDownsamplePass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DiffuseProbeGridDownsamplePass);
      
        public:
            AZ_RTTI(DiffuseProbeGridDownsamplePass, "{C7EF0708-480A-4E12-B968-58DB165EFE4D}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridDownsamplePass, SystemAllocator, 0);

            ~DiffuseProbeGridDownsamplePass() = default;
            static RPI::Ptr<DiffuseProbeGridDownsamplePass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;
        
        private:
            DiffuseProbeGridDownsamplePass(const RPI::PassDescriptor& descriptor);

        };
    } // namespace Render
} // namespace AZ
