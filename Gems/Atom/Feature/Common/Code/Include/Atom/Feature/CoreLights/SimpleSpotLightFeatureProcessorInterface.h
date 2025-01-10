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
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace AZ
{
    class Vector3;

    namespace Render
    {
        //! SimpleSpotLightFeatureProcessorInterface provides an interface to acquire, release, and update a simple spot light.
        class SimpleSpotLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SimpleSpotLightFeatureProcessorInterface, "{1DE04BF2-DD8F-437C-9B6D-4BDAC4BE2BAC}", AZ::RPI::FeatureProcessor);
            
            using LightHandle = RHI::Handle<uint16_t, class SimpleSpotLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Candela;

            //! Creates a new spot light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
            virtual LightHandle AcquireLight() = 0;
            //! Releases a LightHandle which removes the spot light.
            virtual bool ReleaseLight(LightHandle& handle) = 0;
            //! Creates a new LightHandle by copying data from an existing LightHandle.
            virtual LightHandle CloneLight(LightHandle handle) = 0;

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the light's transformation for a given LightHandle.
            virtual void SetTransform(LightHandle handle, const AZ::Transform& transform) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the inner and outer cone angles in radians.
            virtual void SetConeAngles(LightHandle handle, float innerRadians, float outerRadians) = 0;
            //! Specifies if this light affects the diffuse global illumination in the scene.
            virtual void SetAffectsGI(LightHandle handle, bool affectsGI) = 0;
            //! Specifies the contribution of this light to the diffuse global illumination in the scene.
            virtual void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) = 0;
            //! Sets the lighting channel mask
            virtual void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) = 0;
           //! Set a gobo texture to the light
            virtual void SetGoboTexture(LightHandle handle, AZ::Data::Instance<AZ::RPI::Image> goboTexture) = 0;

            // Shadow Settings

            //! Sets if shadows are enabled
            virtual void SetShadowsEnabled(LightHandle handle, bool enabled) = 0;
            //! Sets the shadow bias
            virtual void SetShadowBias(LightHandle handle, float bias) = 0;
            //! Sets the normal shadow bias
            virtual void SetNormalShadowBias(LightHandle handle, float bias) = 0;
            //! Sets the shadowmap size (width and height) of the light.
            virtual void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) = 0;
            //! Specifies filter method of shadows.
            virtual void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) = 0;
            //! Sets sample count for filtering of shadow boundary (up to 64)
            virtual void SetFilteringSampleCount(LightHandle handle, uint16_t count) = 0;
            //! Sets the Esm exponent to use. Higher values produce a steeper falloff in the border areas between light and shadow.
            virtual void SetEsmExponent(LightHandle handle, float exponent) = 0;
            //! Sets if this shadow should be rendered every frame (not cached) or only when it detects a change (cached).
            virtual void SetUseCachedShadows(LightHandle handle, bool useCachedShadows) = 0;

            //! Returns the buffer containing the light data for all simple spot lights
            virtual const Data::Instance<RPI::Buffer> GetLightBuffer() const = 0;
            //! Returns the number of simple spot lights
            virtual uint32_t GetLightCount() const = 0;
        };
    } // namespace Render
} // namespace AZ
