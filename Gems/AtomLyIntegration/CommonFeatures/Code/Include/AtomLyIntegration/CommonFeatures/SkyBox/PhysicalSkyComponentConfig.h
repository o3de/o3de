/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/SkyBox/SkyboxConstants.h>
#include <Atom/Feature/SkyBox/SkyBoxFogSettings.h>

namespace AZ
{
    namespace Render
    {
        class PhysicalSkyComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_CLASS_ALLOCATOR(PhysicalSkyComponentConfig, SystemAllocator)
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
