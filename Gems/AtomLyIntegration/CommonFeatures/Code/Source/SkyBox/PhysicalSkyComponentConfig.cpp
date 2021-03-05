/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
