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

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        //! PointLightFeatureProcessorInterface provides an interface to acquire, release, and update a point light.
        class PointLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::PointLightFeatureProcessorInterface, "{D3E0B016-F3C6-4C7A-A29E-0B3A4FA87806}", AZ::RPI::FeatureProcessor);
            AZ_FEATURE_PROCESSOR(PointLightFeatureProcessorInterface);

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
            //! Sets the bulb radius for the provided LightHandle. Values greater than zero effectively make it a spherical light.
            virtual void SetBulbRadius(LightHandle handle, float bulbRadius) = 0;
            //! Sets if shadows are enabled
            virtual void SetShadowsEnabled(LightHandle handle, bool enabled) = 0;
        };
    } // namespace Render
} // namespace AZ
