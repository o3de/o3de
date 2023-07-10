/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/SkyAtmosphere/SkyAtmosphereComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    void SkyAtmosphereComponentConfig::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereComponentConfig, ComponentConfig>()
                ->Version(2)
                ->Field("OriginMode", &SkyAtmosphereComponentConfig::m_originMode)
                ->Field("AtmosphereHeight", &SkyAtmosphereComponentConfig::m_atmosphereHeight)
                ->Field("GroundAlbedo", &SkyAtmosphereComponentConfig::m_groundAlbedo)
                ->Field("GroundRadius", &SkyAtmosphereComponentConfig::m_groundRadius)
                ->Field("LuminanceFactor", &SkyAtmosphereComponentConfig::m_luminanceFactor)
                ->Field("DrawSun", &SkyAtmosphereComponentConfig::m_drawSun)
                ->Field("SunColor", &SkyAtmosphereComponentConfig::m_sunColor)
                ->Field("SunLuminanceFactor", &SkyAtmosphereComponentConfig::m_sunLuminanceFactor)
                ->Field("SunLimbColor", &SkyAtmosphereComponentConfig::m_sunLimbColor)
                ->Field("SunOrientation", &SkyAtmosphereComponentConfig::m_sun)
                ->Field("SunRadiusFactor", &SkyAtmosphereComponentConfig::m_sunRadiusFactor)
                ->Field("SunFalloffFactor", &SkyAtmosphereComponentConfig::m_sunFalloffFactor)
                ->Field("MinSamples", &SkyAtmosphereComponentConfig::m_minSamples)
                ->Field("MaxSamples", &SkyAtmosphereComponentConfig::m_maxSamples)
                ->Field("MieAbsorption", &SkyAtmosphereComponentConfig::m_mieAbsorption)
                ->Field("MieAbsorptionScale", &SkyAtmosphereComponentConfig::m_mieAbsorptionScale)
                ->Field("MieScattering", &SkyAtmosphereComponentConfig::m_mieScattering)
                ->Field("MieScatteringScale", &SkyAtmosphereComponentConfig::m_mieScatteringScale)
                ->Field("MieExpDistribution", &SkyAtmosphereComponentConfig::m_mieExponentialDistribution)
                ->Field("Absorption", &SkyAtmosphereComponentConfig::m_absorption)
                ->Field("AbsorptionScale", &SkyAtmosphereComponentConfig::m_absorptionScale)
                ->Field("RayleighScattering", &SkyAtmosphereComponentConfig::m_rayleighScattering)
                ->Field("RayleighScatteringScale", &SkyAtmosphereComponentConfig::m_rayleighScatteringScale)
                ->Field("RayleighExpDistribution", &SkyAtmosphereComponentConfig::m_rayleighExponentialDistribution)
                ->Field("ShadowsEnabled", &SkyAtmosphereComponentConfig::m_shadowsEnabled)
                ->Field("FastSkyEnabled", &SkyAtmosphereComponentConfig::m_fastSkyEnabled)
                ->Field("NearClip", &SkyAtmosphereComponentConfig::m_nearClip)
                ->Field("NearFadeDistance", &SkyAtmosphereComponentConfig::m_nearFadeDistance)
                ->Field("FastAerialPerspectiveEnabled", &SkyAtmosphereComponentConfig::m_fastAerialPerspectiveEnabled)
                ->Field("AerialPerspectiveEnabled", &SkyAtmosphereComponentConfig::m_aerialPerspectiveEnabled)
                ->Field("AerialDepthFactor", &SkyAtmosphereComponentConfig::m_aerialDepthFactor)
                ;
        }
    }
} // AZ::Render
