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

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/Feature/CoreLights/SpotLightFeatureProcessorInterface.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <CoreLights/IndexedDataVector.h>
#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/SpotLightShadowmapsPass.h>

namespace AZ
{
    class Vector3;
    class Color;

    namespace Render
    {
        struct SpotLightShadowData
        {
            Matrix4x4 m_depthBiasMatrix = Matrix4x4::CreateIdentity();
            uint32_t m_shadowmapArraySlice = 0; // array slice who has shadowmap in the atlas.
            AZStd::array<uint32_t, 2> m_shadowmapOriginInSlice = { {0, 0 } }; // shadowmap origin in the slice of the atlas.
            uint32_t m_shadowmapSize = static_cast<uint32_t>(ShadowmapSize::None); // width and height of shadowmap.
            uint32_t m_shadowFilterMethod = 0; // filtering method of shadows.
            float m_boundaryScale = 0.f; // the half of boundary of lit/shadowed areas. (in degrees)
            uint32_t m_predictionSampleCount = 0; // sample count to judge whether it is on the shadow boundary or not.
            uint32_t m_filteringSampleCount = 0;
            AZStd::array<float, 2> m_unprojectConstants = { {0, 0} };
            float m_bias = 0.0f;    // Consider making this variable or the slope-scale depth bias be tuneable in the Editor
            PcfMethod m_pcfMethod = PcfMethod::BoundarySearch; 
        };

        class SpotLightFeatureProcessor final
            : public SpotLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::SpotLightFeatureProcessor, "{8823AFBB-761E-42A2-B665-BBDF6F63E10A}", AZ::Render::SpotLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            SpotLightFeatureProcessor() = default;
            virtual ~SpotLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>&) override;
            void Render(const RenderPacket& packet) override;

            // SpotLightFeatureProcessorInterface overrides ...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnit::Candela>& lightColor) override;
            void SetPosition(LightHandle handle, const Vector3& lightPosition) override;
            void SetDirection(LightHandle handle, const Vector3& direction) override;
            void SetBulbRadius(LightHandle handle, float bulbRadius) override;
            void SetConeAngles(LightHandle handle, float innerDegrees, float outerDegrees) override;
            void SetPenumbraBias(LightHandle handle, float penumbraBias) override;
            void SetAttenuationRadius(LightHandle handle, float attenuationRadius) override;
            void SetShadowmapSize(LightHandle lightId, ShadowmapSize shadowmapSize) override;
            void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) override;
            void SetShadowBoundaryWidthAngle(LightHandle handle, float boundaryWidthDegree) override;
            void SetPredictionSampleCount(LightHandle handle, uint16_t count) override;
            void SetFilteringSampleCount(LightHandle handle, uint16_t count) override;
            void SetPcfMethod(LightHandle handle, PcfMethod method);
            void SetSpotLightData(LightHandle handle, const SpotLightData& data) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer() const;
            uint32_t GetLightCount()const;

        private:
            struct LightProperty
            {
                float m_outerConeAngle = 0.f; // in radians.
            };
            struct ShadowProperty
            {
                LightHandle m_shadowHandle;
                RPI::ViewPtr m_shadowmapView;
                uint16_t m_viewTagIndex = SpotLightShadowmapsPass::InvalidIndex;
                bool m_shadowmapViewNeedsUpdate = false;
            };

            SpotLightFeatureProcessor(const SpotLightFeatureProcessor&) = delete;

            // RPI::SceneNotificationBus::Handler overrides...
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;

            uint16_t GetLightIndexInSrg(LightHandle handle) const;

            // shadow specific functions
            ShadowProperty& GetOrCreateShadowProperty(LightHandle handle);
            void PrepareForShadow(LightHandle handle, ShadowmapSize size);
            void CleanUpShadow(LightHandle handle);
            void UpdateShadowmapViews();
            void SetShadowParameterToShadowData();

            void UpdateBulbPositionOffset(SpotLightData& light);

            // This caches SpotLightShadowmapsPass and EsmShadowmapsPass.
            void CachePasses();
            AZStd::vector<RPI::RenderPipelineId> CacheSpotLightShadowmapsPass();
            void CacheEsmShadowmapsPass(const AZStd::vector<RPI::RenderPipelineId>& validPipelineIds);

            //! This updates the parameter of Gaussian filter used in ESM.
            void UpdateFilterParameters();
            void UpdateStandardDeviations();
            void UpdateFilterOffsetsCounts();
            void UpdateShadowmapPositionsInAtlas();
            void SetFilterParameterToPass();
            bool NeedsFilterUpdate(LightHandle shadowHandle) const;

            AZStd::unordered_map<LightHandle, ShadowProperty> m_shadowProperties;
            IndexedDataVector<LightProperty> m_lightProperties;

            AZStd::vector<SpotLightShadowmapsPass*> m_spotLightShadowmapsPasses;
            AZStd::vector<EsmShadowmapsPass*> m_esmShadowmapsPasses;

            GpuBufferHandler m_lightBufferHandler;
            IndexedDataVector<SpotLightData> m_spotLightData;

            GpuBufferHandler m_shadowBufferHandler;
            IndexedDataVector<SpotLightShadowData> m_shadowData;

            GpuBufferHandler m_esmParameterBufferHandler;
            IndexedDataVector<EsmShadowmapsPass::FilterParameter> m_esmParameterData;

            bool m_deviceBufferNeedsUpdate = false;
            bool m_shadowmapPassNeedsUpdate = true;
            bool m_filterParameterNeedsUpdate = false;
            uint32_t m_shadowmapIndexTableBufferNameIndex = 0;

            RHI::ShaderInputConstantIndex m_shadowmapAtlasSizeIndex;
            RHI::ShaderInputConstantIndex m_invShadowmapAtlasSize;

            const Name m_lightTypeName = Name("spot");
        };
    } // namespace Render
} // namespace AZ
