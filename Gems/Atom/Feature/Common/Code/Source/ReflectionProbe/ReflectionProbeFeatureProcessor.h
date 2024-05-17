/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <ReflectionProbe/ReflectionProbe.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeFeatureProcessor final
            : public ReflectionProbeFeatureProcessorInterface,
              private Data::AssetBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(ReflectionProbeFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::ReflectionProbeFeatureProcessor, "{A08C591F-D2AB-4550-852A-4436533DB137}", AZ::Render::ReflectionProbeFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            ReflectionProbeFeatureProcessor() = default;
            virtual ~ReflectionProbeFeatureProcessor() = default;

            // ReflectionProbeFeatureProcessorInterface overrides
            ReflectionProbeHandle AddReflectionProbe(const AZ::Transform& transform, bool useParallaxCorrection) override;
            void RemoveReflectionProbe(ReflectionProbeHandle& handle) override;
            bool IsValidHandle(const ReflectionProbeHandle& handle) const override { return (m_reflectionProbeMap.find(handle) != m_reflectionProbeMap.end()); }

            void SetOuterExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& outerExtents) override;
            AZ::Vector3 GetOuterExtents(const ReflectionProbeHandle& handle) const override;

            void SetInnerExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& innerExtents) override;
            AZ::Vector3 GetInnerExtents(const ReflectionProbeHandle& handle) const override;

            AZ::Obb GetOuterObbWs(const ReflectionProbeHandle& handle) const override;
            AZ::Obb GetInnerObbWs(const ReflectionProbeHandle& handle) const override;

            void SetTransform(const ReflectionProbeHandle& handle, const AZ::Transform& transform) override;
            AZ::Transform GetTransform(const ReflectionProbeHandle& handle) const override;

            void SetCubeMap(const ReflectionProbeHandle& handle, Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath) override;
            Data::Instance<RPI::Image> GetCubeMap(const ReflectionProbeHandle& handle) const override;

            void SetRenderExposure(const ReflectionProbeHandle& handle, float renderExposure) override;
            float GetRenderExposure(const ReflectionProbeHandle& handle) const override;

            void SetBakeExposure(const ReflectionProbeHandle& handle, float bakeExposure) override;
            float GetBakeExposure(const ReflectionProbeHandle& handle) const override;

            bool GetUseParallaxCorrection(const ReflectionProbeHandle& handle) const override;

            void Bake(const ReflectionProbeHandle& handle, BuildCubeMapCallback callback, const AZStd::string& relativePath) override;
            bool CheckCubeMapAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& outCubeMapAsset, CubeMapAssetNotificationType& outNotificationType) override;
            bool IsCubeMapReferenced(const AZStd::string& relativePath) override;
            void ShowVisualization(const ReflectionProbeHandle& handle, bool showVisualization) override;
            void FindReflectionProbes(const AZ::Vector3& position, ReflectionProbeHandleVector& reflectionProbeHandles) override;
            void FindReflectionProbes(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbeHandles) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void OnRenderEnd() override;

        private:

            AZ_DISABLE_COPY_MOVE(ReflectionProbeFeatureProcessor);

            // create the box vertex and index streams, which are used to render the probe volumes
            void CreateBoxMesh();

            // load the shader and retrieve pipeline state, shader, Srg Layout, and drawListTag
            void LoadShader(
                const char* filePath, RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState, Data::Instance<RPI::Shader>& shader,
                RHI::Ptr<RHI::ShaderResourceGroupLayout>& srgLayout, RHI::DrawListTag& drawListTag);

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            void UpdatePipelineStates();

            // AssetBus::MultiHandler overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            // notifies and removes the notification entry
            void HandleAssetNotification(Data::Asset<Data::AssetData> asset, CubeMapAssetNotificationType notificationType);

            // internal helper for FindReflectionProbes
            void FindReflectionProbesInternal(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbes, AZStd::function<bool(const ReflectionProbe*)> filter = {});

            // checks that the ReflectionProbeHandle exists in the ReflectionProbeMap
            bool ValidateHandle(const ReflectionProbeHandle& handle) const;

            // hash table of reflection probe handles for constant-time lookup
            using ReflectionProbePtr = AZStd::shared_ptr<ReflectionProbe>;
            using ReflectionProbeMap = AZStd::unordered_map <ReflectionProbeHandle, ReflectionProbePtr>;
            ReflectionProbeMap m_reflectionProbeMap;

            // list of reflection probes, sorted by size for rendering
            using ReflectionProbeVector = AZStd::vector<ReflectionProbePtr>;
            const size_t InitialProbeAllocationSize = 64;
            ReflectionProbeVector m_reflectionProbes;

            // list of cubemap assets that we need to check during Simulate() to see if they are ready
            struct NotifyCubeMapAssetEntry
            {
                AZStd::string m_relativePath;
                AZ::Data::AssetId m_assetId;
                Data::Asset<RPI::StreamingImageAsset> m_asset;
                CubeMapAssetNotificationType m_notificationType = CubeMapAssetNotificationType::None;
            };
            typedef AZStd::vector<NotifyCubeMapAssetEntry> NotifyCubeMapAssetVector;
            NotifyCubeMapAssetVector m_notifyCubeMapAssets;

            // position structure for the box vertices
            struct Position
            {
                float m_x = 0.0f;
                float m_y = 0.0f;
                float m_z = 0.0f;
            };

            // buffer pool for the vertex and index buffers
            RHI::Ptr<RHI::BufferPool> m_bufferPool;

            // box mesh rendering buffers
            // note that the position and index views are stored in ReflectionRenderData
            AZStd::vector<Position> m_boxPositions;
            AZStd::vector<uint16_t> m_boxIndices;
            RHI::Ptr<RHI::Buffer> m_boxPositionBuffer;
            RHI::Ptr<RHI::Buffer> m_boxIndexBuffer;
            RHI::InputStreamLayout m_boxStreamLayout;

            // contains the rendering data needed by reflection probes
            // it is loaded by the feature processor and passed to the probes to avoid loading it in each probe
            ReflectionRenderData m_reflectionRenderData;

            // flags
            bool m_probeSortRequired = false;
            bool m_meshFeatureProcessorUpdateRequired = false;
            bool m_needUpdatePipelineStates = false;
        };
    } // namespace Render
} // namespace AZ
