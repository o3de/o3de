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

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>

#include <TerrainRenderer/Vector2i.h>

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
        AZ_CLASS_ALLOCATOR(MeshConfiguration, AZ::SystemAllocator);
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

        void SetRebuildDrawPackets();

    private:

        using HeightDataType = uint16_t;
        using NormalDataType = int8_t;
        using NormalXYDataType = AZStd::pair<NormalDataType, NormalDataType>;

        static constexpr AZ::RHI::Format XYPositionFormat = AZ::RHI::Format::R8G8_UNORM;
        static constexpr AZ::RHI::Format HeightFormat = AZ::RHI::Format::R16_UNORM;
        static constexpr AZ::RHI::Format NormalFormat = AZ::RHI::Format::R8G8_SNORM;
        static constexpr uint32_t RayTracingQuads1D = 200;
        static constexpr HeightDataType NoTerrainVertexHeight = AZStd::numeric_limits<HeightDataType>::max();

        enum StreamIndex : uint32_t
        {
            XYPositions,
            Heights,
            Normals,
            LodHeights,
            LodNormals,

            Count,
        };

        struct RtSector
        {
            // Ray tracing structures - Currently no data is shared due to ray tracing's format requirements, in the future this could
            // be de-duplicated with custom ray tracing shaders.
            AZ::Data::Instance<AZ::RPI::Buffer> m_positionsBuffer;
            AZ::Data::Instance<AZ::RPI::Buffer> m_normalsBuffer;

            struct MeshGroup
            {
                AZ::Uuid m_id { AZ::Uuid::CreateRandom() };
                AZ::Render::RayTracingFeatureProcessorInterface::Mesh m_mesh;
                AZ::Render::RayTracingFeatureProcessorInterface::SubMeshVector m_submeshVector;
                bool m_isVisible = false;
            };

            AZStd::array<MeshGroup, 5> m_meshGroups; // 0 is the primary, 1-4 are the quadrants.
            
        };

        struct Sector
        {
            AZ::RHI::GeometryView m_geometryView;
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg;
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::array<AZ::Aabb, 4> m_quadrantAabbs;
            Vector2i m_worldCoord = AZStd::numeric_limits<int32_t>::max();

            // When drawing, either the m_rhiDrawPacket will be used, or some number of the m_rhiDrawPacketQuadrants
            AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_rhiDrawPacket;
            AZStd::array<AZ::RHI::ConstPtr<AZ::RHI::DrawPacket>, 4> m_rhiDrawPacketQuadrant;
            AZStd::array<AZ::RHI::GeometryView, 4> m_quadrantGeometryViews;

            AZ::Data::Instance<AZ::RPI::Buffer> m_heightsNormalsBuffer;
            AZ::Data::Instance<AZ::RPI::Buffer> m_lodHeightsNormalsBuffer;

            // Hold reference to the draw srgs so they don't get released.
            AZStd::fixed_vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>, AZ::RHI::DrawPacketBuilder::DrawItemCountMax> m_perDrawSrgs;

            AZStd::unique_ptr<RtSector> m_rtData;

            bool m_hasData = false;
            bool m_isQueuedForSrgCompile = false;
        };

        struct SectorLodGrid
        {
            AZStd::vector<Sector> m_sectors;

            // The world space sector coord of the top most left item
            Vector2i m_startCoord = AZStd::numeric_limits<int32_t>::max();
        };

        struct ShaderObjectData // Must align with struct in object srg
        {
            AZStd::array<float, 2> m_xyTranslation{ 0.0f, 0.0f };
            float m_xyScale{ 1.0f };
            uint32_t m_lodLevel{ 0 };
            float m_rcpLodLevel{ 1.0f };
        };

        struct alignas(16) ShaderMeshData
        {
            AZStd::array<float, 3> m_mainCameraPosition{ 0.0f, 0.0f, 0.0f };
            float m_firstLodDistance;
            float m_rcpClodDistance;
            float m_rcpGridSize;
            float m_gridToQuadScale;
        };

        struct SectorDataRequest
        {
            AZ::Vector2 m_worldStartPosition;
            float m_vertexSpacing;
            int16_t m_samplesX;
            int16_t m_samplesY;
            AzFramework::Terrain::TerrainDataRequests::Sampler m_samplerType =
                AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP;
            bool m_useVertexOrderRemap = false;
        };

        struct CachedDrawData
        {
            AZ::Data::Instance<AZ::RPI::Shader> m_shader;
            AZ::RPI::ShaderOptionGroup m_shaderOptions;
            const AZ::RHI::PipelineState* m_pipelineState;
            AZ::RHI::DrawListTag m_drawListTag;
            AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_drawSrgLayout;
            AZ::RPI::ShaderVariant m_shaderVariant;
            AZ::Name m_materialPipelineName;
        };

        struct HeightNormalVertex
        {
            HeightDataType m_height;
            NormalXYDataType m_normal;
        };

        struct CandidateSector
        {
            AZ::Aabb m_aabb;
            const AZ::RHI::DrawPacket* m_rhiDrawPacket;
        };

        struct XYPosition
        {
            uint8_t m_posx;
            uint8_t m_posy;
        };

        struct RayTracedItem
        {
            Sector* m_sector;
            uint32_t m_meshGroupIndex;
            uint32_t m_lodLevel;
        };

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataCreateEnd() override;
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        void ClearSectorBuffers();
        bool UpdateGridSize(float distanceToFirstLod);
        void BuildDrawPacket(Sector& sector);
        void BuildRtSector(Sector& sector, uint32_t lodLevel);
        void RebuildSectors();
        void RebuildDrawPackets();
        void RemoveRayTracedMeshes();
        AZ::RHI::StreamBufferView CreateStreamBufferView(AZ::Data::Instance<AZ::RPI::Buffer>& buffer, uint32_t offset = 0);

        void CreateCommonBuffers();
        void UpdateSectorBuffers(Sector& sector, const AZStd::span<const HeightNormalVertex> heightsNormals);
        void UpdateSectorLodBuffers(Sector& sector,
            const AZStd::span<const HeightNormalVertex> originalHeightsNormals,
            const AZStd::span<const HeightNormalVertex> lodHeightsNormals);
        void GatherMeshData(SectorDataRequest request, AZStd::vector<HeightNormalVertex>& meshHeightsNormals, AZ::Aabb& meshAabb, bool& terrainExistsAnywhere);

        void CheckLodGridsForUpdate(AZ::Vector3 newPosition);
        void ProcessSectorUpdates(AZStd::vector<AZStd::vector<Sector*>>& sectorUpdates);

        AZ::Data::Instance<AZ::RPI::Buffer> CreateMeshBufferInstance(
            uint32_t elementSize,
            uint32_t elementCount,
            const void* initialData = nullptr,
            const char* name = nullptr);

        AZ::Data::Instance<AZ::RPI::Buffer> CreateRayTracingMeshBufferInstance(
            AZ::RHI::Format elementFormat,
            uint32_t elementCount,
            const void* initialData = nullptr,
            const char* name = nullptr);

        void UpdateCandidateSectors();
        void CreateAabbQuadrants(const AZ::Aabb& aabb, AZStd::span<AZ::Aabb, 4> quadrantAabb);

        template<typename Callback>
        void ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback);

        MeshConfiguration m_config;
        AZ::RPI::Scene* m_parentScene;
        AZ::Render::RayTracingFeatureProcessorInterface* m_rayTracingFeatureProcessor;

        MaterialInstance m_materialInstance;
        AZStd::vector<CachedDrawData> m_cachedDrawData; // Holds common parts of draw packets

        AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;

        AZ::RHI::ShaderInputNameIndex m_srgMeshDataIndex = "m_meshData";
        AZ::RHI::ShaderInputNameIndex m_patchDataIndex = "m_patchData";

        AZ::Data::Instance<AZ::RPI::Buffer> m_xyPositionsBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_indexBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_rtIndexBuffer;
        AZ::Data::Instance<AZ::RPI::Buffer> m_dummyLodHeightsNormalsBuffer;
        AZ::RHI::IndexBufferView m_indexBufferView;

        AZStd::vector<SectorLodGrid> m_sectorLods;
        AZStd::vector<CandidateSector> m_candidateSectors;
        AZStd::vector<Sector*> m_sectorsThatNeedSrgCompiled;
        uint32_t m_1dSectorCount = 0;

        // Sector x/y positions used to make it easier to calculate x/y positions for ray tracing meshes. This is particularly
        // relevant because the positions may have been reordered for vertex cache efficiency.
        AZStd::vector<XYPosition> m_xyPositions;

        // Set up the initial camera position impossible to force an update.
        AZ::Vector3 m_cameraPosition = AZ::Vector3::CreateAxisX(AZStd::numeric_limits<float>::max());

        AzFramework::Terrain::FloatRange m_worldHeightBounds;
        float m_sampleSpacing = 1.0f;
        AZ::RPI::Material::ChangeId m_lastMaterialChangeId;

        AZStd::vector<uint16_t> m_vertexOrder; // Maps from regular linear order to actual vertex order positions

        // Tracks which sectors are currently in the ray tracing system so they can be easily compared and updated each frame.
        AZStd::vector<RayTracedItem> m_rayTracedItems;

        uint8_t m_gridSize = 0; // number of quads in a single row of a sector
        uint8_t m_gridVerts1D = 0; // number of vertices along sector edge (m_gridSize + 1)
        uint16_t m_gridVerts2D = 0; // number of vertices in sector

        bool m_isInitialized{ false };
        bool m_rebuildSectors{ true };
        bool m_rebuildDrawPackets{ false };
        bool m_rayTracingEnabled{ false };


        AZ::RHI::Handle<uint32_t> m_meshMovedFlag;
    };
}
