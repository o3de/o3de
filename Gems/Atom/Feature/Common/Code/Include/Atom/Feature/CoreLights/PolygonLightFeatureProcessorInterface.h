/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        struct PolygonLightData
        {
            AZStd::array<float, 3> m_position;
            uint32_t m_startEndIndex = 0; // top 16 bits for start, bottom 16 bits for end.

            // Standard RGB Color, except the red sign bit is used to store if points {0, 1, 2} create concave or
            // convex edges. This is used in the shader to determine directionality.
            AZStd::array<float, 3> m_rgbIntensityNits = { 0.0f, 0.0f, 0.0f };

            // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.
            // Negative sign bit used to indicate if the light emits both directions.
            float m_invAttenuationRadiusSquared = 0.0f;

            AZStd::array<float, 3> m_direction = { 0.0f, 0.0f, 1.0f };

            uint32_t m_lightingChannelMask = 1;


            // Convenience functions for setting start / end index.

            uint32_t GetStartIndex()
            {
                return m_startEndIndex >> 16;
            }

            void SetStartIndex(uint32_t startIndex)
            {
                m_startEndIndex = (m_startEndIndex & 0x0000FFFF) | (startIndex << 16);
            }

            uint32_t GetEndIndex()
            {
                return m_startEndIndex & 0x0000FFFF;
            }

            void SetEndIndex(uint32_t endIndex)
            {
                m_startEndIndex = (m_startEndIndex & 0xFFFF0000) | (endIndex & 0x0000FFFF);
            }

        };

        //! PolygonLightFeatureProcessorInterface provides an interface to acquire, release, and update a Polygon light. This is necessary for code outside of
        //! the Atom features gem to communicate with the PolygonLightFeatureProcessor.
        class PolygonLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::PolygonLightFeatureProcessorInterface, "{FB21684B-5752-4943-9D44-C81EB0C0991B}", AZ::RPI::FeatureProcessor);

            using LightHandle = RHI::Handle<uint16_t, class PolygonLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Nit;

            //! Creates a new Polygon light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the Polygon light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            //! Sets the position for a given LightHandle.
            virtual void SetPosition(LightHandle handle, const AZ::Vector3& position) = 0;
            //! Sets the intensity in RGB nits for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the polygon's world space positions for a given LightHandle.
            virtual void SetPolygonPoints(LightHandle handle, const Vector3* vectices, const uint32_t vertexCount, const Vector3& direction) = 0;
            //! Sets if the light is emitted from both directions for a given LightHandle.
            virtual void SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the lighting channel mask
            virtual void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) = 0;

            //! Returns the buffer containing the light data for all polygon lights
            virtual const Data::Instance<RPI::Buffer> GetLightBuffer() const = 0;
            //! Returns the buffer containing the points of all polygon lights
            virtual const Data::Instance<RPI::Buffer> GetLightPointBuffer() const = 0;
            //! Returns the number of directional lights
            virtual uint32_t GetLightCount() const = 0;
        };
    } // namespace Render
} // namespace AZ
