/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/DiffuseGlobalIllumination/DiffuseProbeGridFeatureProcessorInterface.h>
#include <DiffuseGlobalIllumination/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        //! This class manages DiffuseProbeGrids which generate diffuse global illumination
        class DiffuseProbeGridFeatureProcessor final
            : public DiffuseProbeGridFeatureProcessorInterface
            , private Data::AssetBus::MultiHandler
        {
        public:
            AZ_RTTI(AZ::Render::DiffuseProbeGridFeatureProcessor, "{BCD232F9-1EBF-4D0D-A5F4-84AEC933A93C}", DiffuseProbeGridFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            DiffuseProbeGridFeatureProcessor() = default;
            virtual ~DiffuseProbeGridFeatureProcessor() = default;

            // DiffuseProbeGridFeatureProcessorInterface overrides
            DiffuseProbeGridHandle AddProbeGrid(const AZ::Transform& transform, const AZ::Vector3& extents, const AZ::Vector3& probeSpacing) override;
            void RemoveProbeGrid(DiffuseProbeGridHandle& probeGrid) override;
            bool IsValidProbeGridHandle(const DiffuseProbeGridHandle& probeGrid) const override { return (probeGrid.get() != nullptr); }
            bool ValidateExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newExtents) override;
            void SetExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& extents) override;
            void SetTransform(const DiffuseProbeGridHandle& probeGrid, const AZ::Transform& transform) override;
            bool ValidateProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newSpacing) override;
            void SetProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& probeSpacing) override;
            void SetViewBias(const DiffuseProbeGridHandle& probeGrid, float viewBias) override;
            void SetNormalBias(const DiffuseProbeGridHandle& probeGrid, float normalBias) override;
            void SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier) override;
            void Enable(const DiffuseProbeGridHandle& probeGrid, bool enable) override;
            void SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows) override;
            void SetUseDiffuseIbl(const DiffuseProbeGridHandle& probeGrid, bool useDiffuseIbl) override;
            void SetMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridMode mode) override;
            void SetBakedTextures(const DiffuseProbeGridHandle& probeGrid, const DiffuseProbeGridBakedTextures& bakedTextures) override;

            void BakeTextures(
                const DiffuseProbeGridHandle& probeGrid,
                DiffuseProbeGridBakeTexturesCallback callback,
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& relocationTextureRelativePath,
                const AZStd::string& classificationTextureRelativePath) override;

            bool CheckTextureAssetNotification(
                const AZStd::string& relativePath,
                Data::Asset<RPI::StreamingImageAsset>& outTextureAsset,
                DiffuseProbeGridTextureNotificationType& outNotificationType) override;

            bool AreBakedTexturesReferenced(
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& relocationTextureRelativePath,
                const AZStd::string& classificationTextureRelativePath) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            // retrieve the full list of diffuse probe grids
            using DiffuseProbeGridVector = AZStd::vector<AZStd::shared_ptr<DiffuseProbeGrid>>;
            DiffuseProbeGridVector& GetProbeGrids() { return m_diffuseProbeGrids; }

            // retrieve the side list of probe grids that are using real-time (raytraced) mode
            DiffuseProbeGridVector& GetRealTimeProbeGrids() { return m_realTimeDiffuseProbeGrids; }

            // retrieve the side list of probe grids that are using  real-time (raytraced) mode and visible (on screen)
            DiffuseProbeGridVector& GetVisibleRealTimeProbeGrids() { return m_visibleRealTimeDiffuseProbeGrids; }

        private:
            AZ_DISABLE_COPY_MOVE(DiffuseProbeGridFeatureProcessor);

            // create the box vertex and index streams, which are used to render the probe volumes
            void CreateBoxMesh();

            // AssetBus::MultiHandler overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;

            // updates the real-time list for a specific probe grid
            void UpdateRealTimeList(const DiffuseProbeGridHandle& diffuseProbeGrid);

            // adds a notification entry for a new asset
            void AddNotificationEntry(const AZStd::string& relativePath);

            // notifies and removes the notification entry
            void HandleAssetNotification(Data::Asset<Data::AssetData> asset, DiffuseProbeGridTextureNotificationType notificationType);

            // RPI::SceneNotificationBus::Handler overrides
            void OnBeginPrepareRender() override;
            void OnEndPrepareRender() override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;

            void UpdatePipelineStates();
            void UpdatePasses();

            // list of all diffuse probe grids
            const size_t InitialProbeGridAllocationSize = 64;
            DiffuseProbeGridVector m_diffuseProbeGrids;

            // side list of diffuse probe grids that are in real-time mode (subset of m_diffuseProbeGrids)
            DiffuseProbeGridVector m_realTimeDiffuseProbeGrids;

            // side list of diffuse probe grids that are in real-time mode and visible (subset of m_realTimeDiffuseProbeGrids)
            DiffuseProbeGridVector m_visibleRealTimeDiffuseProbeGrids;

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
            // note that the position and index views are stored in DiffuseProbeGridRenderData
            AZStd::vector<Position> m_boxPositions;
            AZStd::vector<uint16_t> m_boxIndices;
            RHI::Ptr<RHI::Buffer> m_boxPositionBuffer;
            RHI::Ptr<RHI::Buffer> m_boxIndexBuffer;
            RHI::InputStreamLayout m_boxStreamLayout;

            // contains the rendering data needed by probe grids
            // it is loaded by the feature processor and passed to the probes to avoid loading it in each probe
            DiffuseProbeGridRenderData m_probeGridRenderData;

            // indicates that the probe grid list needs to be re-sorted, necessary when a probe grid is resized
            bool m_probeGridSortRequired = false;

            // indicates the the diffuse probe grid render pipeline state needs to be updated
            bool m_needUpdatePipelineStates = false;

            // list of texture assets that we need to check during Simulate() to see if they are ready
            struct NotifyTextureAssetEntry
            {
                AZStd::string m_relativePath;
                AZ::Data::AssetId m_assetId;
                Data::Asset<RPI::StreamingImageAsset> m_asset;
                DiffuseProbeGridTextureNotificationType m_notificationType = DiffuseProbeGridTextureNotificationType::None;
            };
            typedef AZStd::vector<NotifyTextureAssetEntry> NotifyTextureAssetVector;
            NotifyTextureAssetVector m_notifyTextureAssets;
        };
    } // namespace Render
} // namespace AZ
