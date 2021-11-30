/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CoreLights/EsmShadowmapsPass.h>

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace Render
    {
        class CascadedShadowmapsPass;
        class EsmShadowmapsPass;

        //! Cascaded shadow specific camera configuration.
        struct CascadeShadowCameraConfiguration
        {
        public:
            CascadeShadowCameraConfiguration();
            ~CascadeShadowCameraConfiguration() = default;

            void SetBaseConfiguration(const Camera::Configuration& config);
            void SetShadowDepthFar(float depthFar);

            float GetFovY() const;
            float GetDepthNear() const;
            float GetDepthFar() const;
            float GetAspectRatio() const;
            float GetDepthCenter(float depthNear, float depthFar) const;
            float GetDepthCenterRatio() const;

            bool HasSameConfiguration(const Camera::Configuration& config) const;

        private:
            void SetDepthCenterRatio();

            Camera::Configuration m_baseConfiguration
            {
                Constants::HalfPi, 0.1f, 100.f, 100.f, 100.f
            };
            float m_shadowDepthFar = FLT_MAX;
            float m_aspectRatio = 0.f;

            // 8 vertices of the frustum are on the surface
            // of the sphere whose center is (0, dc, 0),
            // where dc = (depthNear + depthFar) / 2 * m_depthCenterRatio.
            float m_depthCenterRatio = 0.f; // invalid value
        };

        struct DirectionalLightData
        {
            AZStd::array<float, 3> m_direction = { { 1.0f, 0.0f, 0.0f } };
            float m_angularRadius = 0.0f;
            AZStd::array<float, 3> m_rgbIntensity = { { 0.0f, 0.0f, 0.0f } };
            float padding2 = 0.0f; // Padding between float3s in shader, can be used for other data later.
        };

        // [GFX TODO][ATOM-15172] Look into compacting struct DirectionalLightShadowData
        struct DirectionalLightShadowData
        {
            AZStd::array<Matrix4x4, Shadow::MaxNumberOfCascades> m_lightViewToShadowmapMatrices =
            { {
                    Matrix4x4::CreateIdentity(),
                    Matrix4x4::CreateIdentity(),
                    Matrix4x4::CreateIdentity(),
                    Matrix4x4::CreateIdentity() } };
            AZStd::array<Matrix4x4, Shadow::MaxNumberOfCascades> m_worldToLightViewMatrices =
            { {
                    Matrix4x4::CreateIdentity(),
                    Matrix4x4::CreateIdentity(),
                    Matrix4x4::CreateIdentity(),
                    Matrix4x4::CreateIdentity() } };
            AZStd::array<float, Shadow::MaxNumberOfCascades> m_slopeBiasBase =
            {
                0.f, 0.f, 0.f, 0.f
            };
            float m_boundaryScale = 0.f;
            uint32_t m_shadowmapSize = 1; // width and height of shadowmap
            uint32_t m_cascadeCount = 1;
            // Reduce acne by applying a small amount of bias to apply along shadow-space z.
            float m_shadowBias = 0.0f;
            // Reduces acne by biasing the shadowmap lookup along the geometric normal.
            float m_normalShadowBias;
            uint32_t m_filteringSampleCount = 0;
            uint32_t m_debugFlags = 0;
            uint32_t m_shadowFilterMethod = 0; 
            float m_far_minus_near = 0;
            float m_padding[3];
        };

        static_assert(sizeof(DirectionalLightShadowData) % 16 == 0); // Structured buffers need alignment to be a multiple of 16 bytes.

        class DirectionalLightFeatureProcessor final
            : public DirectionalLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::DirectionalLightFeatureProcessor, "61610178-8DAA-4BF2-AF17-597F20D527DD", AZ::Render::DirectionalLightFeatureProcessorInterface);

            struct CascadeSegment
            {
                // Pipeline view tag of the segment of view frustum
                RPI::PipelineViewTag m_pipelineViewTag;

                // Transient view of the segment of view frustum
                RPI::ViewPtr m_view;

                // AABB of the segment of view frustum
                Aabb m_aabb;

                // Far depth of the segment, i.e., border to the next segment
                float m_borderFarDepth;
            };

            struct ShadowProperty
            {
                // Cascade segment specific properties.
                // Key of the map is the default view (camera view) of the render pipeline.
                AZStd::unordered_map<const RPI::View*, AZStd::fixed_vector<CascadeSegment, Shadow::MaxNumberOfCascades>> m_segments;

                // Default far depth of each cascade.
                AZStd::array<float, Shadow::MaxNumberOfCascades> m_defaultFarDepths;

                // Configuration offers shape of the camera view frustum for each camera view.
                AZStd::unordered_map<const RPI::View*, CascadeShadowCameraConfiguration> m_cameraConfigurations;

                // Shadow specific depth far.
                float m_shadowDepthFar = FLT_MAX;

                // If it is true, shadwomap frustum is split automatically
                // using m_shadowmapFrustumSplitSchemeRatio.
                // Otherwise, user splits it manually.
                bool m_isShadowmapFrustumSplitAutomatic = true;

                // Ratio of shadowmap frustum split scheme (uniform vs logarithm)
                float m_shadowmapFrustumSplitSchemeRatio = 0.8f;

                // This is used in view frustum correction to guess how much
                // bounding sphere projected onto ground is different with the expected.
                float m_groundHeight = 0.f;

                // Radius of the bounding sphere of the camera view frustum (not segment)
                float m_entireFrustumRadius = 0.f;

                // Local center location of the bounding sphere of the camera view frustum (not segment)
                Vector3 m_entireFrustumCenterLocal = Vector3::CreateZero();

                // If true, view frustum correction is enabled using m_cameraHeight.
                bool m_isViewFrustumCorrectionEnabled = false;

                // If true, the frustum of the view will be updated.
                bool m_frustumNeedsUpdate = false;

                // If true, the border of segments will be updated.
                bool m_borderDepthsForSegmentsNeedsUpdate = false;

                // If true, the view of shadowmap will be updated.
                bool m_shadowmapViewNeedsUpdate = false;

                // Shadow filter method of the light
                ShadowFilterMethod m_shadowFilterMethod = ShadowFilterMethod::None;

                // If true, this will reduce the shadow acne introduced by large pcf kernels by estimating the angle of the triangle being shaded
                // with the ddx/ddy functions. 
                bool m_isReceiverPlaneBiasEnabled = true;
            };

            static void Reflect(ReflectContext* context);

            virtual ~DirectionalLightFeatureProcessor() = default;

            // RPI::FeatureProcessor overrides...
            void Activate() override;
            void Deactivate() override;
            void Simulate(const SimulatePacket& packet) override;
            void PrepareViews(const PrepareViewsPacket&, AZStd::vector<AZStd::pair<RPI::PipelineViewTag, RPI::ViewPtr>>&) override;
            void Render(const RenderPacket& packet) override;

            // DirectionalLightFeatureProcessorInterface overrides...
            LightHandle AcquireLight() override;
            bool ReleaseLight(LightHandle& handle) override;
            LightHandle CloneLight(LightHandle handle) override;
            void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnit::Lux>& lightColor) override;
            void SetDirection(LightHandle handle, const Vector3& lightDirection) override;
            void SetAngularDiameter(LightHandle handle, float angularDiameter) override;
            void SetShadowmapSize(LightHandle handle, ShadowmapSize size) override;
            void SetCascadeCount(LightHandle handle, uint16_t cascadeCount) override;
            void SetShadowmapFrustumSplitSchemeRatio(LightHandle handle, float ratio) override;
            void SetCascadeFarDepth(LightHandle handle, uint16_t cascadeIndex, float farDepth) override;
            void SetCameraConfiguration(
                LightHandle handle,
                const Camera::Configuration& cameraConfiguration,
                const RPI::RenderPipelineId& renderPipelineId = RPI::RenderPipelineId()) override;
            void SetShadowFarClipDistance(LightHandle handle, float farDist) override;
            void SetCameraTransform(
                LightHandle handle,
                const Transform& cameraTransform,
                const RPI::RenderPipelineId& renderPipelineId = RPI::RenderPipelineId()) override;
            void SetGroundHeight(LightHandle handle, float groundHeight) override;
            void SetViewFrustumCorrectionEnabled(LightHandle handle, bool enabled) override;
            void SetDebugFlags(LightHandle handle, DebugDrawFlags flags) override;
            void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) override;
            void SetFilteringSampleCount(LightHandle handle, uint16_t count) override;
            void SetShadowReceiverPlaneBiasEnabled(LightHandle handle, bool enable) override;
            void SetShadowBias(LightHandle handle, float bias) override;
            void SetNormalShadowBias(LightHandle handle, float normalShadowBias) override;

            const Data::Instance<RPI::Buffer> GetLightBuffer() const;
            uint32_t GetLightCount() const;

        private:
            // RPI::SceneNotificationBus::Handler overrides...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelinePersistentViewChanged(RPI::RenderPipeline* renderPipeline, RPI::PipelineViewTag viewTag, RPI::ViewPtr newView, RPI::ViewPtr previousView) override;

            //! This prepare for change of render pipelines and camera views.
            void PrepareForChangingRenderPipelineAndCameraView();
            //! This caches valid CascadedShadowmapsPass.
            void CacheCascadedShadowmapsPass();
            //! This caches valid EsmShadowmapsPass.
            void CacheEsmShadowmapsPass();
            //! This add/remove camera views in shadow properties.
            void PrepareCameraViews();
            //! This create/destruct shadow buffer for each render pipeline.
            void PrepareShadowBuffers();
            //! This create/destruct cascade segments.
            void CacheRenderPipelineIdsForPersistentView();
            //! This applies the current configuration to cached passes.
            void SetConfigurationToPasses();

            //! This stop calucation and drawing in the shadowmap passes.
            //! It will be called when no shadow is expected.
            void SleepShadowmapPasses();

            //! This returns the number of cascades.
            //! (= the number of segments of view frustum.)
            uint16_t GetCascadeCount(LightHandle handle) const;

            //! This returns the camera configuration.
            //! If it has not been registered for the given camera view.
            //! it returns one of the fallback render pipeline ID.
            const CascadeShadowCameraConfiguration& GetCameraConfiguration(LightHandle handle, const RPI::View* cameraView) const;

            //! This update view frustum of camera.
            void UpdateFrustums(LightHandle handle);

            //! This sets camera view name to CascadedShadowmapsPass.
            void SetCameraViewNameToPass() const;

            //! This sets number of cascades.
            //!  (= the number of segments of the camera view frustum.)
            void UpdateViewsOfCascadeSegments(LightHandle handle, uint16_t cascadeCount);

            //! This sets width/height/depth/arrayCount of the shadowmap image on ShadowmapPass.
            void SetShadowmapImageSizeArraySize(LightHandle handle);

            //! This updates the parameter of Gaussian filter used in ESM.
            void UpdateFilterParameters(LightHandle handle);
            //! This updates if the filter is enabled.
            void UpdateFilterEnabled(LightHandle handle, const RPI::View* cameraView);
            //! This updates shadowmap position(origin and size) in the atlas for each cascade.
            void UpdateShadowmapPositionInAtlas(LightHandle handle, const RPI::View* cameraView);
            //! This set filter parameters to passes which execute filtering.
            void SetFilterParameterToPass(LightHandle handle, const RPI::View* cameraView);

            //! This update the boundary of each segment.
            void UpdateBorderDepthsForSegments(LightHandle handle);

            //! This updates the shadowmap view.
            void UpdateShadowmapViews(LightHandle handle);

            void UpdateViewsOfCascadeSegments();

            //! This calculate shadow view AABB.
            Aabb CalculateShadowViewAabb(
                LightHandle handle,
                const RPI::View* cameraView,
                uint16_t cascadeIndex,
                const Matrix3x4& lightTransform);

            //! This writes outDepthNear and outDepthFar for the segment of the camera view frustum of cascadeIndex.
            void GetDepthNearFar(
                LightHandle handle,
                const RPI::View* cameraView,
                uint16_t cascadeIndex,
                float& outDepthNear,
                float& outDepthFar) const;

            //! This writes outDepthNear and outDepthFar for full range of the shadowmap
            void GetDepthNearFar(LightHandle handle, const RPI::View* cameraView, float& outDepthNear, float& outDepthFar) const;

            //! This returns the world center position of a segment of view frustum.
            Vector3 GetWorldCenterPosition(
                LightHandle handle,
                const RPI::View* cameraView,
                float depthNear,
                float depthFar) const;

            //! This returns the radius of a segment of view frustum.
            float GetRadius(
                LightHandle handle,
                const RPI::View* cameraView,
                float depthNear,
                float depthFar) const;

            //! This calculates the center of corrected boundary sphere for light rectangle.
            //! Using the Projective corrected Michal Valient shadowmaps technique,
            //! boundary sphere is calculated from light direction and frustum segment.
            Vector3 CalculateCorrectedWorldCenterPosition(
                LightHandle handle,
                const RPI::View* cameraView,
                float depthNear,
                float depthFar) const;

            //! This sets shadow parameter to shadow data.
            void SetShadowParameterToShadowData(LightHandle handle);

            //! This draws bounding boxes of cascades.
            void DrawCascadeBoundingBoxes(LightHandle handle);

            float GetShadowmapSizeFromCameraView(const LightHandle handle, const RPI::View* cameraView) const;
            void SnapAabbToPixelIncrements(const float invShadowmapSize, Vector3& orthoMin, Vector3& orthoMax);

            IndexedDataVector<ShadowProperty> m_shadowProperties;
            // [GFX TODO][ATOM-2012] shadow for multiple directional lights
            LightHandle m_shadowingLightHandle;

            GpuBufferHandler m_lightBufferHandler;
            IndexedDataVector<DirectionalLightData> m_lightData;

            AZStd::unordered_map<const RPI::View*, GpuBufferHandler> m_esmParameterBufferHandlers;
            AZStd::unordered_map<const RPI::View*, IndexedDataVector<EsmShadowmapsPass::FilterParameter>> m_esmParameterData;

            AZStd::unordered_map<const RPI::View*, GpuBufferHandler> m_shadowBufferHandlers;
            AZStd::unordered_map<const RPI::View*, IndexedDataVector<DirectionalLightShadowData>> m_shadowData;

            RHI::ShaderInputNameIndex m_shadowIndexDirectionalLightIndex = "m_shadowIndexDirectionalLight";

            AZStd::unordered_map<const RPI::View*, AZStd::vector<RPI::RenderPipelineId>> m_renderPipelineIdsForPersistentView;
            AZStd::unordered_map<const RPI::View*, AZStd::string> m_cameraViewNames;
            AZStd::unordered_map<RPI::RenderPipelineId, AZStd::vector<CascadedShadowmapsPass*>> m_cascadedShadowmapsPasses;
            AZStd::unordered_map<RPI::RenderPipelineId, AZStd::vector<EsmShadowmapsPass*>> m_esmShadowmapsPasses;

            RPI::AuxGeomFeatureProcessorInterface* m_auxGeomFeatureProcessor = nullptr;
            AZStd::vector<const RPI::View*> m_viewsRetainingAuxGeomDraw;

            bool m_lightBufferNeedsUpdate = false;
            bool m_shadowBufferNeedsUpdate = false;
            uint32_t m_shadowBufferNameIndex = 0;
            uint32_t m_shadowmapIndexTableBufferNameIndex = 0;

            Name m_lightTypeName = Name("directional");
            Name m_directionalShadowFilteringMethodName = Name("o_directional_shadow_filtering_method");
            Name m_directionalShadowReceiverPlaneBiasEnableName = Name("o_directional_shadow_receiver_plane_bias_enable");
            static constexpr const char* FeatureProcessorName = "DirectionalLightFeatureProcessor";
        };
    } // namespace Render
} // namespace AZ
