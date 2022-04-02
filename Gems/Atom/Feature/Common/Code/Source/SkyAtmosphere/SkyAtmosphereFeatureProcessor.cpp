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


#pragma optimize("", off)
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

    void SkyAtmosphereFeatureProcessor::SetSunDirection(AtmosphereId id, const Vector3& direction)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunDirection = direction;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetSunIlluminance(AtmosphereId id, float illuminance)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_sunIlluminance = illuminance;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetMinMaxSamples(AtmosphereId id, uint32_t minSamples, uint32_t maxSamples)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_minSamples = minSamples;
        params.m_maxSamples = maxSamples;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetRaleighScattering(AtmosphereId id, const AZ::Vector3& scattering)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_rayleighScattering = scattering;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetMieScattering(AtmosphereId id, const AZ::Vector3& scattering)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_mieScattering = scattering;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetAbsorptionExtinction(AtmosphereId id, const AZ::Vector3& extinction)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_mieExtinction = extinction;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetGroundAlbedo(AtmosphereId id, const AZ::Vector3& albedo)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_groundAlbedo = albedo;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetPlanetRadius(AtmosphereId id, float radius)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_planetRadius= radius;
        m_passNeedsUpdate = true;
    }

    void SkyAtmosphereFeatureProcessor::SetAtmosphereRadius(AtmosphereId id, float radius)
    {
        auto& params = m_params.GetElement(id.GetIndex());
        params.m_atmosphereRadius = radius;
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
    }

    void SkyAtmosphereFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]]RPI::RenderPipelinePtr renderPipeline)
    {
        CachePasses();
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
                        auto params = m_params.GetElement(id.GetIndex());
                        pass->UpdateAtmospherePassSRG(id, params);
                    }
                }
            }

            m_passNeedsUpdate = false;
        }
    }
}
#pragma optimize("", on)
