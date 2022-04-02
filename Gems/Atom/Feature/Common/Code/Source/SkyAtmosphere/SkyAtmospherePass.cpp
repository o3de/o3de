/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmospherePass.h>
#include <SkyAtmosphere/SkyAtmosphereParentPass.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>


#pragma optimize("", off)
namespace AZ::Render
{
    SkyAtmospherePass::SkyAtmospherePass(const RPI::PassDescriptor& descriptor, SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
        : RPI::ParentPass(descriptor)
        , m_atmosphereId(id)
    {
    }

    RPI::Ptr<SkyAtmospherePass> SkyAtmospherePass::CreateWithPassRequest(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
    {
        // Create a pass request for the descriptor so we can connect it to the parent class input connections
        RPI::PassRequest childRequest;
        childRequest.m_templateName = Name{ "SkyAtmosphereTemplate" };
        childRequest.m_passName = Name( AZStd::string::format("SkyAtmospherePass.%hu", id.GetIndex()) );

        RPI::PassConnection passConnection;
        passConnection.m_localSlot = Name{ "SpecularInputOutput" };
        passConnection.m_attachmentRef.m_pass = Name{ "Parent" };
        passConnection.m_attachmentRef.m_attachment = Name{ "SpecularInputOutput" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = Name{ "ReflectionInputOutput" };
        passConnection.m_attachmentRef.m_attachment = Name{ "ReflectionInputOutput" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = Name{ "SkyBoxDepth" };
        passConnection.m_attachmentRef.m_attachment = Name{ "SkyBoxDepth" };
        childRequest.m_connections.emplace_back(passConnection);

        // Get the template
        const AZStd::shared_ptr<const RPI::PassTemplate> childTemplate =
            RPI::PassSystemInterface::Get()->GetPassTemplate(childRequest.m_templateName);
        AZ_Assert(
            childTemplate,
            "SkyAtmospherePass::CreateWithPassRequest - attempting to create a pass before the template has been created.");

        RPI::PassDescriptor descriptor{ childRequest.m_passName, childTemplate, &childRequest };
        return aznew SkyAtmospherePass(descriptor, id);
    }

    void SkyAtmospherePass::CreateImage(const Name& slotName, const RHI::ImageDescriptor& desc, ImageInstance& image)
    {
        // we need a unique name because there may be multiple sky parent passes
        AZStd::string imageName = RPI::ConcatPassString(GetPathName(), slotName);
        RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);

        Data::Instance<AZ::RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        image = AZ::RPI::AttachmentImage::Create(*pool.get(), desc, Name(imageName), &clearValue, nullptr);
    }

	void SkyAtmospherePass::BuildInternal()
    {
        Base::BuildInternal();

        InitializeConstants(m_constants);

        for (auto child : m_children)
        {
            if (RPI::RenderPass* renderPass = azrtti_cast<RPI::RenderPass*>(child.get()))
            {
                if (auto srg = renderPass->GetShaderResourceGroup())
                {
                    if (auto index = srg->FindShaderInputConstantIndex(Name("m_constants")); index.IsValid())
                    {
                        m_passSrgData.push_back({ index, srg });
                    }
                }
            }
        }

        {
            // this may be possible to do in the Pass file somehow...
            constexpr AZ::u32 width = 256;
            constexpr AZ::u32 height = 64;
            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite, width, height,
                RHI::Format::R16G16B16A16_FLOAT);
            if (!m_transmittanceLUTImage)
            {
                CreateImage(Name("TransmittanceLUTImageAttachment"), imageDesc, m_transmittanceLUTImage);
            }

            // bind attachment to child slots
            auto childPass = FindChildPass(Name("SkyTransmittanceLUTPass"));
            if (childPass)
            {
                childPass->AttachImageToSlot(Name("SkyTransmittanceLUTOutput"), m_transmittanceLUTImage);
            }

            childPass = FindChildPass(Name("BypassFullScreenPass"));
            if (childPass)
            {
                childPass->AttachImageToSlot(Name("SkyTransmittanceLUTInput"), m_transmittanceLUTImage);
            }

            childPass = FindChildPass(Name("SkyRayMarchingPass"));
            if (childPass)
            {
                childPass->AttachImageToSlot(Name("SkyTransmittanceLUTInput"), m_transmittanceLUTImage);
            }
        }

        {
            // this may be possible to do in the Pass file somehow...
            constexpr AZ::u32 width = 200;
            constexpr AZ::u32 height = 200;
            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::ShaderReadWrite, width, height, RHI::Format::R16G16B16A16_FLOAT);
            if (!m_skyViewLUTImage)
            {
                CreateImage(Name("SkyViewLUTImageAttachment"), imageDesc, m_skyViewLUTImage);
            }

            // bind attachment to child slots
            auto childPass = FindChildPass(Name("SkyViewLUTPass"));
            if (childPass)
            {
                childPass->AttachImageToSlot(Name("SkyViewLUTOutput"), m_skyViewLUTImage);
            }

            childPass = FindChildPass(Name("SkyRayMarchingPass"));
            if (childPass)
            {
                childPass->AttachImageToSlot(Name("SkyViewLUTInput"), m_skyViewLUTImage);
            }
        }
    }

    void SkyAtmospherePass::FrameBeginInternal(AZ::RPI::Pass::FramePrepareParams params)
    {
        //for (auto childPass : GetChildren())
        //{

        //}
        Base::FrameBeginInternal(params); 
    }

    void SkyAtmospherePass::UpdateRenderPassSRG(const AtmosphereParams& params)
    {
        m_constants.bottom_radius = params.m_planetRadius;
        m_constants.top_radius = params.m_atmosphereRadius;
        m_constants.gSunIlluminance[0] = params.m_sunIlluminance;
        m_constants.gSunIlluminance[1] = params.m_sunIlluminance;
        m_constants.gSunIlluminance[2] = params.m_sunIlluminance;
        params.m_sunDirection.GetNormalized().StoreToFloat3(m_constants.sun_direction);
        constexpr uint32_t maxSamples{ 64 }; // avoid oversampling (too many loops) causing device removal 
        m_constants.RayMarchMinMaxSPP[0] = static_cast<float>(AZStd::min(maxSamples, params.m_minSamples)); 
        m_constants.RayMarchMinMaxSPP[1] = static_cast<float>(AZStd::min(maxSamples, params.m_maxSamples));

        params.m_rayleighScattering.StoreToFloat3(m_constants.rayleigh_scattering);
        params.m_mieScattering.StoreToFloat3(m_constants.mie_scattering);
        params.m_mieExtinction.StoreToFloat3(m_constants.mie_extinction);
        params.m_absorptionExtinction.StoreToFloat3(m_constants.absorption_extinction);
        params.m_groundAlbedo.StoreToFloat3(m_constants.ground_albedo);

        for (int i = 0; i < 3; ++i)
        {
            m_constants.mie_absorption[i] = AZStd::max(0.f, m_constants.mie_extinction[i] - m_constants.mie_scattering[i]);
        }

        // just locate the srg and constant every time for now to support shader reloading
        for (auto child : m_children)
        {
            if (RPI::RenderPass* renderPass = azrtti_cast<RPI::RenderPass*>(child.get()))
            {
                if (auto srg = renderPass->GetShaderResourceGroup())
                {
                    if (auto index = srg->FindShaderInputConstantIndex(Name("m_constants")); index.IsValid())
                    {
                        srg->SetConstant(index, m_constants);
                    }
                }
            }
        }

        //for (auto iter: m_passSrgData)
        //{
        //    iter.srg->SetConstant(iter.index, m_constants); 
        //}
    }

    void SkyAtmospherePass::InitializeConstants(AtmosphereGPUParams& atmosphereConstants)
    {
        atmosphereConstants.bottom_radius = 6360.f; // km
        atmosphereConstants.top_radius = atmosphereConstants.bottom_radius + 100.f; // 100km

        atmosphereConstants.gResolution[0] = 1920;
        atmosphereConstants.gResolution[1] = 800;
        AZ::Vector3(0.000650f, 0.001881f, 0.000085f).StoreToFloat3(atmosphereConstants.absorption_extinction);
        AZ::Vector3(0.005802f, 0.013558f, 0.033100f).StoreToFloat3(atmosphereConstants.rayleigh_scattering);
        atmosphereConstants.mie_phase_function_g = 0.8f;
        AZ::Vector3 mieScattering(0.003996f, 0.003996f, 0.003996f);
        mieScattering.StoreToFloat3(atmosphereConstants.mie_scattering);

        AZ::Vector3 mieExtinction(0.004440f, 0.004440f, 0.004440f);
        mieExtinction.StoreToFloat3(atmosphereConstants.mie_extinction);

        for (int i = 0; i < 3; ++i)
        {
            atmosphereConstants.mie_absorption[i] = AZStd::max(0.f, atmosphereConstants.mie_extinction[i] - atmosphereConstants.mie_scattering[i]);
        }
        //AZ::Vector3 mieAbsorption = mieExtinction - mieScattering;
        //auto MaxZero3 = [](AZ::Vector3& vec)
        //{
        //    for (int i = 0; i < 3; ++i)
        //    {
        //        if (vec.GetElement(i) < 0.f)
        //        {
        //            vec.SetElement(i, 0.f);
        //        }
        //    }
        //};
        //MaxZero3(mieAbsorption);
        //mieAbsorption.StoreToFloat3(atmosphereConstants.mie_absorption);

        AZ::Vector3(0.0, 0.0, 0.0).StoreToFloat3(atmosphereConstants.ground_albedo);

        struct DensityProfileLayer
        {
            float width;
            float exp_term;
            float exp_scale;
            float linear_term;
            float constant_term;
        };

        struct DensityProfile
        {
            DensityProfileLayer layers[2];
        };

        constexpr float EarthRayleighScaleHeight = 8.0f;
        constexpr float EarthMieScaleHeight = 1.2f;

        DensityProfile rayleigh_density;
        rayleigh_density.layers[0] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        rayleigh_density.layers[1] = { 0.0f, 1.0f, -1.0f / EarthRayleighScaleHeight, 0.0f, 0.0f };

        DensityProfile mie_density;
        mie_density.layers[0] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        mie_density.layers[1] = { 0.0f, 1.0f, -1.0f / EarthMieScaleHeight, 0.0f, 0.0f };

        DensityProfile absorption_density;
        absorption_density.layers[0] = { 25.0f, 0.0f, 0.0f, 1.0f / 15.0f, -2.0f / 3.0f };
        absorption_density.layers[1] = { 0.0f, 0.0f, 0.0f, -1.0f / 15.0f, 8.0f / 3.0f };

        memcpy(atmosphereConstants.rayleigh_density, &rayleigh_density, sizeof(rayleigh_density));
        memcpy(atmosphereConstants.mie_density, &mie_density, sizeof(mie_density));
        memcpy(atmosphereConstants.absorption_density, &absorption_density, sizeof(absorption_density));

        // remaining
        AZ::Vector3(1.0, 1.0, 1.0).StoreToFloat3(atmosphereConstants.gSunIlluminance);
        AZ::Vector3(-0.76823, 0.6316, 0.10441).GetNormalized().StoreToFloat3(atmosphereConstants.sun_direction);
        atmosphereConstants.MultipleScatteringFactor = 1.0f;
        atmosphereConstants.RayMarchMinMaxSPP[0] = 4;
        atmosphereConstants.RayMarchMinMaxSPP[1] = 14;
    }

    void SkyAtmospherePass::ResetInternal()
    {
        // not sure about this cleanup...
        m_transmittanceLUTImage.reset();
        m_skyViewLUTImage.reset();
        m_passSrgData.clear();

        Base::ResetInternal();
    }

    SkyAtmosphereFeatureProcessorInterface::AtmosphereId SkyAtmospherePass::GetAtmosphereId() const
    {
        return m_atmosphereId;
    }
} // namespace AZ::Render

#pragma optimize("", on)
