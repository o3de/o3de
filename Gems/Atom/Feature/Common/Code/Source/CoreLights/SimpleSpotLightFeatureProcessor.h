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
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/SimpleSpotLightFeatureProcessorInterface.h>
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
        static const uint8_t MaxGoboTextureCount = 5;

        struct alignas(16) SimpleSpotLightData
        {
            AZStd::array<float, 16> m_viewProjectionMatrix; // Matrix to transform from world space to the spot light's lighting frustum. 
            AZStd::array<float, 3> m_position = { { 0.0f, 0.0f, 0.0f } };
            float m_invAttenuationRadiusSquared = 0.0f; // Inverse of the distance at which this light no longer has an effect, squared. Also used for falloff calculations.
            AZStd::array<float, 3> m_direction = { { 0.0f, 0.0f, 0.0f } };
            float m_cosInnerConeAngle = 0.707f; // Cosine of the inner cone angle
            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float m_cosOuterConeAngle = 0.707f; // Cosine of the outer cone angle

            uint16_t m_shadowIndex = std::numeric_limits<uint16_t>::max(); // index for ProjectedShadowData. A value of 0xFFFF indicates an illegal index.
            uint32_t m_goboTextureIndex = MaxGoboTextureCount; // index for m_goboTextures.
            float m_affectsGIFactor = 1.0f;
            bool m_affectsGI = true;
            uint32_t m_lightingChannelMask = 1;
        };

        class SimpleSpotLightFeatureProcessor final
            : public SimpleSpotLightFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(SimpleSpotLightFeatureProcessor, AZ::SystemAllocator)
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
            void SetTransform(LightHandle handle, const AZ::Transform& transform) override;
            void SetConeAngles(LightHandle handle, float innerRadians, float outerRadians) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetAffectsGI(LightHandle handle, bool affectsGI) override;
            void SetAffectsGIFactor(LightHandle handle, float affectsGIFactor) override;
            void SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask) override;
            void SetShadowsEnabled(LightHandle handle, bool enabled) override;
            void SetShadowBias(LightHandle handle, float bias) override;
            void SetNormalShadowBias(LightHandle handle, float bias) override;
            void SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize) override;
            void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) override;
            void SetFilteringSampleCount(LightHandle handle, uint16_t count) override;
            void SetEsmExponent(LightHandle handle, float esmExponent) override;
            void SetUseCachedShadows(LightHandle handle, bool useCachedShadows) override;
            void SetGoboTexture(LightHandle handle, AZ::Data::Instance<AZ::RPI::Image> goboTexture) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer() const override;
            uint32_t GetLightCount() const override;

            // SceneNotificationBus::Handler overrides...
            void OnRenderPipelinePersistentViewChanged(
                RPI::RenderPipeline* renderPipeline,
                RPI::PipelineViewTag viewTag,
                RPI::ViewPtr newView,
                RPI::ViewPtr previousView) override;
        private:
            SimpleSpotLightFeatureProcessor(const SimpleSpotLightFeatureProcessor&) = delete;

            static constexpr const char* FeatureProcessorName = "SimpleSpotLightFeatureProcessor";

            void UpdateBounds(LightHandle handle);
            void UpdateShadow(LightHandle handle);

            void UpdateViewProjectionMatrix(LightHandle handle);

            // Convenience function for forwarding requests to the ProjectedShadowFeatureProcessor
            template<typename Functor, typename ParamType>
            void SetShadowSetting(LightHandle handle, Functor&&, ParamType&& param);

            // Cull the lights for a view using the CPU.
            void CullLights(const RPI::ViewPtr& view);

            // Extra light data which is not used directly by the gpu shader
            struct ExtraData
            {
                MeshCommon::BoundsVariant m_boundsVariant;
                AZ::Data::Instance<AZ::RPI::Image> m_goboTexture;
                Transform m_transform;
            };

            MultiIndexedDataVector<SimpleSpotLightData, ExtraData> m_lightData;
            GpuBufferHandler m_lightBufferHandler;
            RHI::Handle<uint32_t> m_lightMeshFlag;
            RHI::Handle<uint32_t> m_shadowMeshFlag;
            RHI::Handle<uint32_t> m_goboTextureFlag;
            bool m_deviceBufferNeedsUpdate = false;

            RHI::ShaderInputNameIndex m_goboTexturesIndex = "m_goboTextures";
            AZStd::vector<AZ::Data::Instance<AZ::RPI::Image>> m_goboTextures;
            bool m_goboArrayChanged = false;

            ProjectedShadowFeatureProcessor* m_shadowFeatureProcessor = nullptr;

            // Handlers to GPU buffer that are being used for CPU culling visibility.
            AZStd::vector<GpuBufferHandler> m_visibleSpotLightsBufferHandlers;
            // Number of buffers being used for visibility in the current frame.
            uint32_t m_visibleSpotLightsBufferUsedCount = 0;
            // Views that have a GPU culling pass per render pipeline.
            AZStd::unordered_map<const RPI::View*, AZStd::vector<const RPI::RenderPipeline*>> m_cpuCulledPipelinesPerView;
        };
    } // namespace Render
} // namespace AZ
