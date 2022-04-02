/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>

namespace AZ::Render
{
    static const char* const SkyAtmospherePassTemplateName = "SkyAtmospherePassTemplate";

    //! This pass really consists of multiple child passes that do the actual work
    //! of rendering the atmosphere resources and atmosphere
    class SkyAtmospherePass final
        : public RPI::ParentPass
    {
        using Base = RPI::ParentPass;
        AZ_RPI_PASS(SkyAtmospherePass);

    public:
        AZ_RTTI(SkyAtmospherePass, "{F89F4F6C-360F-485A-9B5B-12C660375BD1}", Base);
        AZ_CLASS_ALLOCATOR(SkyAtmospherePass, SystemAllocator, 0);
        ~SkyAtmospherePass() = default;

        static RPI::Ptr<SkyAtmospherePass> CreateWithPassRequest(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);

        SkyAtmosphereFeatureProcessorInterface::AtmosphereId GetAtmosphereId() const;

        struct AtmosphereParams 
        {
            bool m_enabled = true;
            AZ::Vector3 m_sunDirection = AZ::Vector3(0,0,-1);
            float m_sunIlluminance = 1.0f;
            float m_planetRadius = 6360.0f;
            float m_atmosphereRadius = 6460.0f;
            uint32_t m_minSamples = 4;
            uint32_t m_maxSamples = 14;
            AZ::Vector3 m_rayleighScattering = AZ::Vector3(0.005802f, 0.013558f, 0.033100f);
            AZ::Vector3 m_mieScattering = AZ::Vector3(0.003996f, 0.003996f, 0.003996f);
            AZ::Vector3 m_absorptionExtinction = AZ::Vector3(0.000650f, 0.001881f, 0.000085f);
            AZ::Vector3 m_mieExtinction = AZ::Vector3(0.004440f, 0.004440f, 0.004440f);
            AZ::Vector3 m_groundAlbedo = AZ::Vector3(0.0f, 0.0f, 0.0f);
        };

        void UpdateRenderPassSRG(const AtmosphereParams& params);

    protected:
        void BuildInternal() override;
        void ResetInternal() override;

        explicit SkyAtmospherePass(const RPI::PassDescriptor& descriptor, SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);

         void FrameBeginInternal(AZ::RPI::Pass::FramePrepareParams params) override;
    private:
        SkyAtmospherePass() = delete;

        using ImageInstance = Data::Instance<RPI::AttachmentImage>;
		using AttachmentInstance = Data::Instance<RPI::PassAttachment>;

        void CreateImage(const Name& slotName, const RHI::ImageDescriptor& desc, ImageInstance& image);

        struct AtmosphereGPUParams 
        {
            float absorption_extinction[3];
            float pad0;

            float rayleigh_scattering[3];
            float mie_phase_function_g;

            float mie_scattering[3];
            float bottom_radius;

            float mie_extinction[3];
            float top_radius;

            float mie_absorption[3];
            float pad1;

            float ground_albedo[3];
            float pad2;

            float rayleigh_density[12];
            float mie_density[12];
            float absorption_density[12];

            float gSunIlluminance[3];
            float pad3;

            float sun_direction[3];
            float MultipleScatteringFactor;

            float RayMarchMinMaxSPP[2]; // x = min, y = max
            float pad4[2];

            uint32_t gResolution[2];
            uint32_t pad5[2];
        };
        void InitializeConstants(AtmosphereGPUParams& atmosphereConstants);

        SkyAtmosphereFeatureProcessorInterface::AtmosphereId m_atmosphereId;

        ImageInstance m_transmittanceLUTImage;
        ImageInstance m_skyViewLUTImage;

        RHI::ShaderInputNameIndex m_constantsIndexName = "m_constants";

        struct PassSrgData
        {
            RHI::ShaderInputConstantIndex index;
            Data::Instance<RPI::ShaderResourceGroup> srg;
        };
        AZStd::vector<PassSrgData> m_passSrgData;

        AtmosphereGPUParams m_constants;
    };
} // namespace AZ::Render
