/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    namespace Render
    {
        void AreaLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AreaLightComponentConfig, ComponentConfig>()
                    ->Version(7) // ATOM-16034
                    ->Field("LightType", &AreaLightComponentConfig::m_lightType)
                    ->Field("Color", &AreaLightComponentConfig::m_color)
                    ->Field("IntensityMode", &AreaLightComponentConfig::m_intensityMode)
                    ->Field("Intensity", &AreaLightComponentConfig::m_intensity)
                    ->Field("AttenuationRadiusMode", &AreaLightComponentConfig::m_attenuationRadiusMode)
                    ->Field("AttenuationRadius", &AreaLightComponentConfig::m_attenuationRadius)
                    ->Field("LightEmitsBothDirections", &AreaLightComponentConfig::m_lightEmitsBothDirections)
                    ->Field("UseFastApproximation", &AreaLightComponentConfig::m_useFastApproximation)
                    // Shutters
                    ->Field("EnableShutters", &AreaLightComponentConfig::m_enableShutters)
                    ->Field("InnerShutterAngleDegrees", &AreaLightComponentConfig::m_innerShutterAngleDegrees)
                    ->Field("OuterShutterAngleDegrees", &AreaLightComponentConfig::m_outerShutterAngleDegrees)
                    // Shadows
                    ->Field("Enable Shadow", &AreaLightComponentConfig::m_enableShadow)
                    ->Field("Shadow Bias", &AreaLightComponentConfig::m_bias)
                    ->Field("Normal Shadow Bias", &AreaLightComponentConfig::m_normalShadowBias)
                    ->Field("Shadowmap Max Size", &AreaLightComponentConfig::m_shadowmapMaxSize)
                    ->Field("Shadow Filter Method", &AreaLightComponentConfig::m_shadowFilterMethod)
                    ->Field("Filtering Sample Count", &AreaLightComponentConfig::m_filteringSampleCount)
                    ->Field("Esm Exponent", &AreaLightComponentConfig::m_esmExponent)
                    ;
            }
        }
        
        AZStd::vector<Edit::EnumConstant<PhotometricUnit>> AreaLightComponentConfig::GetValidPhotometricUnits() const
        {
            AZStd::vector<Edit::EnumConstant<PhotometricUnit>> enumValues =
            {
                // Candela & lumen always supported.
                Edit::EnumConstant<PhotometricUnit>(PhotometricUnit::Candela, "Candela"),
                Edit::EnumConstant<PhotometricUnit>(PhotometricUnit::Lumen, "Lumen"),
            };

            if (RequiresShapeComponent())
            {
                // Lights with surface area also support nits and ev100.
                enumValues.push_back(Edit::EnumConstant<PhotometricUnit>(PhotometricUnit::Nit, "Nit"));
                enumValues.push_back(Edit::EnumConstant<PhotometricUnit>(PhotometricUnit::Ev100Luminance, "Ev100"));
            }
            return enumValues;
        }

        bool AreaLightComponentConfig::RequiresShapeComponent() const
        {
            return m_lightType == LightType::Sphere
                || m_lightType == LightType::SpotDisk
                || m_lightType == LightType::Capsule
                || m_lightType == LightType::Quad
                || m_lightType == LightType::Polygon;
        }

        bool AreaLightComponentConfig::LightTypeIsSelected() const
        {
            return m_lightType != LightType::Unknown;
        }

        bool AreaLightComponentConfig::IsAttenuationRadiusModeAutomatic() const
        {
            return m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic;
        }

        bool AreaLightComponentConfig::SupportsBothDirections() const
        {
            return m_lightType == LightType::Quad
                || m_lightType == LightType::Polygon;
        }

        bool AreaLightComponentConfig::SupportsFastApproximation() const
        {
            return m_lightType == LightType::Quad;
        }
        
        bool AreaLightComponentConfig::SupportsShutters() const
        {
            return m_lightType == LightType::SimpleSpot
                || m_lightType == LightType::SpotDisk;
        }

        bool AreaLightComponentConfig::ShuttersMustBeEnabled() const
        {
            return m_lightType == LightType::SpotDisk;
        }

        bool AreaLightComponentConfig::ShuttersDisabled() const
        {
            return  m_lightType == LightType::SpotDisk && !m_enableShutters;
        }

        bool AreaLightComponentConfig::SupportsShadows() const
        {
            return m_lightType == LightType::SpotDisk || m_lightType == LightType::Sphere;
        }
        
        bool AreaLightComponentConfig::ShadowsDisabled() const
        {
            return !m_enableShadow;
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
                return AZStd::numeric_limits<float>::lowest();
            }
            return 0.0f;
        }

        float AreaLightComponentConfig::GetIntensityMax() const
        {
            // While there is no hard-max, a max must be included when there is a hard min.
            return AZStd::numeric_limits<float>::max();
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
        
        bool AreaLightComponentConfig::IsShadowFilteringDisabled() const
        {
            return (m_shadowFilterMethod == ShadowFilterMethod::None);
        }

        bool AreaLightComponentConfig::IsShadowPcfDisabled() const
        {
            return !(m_shadowFilterMethod == ShadowFilterMethod::Pcf ||
                m_shadowFilterMethod == ShadowFilterMethod::EsmPcf);
        }

        bool AreaLightComponentConfig::IsEsmDisabled() const
        {
            return !(m_shadowFilterMethod == ShadowFilterMethod::Esm || m_shadowFilterMethod == ShadowFilterMethod::EsmPcf);
        }
    }
}
