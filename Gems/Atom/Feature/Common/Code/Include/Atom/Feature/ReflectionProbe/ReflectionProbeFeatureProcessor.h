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

#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <ReflectionProbe/ReflectionProbe.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbeFeatureProcessor final
            : public ReflectionProbeFeatureProcessorInterface,
              private Data::AssetBus::Handler
        {
        public:
            AZ_RTTI(ReflectionProbeFeatureProcessor, "{A08C591F-D2AB-4550-852A-4436533DB137}", ReflectionProbeFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            ReflectionProbeFeatureProcessor() = default;
            virtual ~ReflectionProbeFeatureProcessor() = default;

            // ReflectionProbeFeatureProcessorInterface overrides
            ReflectionProbeHandle AddProbe(const AZ::Transform& transform, bool useParallaxCorrection) override;
            void RemoveProbe(ReflectionProbeHandle& probe) override;
            void SetProbeOuterExtents(const ReflectionProbeHandle& probe, const AZ::Vector3& outerExtents) override;
            void SetProbeInnerExtents(const ReflectionProbeHandle& probe, const AZ::Vector3& innerExtents) override;
            void SetProbeCubeMap(const ReflectionProbeHandle& probe, Data::Instance<RPI::Image>& cubeMapImage) override;
            void SetProbeTransform(const ReflectionProbeHandle& probe, const AZ::Transform& transform) override;
            void BakeProbe(const ReflectionProbeHandle& probe, BuildCubeMapCallback callback) override;
            void NotifyCubeMapAssetReady(const AZStd::string relativePath, NotifyCubeMapAssetReadyCallback callback) override;
            bool IsValidProbeHandle(const ReflectionProbeHandle& probe) const override { return (probe.get() != nullptr); }
            void ShowProbeVisualization(const ReflectionProbeHandle& probe, bool showVisualization) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // find the reflection probe volumes that contain the position
            using ReflectionProbeVector = AZStd::vector<AZStd::shared_ptr<ReflectionProbe>>;
            void FindReflectionProbes(const AZ::Vector3& position, ReflectionProbeVector& reflectionProbes);

        private:

            AZ_DISABLE_COPY_MOVE(ReflectionProbeFeatureProcessor);

            // create the box vertex and index streams, which are used to render the probe volumes
            void CreateBoxMesh();

            // load the shader and retrieve pipeline state, Srg, and drawListTag
            void LoadShader(const char* filePath, RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState, Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset, RHI::DrawListTag& drawListTag);

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;

            void UpdatePipelineStates();

            // AssetBus::Handler overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // notifies and removes the notification entry
            void HandleAssetNotification(Data::Asset<Data::AssetData> asset);

            // list of reflection probes
            const size_t InitialProbeAllocationSize = 64;
            ReflectionProbeVector m_reflectionProbes;

            // list of cubemap assets that we need to check during Simulate() to see if they are ready
            struct NotifyCubeMapAssetEntry
            {
                AZStd::string m_relativePath;
                AZ::Data::AssetId m_assetId;
                NotifyCubeMapAssetReadyCallback m_callback;
                Data::Asset<RPI::StreamingImageAsset> m_asset;
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

            // indicates that the probe list needs to be re-sorted, necessary when a probe is resized
            bool m_probeSortRequired = false;

            bool m_needUpdatePipelineStates = false;
        };
    } // namespace Render
} // namespace AZ
