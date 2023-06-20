/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Vector3.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/Feature/SkyAtmosphere/SkyAtmosphereFeatureProcessorInterface.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ::Render
{
    static const char* const SkyAtmospherePassTemplateName = "SkyAtmospherePassTemplate";

    //! This pass really consists of multiple child passes that do the actual work
    //! of rendering the atmosphere resources and atmosphere
    class SkyAtmospherePass final
        : public RPI::ParentPass
        , public RPI::ShaderReloadNotificationBus::MultiHandler
    {
        using Base = RPI::ParentPass;
        AZ_RPI_PASS(SkyAtmospherePass);

    public:
        AZ_RTTI(SkyAtmospherePass, "{F89F4F6C-360F-485A-9B5B-12C660375BD1}", Base);
        AZ_CLASS_ALLOCATOR(SkyAtmospherePass, SystemAllocator);
        ~SkyAtmospherePass() = default;

        static RPI::Ptr<SkyAtmospherePass> CreateWithPassRequest(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);

        SkyAtmosphereFeatureProcessorInterface::AtmosphereId GetAtmosphereId() const;

        void UpdateRenderPassSRG(const SkyAtmosphereParams& params);

    protected:
        explicit SkyAtmospherePass(const RPI::PassDescriptor& descriptor, SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);

        void FrameBeginInternal(AZ::RPI::Pass::FramePrepareParams params) override;
        void BuildInternal() override;
        void ResetInternal() override;

    private:
        SkyAtmospherePass() = delete;

        void BindLUTs();
        void BuildShaderData();
        bool NeedsShaderDataRebuild() const;
        bool LutParamsEqual(const SkyAtmosphereParams& lhs, const SkyAtmosphereParams& rhs) const;
        void UpdatePassData();

        using ImageInstance = Data::Instance<RPI::AttachmentImage>;
        void CreateImage(const Name& slotName, const RHI::ImageDescriptor& desc, ImageInstance& image);

        struct AtmosphereGPUParams 
        {
            float m_absorption[3] = {.000650f, 0.001881f, 0.000085f};
            float m_nearClip = 0.f;

            float m_rayleighScattering[3] = {0.005802f, 0.013558f, 0.033100f};
            float m_miePhaseFunctionG = 0.8f;

            float m_mieScattering[3] = {0.003996f, 0.003996f, 0.003996f}; // 1/km
            float m_bottomRadius = 6360.f; // km

            float m_mieExtinction[3] = {0.004440f, 0.004440f, 0.004440f}; // 1/km
            float m_topRadius = 6460.f; // km

            float m_mieAbsorption[3] = { 0.000444f, 0.000444f, 0.000444f }; // 1/km
            float m_rayMarchMin = 4.f;

            float m_groundAlbedo[3] = {0.f, 0.f, 0.f};
            float m_rayMarchMax = 14.f;

            float m_rayleighDensityExpScale = -1.f / 8.f;
            float m_mieDensityExpScale = -1.f / 1.2f;
            float m_absorptionDensity0LayerWidth = 25.f;
            float m_absorptionDensity0ConstantTerm = -2.f / 3.f; 

            float m_absorptionDensity0LinearTerm = 1.5f / 15.f;
            float m_absorptionDensity1ConstantTerm = 8.f / 3.f;
            float m_absorptionDensity1LinearTerm = -1.f / 15.f;
            float m_nearFadeDistance;

            float m_sunColor[3];
            float m_sunRadiusFactor = 1.f;

            float m_sunDirection[3] = {0.f,0.f,-1.f};
            float m_sunFalloffFactor = 1.f;

            float m_sunLimbColor[3];
            float m_sunShadowFarClip = 0.f;

            float m_luminanceFactor[3] = {1.f, 1.f, 1.f};
            float m_aerialDepthFactor = 1.f;

            float m_planetOrigin[3] = {0.f, 0.f, 0.f};
            float m_pad4 = 0.f;
        };

        SkyAtmosphereFeatureProcessorInterface::AtmosphereId m_atmosphereId;

        ImageInstance m_transmittanceLUTImage;
        ImageInstance m_skyViewLUTImage;
        ImageInstance m_skyVolumeLUTImage;

        RHI::ShaderInputNameIndex m_constantsIndexName = "m_constants";

        struct AtmospherePassData
        {
            RHI::ShaderInputConstantIndex m_index;
            Data::Instance<RPI::ShaderResourceGroup> m_srg;
            RPI::ShaderOptionGroup m_shaderOptionGroup;
        };
        AZStd::vector<AtmospherePassData> m_atmospherePassData;

        RPI::Ptr<RPI::Pass> m_skyTransmittanceLUTPass = nullptr;
        RPI::Ptr<RPI::Pass> m_skyViewLUTPass = nullptr;
        RPI::Ptr<RPI::Pass> m_skyVolumeLUTPass = nullptr;

        AtmosphereGPUParams m_constants;
        SkyAtmosphereParams m_atmosphereParams;
        bool m_lutUpdateRequired = true;
        bool m_updateConstants = false;

        bool m_enableSkyTransmittanceLUTPass = false;
        bool m_enableFastSky = true;
        bool m_fastAerialPerspectiveEnabled = true;
        bool m_aerialPerspectiveEnabled = true;
        bool m_enableShadows = false;
        bool m_enableSun = true;
    };
} // namespace AZ::Render
