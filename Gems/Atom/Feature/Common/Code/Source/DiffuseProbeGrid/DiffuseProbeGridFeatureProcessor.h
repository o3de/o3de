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

#include <Atom/Feature/DiffuseProbeGrid/DiffuseProbeGridFeatureProcessorInterface.h>
#include <DiffuseProbeGrid/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        //! This class manages DiffuseProbeGrids which generate diffuse global illumination
        class DiffuseProbeGridFeatureProcessor final
            : public DiffuseProbeGridFeatureProcessorInterface
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

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // retrieve the full list of diffuse probe grids
            using DiffuseProbeGridVector = AZStd::vector<AZStd::shared_ptr<DiffuseProbeGrid>>;
            DiffuseProbeGridVector& GetProbeGrids() { return m_diffuseProbeGrids; }

        private:
            AZ_DISABLE_COPY_MOVE(DiffuseProbeGridFeatureProcessor);

            // create the box vertex and index streams, which are used to render the probe volumes
            void CreateBoxMesh();

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;

            void UpdatePipelineStates();
            void UpdatePasses();

            // list of diffuse probe grids
            const size_t InitialProbeGridAllocationSize = 64;
            DiffuseProbeGridVector m_diffuseProbeGrids;

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
        };
    } // namespace Render
} // namespace AZ
