/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

namespace AZ
{
    namespace Render
    {
        //! ShadowmapPass outputs shadowmap depth images.
        class ShadowmapPass final
            : public RPI::RasterPass
        {
            using Base = RPI::RasterPass;
            AZ_RPI_PASS(ShadowmapPass);
      
        public:
            AZ_RTTI(ShadowmapPass, "FCBDDB8C-E565-4780-9E2E-B45F16203F77", Base);
            AZ_CLASS_ALLOCATOR(ShadowmapPass, SystemAllocator, 0);

            ~ShadowmapPass() = default;

            static RPI::Ptr<ShadowmapPass> Create(const RPI::PassDescriptor& descriptor);

            // Creates a common pass template for the child shadowmap passes.
            static void CreatePassTemplate();

            //! Creates a pass descriptor from the input, using the ShadowmapPassTemplate, and adds a pass request to connect to the parent pass.
            //! This function assumes the parent pass has a SkinnedMeshes input slot
            static RPI::Ptr<ShadowmapPass> CreateWithPassRequest(const Name& passName, AZStd::shared_ptr<RPI::RasterPassData> passData);

            //! This updates array slice for this shadowmap.
            void SetArraySlice(uint16_t arraySlice);

            //! This enable/disable clearing of the image view.
            void SetClearEnabled(bool enabled);

            //! This update viewport and scissor for this shadowmap from the given image size.
            void SetViewportScissorFromImageSize(const RHI::Size& imageSize);

            //! This updates viewport and scissor for this shadowmap.
            void SetViewportScissor(const RHI::Viewport& viewport, const RHI::Scissor& scissor);

        private:
            ShadowmapPass() = delete;
            explicit ShadowmapPass(const RPI::PassDescriptor& descriptor);

            // RHI::Pass overrides...
            void BuildInternal() override;

            uint16_t m_arraySlice = 0;
            bool m_clearEnabled = true;
        };
    } // namespace Render
} // namespace AZ
