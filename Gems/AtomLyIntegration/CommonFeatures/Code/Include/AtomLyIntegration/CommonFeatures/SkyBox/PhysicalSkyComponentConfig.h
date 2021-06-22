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
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <SkyBox/SkyBoxFogSettings.h>

namespace AZ
{
    namespace Render
    {
        class PhysicalSkyComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::PhysicalSkyComponentConfig, "{D0A40D6B-F838-46AB-A79C-CC2218C0146C}", AZ::ComponentConfig);

            static void Reflect(ReflectContext* context);

            PhotometricUnit m_intensityMode = PhotometricUnit::Ev100Luminance;
            float m_skyIntensity = PhysicalSkyDefaultIntensity;
            float m_sunIntensity = PhysicalSunDefaultIntensity;

            int m_turbidity = 1;
            float m_sunRadiusFactor = 1.0f;

            SkyBoxFogSettings m_skyBoxFogSettings;

            //! Returns characters for a suffix for the light type including a space. " lm" for lumens for example.
            const char* GetIntensitySuffix() const;

            //! Returns the minimum intensity value allowed depending on the m_intensityMode
            float GetSkyIntensityMin() const;
            float GetSunIntensityMin() const;

            //! Returns the maximum intensity value allowed depending on the m_intensityMode
            float GetSkyIntensityMax() const;
            float GetSunIntensityMax() const;

            bool IsFogDisabled() const { return !m_skyBoxFogSettings.m_enable; }
        };
    }
}
