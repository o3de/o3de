/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/PolygonLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/MultiIndexedDataVector.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {
        class PolygonLightFeatureProcessor final
            : public PolygonLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::PolygonLightFeatureProcessor, "{59E4245F-AD7B-4181-B80A-1B973A50B4C8}", AZ::Render::PolygonLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            PolygonLightFeatureProcessor();
            virtual ~PolygonLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void Render(const RenderPacket& packet) override;

            // PolygonLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetPosition(LightHandle handle, const AZ::Vector3& position) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) override;
            void SetPolygonPoints(LightHandle handle, const Vector3* vectices, const uint32_t vertexCount, const Vector3& direction) override;
            void SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer()const;
            uint32_t GetLightCount()const;

        private:
            PolygonLightFeatureProcessor(const PolygonLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "PolygonLightFeatureProcessor";
            static constexpr uint32_t MaxPolygonPoints = 64;

            struct LightPosition // float4
            {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                float w = 0.0f; // unused

                LightPosition() = default;
                LightPosition(AZ::Vector3 vector)
                {
                    vector.StoreToFloat4(reinterpret_cast<float*>(this));
                }
            };

            // Calculates cross product between vectors p1->p0 and p1->p2
            static Vector3 CrossEdges(const LightPosition& p0, const LightPosition& p1, const LightPosition& p2);

            using PolygonPoints = AZStd::array<LightPosition, MaxPolygonPoints>;
            using PolygonLightDataVector = MultiIndexedDataVector<PolygonLightData, PolygonPoints>;

            // Recalculates the start / end indices of the points for this polygon if it recently moved in memory.
            void EvaluateStartEndIndices(PolygonLightDataVector::IndexType index);

            PolygonLightDataVector m_polygonLightData;
            
            GpuBufferHandler m_lightBufferHandler;
            GpuBufferHandler m_lightPolygonPointBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
