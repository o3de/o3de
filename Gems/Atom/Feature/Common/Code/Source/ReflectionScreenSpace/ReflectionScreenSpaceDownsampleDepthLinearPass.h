/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! This pass downsamples the linear depth into a mip chain
        class ReflectionScreenSpaceDownsampleDepthLinearPass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceDownsampleDepthLinearPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceDownsampleDepthLinearPass, "{5A215A02-2154-48D8-908D-351063BDB372}", ParentPass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceDownsampleDepthLinearPass, SystemAllocator);
          
            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceDownsampleDepthLinearPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit ReflectionScreenSpaceDownsampleDepthLinearPass(const RPI::PassDescriptor& descriptor);

            void CreateChildPassesInternal() override;

            // Pass Overrides...
            void ResetInternal() override;
            void BuildInternal() override;

            uint32_t m_mipLevels = 0;
            RHI::Size m_imageSize;
        };
    }   // namespace RPI
}   // namespace AZ
