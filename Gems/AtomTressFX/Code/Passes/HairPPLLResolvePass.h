/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <TressFX/TressFXConstantBuffers.h>
#include <Rendering/HairCommon.h>
#include <Rendering/HairGlobalSettings.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            class HairFeatureProcessor;

            //! The hair PPLL resolve pass is a full screen pass that runs over the hair fragments list
            //!  that were computed in the raster fill pass and resolves their depth order, transparency
            //! and lighting values to be output to display.
            //! Each pixel on the screen will be processed only once and will iterate through the fragments
            //!  list associated with the pixel's location.
            //! The full screen resolve pass is using the following Srgs:
            //!  - PerPassSrg: hair vertex data data, PPLL buffers and material array
            //!     shared by all passes.
            class HairPPLLResolvePass final
                : public RPI::FullscreenTrianglePass
            {
                AZ_RPI_PASS(HairPPLLResolvePass);

            public:
                AZ_RTTI(HairPPLLResolvePass, "{240940C1-4A47-480D-8B16-176FF3359B01}", RPI::FullscreenTrianglePass);
                AZ_CLASS_ALLOCATOR(HairPPLLResolvePass, SystemAllocator, 0);
                ~HairPPLLResolvePass() = default;

                static RPI::Ptr<HairPPLLResolvePass> Create(const RPI::PassDescriptor& descriptor);

                bool AcquireFeatureProcessor();

                void SetFeatureProcessor(HairFeatureProcessor* featureProcessor)
                {
                    m_featureProcessor = featureProcessor;
                }

                //! Override pass behavior methods
                void InitializeInternal() override;
                void CompileResources(const RHI::FrameGraphCompileContext& context) override;

            private:
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

                HairPPLLResolvePass(const RPI::PassDescriptor& descriptor);

                void UpdateGlobalShaderOptions();

                HairGlobalSettings m_hairGlobalSettings;
                HairFeatureProcessor* m_featureProcessor = nullptr;
                AZ::RPI::ShaderVariantKey m_shaderOptions;
            };

        } // namespace Hair
    }   // namespace Render
}   // namespace AZ
