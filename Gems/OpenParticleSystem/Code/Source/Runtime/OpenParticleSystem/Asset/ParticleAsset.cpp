/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleAsset.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace OpenParticle
{
    const char* ParticleAsset::DisplayName = "Particle Asset";
    const char* ParticleAsset::Group = "Particle";
    const char* ParticleAsset::Extension = "azparticle";

    void ParticleLOD::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleLOD>()
                ->Version(0)
                ->Field("m_distance", &ParticleLOD::m_distance)
                ->Field("m_emitters", &ParticleLOD::m_emitters);
        }
    }

    void ParticleCurve::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleCurve>()
                ->Version(0)
                ->Field("extrapModeLeft", &ParticleCurve::extrapModeLeft)
                ->Field("extrapModeRight", &ParticleCurve::extrapModeRight)
                ->Field("valueFactor", &ParticleCurve::valueFactor)
                ->Field("timeFactor", &ParticleCurve::timeFactor)
                ->Field("tickMode", &ParticleCurve::tickMode)
                ->Field("keyPoints", &ParticleCurve::keyPoints);
        }
    }

    void ParticleRandom::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleRandom>()
                ->Version(0)
                ->Field("min", &ParticleRandom::min)
                ->Field("max", &ParticleRandom::max)
                ->Field("tickMode", &ParticleRandom::tickMode);
        }
    }

    void ParticleDistribution::Reflect(AZ::ReflectContext* context)
    {
        ParticleCurve::Reflect(context);
        ParticleRandom::Reflect(context);
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleDistribution>()
                ->Version(0)
                ->Field("randoms", &ParticleDistribution::randoms)
                ->Field("curves", &ParticleDistribution::curves);
        }
    }

    void ParticleAsset::Reflect(AZ::ReflectContext* context)
    {
        ParticleArchive::Reflect(context);
        ParticleLOD::Reflect(context);
        ParticleDistribution::Reflect(context);
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleAsset, AZ::Data::AssetData>()
                ->Version(0)
                ->Field("archive", &ParticleAsset::m_particleArchive)
                ->Field("m_LODs", &ParticleAsset::m_lods)
                ->Field("m_distribution", &ParticleAsset::m_distribution);
        }
    }

    ParticleAsset::ParticleAsset()
    {
    }

    ParticleAsset::~ParticleAsset()
    {
        m_distribution.randoms.clear();
        m_distribution.curves.clear();

        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    void ParticleAsset::SetReady()
    {
        m_status = AssetStatus::Ready;
    }

    ParticleArchive& ParticleAsset::GetParticleArchive()
    {
        return m_particleArchive;
    }

    AZStd::vector<ParticleLOD>& ParticleAsset::GetLODs()
    {
        return m_lods;
    }

    ParticleDistribution& ParticleAsset::GetDistribution()
    {
        return m_distribution;
    }
} // namespace OpenParticle
