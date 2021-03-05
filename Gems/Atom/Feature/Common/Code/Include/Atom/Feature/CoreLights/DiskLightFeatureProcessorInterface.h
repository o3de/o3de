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

namespace AZ
{
    class Color;
    class Vector3;

    namespace Render
    {
        struct DiskLightData
        {
            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.
            AZStd::array<float, 3> m_direction = { { 0.0f, 0.0f, 0.0f } };
            float m_bothDirectionsFactor = 0.0f; // 0.0f if single direction, -1.0f if both directions.
            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float m_diskRadius = 0.0f; // Radius of disk light in meters.

            void SetLightEmitsBothDirections(bool isBothDirections)
            {
                m_bothDirectionsFactor = isBothDirections ? -1.0f : 0.0f;
            }
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

            //! Sets the intensity in RGB candela for a given LightHandle.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) = 0;
            //! Sets the position for a given LightHandle.
            virtual void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) = 0;
            //! Sets the direction for a given LightHandle.
            virtual void SetDirection(LightHandle handle, const AZ::Vector3& lightDirection) = 0;
            //! Sets if the disk light emits light in both directions for a given LightHandle.
            virtual void SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections) = 0;
            //! Sets the radius in meters at which the provided LightHandle will no longer have an effect.
            virtual void SetAttenuationRadius(LightHandle handle, float attenuationRadius) = 0;
            //! Sets the disk radius for the provided LightHandle.
            virtual void SetDiskRadius(LightHandle handle, float radius) = 0;

            //! Sets all of the the disk data for the provided LightHandle.
            virtual void SetDiskData(LightHandle handle, const DiskLightData& data) = 0;
        };
    } // namespace Render
} // namespace AZ
