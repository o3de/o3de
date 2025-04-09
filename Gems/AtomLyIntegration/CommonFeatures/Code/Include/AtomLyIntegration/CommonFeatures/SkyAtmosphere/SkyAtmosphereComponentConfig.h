/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>

namespace AZ::Render
{
    class SkyAtmosphereComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SkyAtmosphereComponentConfig, SystemAllocator)
        AZ_RTTI(AZ::Render::SkyAtmosphereComponentConfig, "{1874446D-E0AA-4DFF-83A0-F7F76C10A867}", AZ::ComponentConfig);

        enum class AtmosphereOrigin
        {
            GroundAtWorldOrigin,
            GroundAtLocalOrigin,
            PlanetCenterAtLocalOrigin
        };

        static void Reflect(AZ::ReflectContext* context);

        // ground 
        AtmosphereOrigin m_originMode = AtmosphereOrigin::GroundAtWorldOrigin;
        //! ground radius in kilometers
        float m_groundRadius = 6360.0f;
        //! atmosphere height in kilometers
        float m_atmosphereHeight = 100.0f;
        AZ::Vector3 m_groundAlbedo = AZ::Vector3(0.0f, 0.0f, 0.0f);
        AZ::Vector3 m_luminanceFactor = AZ::Vector3(1.0f, 1.0f, 1.0f);

        // rayleigh (air) scattering
        float m_rayleighScatteringScale = 0.033100f;
        AZ::Vector3 m_rayleighScattering = AZ::Vector3(0.175287f, 0.409607f, 1.f);
        float m_rayleighExponentialDistribution = 8.f;

        // mie (aerosols) scattering
        float m_mieScatteringScale = 0.003996f;
        AZ::Vector3 m_mieScattering = AZ::Vector3(1., 1.f, 1.f);
        float m_mieAbsorptionScale = 0.004440f;
        AZ::Vector3 m_mieAbsorption = AZ::Vector3(1.f, 1.f, 1.f);
        float m_mieExponentialDistribution = 1.2f;

        // absorption
        float m_absorptionScale = 0.001881f;
        AZ::Vector3 m_absorption = AZ::Vector3(0.345561f, 1.f, 0.045188f);

        // sun
        bool m_drawSun = true;
        AZ::EntityId m_sun; // optional sun entity to use for orientation
        AZ::Color m_sunColor = AZ::Color(1.f, 1.f, 1.f, 1.f);
        AZ::Color m_sunLimbColor = AZ::Color(1.f, 1.f, 1.f, 1.f);
        float m_sunLuminanceFactor = 0.05f;
        float m_sunRadiusFactor = 1.0f;
        float m_sunFalloffFactor = 1.0f;
        float m_aerialDepthFactor = 1.0f;

        // advanced
        float m_nearClip = 0.f;
        float m_nearFadeDistance = 0.f;
        bool m_fastSkyEnabled = true;
        bool m_fastAerialPerspectiveEnabled = true;
        bool m_aerialPerspectiveEnabled = true;
        bool m_shadowsEnabled = false;
        uint8_t m_minSamples = 4;
        uint8_t m_maxSamples = 14;
    };
} // namespace AZ::Render
