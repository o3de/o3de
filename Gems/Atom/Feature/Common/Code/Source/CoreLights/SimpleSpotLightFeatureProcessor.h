/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Hemisphere.h>
#include <Atom/Feature/CoreLights/LightCommon.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/SimpleSpotLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {

        struct SimpleSpotLightData
        {
            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.
            AZStd::array<float, 3> m_direction = { { 0.0f, 0.0f, 0.0f } };
            float m_cosInnerConeAngle = 0.0f; // Cosine of the inner cone angle
            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float m_cosOuterConeAngle = 0.0f; // Cosine of the outer cone angle

            float m_affectsGIFactor = 1.0f;
            bool m_affectsGI = true;
            float m_padding0 = 0.0f;
            float m_padding1 = 0.0f;
        };

        class SimpleSpotLightFeatureProcessor final
            : public SimpleSpotLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::SimpleSpotLightFeatureProcessor, "{01610AD4-0872-4F80-9F12-22FB7CCF6866}", AZ::Render::SimpleSpotLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            SimpleSpotLightFeatureProcessor();
            virtual ~SimpleSpotLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void Render(const RenderPacket& packet) override;

            // SimpleSpotLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) override;
            void SetPosition(LightHandle handle, const AZ::Vector3& lightPosition) override;
            void SetDirection(LightHandle handle, const AZ::Vector3& lightDirection) override;
            virtual void SetConeAngles(LightHandle handle, float innerRadians, float outerRadians) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;

            const Data::Instance<RPI::Buffer>  GetLightBuffer() const;
            uint32_t GetLightCount()const;

        private:
            SimpleSpotLightFeatureProcessor(const SimpleSpotLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "SimpleSpotLightFeatureProcessor";

            IndexedDataVector<SimpleSpotLightData> m_lightData;
            GpuBufferHandler m_lightBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
