/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/QuadLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {
        class QuadLightFeatureProcessor final
            : public QuadLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::QuadLightFeatureProcessor, "{F1E50245-5F05-475E-857F-221FB17C7E45}", AZ::Render::QuadLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            QuadLightFeatureProcessor();
            virtual ~QuadLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void Render(const RenderPacket& packet) override;

            // QuadLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& color) override;
            void SetPosition(LightHandle handle, const AZ::Vector3& position) override;
            void SetOrientation(LightHandle handle, const AZ::Quaternion& orientation) override;
            void SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections) override;
            void SetUseFastApproximation(LightHandle handle, bool useFastApproximation) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetQuadDimensions(LightHandle handle, float width, float height) override;
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;
            void SetQuadData(LightHandle handle, const QuadLightData& data) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer()const;
            uint32_t GetLightCount()const;

        private:
            QuadLightFeatureProcessor(const QuadLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "QuadLightFeatureProcessor";

            IndexedDataVector<QuadLightData> m_quadLightData;
            GpuBufferHandler m_lightBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
