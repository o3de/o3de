/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/SkyBox/PhysicalSkyComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void PhysicalSkyComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PhysicalSkyComponentConfig, ComponentConfig>()
                    ->Version(3)
                    ->Field("IntensityMode", &PhysicalSkyComponentConfig::m_intensityMode)
                    ->Field("SkyIntensity", &PhysicalSkyComponentConfig::m_skyIntensity)
                    ->Field("SunIntensity", &PhysicalSkyComponentConfig::m_sunIntensity)
                    ->Field("Turbidity", &PhysicalSkyComponentConfig::m_turbidity)
                    ->Field("SunRadiusFactor", &PhysicalSkyComponentConfig::m_sunRadiusFactor)
                    ->Field("FogSettings", &PhysicalSkyComponentConfig::m_skyBoxFogSettings)
                    ;
            }
        }

        const char* PhysicalSkyComponentConfig::GetIntensitySuffix() const
        {
            return PhotometricValue::GetTypeSuffix(m_intensityMode);
        }

        float PhysicalSkyComponentConfig::GetSunIntensityMin() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Nit:
                return 0.1f;
            case PhotometricUnit::Ev100Luminance:
                return -4.0f;
            }
            return 0.0f;
        }

        float PhysicalSkyComponentConfig::GetSunIntensityMax() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Nit:
                return 100'000.0f;
            case PhotometricUnit::Ev100Luminance:
                return 16.0f;
            }
            return 0.0f;
        }

        float PhysicalSkyComponentConfig::GetSkyIntensityMin() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Nit:
                return 0.1f;
            case PhotometricUnit::Ev100Luminance:
                return -4.0f;
            }
            return 0.0f;
        }

        float PhysicalSkyComponentConfig::GetSkyIntensityMax() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Nit:
                return 5000.0f;
            case PhotometricUnit::Ev100Luminance:
                return 11.0;
            }
            return 0.0f;
        }
    }
}
