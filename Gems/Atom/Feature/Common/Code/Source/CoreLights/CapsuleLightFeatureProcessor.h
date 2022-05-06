/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/CapsuleLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {
        class CapsuleLightFeatureProcessor final
            : public CapsuleLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::CapsuleLightFeatureProcessor, "{0FC290C5-DD28-4194-8C0B-B90C3291BAF6}", AZ::Render::CapsuleLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            CapsuleLightFeatureProcessor();
            virtual ~CapsuleLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket & packet) override;
            void Render(const RenderPacket & packet) override;

            // CapsuleLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightColor) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetCapsuleLineSegment(LightHandle handle, const Vector3& startPoint, const Vector3& endPoint) override;
            void SetCapsuleRadius(LightHandle handle, float radius) override;
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;
            void SetCapsuleData(LightHandle handle, const CapsuleLightData& data) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer()const;
            uint32_t GetLightCount()const;

        private:
            CapsuleLightFeatureProcessor(const CapsuleLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "CapsuleLightFeatureProcessor";

            IndexedDataVector<CapsuleLightData> m_capsuleLightData;
            GpuBufferHandler m_lightBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
