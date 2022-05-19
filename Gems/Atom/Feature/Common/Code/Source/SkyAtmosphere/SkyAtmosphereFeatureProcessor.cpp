/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkyAtmosphere/SkyAtmosphereFeatureProcessor.h>

#include <AzCore/Math/MatrixUtils.h>
#include <Math/GaussianMathFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>


namespace AZ::Render
{
    void SkyAtmosphereFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<SkyAtmosphereFeatureProcessor, FeatureProcessor>()
                ->Version(0);
        }
    }
    
    void SkyAtmosphereFeatureProcessor::Activate()
    {
        CachePasses();
        EnableSceneNotification();
    }
    
    void SkyAtmosphereFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();

        m_params.Clear();

        m_skyAtmosphereParentPasses.clear();
    }

    
    SkyAtmosphereFeatureProcessor::AtmosphereId SkyAtmosphereFeatureProcessor::CreateAtmosphere()
    {
        size_t index = m_params.Reserve();
        if (index >= std::numeric_limits<AtmosphereId::IndexType>::max())
        {
            m_params.Release(index);
            return AtmosphereId::Null;
        }

        AtmosphereId id = AtmosphereId(aznumeric_cast<AtmosphereId::IndexType>(index));
        m_atmosphereIds.insert(id);
        InitializeAtmosphere(id);

        return id;
    }

    void SkyAtmosphereFeatureProcessor::ReleaseAtmosphere(AtmosphereId id)
    {
        if (id.IsValid())
        {
            m_params.Release(id.GetIndex());
        }
        m_atmosphereIds.erase(id);

        for (auto pass : m_skyAtmosphereParentPasses )
        {
            pass->ReleaseAtmospherePass(id);
        }

        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::Enable(AtmosphereId id, bool enable)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_enabled = enable;
        m_passNeedsUpdate = true;
    }

    bool SkyAtmosphereFeatureProcessor::IsEnabled(AtmosphereId id)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        return params.m_enabled;
    }

    void SkyAtmosphereFeatureProcessor::SetFastSkyEnabled(AtmosphereId id, bool enabled)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_fastSkyEnabled = enabled;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetSunDirection(AtmosphereId id, const Vector3& direction)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunDirection = direction;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetLuminanceFactor(AtmosphereId id, const AZ::Vector3& factor)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_luminanceFactor = factor;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetMinMaxSamples(AtmosphereId id, uint32_t minSamples, uint32_t maxSamples)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_minSamples = minSamples;
        params.m_maxSamples = maxSamples;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetRayleighScattering(AtmosphereId id, const AZ::Vector3& scattering)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_rayleighScattering = scattering;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetRayleighExpDistribution(AtmosphereId id, float distribution)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_rayleighExpDistribution = distribution;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetMieScattering(AtmosphereId id, const AZ::Vector3& scattering)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_mieScattering = scattering;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetMieAbsorption(AtmosphereId id, const AZ::Vector3& absorption)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_mieAbsorption = absorption;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetMieExpDistribution(AtmosphereId id, float distribution)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_mieExpDistribution = distribution;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetAbsorption(AtmosphereId id, const AZ::Vector3& absorption)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_mieAbsorption = absorption;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetGroundAlbedo(AtmosphereId id, const AZ::Vector3& albedo)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_groundAlbedo = albedo;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetPlanetOrigin(AtmosphereId id, const AZ::Vector3& planetOrigin)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_planetOrigin = planetOrigin;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetPlanetRadius(AtmosphereId id, float radius)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_planetRadius= radius;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetAtmosphereRadius(AtmosphereId id, float radius)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_atmosphereRadius = radius;
        params.m_lutUpdateRequired = true;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetShadowsEnabled(AtmosphereId id, bool enabled)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_shadowsEnabled = enabled;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetSunEnabled(AtmosphereId id, bool enabled)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunEnabled = enabled;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetSunColor(AtmosphereId id, const Color& color)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunColor = color;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetSunFalloffFactor(AtmosphereId id, float factor)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunFalloffFactor = factor;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetSunRadiusFactor(AtmosphereId id, float factor)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunRadiusFactor = factor;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::InitializeAtmosphere(AtmosphereId id)
    {
        m_passNeedsUpdate = true;
        for (auto pass : m_skyAtmosphereParentPasses )
        {
            pass->CreateAtmospherePass(id);
        }
    }

    void SkyAtmosphereFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
    {
        CachePasses();
        UpdateBackgroundClearColor();
    }

    void SkyAtmosphereFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]]RPI::RenderPipelinePtr renderPipeline)
    {
        CachePasses();
        UpdateBackgroundClearColor();
    }

    void SkyAtmosphereFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
    {
        CachePasses();
    }
    
    void SkyAtmosphereFeatureProcessor::CachePasses()
    {
        m_skyAtmosphereParentPasses.clear();

        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithTemplateName(Name("SkyAtmosphereParentTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [this](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
            {
                SkyAtmosphereParentPass* parentPass = static_cast<SkyAtmosphereParentPass*>(pass);
                m_skyAtmosphereParentPasses.emplace_back(parentPass);
                return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            });


        // make sure we update all the LUTS
        for (auto id : m_atmosphereIds)
        {
            if (id.IsValid())
            {
                auto& params = m_params.GetElement(id.GetIndex());
                params.m_lutUpdateRequired = true;

                // make sure all removed atmosphere passes are restored
                InitializeAtmosphere(id);
            }
        }

        m_passNeedsUpdate = true;
    }
    
    void SkyAtmosphereFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "SkyAtmosphereFeatureProcessor: Simulate");

        if (m_passNeedsUpdate)
        {
            for (auto pass : m_skyAtmosphereParentPasses)
            {
                for (auto id : m_atmosphereIds)
                {
                    if (id.IsValid())
                    {
                        auto& params = m_params.GetElement(id.GetIndex());
                        pass->UpdateAtmospherePassSRG(id, params);

                        // reset the lut update flag now that we've passed on the changes
                        params.m_lutUpdateRequired = false;
                    }
                }
            }

            m_passNeedsUpdate = false;
        }
    }

    void SkyAtmosphereFeatureProcessor::UpdateBackgroundClearColor()
    {
        // This function is only necessary for now because the default clear value
        // color is not black, and is set in various .pass files in places a user
        // is unlikely to find.  Unfortunately, the viewport will revert to the
        // grey color when resizing momentarily.
        const RHI::ClearValue blackClearValue = RHI::ClearValue::CreateVector4Float(0.f, 0.f, 0.f, 0.f);
        RPI::PassFilter passFilter;
        AZStd::string slot;

        auto setClearValue = [&](RPI::Pass* pass)-> RPI::PassFilterExecutionFlow
        {
            Name slotName = Name::FromStringLiteral(slot);
            if (auto binding = pass->FindAttachmentBinding(slotName))
            {
                binding->m_unifiedScopeDesc.m_loadStoreAction.m_clearValue = blackClearValue;
            }
            return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
        };

        slot = "SpecularOutput";
        passFilter= RPI::PassFilter::CreateWithTemplateName(Name("ForwardPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
        passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ForwardMSAAPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);

        slot = "ReflectionOutput";
        passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ReflectionGlobalFullscreenPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
    }
}
