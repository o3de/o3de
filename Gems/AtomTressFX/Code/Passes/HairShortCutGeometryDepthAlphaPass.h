/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Passes/HairGeometryRasterPass.h>

namespace AZ
{
    namespace RHI
    {
        struct DrawItem;
    }

    namespace Render
    {
        namespace Hair
        {
            //! This geometry pass uses the following Srgs:
            //!  - PerPassSrg shared by all hair passes for the shared dynamic buffer 
            //!  - PerMaterialSrg - used solely by this pass to alter the vertices and apply the visual
            //!     hair properties to each fragment.
            //!  - HairDynamicDataSrg (PerObjectSrg) - shared buffers views for this hair object only.
            //!  - PerViewSrg and PerSceneSrg - as per the data from Atom.
            class HairShortCutGeometryDepthAlphaPass
                : public HairGeometryRasterPass
            {
                AZ_RPI_PASS(HairShortCutGeometryDepthAlphaPass);

            public:
                AZ_RTTI(HairShortCutGeometryDepthAlphaPass, "{F09A0411-B1FF-4085-98E7-6B8B0E1B2C3D}", HairGeometryRasterPass);
                AZ_CLASS_ALLOCATOR(HairShortCutGeometryDepthAlphaPass, SystemAllocator, 0);

                static RPI::Ptr<HairShortCutGeometryDepthAlphaPass> Create(const RPI::PassDescriptor& descriptor);

            protected:
                explicit HairShortCutGeometryDepthAlphaPass(const RPI::PassDescriptor& descriptor);

                // Pass behavior overrides
                void BuildInternal() override;
            };

        } // namespace Hair
    } // namespace Render
}   // namespace AZ
