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

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

namespace AZ::RPI
{
    class BufferAsset;
    class ModelAssetCreator;
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

        bool operator==(const MeshConfiguration& other) const
        {
            return m_renderDistance == other.m_renderDistance &&
                m_firstLodDistance == other.m_firstLodDistance;
        }

        bool operator!=(const MeshConfiguration& other) const
        {
            return !(other == *this);
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

        static constexpr int32_t GridSize{ 64 }; // number of terrain quads (vertices are m_gridSize + 1)

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
            AZ::Data::Instance<AZ::RPI::Model> m_model;
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg;
            AZ::RPI::MeshDrawPacket m_drawPacket;
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            int32_t m_worldX = AZStd::numeric_limits<int32_t>::max();
            int32_t m_worldY = AZStd::numeric_limits<int32_t>::max();
        };

        struct StackData
        {
            AZStd::vector<StackSectorData> m_sectors;

            // The world space sector coord of the top most left item
            int32_t m_startCoordX = 0;
            int32_t m_startCoordY = 0;
        };

        struct ShaderObjectData // Must align with struct in Object Srg
        {
            AZStd::array<float, 2> m_xyTranslation{ 0.0f, 0.0f };
            float m_xyScale{ 1.0f };
            uint32_t m_lodLevel{ 0 };
        };

        struct ShaderMeshData
        {
            AZStd::array<float, 3> m_mainCameraPosition{ 0.0f, 0.0f, 0.0f };
            float m_firstLodDistance;
        };

        struct SectorUpdateContext
        {
            uint32_t m_lodLevel;
            StackSectorData* m_sector;
        };

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataCreateEnd() override;
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        void RebuildSectors();
        void RebuildDrawPackets();

        AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> CreateBufferAsset(
            const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName);

        void InitializeTerrainPatch(uint16_t gridSize, PatchData& patchdata);
        bool CreateLod(AZ::RPI::ModelAssetCreator& modelAssetCreator, const AZ::RPI::BufferAssetView& zPositions, const AZ::RPI::BufferAssetView& normals);
        bool InitializeCommonSectorData();
        bool InitializeDefaultSectorModel();
        AZ::Data::Instance<AZ::RPI::Model> InitializeSectorModel(uint16_t gridSize, const AZ::Vector2& worldStartPosition, float vertexSpacing, AZ::Aabb& modelAabb);
        AZ::Data::Instance<AZ::RPI::Model> InitializeSectorModel(const AZ::RPI::BufferAssetView& heights, const AZ::RPI::BufferAssetView& normals);

        void CheckStacksForUpdate(AZ::Vector3 newPosition);
        void ProcessSectorUpdates(AZStd::span<SectorUpdateContext> sectorUpdates);

        template<typename Callback>
        void ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback);

        MeshConfiguration m_config;
        MaterialInstance m_materialInstance;
        AZ::RPI::Scene* m_parentScene;

        AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler m_handleGlobalShaderOptionUpdate;

        AZ::RHI::ShaderInputNameIndex m_srgMeshDataIndex = "m_meshData";
        AZ::RHI::ShaderInputNameIndex m_patchDataIndex = "m_patchData";

        AZ::Data::Instance<AZ::RPI::Model> m_defaultSectorModel;
        AZ::RPI::BufferAssetView m_sectorXyPositionsBufferAssetView;
        AZ::RPI::BufferAssetView m_sectorZPositionsBufferAssetView; // common buffer of 0's for when Z positions are derived from textures instead of baked into the mesh.
        AZ::RPI::BufferAssetView m_sectorNormalsBufferAssetView; // common buffer of 0's for when normals are derived from textures instead of baked into the mesh.
        AZ::RPI::BufferAssetView m_sectorIndicesBufferAssetView;

        AZStd::vector<StackData> m_sectorStack;
        uint32_t m_1dSectorCount = 0;

        AZ::Vector3 m_previousCameraPosition = AZ::Vector3::CreateZero();

        AZ::Aabb m_worldBounds{ AZ::Aabb::CreateNull() };
        float m_sampleSpacing = 1.0f;
        AZ::RPI::Material::ChangeId m_lastMaterialChangeId;

        bool m_isInitialized{ false };
        bool m_rebuildSectors{ true };
        bool m_forceRebuildDrawPackets{ false };

    };
}
