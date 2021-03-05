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

#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        void AreaLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AreaLightComponentConfig, ComponentConfig>()
                    ->Version(3)
                    ->Field("Color", &AreaLightComponentConfig::m_color)
                    ->Field("IntensityMode", &AreaLightComponentConfig::m_intensityMode)
                    ->Field("Intensity", &AreaLightComponentConfig::m_intensity)
                    ->Field("AttenuationRadiusMode", &AreaLightComponentConfig::m_attenuationRadiusMode)
                    ->Field("AttenuationRadius", &AreaLightComponentConfig::m_attenuationRadius)
                    ->Field("LightEmitsBothDirections", &AreaLightComponentConfig::m_lightEmitsBothDirections)
                    ->Field("UseFastApproximation", &AreaLightComponentConfig::m_useFastApproximation)
                    ;
            }
        }

        bool AreaLightComponentConfig::IsAttenuationRadiusModeAutomatic() const
        {
            return m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic;
        }

        bool AreaLightComponentConfig::Is2DSurface() const
        {
            return m_shapeType == AZ_CRC_CE("DiskShape")
                || m_shapeType == AZ_CRC_CE("QuadShape")
                || m_shapeType == AZ_CRC_CE("PolygonPrism");
        }

        bool AreaLightComponentConfig::SupportsFastApproximation() const
        {
            return m_shapeType == AZ_CRC_CE("QuadShape");
        }

        const char* AreaLightComponentConfig::GetIntensitySuffix() const
        {
            return PhotometricValue::GetTypeSuffix(m_intensityMode);
        }

        float AreaLightComponentConfig::GetIntensityMin() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Candela:
            case PhotometricUnit::Lumen:
            case PhotometricUnit::Nit:
                return 0.0f;
            case PhotometricUnit::Ev100Luminance:
                return -10.0f;
            }
            return 0.0f;
        }

        float AreaLightComponentConfig::GetIntensityMax() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Candela:
            case PhotometricUnit::Lumen:
            case PhotometricUnit::Nit:
                return 1'000'000.0f;
            case PhotometricUnit::Ev100Luminance:
                return 20.0f;
            }
            return 0.0f;
        }

        float AreaLightComponentConfig::GetIntensitySoftMin() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Candela:
            case PhotometricUnit::Lumen:
            case PhotometricUnit::Nit:
            case PhotometricUnit::Ev100Luminance:
                return 0.0f;
            }
            return 0.0f;
        }

        float AreaLightComponentConfig::GetIntensitySoftMax() const
        {
            switch (m_intensityMode)
            {
            case PhotometricUnit::Candela:
            case PhotometricUnit::Lumen:
            case PhotometricUnit::Nit:
                return 1'000.0f;
            case PhotometricUnit::Ev100Luminance:
                return 16.0f;
            }
            return 0.0f;
        }

    }
}
