/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>

namespace AZ::Render
{
    class SkyAtmosphereComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(AZ::Render::SkyAtmosphereComponentConfig, "{1874446D-E0AA-4DFF-83A0-F7F76C10A867}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        float m_atmosphereRadius = 6460.0f;
        float m_planetRadius = 6360.0f;

        AZ::EntityId m_sun; // optional sun entity to use for orientation
        float m_sunIlluminance = 1.0f;
        uint32_t m_minSamples = 4;
        uint32_t m_maxSamples = 14;
        AZ::Vector3 m_rayleighScattering = AZ::Vector3(0.005802f, 0.013558f, 0.033100f);
        AZ::Vector3 m_mieScattering = AZ::Vector3(0.003996f, 0.003996f, 0.003996f);
        AZ::Vector3 m_absorptionExtinction = AZ::Vector3(0.000650f, 0.001881f, 0.000085f);
        AZ::Vector3 m_mieExtinction = AZ::Vector3(0.004440f, 0.004440f, 0.004440f);
        AZ::Vector3 m_groundAlbedo = AZ::Vector3(0.0f, 0.0f, 0.0f);
    };
} // namespace AZ::Render
