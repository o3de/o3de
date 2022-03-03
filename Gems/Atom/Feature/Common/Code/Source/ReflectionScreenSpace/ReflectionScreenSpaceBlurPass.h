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
        //! This pass performs a separable Gaussian blur of the input reflection image
        //! to the lower mip levels of that image.  The blurred mips are used as roughness levels
        //! when applying reflection data to a surface material.
        class ReflectionScreenSpaceBlurPass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(ReflectionScreenSpaceBlurPass);

        public:
            AZ_RTTI(Render::ReflectionScreenSpaceBlurPass, "{BC3D92C5-E38A-46FE-8EBD-CAD14E505946}", ParentPass);
            AZ_CLASS_ALLOCATOR(Render::ReflectionScreenSpaceBlurPass, SystemAllocator, 0);
          
            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<ReflectionScreenSpaceBlurPass> Create(const RPI::PassDescriptor& descriptor);

            //! The total number of mip levels in the blur (including mip0)
            static const uint32_t NumMipLevels = 5;

        private:
            explicit ReflectionScreenSpaceBlurPass(const RPI::PassDescriptor& descriptor);

            void CreateChildPassesInternal() override;

            // Pass Overrides...
            void ResetInternal() override;
            void BuildInternal() override;

            AZStd::vector<RPI::Ptr<RPI::FullscreenTrianglePass>> m_verticalBlurChildPasses;
            AZStd::vector<RPI::Ptr<RPI::FullscreenTrianglePass>> m_horizontalBlurChildPasses;
        };
    }   // namespace RPI
}   // namespace AZ
