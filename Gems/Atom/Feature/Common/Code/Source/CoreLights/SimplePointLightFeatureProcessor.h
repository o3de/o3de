/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Sphere.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/SimplePointLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {
        class SimplePointLightFeatureProcessor final
            : public SimplePointLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::SimplePointLightFeatureProcessor, "{310CE42A-FAD1-4778-ABF5-0DE04AC92246}", AZ::Render::SimplePointLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            struct ShaderData
            {
                AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
                float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.

                AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
                float m_affectsGIFactor = 1.0f;

                bool m_affectsGI = true;
                float m_padding0 = 0.0f;
                float m_padding1 = 0.0f;
                float m_padding2 = 0.0f;
            };

            SimplePointLightFeatureProcessor();
            virtual ~SimplePointLightFeatureProcessor() = default;

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
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer() const;
            uint32_t GetLightCount()const;

        private:
            SimplePointLightFeatureProcessor(const SimplePointLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "SimplePointLightFeatureProcessor";

            using LightContainer = IndexedDataVector<ShaderData>;
            LightContainer m_lightData;
            GpuBufferHandler m_lightBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;

        };
    } // namespace Render
} // namespace AZ
