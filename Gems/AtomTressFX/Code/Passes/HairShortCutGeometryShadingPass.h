/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Passes/HairGeometryRasterPass.h>
#include <Rendering/HairGlobalSettings.h>

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
            class HairShortCutGeometryShadingPass
                : public HairGeometryRasterPass
            {
                AZ_RPI_PASS(HairShortCutGeometryShadingPass);

            public:
                AZ_RTTI(HairShortCutGeometryShadingPass, "{11BA673D-0788-4B25-978D-9737BF4E48FE}", HairGeometryRasterPass);
                AZ_CLASS_ALLOCATOR(HairShortCutGeometryShadingPass, SystemAllocator);

                static RPI::Ptr<HairShortCutGeometryShadingPass> Create(const RPI::PassDescriptor& descriptor);

                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            protected:
                AZ::Name o_enableShadows;
                AZ::Name o_enableDirectionalLights;
                AZ::Name o_enablePunctualLights;
                AZ::Name o_enableAreaLights;
                AZ::Name o_enableIBL;
                AZ::Name o_hairLightingModel;
                AZ::Name o_enableMarschner_R;
                AZ::Name o_enableMarschner_TRT;
                AZ::Name o_enableMarschner_TT;
                AZ::Name o_enableLongtitudeCoeff;
                AZ::Name o_enableAzimuthCoeff;

                explicit HairShortCutGeometryShadingPass(const RPI::PassDescriptor& descriptor);

                void UpdateGlobalShaderOptions();

                // Pass behavior overrides
                void BuildInternal() override;
                void InitializeInternal() override;

                HairGlobalSettings m_hairGlobalSettings;
            };

        } // namespace Hair
    } // namespace Render
}   // namespace AZ
