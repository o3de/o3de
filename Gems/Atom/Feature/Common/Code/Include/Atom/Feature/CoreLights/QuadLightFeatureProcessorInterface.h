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

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        namespace QuadLightFlag
        {
            // This must match the enum in QuadLight.azsli and LightCulling.azsl.
            enum Flag : uint32_t
            {
                None                 = 0b0000,
                EmitBothDirections   = 0b0001,
                UseFastApproximation = 0b0010,
            };
        }

        struct QuadLightData
        {

            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.

            AZStd::array<float, 3> m_leftDir = { { 1.0f, 0.0f, 0.0f } }; // direction from center to left edge of quad.
            float m_halfWidth = 0.0f;

            AZStd::array<float, 3> m_upDir = { { 0.0f, 1.0f, 0.0f } }; // direction from center to top edge of quad.
            float m_halfHeight = 0.0f;

            AZStd::array<float, 3> m_rgbIntensityNits = { { 0.0f, 0.0f, 0.0f } };
            uint32_t m_flags = 0x0;

            float m_affectsGIFactor = 1.0f;
            bool m_affectsGI = true;
            uint32_t m_lightingChannelMask = 1;
            float m_padding0 = 0.0f;

            void SetFlag(QuadLightFlag::Flag flag, bool value)
            {
                m_flags = value ?
                    m_flags | flag :
                    m_flags & ~flag;
            }
        };

        //! QuadLightFeatureProcessorInterface provides an interface to acquire, release, and update a quad light. This is necessary for code outside of
        //! the Atom features gem to communicate with the QuadLightFeatureProcessor.
        class QuadLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::QuadLightFeatureProcessorInterface, "{D86216E4-92A8-43BE-A5E4-883489C6AF06}", AZ::RPI::FeatureProcessor);

            using LightHandle = RHI::Handle<uint16_t, class QuadLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Nit;

            //! Creates a new quad light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the quad light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the position for a given LightHandle.
            virtual void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) = 0;
            //! Sets the direction for a given LightHandle.
            virtual void SetOrientation(LightHandle handle, const AZ::Quaternion& lightOrientation) = 0;
            //! Sets if the quad light emits light in both directions for a given LightHandle.
            virtual void SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections) = 0;
            //! Sets whether to use a fast approximation instead of the default high quality linearly transformed cosine lighting.
            virtual void SetUseFastApproximation(LightHandle handle, bool useFastApproximation) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the quad radius for the provided LightHandle.
            virtual void SetQuadDimensions(LightHandle handle, float width, float height) = 0;
            //! Specifies if this light affects the diffuse global illumination in the scene.
            virtual void SetAffectsGI(LightHandle handle, bool affectsGI) = 0;
            //! Specifies the contribution of this light to the diffuse global illumination in the scene.
            virtual void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) = 0;
            //! Sets the lighting channel mask
            virtual void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) = 0;

            //! Sets all of the the quad data for the provided LightHandle.
            virtual void SetQuadData(LightHandle handle, const QuadLightData& data) = 0;

            //! Returns the buffer containing the light data for all quad lights
            virtual const Data::Instance<RPI::Buffer> GetLightBuffer() const = 0;
            //! Returns the number of quad lights
            virtual uint32_t GetLightCount() const = 0;
        };
    } // namespace Render
} // namespace AZ
