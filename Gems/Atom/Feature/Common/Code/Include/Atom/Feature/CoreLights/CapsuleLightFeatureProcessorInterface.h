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

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        struct CapsuleLightData
        {
            AZStd::array<float, 3> m_startPoint = { { 0.0f, 0.0f, 0.0f } }; //! one of the end points of the capsule
            float m_radius = 0.0f; //! Radius of the capsule, ie distance from line segment to surface.

            AZStd::array<float, 3> m_direction = { { 0.0f, 0.0f, 0.0f } }; //! normalized vector from the start point towards the other end point.
            float m_length = 0.0f; //! length of the line segment making up the inside of the capsule. Doesn't include caps (0 length capsule == sphere)

            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } }; //! total rgb luminous intensity of the capsule in candela
            float m_invAttenuationRadiusSquared = 0.0f; //! Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.

            float m_affectsGIFactor = 1.0f;
            bool m_affectsGI = true;
            uint32_t m_lightingChannelMask = 1;
            float m_padding0 = 0.0f;
        };

        //! CapsuleLightFeatureProcessorInterface provides an interface to acquire, release, and update a capsule light. This is necessary for code outside of
        //! the Atom features gem to communicate with the CapsuleLightFeatureProcessor.
        class CapsuleLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::CapsuleLightFeatureProcessorInterface, "{41CAF69D-6A0B-461F-BE3D-6367673646D4}", AZ::RPI::FeatureProcessor); 

            using LightHandle = RHI::Handle<uint16_t, class CapsuleLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Candela;

            //! Creates a new capsule light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the capsule light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the start point and end point of the interior line segment of the capsule. If these points are equivalent then the capsule is a sphere.
            virtual void SetCapsuleLineSegment(LightHandle handle, const Vector3& startPoint, const Vector3& endPoint) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the capsule radius for the provided LightHandle.
            virtual void SetCapsuleRadius(LightHandle handle, float radius) = 0;
            //! Specifies if this light affects the diffuse global illumination in the scene.
            virtual void SetAffectsGI(LightHandle handle, bool affectsGI) = 0;
            //! Specifies the contribution of this light to the diffuse global illumination in the scene.
            virtual void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) = 0;
            //! Sets the lighting channel mask
            virtual void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) = 0;

            //! Sets all of the the capsule data for the provided LightHandle.
            virtual void SetCapsuleData(LightHandle handle, const CapsuleLightData& data) = 0;

            //! Returns the buffer containing the light data for all capsule lights
            virtual const Data::Instance<RPI::Buffer> GetLightBuffer() const = 0;
            //! Returns the number of capsule lights
            virtual uint32_t GetLightCount() const = 0;
        };
    } // namespace Render
} // namespace AZ
