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
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class AreaLightRequests
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::AreaLightRequests, "{BC54532C-F3C8-4942-99FC-58D2E3D3DD54}");

            //! Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

            virtual ~AreaLightRequests() {}

            //! Gets an area light's color. This value is indepedent from its intensity.
            virtual const Color& GetColor() const = 0;

            //! Sets an area light's color. This value is indepedent from its intensity.
            virtual void SetColor(const Color& color) = 0;

            //! Gets an area light's intensity. This value is indepedent from its color.
            virtual float GetIntensity() const = 0;

            //! Gets whether an area light emits light in both directions from a 2D surface. Only applies to 2D shape types.
            virtual bool GetLightEmitsBothDirections() const = 0;

            //! Sets whether an area light emits light in both directions from a 2D surface. Only applies to 2D shape types.
            virtual void SetLightEmitsBothDirections(bool value) = 0;

            //! Gets whether the light is using the default high quality linearly transformed cosine lights (false) or a faster approximation (true).
            virtual bool GetUseFastApproximation() const = 0;

            //! Sets whether the light should use the default high quality linearly transformed cosine lights (false) or a faster approximation (true).
            virtual void SetUseFastApproximation(bool useFastApproximation) = 0;

            //! Gets an area light's photometric type.
            virtual PhotometricUnit GetIntensityMode() const = 0;

            //! Sets an area light's intensity and intensity mode. This value is indepedent from its color.
            virtual void SetIntensity(float intensity, PhotometricUnit intensityMode) = 0;

            //! Sets an area light's intensity. This value is indepedent from its color.
            //! Assumes no change in the current photometric unit of the intensity.
            virtual void SetIntensity(float intensity) = 0;

            //! Gets the distance at which the area light will no longer affect lighting.
            virtual float GetAttenuationRadius() const = 0;

            //! Set the distance and which an area light will no longer affect lighitng. Setting this forces the RadiusCalculation to Explicit mode.
            virtual void SetAttenuationRadius(float radius) = 0;

            /*
             * If this is set to Automatic, the radius will immediately be recalculated based on the intensity.
             * If this is set to Explicit, the radius value will be unchanged from its previous value.
            */
            virtual void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) = 0;

            //! Sets the photometric unit to the one provided and converts the intensity to the photometric unit so actual light intensity remains constant.
            virtual void ConvertToIntensityMode(PhotometricUnit intensityMode) = 0;
        };

        //! The EBus for requests to for setting and getting light component properties.
        typedef AZ::EBus<AreaLightRequests> AreaLightRequestBus;

        class AreaLightNotifications
            : public ComponentBus
        {
        public:
            AZ_RTTI(AZ::Render::AreaLightNotifications, "{7363728D-E3EE-4AC8-AAA7-C299782763F0}");

            virtual ~AreaLightNotifications() {}

            /**
             * Signals that the color of the light changed.
             * @param color A reference to the new color of the light.
             */
            virtual void OnColorChanged(const Color& /*color*/) { }

            /**
             * Signals that the intensity of the light changed.
             * @param intensity A reference to the new intensity of the light.
             * @param intenstiyMode A reference to the intensity mode of the light (lux or lumens).
             */
            virtual void OnIntensityChanged(float /*intensity*/, PhotometricUnit /*intenstiyMode*/) { }

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

        };

        //! The EBus for light notification events.
        typedef AZ::EBus<AreaLightNotifications> AreaLightNotificationBus;

    } // namespace Render
} // namespace AZ
