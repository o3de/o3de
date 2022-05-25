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
        AZ_CLASS_ALLOCATOR(SkyAtmospherePass, SystemAllocator, 0);
        ~SkyAtmospherePass() = default;

        static RPI::Ptr<SkyAtmospherePass> CreateWithPassRequest(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);

        SkyAtmosphereFeatureProcessorInterface::AtmosphereId GetAtmosphereId() const;

        void UpdateRenderPassSRG(const SkyAtmosphereParams& params);

    protected:
        void BuildInternal() override;
        void ResetInternal() override;

        explicit SkyAtmospherePass(const RPI::PassDescriptor& descriptor, SkyAtmosphereFeatureProcessorInterface::AtmosphereId id);

         void FrameBeginInternal(AZ::RPI::Pass::FramePrepareParams params) override;
         void FrameEndInternal() override;
    private:
        SkyAtmospherePass() = delete;

        using ImageInstance = Data::Instance<RPI::AttachmentImage>;
		using AttachmentInstance = Data::Instance<RPI::PassAttachment>;

        void CreateImage(const Name& slotName, const RHI::ImageDescriptor& desc, ImageInstance& image);

        struct AtmosphereGPUParams 
        {
            float m_absorption[3] = {.000650f, 0.001881f, 0.000085f};
            float m_pad0 = 0.f; // not used

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
            float m_pad1; // not used

            float m_sunColor[3];
            float m_sunRadiusFactor = 1.f;

            float m_sunDirection[3] = {0.f,0.f,-1.f};
            float m_sunFalloffFactor = 1.f;

            float m_sunLimbColor[3];
            float m_pad2 = 0.f; // not used

            float m_luminanceFactor[3] = {1.f, 1.f, 1.f};
            float m_pad3 = 0.f; // not used

            float m_planetOrigin[3] = {0.f, 0.f, 0.f};
            float m_pad4 = 0.f;
        };

        //! ShaderReloadNotificationBus
        void OnShaderReinitialized(const RPI::Shader& shader) override;
        void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;

        void RegisterForShaderNotifications();
        void BindLUTs();
        void BuildShaderData();
        bool LutParamsEqual(const SkyAtmosphereParams& lhs, const SkyAtmosphereParams& rhs) const;

        SkyAtmosphereFeatureProcessorInterface::AtmosphereId m_atmosphereId;

        ImageInstance m_transmittanceLUTImage;
        ImageInstance m_skyViewLUTImage;

        RHI::ShaderInputNameIndex m_constantsIndexName = "m_constants";

        struct PassData
        {
            RHI::ShaderInputConstantIndex m_index;
            Data::Instance<RPI::ShaderResourceGroup> m_srg;
            Data::Instance<RPI::Shader> m_shader;
            RPI::ShaderOptionGroup m_shaderOptionGroup;
        };
        AZStd::vector<PassData> m_passData;
        RPI::Ptr<RPI::Pass> m_skyTransmittanceLUTPass = nullptr;

        AtmosphereGPUParams m_constants;
        SkyAtmosphereParams m_atmosphereParams;
        bool m_lutUpdateRequired = true;

        AZ::Name o_enableShadows;
        AZ::Name o_enableFastSky;
        AZ::Name o_enableSun;
    };
} // namespace AZ::Render
