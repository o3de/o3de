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

namespace AZ
{
    class Vector3;

    namespace Render
    {
        //! SimplePointLightFeatureProcessorInterface provides an interface to acquire, release, and update a point light.
        class SimplePointLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SimplePointLightFeatureProcessorInterface, "{B6FABD69-ED5B-4D6C-8695-27CB95D13CE4}", AZ::RPI::FeatureProcessor);

            using LightHandle = RHI::Handle<uint16_t, class PointLight>;
            static constexpr PhotometricUnit PhotometricUnitType = PhotometricUnit::Candela;

            //! Creates a new point light which can be referenced by the returned LightHandle. Must be released via ReleaseLight() when no longer needed.
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
            //! Specifies if this light affects the diffuse global illumination in the scene.
            virtual void SetAffectsGI(LightHandle handle, bool affectsGI) = 0;
            //! Specifies the contribution of this light to the diffuse global illumination in the scene.
            virtual void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) = 0;
        };
    } // namespace Render
} // namespace AZ
