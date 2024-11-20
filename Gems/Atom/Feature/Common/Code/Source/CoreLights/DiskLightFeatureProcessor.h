/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/DiskLightFeatureProcessorInterface.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/Mesh/MeshCommon.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/MultiIndexedDataVector.h>
#include <CoreLights/LightCommon.h>
#include <Shadows/ProjectedShadowFeatureProcessor.h>

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
            AZ_CLASS_ALLOCATOR(DiskLightFeatureProcessor, AZ::SystemAllocator)
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
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetDiskRadius(LightHandle handle, float radius) override;
            void SetConstrainToConeLight(LightHandle handle, bool useCone) override;
            void SetConeAngles(LightHandle handle, float innerDegrees, float outerDegrees) override;
            void SetShadowsEnabled(LightHandle handle, bool enabled) override;
            void SetShadowBias(LightHandle handle, float bias) override;
            void SetNormalShadowBias(LightHandle handle, float bias) override;
            void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) override;
            void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) override;
            void SetFilteringSampleCount(LightHandle handle, uint16_t count) override;
            void SetEsmExponent(LightHandle handle, float esmExponent) override;
            void SetUseCachedShadows(LightHandle handle, bool useCachedShadows) override;
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;
            void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) override;

            void SetDiskData(LightHandle handle, const DiskLightData& data) override;
            const DiskLightData& GetDiskData(LightHandle handle) const override;

            const Data::Instance<RPI::Buffer> GetLightBuffer() const override;
            uint32_t GetLightCount() const override;

        private:

            static constexpr const char* FeatureProcessorName = "DiskLightFeatureProcessor";

            DiskLightFeatureProcessor(const DiskLightFeatureProcessor&) = delete;
            
            static void UpdateBulbPositionOffset(DiskLightData& light);

            void ValidateAndSetConeAngles(LightHandle handle, float innerRadians, float outerRadians);
            void UpdateBounds(LightHandle handle);
            void UpdateShadow(LightHandle handle);

            // Convenience function for forwarding requests to the ProjectedShadowFeatureProcessor
            template <typename Functor, typename ParamType>
            void SetShadowSetting(LightHandle handle, Functor&&, ParamType&& param);

            ProjectedShadowFeatureProcessor* m_shadowFeatureProcessor = nullptr;

            MultiIndexedDataVector<DiskLightData, MeshCommon::BoundsVariant> m_lightData;
            GpuBufferHandler m_lightBufferHandler;
            RHI::Handle<uint32_t> m_lightMeshFlag;
            RHI::Handle<uint32_t> m_shadowMeshFlag;

            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
