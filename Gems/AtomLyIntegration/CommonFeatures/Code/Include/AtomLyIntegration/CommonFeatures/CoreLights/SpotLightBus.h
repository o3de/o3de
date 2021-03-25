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
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/SpotLightComponentConfig.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    namespace Render
    {
        class SpotLightRequests
            : public ComponentBus
        {
        public:
            //! Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            virtual ~SpotLightRequests() = default;

            //! Gets a spot light's color. This value is independent from its intensity.
            virtual const Color& GetColor() const = 0;

            //! Sets a spot light's color. This value is independent from its intensity.
            virtual void SetColor(const Color& color) = 0;

            //! Gets a spot light's intensity. This value is independent from its color.
            virtual float GetIntensity() const = 0;

            //! Sets a spot light's intensity. This value is independent from its color.
            virtual void SetIntensity(float intensity) = 0;
            
            //! Gets a spot light's bulb radius in meters.
            virtual float GetBulbRadius() const = 0;

            //! Sets a spot light's bulb radius in meters.
            virtual void SetBulbRadius(float bulbRadius) = 0;

            //! @return Returns inner cone angle of the spot light in degrees.
            virtual float GetInnerConeAngleInDegrees() const = 0;
            //! @brief Sets inner cone angle of the spot light in degrees.
            virtual void SetInnerConeAngleInDegrees(float degrees) = 0;

            //! @return Returns outer cone angle of the spot light in degrees.
            virtual float GetOuterConeAngleInDegrees() const = 0;
            //! @brief Sets outer cone angle of the spot light in degrees.
            virtual void SetOuterConeAngleInDegrees(float degrees) = 0;

            //! @return Returns penumbra bias for the falloff curve of the spot light.
            virtual float GetPenumbraBias() const = 0;
            //! @brief Sets penumbra bias for the falloff curve of the spot light.
            virtual void SetPenumbraBias(float penumbraBias) = 0;

            //! @return Returns radius attenuation of the spot light.
            virtual float GetAttenuationRadius() const = 0;
            //! @return Sets radius attenuation of the spot light.
            virtual void SetAttenuationRadius(float radius) = 0;

            //! @return Returns radius attenuation mode (Auto or Explicit).
            virtual LightAttenuationRadiusMode GetAttenuationRadiusMode() const = 0;
            //! If this is set to Automatic, the radius will immediately be recalculated based on the intensity.
            //! If this is set to Explicit, the radius value will be unchanged from its previous value.
            virtual void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) = 0;

            //! @return the flag whether attenuation radius calculation is automatic or not.
            virtual bool GetAttenuationRadiusIsAutomatic() const
            {
                return (GetAttenuationRadiusMode() == LightAttenuationRadiusMode::Automatic);
            }
            //! This sets flag whether attenuation radius calculation is automatic or not.
            //! @param flag flag whether attenuation radius calculation is automatic or not.
            virtual void SetAttenuationRadiusIsAutomatic(bool flag)
            {
                SetAttenuationRadiusMode(flag ? LightAttenuationRadiusMode::Automatic : LightAttenuationRadiusMode::Explicit);
            }

            //! @return the flag indicates this light have shadow or not.
            virtual bool GetEnableShadow() const = 0;
            //! This specify this spot light uses shadow or not.
            //! @param enabled true if shadow is used, false otherwise.
            virtual void SetEnableShadow(bool enabled) = 0;

            //! @return the size of shadowmap (width and height).
            virtual ShadowmapSize GetShadowmapSize() const = 0;

            //! This specifies the size of shadowmap to size x size.
            virtual void SetShadowmapSize(ShadowmapSize size) = 0;

            //! This gets the filter method of shadows.
            //! @return filter method
            virtual ShadowFilterMethod GetShadowFilterMethod() const = 0;

            //! This specifies filter method of shadows.
            //! @param method Filter method.
            virtual void SetShadowFilterMethod(ShadowFilterMethod method) = 0;

            //! This gets the width of boundary between shadowed area and lit area.
            //! The width is given by the angle, and the units are in degrees.
            //! @return Boundary width.  The degree of the shadowed region is gradually changed on the boundary.
            virtual float GetSofteningBoundaryWidthAngle() const = 0;

            //! This specifies the width of boundary between shadowed area and lit area.
            //! @param width Boundary width.  The degree of shadowed is gradually changed on the boundary.
            //! If width == 0, softening edge is disabled.  Units are in degrees.
            virtual void SetSofteningBoundaryWidthAngle(float degrees) = 0;

            //! This gets the sample count to predict boundary of shadow.
            //! @return Sample Count for prediction of whether the pixel is on the boundary (up to 16)
            virtual uint32_t GetPredictionSampleCount() const = 0;

            //! This sets the sample count to predict boundary of shadow.
            //! @param count Sample count for prediction of whether the pixel is on the boundary (up to 16)
            //! This value should be less than or equal to m_filteringSampleCount.
            virtual void SetPredictionSampleCount(uint32_t count) = 0;

            //! This gets the sample count for filtering of the shadow boundary.
            //! @return Sample Count for filtering (up to 64)
            virtual uint32_t GetFilteringSampleCount() const = 0;

            //! This sets the sample count for filtering of the shadow boundary.
            //! @param count Sample Count for filtering (up to 64)
            virtual void SetFilteringSampleCount(uint32_t count) = 0;

            //! This gets the type of Pcf (percentage-closer filtering) to use.
            virtual PcfMethod GetPcfMethod() const = 0;

            //! This sets the type of Pcf (percentage-closer filtering) to use.
            //! @param method The Pcf method to use.
            virtual void SetPcfMethod(PcfMethod method) = 0;
        };

        /// The EBus for requests to for setting and getting spot light component properties.
        typedef AZ::EBus<SpotLightRequests> SpotLightRequestBus;

        class SpotLightNotifications
            : public ComponentBus
        {
        public:
            virtual ~SpotLightNotifications() = default;

            //! @brief Signals that the intensity of the light changed.
            virtual void OnIntensityChanged(float intensity) { AZ_UNUSED(intensity); }

            //! @brief Signals that the color of the light changed.
            virtual void OnColorChanged(const Color& color) { AZ_UNUSED(color); }

            //! @brief Signals that the cone angles of the spot light have changed.
            virtual void OnConeAnglesChanged(float innerConeAngleDegrees, float outerConeAngleDegrees) { AZ_UNUSED(innerConeAngleDegrees); AZ_UNUSED(outerConeAngleDegrees); }

            //! @brief Signals that the attenuation radius has changed.
            virtual void OnAttenuationRadiusChanged(float attenuationRadius) { AZ_UNUSED(attenuationRadius); }

            //! @brief Signals that the penumbra bias has changed.
            virtual void OnPenumbraBiasChanged(float penumbraBias) { AZ_UNUSED(penumbraBias); }
        };

        //! The EBus for spot light notification events.
        typedef AZ::EBus<SpotLightNotifications> SpotLightNotificationBus;

    } // namespace Render
} // namespace AZ
