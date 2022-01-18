/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        struct PointLightData
        {
            AZStd::array<float, 3> m_position = {{0.0f, 0.0f, 0.0f}};

            // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.
            float m_invAttenuationRadiusSquared = 0.0f;

            AZStd::array<float, 3> m_rgbIntensity = {{0.0f, 0.0f, 0.0f}};

            // Radius of spherical light in meters.
            float m_bulbRadius = 0.0f; 

            static const int NumShadowFaces = 6;
            AZStd::array<uint16_t, NumShadowFaces> m_shadowIndices = {{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}};
            uint32_t m_padding;
        };

        //! PointLightFeatureProcessorInterface provides an interface to acquire, release, and update a point light.
        class PointLightFeatureProcessorInterface : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::PointLightFeatureProcessorInterface, "{D3E0B016-F3C6-4C7A-A29E-0B3A4FA87806}", AZ::RPI::FeatureProcessor);
            AZ_FEATURE_PROCESSOR(PointLightFeatureProcessorInterface);

            using LightHandle = RHI::Handle<uint16_t, class PointLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Candela;

            //! Creates a new point light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no
            //! longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the point light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the position for a given LightHandle.
            virtual void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the bulb radius for the provided LightHandle. Values greater than zero effectively make it a spherical light.
            virtual void SetBulbRadius(LightHandle handle, float bulbRadius) = 0;
            //! Sets if shadows are enabled
            virtual void SetShadowsEnabled(LightHandle handle, bool enabled) = 0;
            //! Sets the shadowmap size (width and height) of the light.
            virtual void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) = 0;
            //! Sets the shadow bias
            virtual void SetShadowBias(LightHandle handle, float bias) = 0;
            //! Specifies filter method of shadows.
            virtual void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) = 0;
            //! Sets sample count for filtering of shadow boundary (up to 64)
            virtual void SetFilteringSampleCount(LightHandle handle, uint16_t count) = 0;
            //! Sets the Esm exponent to use. Higher values produce a steeper falloff in the border areas between light and shadow.
            virtual void SetEsmExponent(LightHandle handle, float exponent) = 0;
            //! Sets the normal shadow bias. Reduces acne by biasing the shadowmap lookup along the geometric normal.
            virtual void SetNormalShadowBias(LightHandle handle, float bias) = 0;
            //! Sets all of the the point data for the provided LightHandle.
            virtual void SetPointData(LightHandle handle, const PointLightData& data) = 0;
        };
    } // namespace Render
} // namespace AZ
