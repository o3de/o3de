/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass downsamples the scene for use by the DiffuseProbeGridRenderPass.
        class DiffuseProbeGridDownsamplePass
            : public RPI::FullscreenTrianglePass
        {
            using Base = RPI::FullscreenTrianglePass;
            AZ_RPI_PASS(DiffuseProbeGridDownsamplePass);

        public:
            AZ_RTTI(Render::DiffuseProbeGridDownsamplePass, "{B3331B68-F974-44D6-806B-2CFFB4B6B563}", Base);
            AZ_CLASS_ALLOCATOR(Render::DiffuseProbeGridDownsamplePass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<DiffuseProbeGridDownsamplePass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit DiffuseProbeGridDownsamplePass(const RPI::PassDescriptor& descriptor);

            // Pass behavior overrides...
            bool IsEnabled() const override;
        };
    }   // namespace RPI
}   // namespace AZ
