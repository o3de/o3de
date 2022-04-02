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

        virtual void SetSunDirection(AtmosphereId id, const Vector3& direction) = 0;
        virtual void SetSunIlluminance(AtmosphereId id, float illuminance) = 0;
        virtual void SetMinMaxSamples(AtmosphereId id, uint32_t minSamples, uint32_t maxSamples) = 0;
        virtual void SetRaleighScattering(AtmosphereId id, const AZ::Vector3& scattering) = 0;
        virtual void SetMieScattering(AtmosphereId id, const AZ::Vector3& scattering) = 0;
        virtual void SetAbsorptionExtinction(AtmosphereId id, const AZ::Vector3& extinction) = 0;
        virtual void SetGroundAlbedo(AtmosphereId id, const AZ::Vector3& albedo) = 0;

        virtual void SetPlanetRadius(AtmosphereId id, float radius) = 0;
        virtual void SetAtmosphereRadius(AtmosphereId id, float radius) = 0;
    };
} // namespace AZ::Render
