/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DiffuseProbeGrid/DiffuseProbeGridFeatureProcessorInterface.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Render/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        class SpecularReflectionsFeatureProcessorInterface;

        //! This class manages DiffuseProbeGrids which generate diffuse global illumination
        class DiffuseProbeGridFeatureProcessor final
            : public DiffuseProbeGridFeatureProcessorInterface
            , private Data::AssetBus::MultiHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridFeatureProcessor, SystemAllocator)
            AZ_RTTI(AZ::Render::DiffuseProbeGridFeatureProcessor, "{BCD232F9-1EBF-4D0D-A5F4-84AEC933A93C}", AZ::Render::DiffuseProbeGridFeatureProcessorInterface);

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
            void SetNumRaysPerProbe(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridNumRaysPerProbe numRaysPerProbe) override;
            void SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier) override;
            void Enable(const DiffuseProbeGridHandle& probeGrid, bool enable) override;
            void SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows) override;
            void SetUseDiffuseIbl(const DiffuseProbeGridHandle& probeGrid, bool useDiffuseIbl) override;
            void SetMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridMode mode) override;
            void SetScrolling(const DiffuseProbeGridHandle& probeGrid, bool scrolling) override;
            void SetEdgeBlendIbl(const DiffuseProbeGridHandle& probeGrid, bool edgeBlendIbl) override;
            void SetFrameUpdateCount(const DiffuseProbeGridHandle& probeGrid, uint32_t frameUpdateCount) override;
            void SetTransparencyMode(const DiffuseProbeGridHandle& probeGrid, DiffuseProbeGridTransparencyMode transparencyMode) override;
            void SetEmissiveMultiplier(const DiffuseProbeGridHandle& probeGrid, float emissiveMultiplier) override;
            void SetBakedTextures(const DiffuseProbeGridHandle& probeGrid, const DiffuseProbeGridBakedTextures& bakedTextures) override;
            void SetVisualizationEnabled(const DiffuseProbeGridHandle& probeGrid, bool visualizationEnabled) override;
            void SetVisualizationShowInactiveProbes(const DiffuseProbeGridHandle& probeGrid, bool visualizationShowInactiveProbes) override;
            void SetVisualizationSphereRadius(const DiffuseProbeGridHandle& probeGrid, float visualizationSphereRadius) override;

            bool CanBakeTextures() override;
            void BakeTextures(
                const DiffuseProbeGridHandle& probeGrid,
                DiffuseProbeGridBakeTexturesCallback callback,
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& probeDataTextureRelativePath) override;

            bool CheckTextureAssetNotification(
                const AZStd::string& relativePath,
                Data::Asset<RPI::StreamingImageAsset>& outTextureAsset,
                DiffuseProbeGridTextureNotificationType& outNotificationType) override;

            bool AreBakedTexturesReferenced(
                const AZStd::string& irradianceTextureRelativePath,
                const AZStd::string& distanceTextureRelativePath,
                const AZStd::string& probeDataTextureRelativePath) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            // retrieve the full list of diffuse probe grids
            using DiffuseProbeGridVector = AZStd::vector<AZStd::shared_ptr<DiffuseProbeGrid>>;
            DiffuseProbeGridVector& GetProbeGrids() { return m_diffuseProbeGrids; }

            // retrieve the side list of probe grids that are using real-time (raytraced) mode
            DiffuseProbeGridVector& GetRealTimeProbeGrids() { return m_realTimeDiffuseProbeGrids; }

            // retrieve the side list of probe grids that are visible (on screen), both real-time (raytraced) and baked
            DiffuseProbeGridVector& GetVisibleProbeGrids() { return m_visibleDiffuseProbeGrids; }

            // retrieve the side list of probe grids that are real-time (raytraced) and visible (on screen)
            DiffuseProbeGridVector& GetVisibleRealTimeProbeGrids() { return m_visibleRealTimeDiffuseProbeGrids; }

            // returns the RayTracingBufferPool used for the DiffuseProbeGrid visualization
            RHI::RayTracingBufferPools& GetVisualizationBufferPools() { return *m_visualizationBufferPools; }

            // returns the RayTracingBlas for the visualization model
            const RHI::Ptr<RHI::RayTracingBlas>& GetVisualizationBlas() const { return m_visualizationBlas; }
            RHI::Ptr<RHI::RayTracingBlas>& GetVisualizationBlas() { return m_visualizationBlas; }

            // adds a worldspace position and direction for an irradiance query, returns the index of the query result in the output buffer
            uint32_t AddIrradianceQuery(const AZ::Vector3& position, const AZ::Vector3& direction);

            // clears the irradiance queries, called by DiffuseProbeGridQueryPass at the end of the frame
            void ClearIrradianceQueries();

            // irradiance query accessors
            uint32_t GetIrradianceQueryCount() const { return aznumeric_cast<uint32_t>(m_irradianceQueries.size()); }
            const Data::Instance<RPI::Buffer>& GetQueryBuffer() const { return m_queryBuffer.GetCurrentBuffer(); }
            const RHI::AttachmentId GetQueryBufferAttachmentId() const { return m_queryBufferAttachmentId; }
            const RHI::BufferViewDescriptor& GetQueryBufferViewDescriptor() const { return m_queryBufferViewDescriptor; }

            // Returns the Scene SRG that can tbe use with precompiled shaders
            RPI::ShaderResourceGroup* GetSceneSrg() const;

            // Returns the View SRG for a pipeline and PipelineViewTag combination that can be used with precompiled shaders
            RPI::ShaderResourceGroup* GetViewSrg(RPI::RenderPipeline* pipeline, RPI::PipelineViewTag viewTag) const;     

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
            void OnRenderPipelineChanged(RPI::RenderPipeline* pipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;
            void OnRenderPipelinePersistentViewChanged(
                RPI::RenderPipeline* renderPipeline, RPI::PipelineViewTag viewTag, RPI::ViewPtr newView, RPI::ViewPtr previousView) override;

            
            // FeatureProcessor overrides
            void AddRenderPasses(RPI::RenderPipeline* renderPipeline) override;
            
            void AddPassRequest(RPI::RenderPipeline* renderPipeline, const char* passRequestAssetFilePath, const char* insertionPointPassName);
            void UpdatePipelineStates();
            void UpdatePasses();

            // loads the probe visualization model and creates the BLAS
            void OnVisualizationModelAssetReady(Data::Asset<Data::AssetData> asset);

            // list of all diffuse probe grids
            const size_t InitialProbeGridAllocationSize = 64;
            DiffuseProbeGridVector m_diffuseProbeGrids;

            // side list of diffuse probe grids that are in real-time mode (subset of m_diffuseProbeGrids)
            DiffuseProbeGridVector m_realTimeDiffuseProbeGrids;

            // side list of diffuse probe grids that are visible, both real-time and baked modes (subset of m_diffuseProbeGrids)
            DiffuseProbeGridVector m_visibleDiffuseProbeGrids;

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

            // visualization
            RHI::Ptr<RHI::RayTracingBufferPools> m_visualizationBufferPools;
            Data::Asset<RPI::ModelAsset> m_visualizationModelAsset;
            RHI::Ptr<RHI::RayTracingBlas> m_visualizationBlas;
            Data::Instance<RPI::Model> m_visualizationModel;
            RHI::StreamBufferView m_visualizationVB;
            RHI::IndexBufferView m_visualizationIB;

            // irradiance queries
            struct IrradianceQuery
            {
                AZ::Vector3 m_position;
                AZ::Vector3 m_direction;
            };
            using IrradianceQueryVector = AZStd::vector<IrradianceQuery>;

            IrradianceQueryVector m_irradianceQueries;
            RHI::BufferViewDescriptor m_queryBufferViewDescriptor;
            RHI::AttachmentId m_queryBufferAttachmentId;
            RPI::RingBuffer m_queryBuffer{ "DiffuseQueryBuffer", RPI::CommonBufferPoolType::ReadWrite, sizeof(IrradianceQuery) };

            // SSR state, for controlling the DiffuseProbeGridQueryPass in the SSR pipeline
            SpecularReflectionsFeatureProcessorInterface* m_specularReflectionsFeatureProcessor = nullptr;
            bool m_ssrRayTracingEnabled = false;

            // Shader that contains the scene and view SRGs for precompiled shaders
            Data::Instance<RPI::Shader> m_sceneAndViewShader;

            // SRG for copying the Scene SRG shader inputs
            Data::Instance<RPI::ShaderResourceGroup> m_sceneShaderResourceGroup = nullptr;

            // SRGs for copying the View SRGs shader inputs
            using ViewShaderResourceGroups = AZStd::unordered_map<RPI::PipelineViewTag, Data::Instance<RPI::ShaderResourceGroup>>;
            AZStd::unordered_map<RPI::RenderPipeline*, ViewShaderResourceGroups> m_viewShaderResourceGroups;
        };
    } // namespace Render
} // namespace AZ
