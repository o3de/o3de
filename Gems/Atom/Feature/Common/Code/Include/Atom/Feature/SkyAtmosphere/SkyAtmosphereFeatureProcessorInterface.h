/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Color.h>

namespace AZ::Render
{
    struct SkyAtmosphereParams
    {
        //! Params that are used for LUT generation
        AZ::Vector3 m_luminanceFactor;
        AZ::Vector3 m_rayleighScattering;
        AZ::Vector3 m_mieScattering;
        AZ::Vector3 m_mieAbsorption;
        AZ::Vector3 m_absorption;
        AZ::Vector3 m_groundAlbedo;
        float m_rayleighExpDistribution;
        float m_mieExpDistribution;
        float m_planetRadius;
        float m_atmosphereRadius;

        //! General params
        AZ::Vector3 m_planetOrigin;
        uint8_t m_minSamples;
        uint8_t m_maxSamples;
        AZ::Vector3 m_sunDirection;
        AZ::Color m_sunColor;
        AZ::Color m_sunLimbColor;
        float m_sunFalloffFactor;
        float m_sunRadiusFactor;
        float m_sunShadowsFarClip;
        float m_nearClip;
        float m_nearFadeDistance;
        float m_aerialDepthFactor;
        bool m_shadowsEnabled;
        bool m_sunEnabled;
        bool m_fastSkyEnabled;
        bool m_fastAerialPerspectiveEnabled;
        bool m_aerialPerspectiveEnabled;
    };

    class SkyAtmosphereFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
        public:
        AZ_RTTI(AZ::Render::SkyAtmosphereFeatureProcessorInterface, "{00C9FD3D-2A1B-49EA-97E3-952EF6C1C451}"); 

        using AtmosphereId = RHI::Handle<uint16_t, SkyAtmosphereFeatureProcessorInterface>;

        virtual AtmosphereId CreateAtmosphere() = 0;
        virtual void ReleaseAtmosphere(AtmosphereId id) = 0;
        virtual void SetAtmosphereParams(AtmosphereId id, const SkyAtmosphereParams& params) = 0;
        virtual void SetAtmosphereEnabled(AtmosphereId id, bool enabled) = 0;
        virtual bool GetAtmosphereEnabled(AtmosphereId id) = 0;
    };
} // namespace AZ::Render
