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
        };
    } //  namespace Render
} // namespace AZ
