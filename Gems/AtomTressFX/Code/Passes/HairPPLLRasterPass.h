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
            //! A HairPPLLRasterPass is used for the hair fragments fill render after the data
            //! went through the skinning and simulation passes.
            //! The output of this pass is the general list of fragment data that can now be
            //!  traversed for depth resolve and lighting.
            //! The Fill pass uses the following Srgs:
            //!  - PerPassSrg shared by all hair passes for the shared dynamic buffer and the PPLL buffers
            //!  - PerMaterialSrg - used solely by this pass to alter the vertices and apply the visual
            //!     hair properties to each fragment.
            //!  - HairDynamicDataSrg (PerObjectSrg) - shared buffers views for this hair object only.
            //!  - PerViewSrg and PerSceneSrg - as per the data from Atom.
            class HairPPLLRasterPass
                : public HairGeometryRasterPass
            {
                AZ_RPI_PASS(HairPPLLRasterPass);

            public:
                AZ_RTTI(HairPPLLRasterPass, "{6614D7DD-24EE-4A2B-B314-7C035E2FB3C4}", HairGeometryRasterPass);
                AZ_CLASS_ALLOCATOR(HairPPLLRasterPass, SystemAllocator);

                //! Creates a HairPPLLRasterPass
                static RPI::Ptr<HairPPLLRasterPass> Create(const RPI::PassDescriptor& descriptor);

            protected:
                explicit HairPPLLRasterPass(const RPI::PassDescriptor& descriptor);

                // Pass behavior overrides
                void BuildInternal() override;
                void InitializeInternal() override;                
            };

        } // namespace Hair
    } // namespace Render
}   // namespace AZ
