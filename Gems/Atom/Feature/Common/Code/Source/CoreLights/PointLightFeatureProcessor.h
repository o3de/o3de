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

#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/PointLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <CoreLights/IndexedDataVector.h>
#include <Shadows/ProjectedShadowFeatureProcessor.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {

        struct PointLightData
        {
            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.
            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float m_bulbRadius = 0.0f; // Radius of spherical light in meters.

            static const int NumShadowFaces = 6;

            AZStd::array<uint16_t, NumShadowFaces> m_shadowIndices = {{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}};
            uint32_t m_padding;
        };

        class PointLightFeatureProcessor final
            : public PointLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::PointLightFeatureProcessor, "{C16A39D6-0DDA-4511-9E35-42968702D3B4}", AZ::Render::PointLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            PointLightFeatureProcessor();
            virtual ~PointLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void Render(const RenderPacket& packet) override;

            // PointLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) override;
            void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetBulbRadius(LightHandle handle, float bulbRadius) override;
            void SetShadowsEnabled(LightHandle handle, bool enabled) override;
            void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) override;

            const Data::Instance<RPI::Buffer>  GetLightBuffer() const;
            uint32_t GetLightCount()const;

        private:
            PointLightFeatureProcessor(const PointLightFeatureProcessor&) = delete;
            using ShadowId = ProjectedShadowFeatureProcessor::ShadowId;

            static constexpr const char* FeatureProcessorName = "PointLightFeatureProcessor";
            void UpdateShadow(LightHandle handle);
            // Convenience function for forwarding requests to the ProjectedShadowFeatureProcessor
            template<typename Functor, typename ParamType>
            void SetShadowSetting(LightHandle handle, Functor&&, ParamType&& param, const int lightIndex);
            ProjectedShadowFeatureProcessor* m_shadowFeatureProcessor = nullptr;

            IndexedDataVector<PointLightData> m_pointLightData;
            GpuBufferHandler m_lightBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;

            AZStd::array<AZ::Vector3, PointLightData::NumShadowFaces> m_directions;
        };
    } // namespace Render
} // namespace AZ
