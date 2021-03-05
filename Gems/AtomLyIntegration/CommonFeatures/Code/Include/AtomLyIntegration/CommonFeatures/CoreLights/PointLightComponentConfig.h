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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        struct PointLightComponentConfig final
            : public ComponentConfig
        {
            AZ_RTTI(PointLightComponentConfig, "{B6FC35BA-D22F-4C20-BFFC-3FE7A48858FA}", ComponentConfig);
            static void Reflect(AZ::ReflectContext* context);

            static constexpr float DefaultIntensity = 800.0f; // 800 lumes is roughly equivalent to a 60 watt incandescent bulb
            static constexpr float DefaultBulbRadius = 0.05f; // 5cm

            AZ::Color m_color = AZ::Color::CreateOne();
            PhotometricUnit m_intensityMode = PhotometricUnit::Lumen;
            float m_intensity = DefaultIntensity;
            float m_attenuationRadius = 0.0f;
            float m_bulbRadius = DefaultBulbRadius;
            LightAttenuationRadiusMode m_attenuationRadiusMode = LightAttenuationRadiusMode::Automatic;

            // Not serialized, but used to keep scaled and unscaled properties in sync.
            float m_scale = 1.0f;

            // These values are used to deal adjusting the brightness and bulb radius based on the transform component's scale
            // so that point lights scale concistently with meshes. Not serialized.
            float m_unscaledIntensity = DefaultIntensity;
            float m_unscaledBulbRadius = DefaultBulbRadius;

            //! Updates scale and adjusts the values of intensity and bulb radius based on the new scale and the unscaled values.
            void UpdateScale(float newScale)
            {
                m_scale = newScale;

                m_intensity = m_unscaledIntensity;

                // Lumens & Candela aren't based on surface area, so scale them.
                if (!IsAreaBasedIntensityMode())
                {
                    // Light surface area and brightness increases at scale^2 because of equation of sphere surface area.
                    m_intensity *= m_scale * m_scale;
                }

                m_bulbRadius = m_unscaledBulbRadius * m_scale;
            }

            //! Updates the unscaled intensity based on the current scaled value.
            void UpdateUnscaledIntensity()
            {
                m_unscaledIntensity = m_intensity;

                // Lumens & Candela aren't based on surface area, so scale them.
                if (!IsAreaBasedIntensityMode())
                {
                    // Light surface area and brightness increases at scale^2 because of equation of sphere surface area.
                    m_unscaledIntensity /= m_scale * m_scale;
                }
            }

            //! Updates the unscaled bulb radius based on the current scaled value.
            void UpdateUnscaledBulbRadius()
            {
                m_unscaledBulbRadius = m_bulbRadius / m_scale;
            }

            // Returns true if the intensity mode is an area based light unit (not lumens or candela)
            bool IsAreaBasedIntensityMode()
            {
                return m_intensityMode != PhotometricUnit::Lumen && m_intensityMode != PhotometricUnit::Candela;
            }

            // Returns the surface area of the light bulb. 4.0 * pi * m_bulbRadius^2
            float GetArea()
            {
                return 4.0f * Constants::Pi * m_bulbRadius * m_bulbRadius;
            }

            // The following functions provide information to an EditContext...

            //! Returns true if m_attenuationRadiusMode is set to LightAttenuationRadiusMode::Automatic
            bool IsAttenuationRadiusModeAutomatic() const
            {
                return m_attenuationRadiusMode == LightAttenuationRadiusMode::Automatic;
            }

            //! Returns characters for a suffix for the light type including a space. " lm" for lumens for example.
            const char* GetIntensitySuffix() const
            {
                return PhotometricValue::GetTypeSuffix(m_intensityMode);
            }

        };
    }
}
