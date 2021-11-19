/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/EditContext.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        struct AreaLightComponentConfig final
            : public ComponentConfig
        {
            AZ_RTTI(AZ::Render::AreaLightComponentConfig, "{11C08FED-7F94-4926-8517-46D08E4DD837}", ComponentConfig);
            static void Reflect(AZ::ReflectContext* context);

            enum class LightType : uint8_t
            {
                Unknown,
                Sphere,
                SpotDisk,
                Capsule,
                Quad,
                Polygon,
                SimplePoint,
                SimpleSpot,

                LightTypeCount,
            };

            static constexpr float CutoffIntensity = 0.1f;

            AZ::Color m_color = AZ::Color::CreateOne();
            float m_intensity = 100.0f;
            float m_attenuationRadius = 0.0f;
            PhotometricUnit m_intensityMode = PhotometricUnit::Lumen;
            LightAttenuationRadiusMode m_attenuationRadiusMode = LightAttenuationRadiusMode::Automatic;
            bool m_lightEmitsBothDirections = false;
            bool m_useFastApproximation = false;
            AZ::Crc32 m_shapeType;

            bool m_enableShutters = false;
            LightType m_lightType = LightType::Unknown;
            float m_innerShutterAngleDegrees = 35.0f;
            float m_outerShutterAngleDegrees = 45.0f;

            // Shadows (only used for supported shapes)
            bool m_enableShadow = false;
            float m_bias = 0.1f;
            ShadowmapSize m_shadowmapMaxSize = ShadowmapSize::Size256;
            ShadowFilterMethod m_shadowFilterMethod = ShadowFilterMethod::None;
            uint16_t m_filteringSampleCount = 12;
            float m_esmExponent = 87.0f;

            // The following functions provide information to an EditContext...

            AZStd::vector<Edit::EnumConstant<PhotometricUnit>> GetValidPhotometricUnits() const;

            bool RequiresShapeComponent() const;

            //! Returns true if the light type is anything other than unknown.
            bool LightTypeIsSelected() const;

            //! Returns true if m_attenuationRadiusMode is set to LightAttenuationRadiusMode::Automatic
            bool IsAttenuationRadiusModeAutomatic() const;

            //! Returns true if the shape type can emit light from both sides
            bool SupportsBothDirections() const;

            //! Returns true if the light type supports a faster and less accurate approximation for the lighting algorithm.
            bool SupportsFastApproximation() const;

            //! Returns true if the light type supports restricting the light beam to an angle
            bool SupportsShutters() const;

            //! Returns true if the light type supports shutters, but they must be turned on.
            bool ShuttersMustBeEnabled() const;

            //! Returns true if shutters are turned off
            bool ShuttersDisabled() const;

            //! Returns true if the light type supports shadows.
            bool SupportsShadows() const;
            
            //! Returns true if shadows are turned on
            bool ShadowsDisabled() const;

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

            //! Returns true if shadow filtering is disabled.
            bool IsShadowFilteringDisabled() const;
            
            //! Returns true if pcf shadows are disabled.
            bool IsShadowPcfDisabled() const;
            
            //! Returns true if exponential shadow maps are disabled.
            bool IsEsmDisabled() const;
        };
    }
}
