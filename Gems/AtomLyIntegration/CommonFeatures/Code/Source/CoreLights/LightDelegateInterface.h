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
            //! Sets the maximum resolution of the shadow map
            virtual void SetShadowmapMaxSize(ShadowmapSize size) = 0;
            //! Sets the filter method for the shadow
            virtual void SetShadowFilterMethod(ShadowFilterMethod method) = 0;
            //! Sets the width of boundary between shadowed area and lit area in degrees.
            virtual void SetSofteningBoundaryWidthAngle(float widthInDegrees) = 0;
            //! Sets the sample count to predict the boundary of the shadow. Max 16, should be less than filtering sample count.
            virtual void SetPredictionSampleCount(uint32_t count) = 0;
            //! Sets the sample count for filtering of the shadow boundary, max 64.
            virtual void SetFilteringSampleCount(uint32_t count) = 0;
            //! Sets the Pcf (Percentage closer filtering) method to use.
            virtual void SetPcfMethod(PcfMethod method) = 0;
        };
    } //  namespace Render
} // namespace AZ
