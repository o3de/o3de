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

#include <AtomLyIntegration/CommonFeatures/CoreLights/SpotLightComponentConfig.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void SpotLightComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpotLightComponentConfig, ComponentConfig>()
                    ->Version(4)
                    ->Field("Color", &SpotLightComponentConfig::m_color)
                    ->Field("Intensity", &SpotLightComponentConfig::m_intensity)
                    ->Field("IntensityMode", &SpotLightComponentConfig::m_intensityMode)
                    ->Field("Bulb Radius", &SpotLightComponentConfig::m_bulbRadius)
                    ->Field("Inner Cone Angle", &SpotLightComponentConfig::m_innerConeDegrees)
                    ->Field("Outer Cone Angle", &SpotLightComponentConfig::m_outerConeDegrees)
                    ->Field("Attenuation Radius", &SpotLightComponentConfig::m_attenuationRadius)
                    ->Field("Attenuation Radius Mode", &SpotLightComponentConfig::m_attenuationRadiusMode)
                    ->Field("Penumbra Bias", &SpotLightComponentConfig::m_penumbraBias)
                    ->Field("Enabled Shadow", &SpotLightComponentConfig::m_enabledShadow)
                    ->Field("Shadowmap Size", &SpotLightComponentConfig::m_shadowmapSize)
                    ->Field("Shadow Filter Method", &SpotLightComponentConfig::m_shadowFilterMethod)
                    ->Field("Softening Boundary Width", &SpotLightComponentConfig::m_boundaryWidthInDegrees)
                    ->Field("Prediction Sample Count", &SpotLightComponentConfig::m_predictionSampleCount)
                    ->Field("Filtering Sample Count", &SpotLightComponentConfig::m_filteringSampleCount)
                    ->Field("Pcf Method", &SpotLightComponentConfig::m_pcfMethod);
            }
        }

        bool SpotLightComponentConfig::IsAttenuationRadiusModeAutomatic() const
        {
            return m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic;
        }

        const char* SpotLightComponentConfig::GetIntensitySuffix() const
        {
            return PhotometricValue::GetTypeSuffix(m_intensityMode);
        }

        float SpotLightComponentConfig::GetConeDegrees() const
        {
            return (m_enabledShadow && m_shadowmapSize != ShadowmapSize::None) ? MaxSpotLightConeAngleDegreeWithShadow : MaxSpotLightConeAngleDegree;
        }

        bool SpotLightComponentConfig::IsShadowFilteringDisabled() const
        {
            return (m_shadowFilterMethod == ShadowFilterMethod::None);
        }

        bool SpotLightComponentConfig::IsShadowPcfDisabled() const
        {
            return !(m_shadowFilterMethod == ShadowFilterMethod::Pcf ||
                m_shadowFilterMethod == ShadowFilterMethod::EsmPcf);
        }

        bool SpotLightComponentConfig::IsPcfBoundarySearchDisabled() const
        {
            if (IsShadowPcfDisabled())
            {
                return true;
            }

            return m_pcfMethod != PcfMethod::BoundarySearch;
        }

    } // namespace Render
} // namespace AZ
