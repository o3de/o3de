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

#include <Atom/Feature/CoreLights/DiskLightFeatureProcessorInterface.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <CoreLights/IndexedDataVector.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {
        class DiskLightFeatureProcessor final
            : public DiskLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::DiskLightFeatureProcessor, "{F69C0188-2C1C-47A5-8187-17433C34AC2B}", AZ::Render::DiskLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            DiskLightFeatureProcessor();
            virtual ~DiskLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket & packet) override;
            void Render(const RenderPacket & packet) override;

            // DiskLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) override;
            void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) override;
            void SetDirection(LightHandle handle, const AZ::Vector3& lightDirection) override;
            void SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetDiskRadius(LightHandle handle, float radius) override;
            void SetDiskData(LightHandle handle, const DiskLightData& data) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer()const;
            uint32_t GetLightCount()const;

        private:
            DiskLightFeatureProcessor(const DiskLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "DiskLightFeatureProcessor";

            IndexedDataVector<DiskLightData> m_diskLightData;
            GpuBufferHandler m_lightBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
