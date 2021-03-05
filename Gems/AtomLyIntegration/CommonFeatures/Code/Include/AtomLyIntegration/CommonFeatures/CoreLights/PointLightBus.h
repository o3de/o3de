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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

#include <AtomLyIntegration/CommonFeatures/CoreLights/PointLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class PointLightRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(PointLightRequests, "{359BE514-DBEB-4D6A-B283-F8C5E83CD477}");

            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            virtual ~PointLightRequests() {}

            /// Gets a point light's color. This value is indepedent from its intensity.
            virtual const Color& GetColor() const = 0;

            /// Sets a point light's color. This value is indepedent from its intensity.
            virtual void SetColor(const Color& color) = 0;

            /// Gets a point light's intensity. This value is indepedent from its color.
            virtual float GetIntensity() const = 0;

            //! Gets a point light's photometric type.
            virtual PhotometricUnit GetIntensityMode() const = 0;

            /// Sets a point light's intensity. This value is indepedent from its color.
            virtual void SetIntensity(float intensity) = 0;

            //! Sets a point light's intensity and intensity mode. This value is indepedent from its color.
            virtual void SetIntensity(float intensity, PhotometricUnit intensityMode) = 0;

            /// Gets the distance at which the point light will no longer affect lighting.
            virtual float GetAttenuationRadius() const = 0;

            /// Set the distance and which a point light will no longer affect lighitng. Setting this forces the RadiusCalculation to Explicit mode.
            virtual void SetAttenuationRadius(float radius) = 0;

            /// Gets the size in meters of the sphere representing the light bulb.
            virtual float GetBulbRadius() const = 0;

            /// Sets the size in meters of the sphere representing the light bulb in meters.
            virtual void SetBulbRadius(float bulbSize) = 0;

            /*
             * If this is set to Automatic, the radius will immediately be recalculated based on the intensity.
             * If this is set to Explicit, the radius value will be unchanged from its previous value.
            */
            virtual void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) = 0;

            /// Gets the flag whether attenuation radius calculation is automatic or not.
            virtual bool GetAttenuationRadiusIsAutomatic() const = 0;

            /// Sets the flag whether attenuation radius calculation is automatic or not.
            virtual void SetAttenuationRadiusIsAutomatic(bool flag)
            {
                SetAttenuationRadiusMode(flag ? LightAttenuationRadiusMode::Automatic : LightAttenuationRadiusMode::Explicit);
            }

            //! Sets the photometric unit to the one provided and converts the intensity to the photometric unit so actual light intensity remains constant.
            virtual void ConvertToIntensityMode(PhotometricUnit intensityMode) = 0;
        };

        /// The EBus for requests to for setting and getting light component properties.
        typedef AZ::EBus<PointLightRequests> PointLightRequestBus;

        class PointLightNotifications
            : public ComponentBus
        {
        public:
            AZ_RTTI(PointLightNotifications, "{7363728D-E3EE-4AC8-AAA7-C299782763F0}");

            virtual ~PointLightNotifications() {}

            /**
             * Signals that the color of the light changed.
             * @param color A reference to the new color of the light.
             */
            virtual void OnColorChanged(const Color& /*color*/) { }

            /**
             * Signals that the intensity of the light changed.
             * @param color A reference to the new intensity of the light.
             */
            virtual void OnIntensityChanged(float /*intensity*/) { }

            /**
             * Signals that the color or intensity of the light changed. This is useful when both the color and intensity are need in the same call.
             * @param color A reference to the new color of the light.
             * @param color A reference to the new intensity of the light.
             */
            virtual void OnColorOrIntensityChanged(const Color& /*color*/, float /*intensity*/) { }

            /**
             * Signals that the attenuation radius of the light changed.
             * @param attenuationRadius The distance at which this light no longer affects lighting.
             */
            virtual void OnAttenutationRadiusChanged(float /*attenuationRadius*/) { }

            /**
             * Signals that the bulb size of the light changed.
             * @param bulbRadius The size in meters of the sphere representing the light bulb in meters.
             */
            virtual void OnBulbRadiusChanged(float /*bulbRadius*/) { }

        };

        /// The EBus for light notification events.
        typedef AZ::EBus<PointLightNotifications> PointLightNotificationBus;

    } // namespace Render
} // namespace AZ
