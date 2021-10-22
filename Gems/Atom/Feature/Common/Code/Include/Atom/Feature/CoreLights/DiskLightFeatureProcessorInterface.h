/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        struct DiskLightData
        {
            enum Flags
            {
                UseConeAngle = 0b1,
            };

            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.

            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float m_diskRadius = 0.0f; // Radius of disk light in meters.

            AZStd::array<float, 3> m_direction = { { 1.0f, 0.0f, 0.0f } };
            uint32_t m_flags = 0; // See Flags enum above.

            float m_cosInnerConeAngle = 0.0f; // cosine of inner cone angle
            float m_cosOuterConeAngle = 0.0f; // cosine of outer cone angle
            float m_bulbPositionOffset = 0.0f; // Distance from the light disk surface to the tip of the cone of the light. m_bulbRadius * tanf(pi/2 - m_outerConeAngle).
            uint16_t m_shadowIndex = std::numeric_limits<uint16_t>::max(); // index for ProjectedShadowData. A value of 0xFFFF indicates an illegal index.
            uint16_t m_padding; // Explicit padding.
        };

        //! DiskLightFeatureProcessorInterface provides an interface to acquire, release, and update a disk light. This is necessary for code outside of
        //! the Atom features gem to communicate with the DiskLightFeatureProcessor.
        class DiskLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DiskLightFeatureProcessorInterface, "{A78A8FBD-1494-4EF9-9C05-AF153FDB1F17}"); 

            using LightHandle = RHI::Handle<uint16_t, class DiskLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Candela;

            //! Creates a new disk light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the disk light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            // Generic Disk Light Settings

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the position for a given LightHandle.
            virtual void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) = 0;
            //! Sets the direction for a given LightHandle.
            virtual void SetDirection(LightHandle handle, const AZ::Vector3& lightDirection) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the disk radius for the provided LightHandle.
            virtual void SetDiskRadius(LightHandle handle, float radius) = 0;

            // Cone Angle Settings 

            //! Sets whether the disk should constrain its light to a cone. (use SetInnerConeAngle and SetOuterConeAngle to set cone angle parameters)
            virtual void SetConstrainToConeLight(LightHandle handle, bool useCone) = 0;
            //! Sets the inner and outer cone angles in radians.
            virtual void SetConeAngles(LightHandle handle, float innerRadians, float outerRadians) = 0;

            // Shadow Settings

            //! Sets if shadows are enabled
            virtual void SetShadowsEnabled(LightHandle handle, bool enabled) = 0;
            //! Sets the shadow bias
            virtual void SetShadowBias(LightHandle handle, float bias) = 0;
            //! Sets the shadowmap size (width and height) of the light.
            virtual void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) = 0;
            //! Specifies filter method of shadows.
            virtual void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) = 0;
            //! Sets sample count for filtering of shadow boundary (up to 64)
            virtual void SetFilteringSampleCount(LightHandle handle, uint16_t count) = 0;
            //! Sets the Esm exponent to use. Higher values produce a steeper falloff in the border areas between light and shadow.
            virtual void SetEsmExponent(LightHandle handle, float exponent) = 0;

            //! Sets all of the the disk data for the provided LightHandle.
            virtual void SetDiskData(LightHandle handle, const DiskLightData& data) = 0;


        };
    } // namespace Render
} // namespace AZ
