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
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>

namespace AZ::Render
{
    SkyAtmospherePass::SkyAtmospherePass(const RPI::PassDescriptor& descriptor, SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
        : RPI::ParentPass(descriptor)
        , m_atmosphereId(id)
    {
        o_enableShadows = AZ::Name("o_enableShadows");
        o_enableFastSky = AZ::Name("o_enableFastSky");
        o_enableSun = AZ::Name("o_enableSun");

        InitializeConstants(m_constants);
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

        passConnection.m_localSlot = Name{ "DirectionalShadowmap" };
        passConnection.m_attachmentRef.m_attachment = Name{ "DirectionalShadowmap" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = Name{ "DirectionalESM" };
        passConnection.m_attachmentRef.m_attachment = Name{ "DirectionalESM" };
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

    void SkyAtmospherePass::OnShaderReinitialized([[maybe_unused]] const RPI::Shader& shader)
    {
        BuildShaderData(); 

        for (auto passData : m_passData)
        {
            passData.m_srg->SetConstant(passData.m_index, m_constants);
            auto key = passData.m_shaderOptionGroup.GetShaderVariantKeyFallbackValue();
            passData.m_srg->SetShaderVariantKeyFallbackValue(key);
        }

        //if (!m_children.empty())
        //{
        //    //BindLUTs();

        //    // just locate the srg and constant every time for now to support shader reloading
        //    for (auto child : m_children)
        //    {
        //        if (RPI::RenderPass* renderPass = azrtti_cast<RPI::RenderPass*>(child.get()))
        //        {
        //            if (auto srg = renderPass->GetShaderResourceGroup())
        //            {
        //                if (auto index = srg->FindShaderInputConstantIndex(Name("m_constants")); index.IsValid())
        //                {
        //                    srg->SetConstant(index, m_constants);
        //                }
        //            }
        //        }
        //    }

        //}

        if (m_skyTransmittanceLUTPass)
        {
            // enable the LUT child pass to update the LUT render target
            m_skyTransmittanceLUTPass->SetEnabled(true);
        }
    }

    void SkyAtmospherePass::RegisterForShaderNotifications()
    {
        RPI::ShaderReloadNotificationBus::MultiHandler::BusDisconnect();

        for (const auto passData : m_passData)
        {
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(passData.m_shader->GetAssetId());
        }
        //for (auto child : m_children)
        //{
        //    if (auto fullscreenTrianglePass = azdynamic_cast<RPI::FullscreenTrianglePass>(child); fullscreenTrianglePass != nullptr)
        //    {
        //        RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(fullscreenTrianglePass->GetShader()->GetAssetId());
        //    }
        //    else if (auto computePass = azdynamic_cast<RPI::ComputePass>(child); computePass != nullptr)
        //    {
        //        RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(computePass->GetShader()->GetAssetId());
        //    }
        //}

        /*
        if (m_shaders.empty())
        {
            const AZStd::vector<AZStd::string> shaderPaths = {
                "Shaders/skyatmosphere/skytransmittancelut.azshader",
                "Shaders/skyatmosphere/skyviewlut.azshader",
                "Shaders/skyatmosphere/skyraymarching.azshader"
            };
            m_shaders.reserve(shaderPaths.size());

            for (const auto shaderPath : shaderPaths)
            {
                auto asset = RPI::FindShaderAsset(shaderPath.c_str());
                if (!asset.GetId().IsValid())
                {
                    AZ_Error("SkyAtmospherePass", false, "Failed to find shader asset %s", shaderPath.c_str());
                    continue;
                }
                auto shader = RPI::Shader::FindOrCreate(asset);
                if (!shader)
                {
                    AZ_Error("SkyAtmospherePass", false, "Failed to find or create %s", shaderPath.c_str());
                    continue;
                }
                m_shaders.emplace_back(AZStd::move(shader));
            }

            if (m_shaders.size() != shaderPaths.size())
            {
                m_shaders.clear();
            }
        }

        for (const auto shader : m_shaders)
        {
            RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect(shader->GetAssetId());
        }
        */
    }

    void SkyAtmospherePass::BindLUTs()
    {
        {
            // create and bind transmittance LUT

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

            childPass = FindChildPass(Name("SkyViewLUTPass"));
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
            // create and bind sky view LUT

            // this may be possible to do in the Pass file somehow...
            constexpr AZ::u32 width = 192;
            constexpr AZ::u32 height = 108;
            RHI::ImageDescriptor imageDesc = RHI::ImageDescriptor::Create2D(
                RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite, width, height, RHI::Format::R11G11B10_FLOAT);
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

    void SkyAtmospherePass::BuildShaderData()
    {
        for (auto child : m_children)
        {
            if (RPI::RenderPass* renderPass = azrtti_cast<RPI::RenderPass*>(child.get()))
            {
                auto srg = renderPass->GetShaderResourceGroup();
                if (!srg)
                {
                    continue;
                }

                auto index = srg->FindShaderInputConstantIndex(Name("m_constants"));
                if(!index.IsValid())
                {
                    continue;
                }

                Data::Instance<RPI::Shader> shader;
                if (auto fullscreenPass = azrtti_cast<RPI::FullscreenTrianglePass*>(renderPass); fullscreenPass != nullptr)
                {
                    shader = fullscreenPass->GetShader();
                }
                else if (auto computePass = azrtti_cast<RPI::ComputePass*>(renderPass); computePass != nullptr)
                {
                    shader = computePass->GetShader();
                }

                if (!shader)
                {
                    continue;
                }

                RPI::ShaderOptionGroup shaderOptionGroup = shader->CreateShaderOptionGroup();
                m_passData.push_back({ index, srg, AZStd::move(shader), AZStd::move(shaderOptionGroup) });
            }
        }
    }

	void SkyAtmospherePass::BuildInternal()
    {
        Base::BuildInternal();

        BuildShaderData();

        m_skyTransmittanceLUTPass = FindChildPass(Name("SkyTransmittanceLUTPass"));

        RegisterForShaderNotifications();

        BindLUTs();
    }

    void SkyAtmospherePass::FrameBeginInternal(AZ::RPI::Pass::FramePrepareParams params)
    {
        Base::FrameBeginInternal(params); 
    }

    void SkyAtmospherePass::FrameEndInternal()
    {
        Base::FrameEndInternal(); 

        if (m_skyTransmittanceLUTPass && m_skyTransmittanceLUTPass->IsEnabled())
        {
            m_skyTransmittanceLUTPass->SetEnabled(false);
        }
    }

    void SkyAtmospherePass::UpdateRenderPassSRG(const AtmosphereParams& params)
    {
        m_constants.m_fastSkyEnabled = params.m_fastSkyEnabled ? 1.f : 0.f;
        m_constants.m_bottomRadius = params.m_planetRadius;
        m_constants.m_topRadius = params.m_atmosphereRadius;
        m_constants.m_sunEnabled = params.m_sunEnabled ? 1.f : 0.f;
        m_constants.m_sunRadiusFactor = params.m_sunRadiusFactor;
        m_constants.m_sunFalloffFactor = params.m_sunFalloffFactor;
        params.m_sunColor.GetAsVector3().StoreToFloat3(m_constants.m_sunColor);
        params.m_sunDirection.GetNormalized().StoreToFloat3(m_constants.m_sunDirection);
        params.m_planetOrigin.StoreToFloat3(m_constants.m_planetOrigin);

        // avoid oversampling (too many loops) causing device removal
        constexpr uint32_t maxSamples{ 64 };  
        if (params.m_minSamples > maxSamples)
        {
            AZ_WarningOnce("SkyAtmosphere", false, "Clamping min samples to %ul to avoid device removal", maxSamples);
            m_constants.m_rayMarchMin = maxSamples; 
        }
        else
        {
            m_constants.m_rayMarchMin = aznumeric_cast<float>(params.m_minSamples); 
        }

        if (params.m_maxSamples > maxSamples)
        {
            AZ_WarningOnce("SkyAtmosphere", false, "Clamping max samples to %ul to avoid device removal", maxSamples);
            m_constants.m_rayMarchMax = maxSamples; 
        }
        else
        {
            m_constants.m_rayMarchMax = aznumeric_cast<float>(params.m_maxSamples); 
        }

        if (params.m_lutUpdateRequired)
        {
            params.m_luminanceFactor.StoreToFloat3(m_constants.m_luminanceFactor);
            params.m_rayleighScattering.StoreToFloat3(m_constants.m_rayleighScattering);
            params.m_mieScattering.StoreToFloat3(m_constants.m_mieScattering);
            params.m_mieAbsorption.StoreToFloat3(m_constants.m_mieAbsorption);
            (params.m_mieScattering + params.m_mieAbsorption).StoreToFloat3(m_constants.m_mieExtinction);
            params.m_absorption.StoreToFloat3(m_constants.m_absorption);
            params.m_groundAlbedo.StoreToFloat3(m_constants.m_groundAlbedo);

            const float atmosphereHeight = params.m_atmosphereRadius - params.m_planetRadius;
            if (atmosphereHeight > 0 && params.m_rayleighExpDistribution > 0 && params.m_mieExpDistribution > 0)
            {
                // prevent rayleigh and mie distributions being larger than the atmosphere size
                m_constants.m_rayleighDensityExpScale = -1.f / static_cast<float>(AZStd::min(params.m_rayleighExpDistribution, atmosphereHeight));
                m_constants.m_mieDensityExpScale = -1.f / static_cast<float>(AZStd::min(params.m_mieExpDistribution, atmosphereHeight));
            }

            // absorption density layer uses a tent distribution
            // for now we'll base this distribution on earth settings for ozone
            m_constants.m_absorptionDensity0LayerWidth = atmosphereHeight * 0.25f; // altitude at which absorption reaches its maximum value
            m_constants.m_absorptionDensity0LinearTerm = 1.f / 15.f;
            m_constants.m_absorptionDensity0ConstantTerm = -2.f / 3.f;
            m_constants.m_absorptionDensity1LinearTerm = -1.f / 15.f;
            m_constants.m_absorptionDensity1ConstantTerm = 8.f / 3.f;
        }

        for (auto passData : m_passData)
        {
            passData.m_srg->SetConstant(passData.m_index, m_constants);

            passData.m_shaderOptionGroup.SetValue(o_enableShadows, AZ::RPI::ShaderOptionValue{ params.m_shadowsEnabled });
            passData.m_shaderOptionGroup.SetValue(o_enableFastSky, AZ::RPI::ShaderOptionValue{ params.m_fastSkyEnabled });
            passData.m_shaderOptionGroup.SetValue(o_enableSun, AZ::RPI::ShaderOptionValue{ params.m_sunEnabled });

            auto key = passData.m_shaderOptionGroup.GetShaderVariantKeyFallbackValue();

            passData.m_srg->SetShaderVariantKeyFallbackValue(key);
        }

        //if (!m_children.empty())
        //{
        //    for (auto shader : m_shaders)
        //    {
        //        RPI::ShaderOptionGroup shaderOption = shader->CreateShaderOptionGroup();

        //        shaderOption.SetValue(o_enableShadows, AZ::RPI::ShaderOptionValue{ params.m_shadowsEnabled });
        //        shaderOption.SetValue(o_enableFastSky, AZ::RPI::ShaderOptionValue{ params.m_fastSkyEnabled });
        //        shaderOption.SetValue(o_enableSun, AZ::RPI::ShaderOptionValue{ params.m_sunEnabled });

        //        auto key = shaderOption.GetShaderVariantKeyFallbackValue();
        //    }

        //    // just locate the srg and constant every time for now to support shader reloading
        //    for (auto child : m_children)
        //    {

        //        RPI::ShaderOptionGroup shaderOption;
        //        if (auto fullscreenTrianglePass = azdynamic_cast<RPI::FullscreenTrianglePass>(child); fullscreenTrianglePass != nullptr)
        //        {
        //            shaderOption = fullscreenTrianglePass->GetShader()->CreateShaderOptionGroup();
        //        }
        //        else if (auto computePass = azdynamic_cast<RPI::ComputePass>(child); computePass != nullptr)
        //        {
        //            shaderOption = computePass->GetShader()->CreateShaderOptionGroup();
        //        }

        //        shaderOption.SetValue(o_enableShadows, AZ::RPI::ShaderOptionValue{ params.m_shadowsEnabled });
        //        shaderOption.SetValue(o_enableFastSky, AZ::RPI::ShaderOptionValue{ params.m_fastSkyEnabled });
        //        shaderOption.SetValue(o_enableSun, AZ::RPI::ShaderOptionValue{ params.m_sunEnabled });

        //        auto key = shaderOption.GetShaderVariantKeyFallbackValue();

        //        if (RPI::RenderPass* renderPass = azrtti_cast<RPI::RenderPass*>(child.get()))
        //        {
        //            if (auto srg = renderPass->GetShaderResourceGroup())
        //            {
        //                if (auto index = srg->FindShaderInputConstantIndex(Name("m_constants")); index.IsValid())
        //                {
        //                    srg->SetConstant(index, m_constants);
        //                    srg->SetShaderVariantKeyFallbackValue(key);
        //                }
        //            }
        //        }
        //    }

        //    if (params.m_lutUpdateRequired)
        //    {
        //        // enable the LUT child pass to update the LUT render target
        //        auto childPass = FindChildPass(Name("SkyTransmittanceLUTPass"));
        //        if (childPass)
        //        {
        //            childPass->SetEnabled(true);
        //        }
        //    }
        //}


        if (params.m_lutUpdateRequired && m_skyTransmittanceLUTPass)
        {
            m_skyTransmittanceLUTPass->SetEnabled(true);
        }
    }

    void SkyAtmospherePass::InitializeConstants(AtmosphereGPUParams& atmosphereConstants)
    {
        atmosphereConstants.m_fastSkyEnabled = 1.f; // enabled
        atmosphereConstants.m_bottomRadius = 6360.f; // km
        atmosphereConstants.m_topRadius = atmosphereConstants.m_bottomRadius + 100.f; // 100km

        AZ::Vector3(0.000650f, 0.001881f, 0.000085f).StoreToFloat3(atmosphereConstants.m_absorption);
        AZ::Vector3(0.005802f, 0.013558f, 0.033100f).StoreToFloat3(atmosphereConstants.m_rayleighScattering);

        constexpr float mieScattering = 0.004440f;
        constexpr float mieExtinction = 0.003996f;
        constexpr float mieAbsorption = mieScattering - mieExtinction;
        AZ::Vector3(mieScattering, mieScattering, mieScattering).StoreToFloat3(atmosphereConstants.m_mieScattering);
        AZ::Vector3(mieExtinction, mieExtinction, mieExtinction).StoreToFloat3(atmosphereConstants.m_mieExtinction);
        AZ::Vector3(mieAbsorption, mieAbsorption, mieAbsorption).StoreToFloat3(atmosphereConstants.m_mieAbsorption);

        atmosphereConstants.m_miePhaseFunctionG = 0.8f;

        AZ::Vector3(0.0, 0.0, 0.0).StoreToFloat3(atmosphereConstants.m_groundAlbedo);

        constexpr float EarthRayleighScaleHeight = 8.0f;
        constexpr float EarthMieScaleHeight = 1.2f;

        atmosphereConstants.m_rayleighDensityExpScale = -1.f / EarthRayleighScaleHeight;
        atmosphereConstants.m_mieDensityExpScale = -1.f / EarthMieScaleHeight;
        atmosphereConstants.m_absorptionDensity0LayerWidth = 25.f;
        atmosphereConstants.m_absorptionDensity0ConstantTerm = -2.f / 3.f; 
        atmosphereConstants.m_absorptionDensity0LinearTerm = 1.5f / 15.f;
        atmosphereConstants.m_absorptionDensity1ConstantTerm = 8.f / 3.f;
        atmosphereConstants.m_absorptionDensity1LinearTerm = -1.f / 15.f;

        AZ::Vector3(1.0, 1.0, 1.0).StoreToFloat3(atmosphereConstants.m_luminanceFactor);

        atmosphereConstants.m_sunEnabled = 1.f;
        atmosphereConstants.m_sunRadiusFactor = 1.f;
        atmosphereConstants.m_sunFalloffFactor = 1.f;
        AZ::Vector3(1.0, 1.0, 1.0).StoreToFloat3(atmosphereConstants.m_sunColor);
        AZ::Vector3(-0.76823, 0.6316, 0.10441).GetNormalized().StoreToFloat3(atmosphereConstants.m_sunDirection);
    }

    void SkyAtmospherePass::ResetInternal()
    {
        // not sure about this cleanup...
        m_transmittanceLUTImage.reset();
        m_skyViewLUTImage.reset();
        m_passData.clear();

        Base::ResetInternal();
    }

    SkyAtmosphereFeatureProcessorInterface::AtmosphereId SkyAtmospherePass::GetAtmosphereId() const
    {
        return m_atmosphereId;
    }
} // namespace AZ::Render
