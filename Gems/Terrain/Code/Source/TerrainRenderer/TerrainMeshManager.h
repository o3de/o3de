/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>

#include <AtomCore/Instance/Instance.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

namespace AZ::RPI
{
    class Scene;
}

namespace AZ::RHI
{
    struct BufferViewDescriptor;
}

namespace Terrain
{

    struct MeshConfiguration
    {
        AZ_CLASS_ALLOCATOR(MeshConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(MeshConfiguration, "{D94D831B-67C0-46C5-9707-AACD2716A2C0}");

        MeshConfiguration() = default;
        virtual ~MeshConfiguration() = default;

        float m_renderDistance = 4096.0f;
        float m_firstLodDistance = 128.0f;
        bool m_clodEnabled = true;
        float m_clodDistance = 16.0f;

        bool operator==(const MeshConfiguration& other) const
        {
            return m_renderDistance == other.m_renderDistance
                && m_firstLodDistance == other.m_firstLodDistance
                && m_clodEnabled == other.m_clodEnabled
                && m_clodDistance == other.m_clodDistance
                ;
        }

        bool CheckWouldRequireRebuild(const MeshConfiguration& other) const
        {
            return !(m_renderDistance == other.m_renderDistance
                && m_firstLodDistance == other.m_firstLodDistance
                && m_clodEnabled == other.m_clodEnabled
                );
        }

        bool operator!=(const MeshConfiguration& other) const
        {
            return !(other == *this);
        }

        bool IsClodDisabled() // Since the edit context attribute is "ReadOnly" instead of "Enabled", the logic needs to be reversed.
        {
            return !m_clodEnabled;
        }

    };

    class TerrainMeshManager
        : private AzFramework::Terrain::TerrainDataNotificationBus::Handler
        , private AZ::RPI::SceneNotificationBus::Handler
    {
    private:
        
        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;

    public:

        AZ_RTTI(TerrainMeshManager, "{62C84AD8-05FE-4C78-8501-A2DB6731B9B7}");
        AZ_DISABLE_COPY_MOVE(TerrainMeshManager);

        TerrainMeshManager();
        ~TerrainMeshManager();

        void Initialize(AZ::RPI::Scene& parentScene);
        void SetConfiguration(const MeshConfiguration& config);
        bool IsInitialized() const;
        void Reset();
        void SetMaterial(MaterialInstance materialInstance);

        void Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

        void DrawMeshes(const AZ::RPI::FeatureProcessor::RenderPacket& process, const AZ::RPI::ViewPtr mainView);

        static constexpr int32_t GridSizeExponent = 6; // 2^6 = 64
        static constexpr int32_t GridSize{ 1 << GridSizeExponent }; // number of terrain quads (vertices are m_gridSize + 1)
        static constexpr int32_t GridVerts1D = GridSize + 1;

    private:
        
        struct VertexPosition
        {
            float m_posx;
            float m_posy;
        };

        struct PatchData
        {
            AZStd::vector<VertexPosition> m_xyPositions;
            AZStd::vector<float> m_heights;
            AZStd::vector<uint16_t> m_indices;
        };

        struct StackSectorData
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg;
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            int32_t m_worldX = AZStd::numeric_limits<int32_t>::max();
            int32_t m_worldY = AZStd::numeric_limits<int32_t>::max();

            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_rhiDrawPacket;
            AZ::Data::Instance<AZ::RPI::Buffer> m_heightsBuffer;
            AZ::Data::Instance<AZ::RPI::Buffer> m_normalsBuffer;
            AZ::Data::Instance<AZ::RPI::Buffer> m_lodHeightsBuffer;
            AZ::Data::Instance<AZ::RPI::Buffer> m_lodNormalsBuffer;
            AZStd::array<AZ::RHI::StreamBufferView, 5> m_streamBufferViews;

            // Hold reference to the draw srgs so they don't get released.
            AZStd::fixed_vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>, AZ::RHI::DrawPacketBuilder::DrawItemCountMax> m_perDrawSrgs;

            bool m_hasData = false;
        };

        struct StackData
        {
            AZStd::vector<StackSectorData> m_sectors;

            // The world space sector coord of the top most left item
            int32_t m_startCoordX = AZStd::numeric_limits<int32_t>::max();
            int32_t m_startCoordY = AZStd::numeric_limits<int32_t>::max();
        };

        struct ShaderObjectData // Must align with struct in object srg
        {
            AZStd::array<float, 2> m_xyTranslation{ 0.0f, 0.0f };
            float m_xyScale{ 1.0f };
            uint32_t m_lodLevel{ 0 };
            float m_rcpLodLevel{ 1.0f };
        };

        struct ShaderMeshData
        {
            AZStd::array<float, 3> m_mainCameraPosition{ 0.0f, 0.0f, 0.0f };
            float m_firstLodDistance;
            float m_rcpClodDistance;
        };

