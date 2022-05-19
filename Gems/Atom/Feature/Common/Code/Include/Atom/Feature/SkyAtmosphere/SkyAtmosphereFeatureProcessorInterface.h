/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::Render
{
    class SkyAtmosphereFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
        public:
        AZ_RTTI(AZ::Render::SkyAtmosphereFeatureProcessorInterface, "{00C9FD3D-2A1B-49EA-97E3-952EF6C1C451}"); 

        using AtmosphereId = RHI::Handle<uint16_t, SkyAtmosphereFeatureProcessorInterface>;

        virtual AtmosphereId CreateAtmosphere() = 0;
        virtual void ReleaseAtmosphere(AtmosphereId id) = 0;

        virtual void Enable(AtmosphereId id, bool enable) = 0;
        virtual bool IsEnabled(AtmosphereId id) = 0;

        //! Set the ozone layer absorption values
        virtual void SetAbsorption(AtmosphereId id, const AZ::Vector3& absorption) = 0;

        //! Set the atmosphere radius in kilometers
        virtual void SetAtmosphereRadius(AtmosphereId id, float radiusInKm) = 0;

        //! Enable or disable the sky view LUT optimization
        virtual void SetFastSkyEnabled(AtmosphereId id, bool enabled) = 0;

        //! Set the ground albedo
        virtual void SetGroundAlbedo(AtmosphereId id, const AZ::Vector3& albedo) = 0;

        //! Set the sky luminance to artistically adjust the sky brightness
        virtual void SetLuminanceFactor(AtmosphereId id, const AZ::Vector3& factor) = 0;

        //! Set Mie (aerosole) particle scattering options
        virtual void SetMieScattering(AtmosphereId id, const AZ::Vector3& scattering) = 0;
        virtual void SetMieAbsorption(AtmosphereId id, const AZ::Vector3& absorption) = 0;
        virtual void SetMieExpDistribution(AtmosphereId id, float distribution) = 0;

        //! Adjust how many samples are used when sampling the atmosphere
        virtual void SetMinMaxSamples(AtmosphereId id, uint32_t minSamples, uint32_t maxSamples) = 0;

        //! Set the world-space origin of the planet
        virtual void SetPlanetOrigin(AtmosphereId id, const AZ::Vector3& planetOrigin) = 0;

        //! Set the radius of the planet in kilometers
        virtual void SetPlanetRadius(AtmosphereId id, float radiusInKm) = 0;

        //! Set Rayleigh (air) molecule scattering options
        virtual void SetRayleighScattering(AtmosphereId id, const AZ::Vector3& scattering) = 0;
        virtual void SetRayleighExpDistribution(AtmosphereId id, float distribution) = 0;

        //! Enable/disable atmosphere shadowing
        virtual void SetShadowsEnabled(AtmosphereId id, bool enabled) = 0;

        //! Sun options
        virtual void SetSunEnabled(AtmosphereId id, bool enabled) = 0;
        virtual void SetSunDirection(AtmosphereId id, const AZ::Vector3& direction) = 0;
        virtual void SetSunColor(AtmosphereId id, const AZ::Color& color) = 0;
        virtual void SetSunLimbColor(AtmosphereId id, const AZ::Color& color) = 0;
        virtual void SetSunFalloffFactor(AtmosphereId id, float factor) = 0;
        virtual void SetSunRadiusFactor(AtmosphereId id, float factor) = 0;
    };
} // namespace AZ::Render
