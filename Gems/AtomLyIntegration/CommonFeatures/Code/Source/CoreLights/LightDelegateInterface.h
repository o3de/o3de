/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AZ
{
    class Color;
    class Transform;

    namespace Render
    {
        //! Delegate for managing light shape specific functionality in the AreaLightComponentController.
        class LightDelegateInterface
        {
        public:
            virtual ~LightDelegateInterface() {};

            //! Sets the area light component config so delegates don't have to cache the same data locally.
            virtual void SetConfig(const AreaLightComponentConfig* config) = 0;
            //! Sets the color of the light independent of light intensity. The color is a mask on the total light intensity.
            virtual void SetChroma(const AZ::Color& chroma) = 0;
            //! Sets the light intensity
            virtual void SetIntensity(float intensity) = 0;
            //! Sets the light unit, and returns the converted light intensity.
            virtual float SetPhotometricUnit(PhotometricUnit unit) = 0;
            //! Sets the maximum distance from any part of the surface of the area light at which this light will have an effect.
            virtual void SetAttenuationRadius(float radius) = 0;
            //! Gets the light intensity
            virtual const PhotometricValue& GetPhotometricValue() const = 0;
            //! Gets the surface area of the shape
            virtual float GetSurfaceArea() const = 0;
            //! Returns the number of steradians covered by this light type.
            virtual float GetEffectiveSolidAngle() const = 0;
            //! Sets if this shape is double-sided (only applicable for 2d shapes)
            virtual void SetLightEmitsBothDirections([[maybe_unused]] bool lightEmitsBothDirections) = 0;
            //! Sets if this light uses linearly transformed cosines (false) or a faster approximation (true). Only applicable for shapes that support LTC.
            virtual void SetUseFastApproximation([[maybe_unused]] bool useFastApproximation) = 0;
            //! Calculates the atteunation radius for this light type based on a threshold value.
            virtual float CalculateAttenuationRadius(float lightThreshold) const = 0;
            //! Handle any additional debug display drawing for beyond what the shape already provides.
            virtual void DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const = 0;
            //! Turns the visibility of this light on/off.
            virtual void SetVisibility(bool visibility) = 0;

            // Shutters

            // Sets if the light should be restricted to shutter angles.
            virtual void SetEnableShutters(bool enabled) = 0;
            // Sets the inner and outer angles of the shutters in degrees for where the light
            // beam starts to attenuate (inner) to where it is completely occluded (outer).
            virtual void SetShutterAngles(float innerAngleDegrees, float outerAngleDegrees) = 0;

            // Shadows

            //! Sets if shadows should be enabled
            virtual void SetEnableShadow(bool enabled) = 0;
            //! Sets the shadow bias
            virtual void SetShadowBias(float bias) = 0;
            //! Sets the maximum resolution of the shadow map
            virtual void SetShadowmapMaxSize(ShadowmapSize size) = 0;
            //! Sets the filter method for the shadow
            virtual void SetShadowFilterMethod(ShadowFilterMethod method) = 0;
            //! Sets the sample count for filtering of the shadow boundary, max 64.
            virtual void SetFilteringSampleCount(uint32_t count) = 0;
            //! Sets the Esm exponent to use. Higher values produce a steeper falloff between light and shadow.
            virtual void SetEsmExponent(float exponent) = 0;
        };
    } //  namespace Render
} // namespace AZ