        struct SectorUpdateContext
        {
            uint32_t m_lodLevel;
            StackSectorData* m_sector;
        };

        struct SectorDataRequest
        {
            AZ::Vector2 m_worldStartPosition;
            float m_vertexSpacing;
            int16_t m_samplesX;
            int16_t m_samplesY;
            AzFramework::Terrain::TerrainDataRequests::Sampler m_samplerType =
                AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP;
        };

        struct CachedDrawData
        {
            AZ::Data::Instance<AZ::RPI::Shader> m_shader;
            AZ::RPI::ShaderOptionGroup m_shaderOptions;
            const AZ::RHI::PipelineState* m_pipelineState;
            AZ::RHI::DrawListTag m_drawListTag;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_drawSrgLayout;
            AZ::RPI::ShaderVariant m_shaderVariant;
        };

        using HeightDataType = uint16_t;
        using NormalDataType = AZStd::pair<int16_t, int16_t>;
        static constexpr AZ::RHI::Format XYPositionFormat = AZ::RHI::Format::R32G32_FLOAT;
        static constexpr AZ::RHI::Format HeightFormat = AZ::RHI::Format::R16_UNORM;
        static constexpr AZ::RHI::Format NormalFormat = AZ::RHI::Format::R16G16_SNORM;
        static constexpr uint32_t RayTracingQuads1D = 200;
        static constexpr HeightDataType NoTerrainVertexHeight = AZStd::numeric_limits<HeightDataType>::max();

        // AZ::RPI::SceneNotificationBus overrides...
        void OnRenderPipelineAdded(AZ::RPI::RenderPipelinePtr pipeline) override;
        void OnRenderPipelinePassesChanged(AZ::RPI::RenderPipeline* renderPipeline) override;

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataCreateEnd() override;
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        void BuildDrawPacket(StackSectorData& sector);
        void RebuildSectors();
        void RebuildDrawPackets();
        AZ::RHI::StreamBufferView CreateStreamBufferView(AZ::Data::Instance<AZ::RPI::Buffer>& buffer);

        void InitializeTerrainPatch(uint16_t gridSize, PatchData& patchdata);
        void InitializeCommonSectorData();
        AZ::Data::Instance<AZ::RPI::Buffer> CreateMeshBufferInstance(AZ::RHI::Format format, uint64_t elementCount, const void* initialData = nullptr, const char* name = nullptr);
        void UpdateSectorBuffers(StackSectorData& sector, const AZStd::span<const HeightDataType> heights, const AZStd::span<const NormalDataType> normals);
        void UpdateSectorLodBuffers(StackSectorData& sector,
            const AZStd::span<const HeightDataType> originalHeights, const AZStd::span<const NormalDataType> originalNormals,
            const AZStd::span<const HeightDataType> lodHeights, const AZStd::span<const NormalDataType> lodNormals);
        void GatherMeshData(SectorDataRequest request, AZStd::vector<HeightDataType>& meshHeights, AZStd::vector<NormalDataType>& meshNormals, AZ::Aabb& meshAabb, bool& terrainExistsAnywhere);

        void CheckStacksForUpdate(AZ::Vector3 newPosition);
        void ProcessSectorUpdates(AZStd::span<SectorUpdateContext> sectorUpdates);
        void UpdateRaytracingData(const AZ::Aabb& bounds);

        template<typename Callback>
        void ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback);

        MeshConfiguration m_config;
        AZ::RPI::Scene* m_parentScene;

        MaterialInstance m_materialInstance;
        AZStd::vector<CachedDrawData> m_cachedDrawData; // Holds common parts of draw packets

        AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;

        AZ::RHI::ShaderInputNameIndex m_srgMeshDataIndex = "m_meshData";
        AZ::RHI::ShaderInputNameIndex m_patchDataIndex = "m_patchData";

        AZ::Data::Instance<AZ::RPI::Buffer> m_xyPositionsBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_indexBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_dummyLodHeightsBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_dummyLodNormalsBuffer;
        AZ::RHI::IndexBufferView m_indexBufferView;

        // Currently ray tracing meshes are kept separate from the regular meshes. The intention is to
        // combine them in the future to support terrain lods in ray tracing.
        AZ::Data::Instance<AZ::RPI::Buffer> m_raytracingPositionsBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_raytracingNormalsBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_raytracingIndexBuffer;

        AZStd::vector<StackData> m_sectorStack;
        uint32_t m_1dSectorCount = 0;

        AZ::Vector3 m_previousCameraPosition = AZ::Vector3::CreateZero();

        AZ::Aabb m_worldBounds{ AZ::Aabb::CreateNull() };
        float m_sampleSpacing = 1.0f;
        AZ::RPI::Material::ChangeId m_lastMaterialChangeId;

        bool m_isInitialized{ false };
        bool m_rebuildSectors{ true };
        bool m_rebuildDrawPackets{ false };

    };
}
