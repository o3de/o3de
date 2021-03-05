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

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        struct SpotLightData
        {
            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.

            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float m_innerConeAngle; // cosine of the angle from the direction axis at which this light starts to fall off.

            AZStd::array<float, 3> m_direction = { { 1.0f, 0.0f, 0.0f } };
            float m_outerConeAngle; // cosine of the angle from the direction axis at which this light no longer has an effect.

            float m_penumbraBias = 0.0f; // controls biasing the falloff curve between inner and outer cone angles.

            int32_t m_shadowIndex = -1; // index for SpotLightShadowData.
                                        // a minus value indicates an illegal index.

            float m_bulbRadius = 0.0f; // Size of the disk in meters representing the spot light bulb.

            float m_bulbPostionOffset = 0.0f; // Distance from the light disk surface to the tip of the cone of the light. m_bulbRadius * tanf(pi/2 - m_outerConeAngle).
        };

        //! SpotLightFeatureProcessorInterface provides an interface to acquire, release, and update a spot light. This is necessary for code outside of
        //! the Atom features gem to communicate with the SpotLightFeatureProcessor.
        class SpotLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SpotLightFeatureProcessorInterface, "{9424429B-C5E9-4CF2-9512-7911778E2836}", AZ::RPI::FeatureProcessor);

            using LightHandle = RHI::Handle<uint16_t, class SpotLight>;

            //! Creates a new spot light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the spot light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnit::Candela>& lightColor) = 0;
            //! Sets the position of the spot light.
            virtual void SetPosition(LightHandle handle, const Vector3& lightPosition) = 0;
            //! Sets the direction of the spot light. direction should be normalized.
            virtual void SetDirection(LightHandle handle, const Vector3& direction) = 0;
            //! Sets the bulb radius of the spot light in meters.
            virtual void SetBulbRadius(LightHandle handle, float bulbRadius) = 0;
            //! Sets the inner and outer cone angles in degrees.
            virtual void SetConeAngles(LightHandle handle, float innerDegrees, float outerDegrees) = 0;
            //! Sets a -1 to +1 value that adjusts the bias of the interpolation of light intensity from the inner cone to the outer.
            virtual void SetPenumbraBias(LightHandle handle, float penumbraBias) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the shadowmap size (width and height) of the light.
            virtual void SetShadowmapSize(LightHandle handle, ShadowmapSize shadowmapSize) = 0;

            //! This specifies filter method of shadows.
            //! @param handle the light handle.
            //! @param method filter method.
            virtual void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) = 0;

            //! This specifies the width of boundary between shadowed area and lit area.
            //! @param handle the light handle.
            //! @param width Boundary width. The degree of shadowed gradually changes on the boundary.
            //! If width == 0, softening edge is disabled. Units are in degrees.
            virtual void SetShadowBoundaryWidthAngle(LightHandle handle, float boundaryWidthDegree) = 0;

            //! This sets sample count to predict boundary of shadow.
            //! @param handle the light handle.
            //! @param count Sample Count for prediction of whether the pixel is on the boundary (up to 16)
            //! The value should be less than or equal to m_filteringSampleCount.
            virtual void SetPredictionSampleCount(LightHandle handle, uint16_t count) = 0;

            //! This sets sample count for filtering of shadow boundary.
            //! @param handle the light handle.
            //! @param count Sample Count for filtering (up to 64)
            virtual void SetFilteringSampleCount(LightHandle handle, uint16_t count) = 0;

            //! Sets all of the the spot light data for the provided LightHandle.
            virtual void SetSpotLightData(LightHandle handle, const SpotLightData& data) = 0;
        };
    } // namespace Render
} // namespace AZ
