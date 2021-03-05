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
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        struct AreaLightComponentConfig final
            : public ComponentConfig
        {
            AZ_RTTI(AZ::Render::AreaLightComponentConfig, "{11C08FED-7F94-4926-8517-46D08E4DD837}", ComponentConfig);
            static void Reflect(AZ::ReflectContext* context);

            static constexpr float CutoffIntensity = 0.1f;

            AZ::Color m_color = AZ::Color::CreateOne();
            float m_intensity = 100.0f;
            float m_attenuationRadius = 0.0f;
            PhotometricUnit m_intensityMode = PhotometricUnit::Lumen;
            LightAttenuationRadiusMode m_attenuationRadiusMode = LightAttenuationRadiusMode::Automatic;
            bool m_lightEmitsBothDirections = false;
            bool m_useFastApproximation = false;
            AZ::Crc32 m_shapeType;

            // The following functions provide information to an EditContext...

            //! Returns true if m_attenuationRadiusMode is set to LightAttenuationRadiusMode::Automatic
            bool IsAttenuationRadiusModeAutomatic() const;

            //! Returns true if the shape type is a 2D surface
            bool Is2DSurface() const;

            //! Returns true if the light type supports a faster and less accurate approximation for the lighting algorithm.
            bool SupportsFastApproximation() const;

            //! Returns characters for a suffix for the light type including a space. " lm" for lumens for example.
            const char* GetIntensitySuffix() const;

            //! Returns the minimum intensity value allowed depending on the m_intensityMode
            float GetIntensityMin() const;

            //! Returns the maximum intensity value allowed depending on the m_intensityMode
            float GetIntensityMax() const;

            //! Returns the minimum intensity value for UI depending on the m_intensityMode, but users may still type in a lesser value depending on GetIntensityMin().
            float GetIntensitySoftMin() const;

            //! Returns the maximum intensity value for UI depending on the m_intensityMode, but users may still type in a greater value depending on GetIntensityMin().
            float GetIntensitySoftMax() const;

        };
    }
}
