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
#include <Atom/Feature/CoreLights/ShadowConstants.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        struct SpotLightComponentConfig final
            : ComponentConfig
        {
            AZ_RTTI(SpotLightComponentConfig, "{20C882C8-615E-4272-93A8-BE9102E6EFED}", ComponentConfig);
            static void Reflect(AZ::ReflectContext* context);

            AZ::Color m_color = AZ::Color::CreateOne();
            float m_intensity = 100.0f;
            PhotometricUnit m_intensityMode = PhotometricUnit::Lumen;
            float m_bulbRadius = 0.075;
            float m_innerConeDegrees = 45.0f;
            float m_outerConeDegrees = 55.0f;
            float m_attenuationRadius = 20.0f;
            float m_penumbraBias = 0.0f;
            LightAttenuationRadiusMode m_attenuationRadiusMode = LightAttenuationRadiusMode::Automatic;
            bool m_enabledShadow = false;
            ShadowmapSize m_shadowmapSize = MaxShadowmapImageSize;
            ShadowFilterMethod m_shadowFilterMethod = ShadowFilterMethod::None;
            float m_boundaryWidthInDegrees = 0.25f;
            uint16_t m_predictionSampleCount = 4;
            uint16_t m_filteringSampleCount = 32;

            // The following functions provide information to an EditContext...

            //! Returns true if m_attenuationRadiusMode is set to LightAttenuationRadiusMode::Automatic
            bool IsAttenuationRadiusModeAutomatic() const;

            //! Returns characters for a suffix for the light type including a space. " lm" for lumens for example.
            const char* GetIntensitySuffix() const;

            float GetConeDegrees() const;
            bool IsShadowFilteringDisabled() const;
            bool IsShadowPcfDisabled() const;
        };
    }
}
