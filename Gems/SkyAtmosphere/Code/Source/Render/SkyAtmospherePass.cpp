/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/SkyAtmospherePass.h>
#include <Render/SkyAtmosphereParentPass.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>

namespace SkyAtmosphere
{
    SkyAtmospherePass::SkyAtmospherePass(const AZ::RPI::PassDescriptor& descriptor, SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
        : AZ::RPI::ParentPass(descriptor)
        , m_atmosphereId(id)
    {
    }

    AZ::RPI::Ptr<SkyAtmospherePass> SkyAtmospherePass::CreateWithPassRequest(SkyAtmosphereFeatureProcessorInterface::AtmosphereId id)
    {
        // Create a pass request for the descriptor so we can connect it to the parent class input connections
        AZ::RPI::PassRequest childRequest;
        childRequest.m_templateName = AZ::Name{ "SkyAtmosphereTemplate" };
        childRequest.m_passName = AZ::Name( AZStd::string::format("SkyAtmospherePass.%hu", id.GetIndex()) );

        AZ::RPI::PassConnection passConnection;
        passConnection.m_localSlot = AZ::Name{ "SpecularInputOutput" };
        passConnection.m_attachmentRef.m_pass = AZ::Name{ "Parent" };
        passConnection.m_attachmentRef.m_attachment = AZ::Name{ "SpecularInputOutput" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = AZ::Name{ "ReflectionInputOutput" };
        passConnection.m_attachmentRef.m_attachment = AZ::Name{ "ReflectionInputOutput" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = AZ::Name{ "SkyBoxDepth" };
        passConnection.m_attachmentRef.m_attachment = AZ::Name{ "SkyBoxDepth" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = AZ::Name{ "DirectionalShadowmap" };
        passConnection.m_attachmentRef.m_attachment = AZ::Name{ "DirectionalShadowmap" };
        childRequest.m_connections.emplace_back(passConnection);

        passConnection.m_localSlot = AZ::Name{ "DirectionalESM" };
        passConnection.m_attachmentRef.m_attachment = AZ::Name{ "DirectionalESM" };
        childRequest.m_connections.emplace_back(passConnection);

        const AZStd::shared_ptr<const AZ::RPI::PassTemplate> childTemplate =
            AZ::RPI::PassSystemInterface::Get()->GetPassTemplate(childRequest.m_templateName);
        AZ_Assert(
            childTemplate,
            "SkyAtmospherePass::CreateWithPassRequest - attempting to create a pass before the template has been created.");

        AZ::RPI::PassDescriptor descriptor{ childRequest.m_passName, childTemplate, &childRequest };
        return aznew SkyAtmospherePass(descriptor, id);
    }

    void SkyAtmospherePass::CreateImage(const AZ::Name& slotName, const AZ::RHI::ImageDescriptor& desc, ImageInstance& image)
    {
        // we need a unique AZ::Name because there may be multiple sky parent passes
        AZStd::string imageName = AZ::RPI::ConcatPassString(GetPathName(), slotName);
        AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);

        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        image = AZ::RPI::AttachmentImage::Create(*pool.get(), desc, AZ::Name(imageName), &clearValue, nullptr);
    }

    void SkyAtmospherePass::BindLUTs()
    {
        auto bindImageToSlot = [&](const ImageInstance& image, const AZ::Name& slotName, const AZ::Name& passName)
        {
            auto pass = FindChildPass(passName);
            if (!pass)
            {
                AZ_Warning("SkyAtmospherePass", false, "Failed to find pass %s", passName.GetCStr());
                return;
            }

            auto binding = pass->FindAttachmentBinding(slotName);
            if (!binding)
            {
                AZ_Warning("SkyAtmospherePass", false, "Failed to find binding for slot %s", slotName.GetCStr());
                return;
            }

            if (!binding->GetAttachment())
            {
                pass->AttachImageToSlot(slotName, image);
            }
        };

        {
            // create and bind transmittance LUT
            constexpr AZ::u32 width = 256;
            constexpr AZ::u32 height = 64;
            AZ::RHI::ImageDescriptor imageDesc = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite, width, height,
                AZ::RHI::Format::R16G16B16A16_FLOAT);
            if (!m_transmittanceLUTImage)
            {
                CreateImage(AZ::Name("TransmittanceLUTImageAttachment"), imageDesc, m_transmittanceLUTImage);
            }

            bindImageToSlot(m_transmittanceLUTImage, AZ::Name("SkyTransmittanceLUTOutput"), AZ::Name("SkyTransmittanceLUTPass"));
            bindImageToSlot(m_transmittanceLUTImage, AZ::Name("SkyTransmittanceLUTInput"), AZ::Name("SkyViewLUTPass"));
            bindImageToSlot(m_transmittanceLUTImage, AZ::Name("SkyTransmittanceLUTInput"), AZ::Name("SkyVolumeLUTPass"));
            bindImageToSlot(m_transmittanceLUTImage, AZ::Name("SkyTransmittanceLUTInput"), AZ::Name("SkyRayMarchingPass"));
        }

        {
            // create and bind sky view LUT
            constexpr AZ::u32 width = 192;
            constexpr AZ::u32 height = 108;
            AZ::RHI::ImageDescriptor imageDesc = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite, width, height, AZ::RHI::Format::R11G11B10_FLOAT);
            if (!m_skyViewLUTImage)
            {
                CreateImage(AZ::Name("SkyViewLUTImageAttachment"), imageDesc, m_skyViewLUTImage);
            }

            bindImageToSlot(m_skyViewLUTImage, AZ::Name("SkyViewLUTOutput"), AZ::Name("SkyViewLUTPass"));
            bindImageToSlot(m_skyViewLUTImage, AZ::Name("SkyViewLUTInput"), AZ::Name("SkyRayMarchingPass"));
        }

        {
            // create and bind sky volume LUT
            constexpr AZ::u32 width = 32;
            constexpr AZ::u32 height = 32;
            constexpr AZ::u32 depth = 32;
            AZ::RHI::ImageDescriptor imageDesc = AZ::RHI::ImageDescriptor::Create3D(
                AZ::RHI::ImageBindFlags::Color | AZ::RHI::ImageBindFlags::ShaderReadWrite, width, height, depth, AZ::RHI::Format::R16G16B16A16_FLOAT);
            if (!m_skyVolumeLUTImage)
            {
                CreateImage(AZ::Name("SkyVolumeLUTImageAttachment"), imageDesc, m_skyVolumeLUTImage);
            }

            bindImageToSlot(m_skyVolumeLUTImage, AZ::Name("SkyVolumeLUTOutput"), AZ::Name("SkyVolumeLUTPass"));
            bindImageToSlot(m_skyVolumeLUTImage, AZ::Name("SkyVolumeLUTInput"), AZ::Name("SkyRayMarchingPass"));
        }
    }

    void SkyAtmospherePass::BuildShaderData()
    {
        m_atmospherePassData.clear();

        for (auto child : m_children)
        {
            if (AZ::RPI::RenderPass* renderPass = azrtti_cast<AZ::RPI::RenderPass*>(child.get()))
            {
                auto srg = renderPass->GetShaderResourceGroup();
                if (!srg)
                {
                    continue;
                }

                auto index = srg->FindShaderInputConstantIndex(AZ::Name("m_constants"));
                if(!index.IsValid())
                {
                    continue;
                }

                AZ::Data::Instance<AZ::RPI::Shader> shader;
                if (auto fullscreenPass = azrtti_cast<AZ::RPI::FullscreenTrianglePass*>(renderPass); fullscreenPass != nullptr)
                {
                    shader = fullscreenPass->GetShader();
                }
                else if (auto computePass = azrtti_cast<AZ::RPI::ComputePass*>(renderPass); computePass != nullptr)
                {
                    shader = computePass->GetShader();
                }

                if (!shader)
                {
                    continue;
                }

                AZ::RPI::ShaderOptionGroup shaderOptionGroup = shader->CreateShaderOptionGroup();
                m_atmospherePassData.push_back({ index, srg, AZStd::move(shaderOptionGroup) });
            }
        }

        m_updateConstants = true;
    }

    void SkyAtmospherePass::BuildInternal()
    {
        Base::BuildInternal();

        BuildShaderData();

        m_skyTransmittanceLUTPass = FindChildPass(AZ::Name("SkyTransmittanceLUTPass"));
        m_skyViewLUTPass = FindChildPass(AZ::Name("SkyViewLUTPass"));
        m_skyVolumeLUTPass = FindChildPass(AZ::Name("SkyVolumeLUTPass"));

        BindLUTs();

        m_enableSkyTransmittanceLUTPass = true;
    }

    void SkyAtmospherePass::UpdatePassData()
    {
        uint32_t childIndex = 0;
        for (auto passData : m_atmospherePassData)
        {
            passData.m_srg->SetConstant(passData.m_index, m_constants);

            passData.m_shaderOptionGroup.SetValue(AZ::Name("o_enableShadows"), AZ::RPI::ShaderOptionValue{ m_enableShadows });
            passData.m_shaderOptionGroup.SetValue(AZ::Name("o_enableFastSky"), AZ::RPI::ShaderOptionValue{ m_enableFastSky });
            passData.m_shaderOptionGroup.SetValue(AZ::Name("o_enableSun"), AZ::RPI::ShaderOptionValue{ m_enableSun });
            passData.m_shaderOptionGroup.SetValue(AZ::Name("o_enableFastAerialPerspective"), AZ::RPI::ShaderOptionValue{ m_fastAerialPerspectiveEnabled });
            passData.m_shaderOptionGroup.SetValue(AZ::Name("o_enableAerialPerspective"), AZ::RPI::ShaderOptionValue{ m_aerialPerspectiveEnabled });

            const auto& pass = m_children[childIndex];
            if (auto fullscreenPass = azrtti_cast<AZ::RPI::FullscreenTrianglePass*>(pass); fullscreenPass != nullptr)
            {
                fullscreenPass->UpdateShaderOptions(passData.m_shaderOptionGroup.GetShaderVariantId());
            }
            else if (auto computePass = azrtti_cast<AZ::RPI::ComputePass*>(pass); computePass != nullptr)
            {
                computePass->UpdateShaderOptions(passData.m_shaderOptionGroup.GetShaderVariantId());
            }
            childIndex++;
        }
    }

    bool SkyAtmospherePass::NeedsShaderDataRebuild() const
    {
        if (m_children.empty())
        {
            return false;
        }
        else if (m_children.size() != m_atmospherePassData.size())
        {
            return true;
        }

        // SRG may change due to a shader reload
        for (int i = 0; i < m_atmospherePassData.size(); ++i)
        {
            if (AZ::RPI::RenderPass* renderPass = azrtti_cast<AZ::RPI::RenderPass*>(m_children[i].get()))
            {
                auto srg = renderPass->GetShaderResourceGroup();
                if (m_atmospherePassData[i].m_srg != srg)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void SkyAtmospherePass::FrameBeginInternal(AZ::RPI::Pass::FramePrepareParams params)
    {
        if (NeedsShaderDataRebuild())
        {
            BuildShaderData();
        }

        if (m_updateConstants && !m_atmospherePassData.empty())
        {
            m_updateConstants = false;
            UpdatePassData();
        }

        if (m_skyTransmittanceLUTPass)
        {
            if (m_enableSkyTransmittanceLUTPass)
            {
                m_skyTransmittanceLUTPass->SetEnabled(true);

                // we automatically disable the pass after updating until LUT params change again
                m_enableSkyTransmittanceLUTPass = false;
            }
            else if (m_skyTransmittanceLUTPass->IsEnabled())
            {
                m_skyTransmittanceLUTPass->SetEnabled(false);
            }
        }

        if (m_skyViewLUTPass) 
        {
            if (m_enableFastSky != m_skyViewLUTPass->IsEnabled())
            {
                m_skyViewLUTPass->SetEnabled(m_enableFastSky);
            }
        }

        if (m_skyVolumeLUTPass) 
        {
            bool enableVolumePass = m_fastAerialPerspectiveEnabled && m_aerialPerspectiveEnabled;
            if (enableVolumePass != m_skyVolumeLUTPass->IsEnabled())
            {
                m_skyVolumeLUTPass->SetEnabled(enableVolumePass);
            }
        }

        Base::FrameBeginInternal(params); 
    }

    bool SkyAtmospherePass::LutParamsEqual(const SkyAtmosphereParams& lhs, const SkyAtmosphereParams& rhs) const
    {
        return lhs.m_rayleighExpDistribution ==  rhs.m_rayleighExpDistribution &&
            lhs.m_mieExpDistribution == rhs.m_mieExpDistribution &&
            lhs.m_planetRadius ==  rhs.m_planetRadius &&
            lhs.m_atmosphereRadius == rhs.m_atmosphereRadius &&
            lhs.m_luminanceFactor.IsClose(rhs.m_luminanceFactor) &&
            lhs.m_rayleighScattering.IsClose(rhs.m_rayleighScattering) &&
            lhs.m_mieScattering.IsClose(rhs.m_mieScattering) &&
            lhs.m_mieAbsorption.IsClose(rhs.m_mieAbsorption) &&
            lhs.m_absorption.IsClose(rhs.m_absorption) &&
            lhs.m_groundAlbedo.IsClose(rhs.m_groundAlbedo);
    }

    void SkyAtmospherePass::UpdateRenderPassSRG(const SkyAtmosphereParams& params)
    {
        m_constants.m_bottomRadius = params.m_planetRadius;
        m_constants.m_topRadius = params.m_atmosphereRadius;
        m_constants.m_sunRadiusFactor = params.m_sunRadiusFactor;
        m_constants.m_sunFalloffFactor = params.m_sunFalloffFactor;
        params.m_sunColor.GetAsVector3().StoreToFloat3(m_constants.m_sunColor);
        params.m_sunLimbColor.GetAsVector3().StoreToFloat3(m_constants.m_sunLimbColor);
        params.m_sunDirection.GetNormalized().StoreToFloat3(m_constants.m_sunDirection);
        params.m_planetOrigin.StoreToFloat3(m_constants.m_planetOrigin);

        m_constants.m_sunShadowFarClip = params.m_sunShadowsFarClip * 0.001f; // scale to km 
        m_constants.m_nearClip = params.m_nearClip;
        m_constants.m_nearFadeDistance = params.m_nearFadeDistance;
        m_constants.m_aerialDepthFactor = params.m_aerialDepthFactor;

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

        // update LUT params the first time or when they change
        if (m_lutUpdateRequired || !LutParamsEqual(m_atmosphereParams, params))
        {
            m_lutUpdateRequired = false;

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

            m_enableSkyTransmittanceLUTPass = true;
        }

        m_atmosphereParams = params;
        m_enableShadows = params.m_shadowsEnabled;
        m_enableFastSky = params.m_fastSkyEnabled;
        m_fastAerialPerspectiveEnabled = params.m_fastAerialPerspectiveEnabled;
        m_aerialPerspectiveEnabled = params.m_aerialPerspectiveEnabled;
        m_enableSun = params.m_sunEnabled;


        // UpdateRenderPassSRG can be called before the child passes are ready
        // so we store the constants and set them in FrameBeginInternal 
        m_updateConstants = true;
    }

    void SkyAtmospherePass::ResetInternal()
    {
        m_transmittanceLUTImage.reset();
        m_skyViewLUTImage.reset();
        m_skyVolumeLUTImage.reset();
        m_atmospherePassData.clear();

        Base::ResetInternal();
    }

    SkyAtmosphereFeatureProcessorInterface::AtmosphereId SkyAtmospherePass::GetAtmosphereId() const
    {
        return m_atmosphereId;
    }
} // AZ::Namespace AZ::Render
