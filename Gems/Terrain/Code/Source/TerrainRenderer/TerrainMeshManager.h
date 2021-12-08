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

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>


namespace AZ::RPI
{
    class BufferAsset;
    class Model;
}

namespace AZ::RHI
{
    struct BufferViewDescriptor;
}

namespace Terrain
{
    class TerrainMeshManager
        : private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    private:
        
        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;

    public:

        AZ_RTTI(TerrainMeshManager, "{62C84AD8-05FE-4C78-8501-A2DB6731B9B7}");
        AZ_DISABLE_COPY_MOVE(TerrainMeshManager);
        
        TerrainMeshManager() = default;
        ~TerrainMeshManager() = default;

        void Initialize();
        bool IsInitialized() const;
        void Reset();

        bool CheckRebuildSurfaces(MaterialInstance materialInstance, AZ::RPI::Scene& parentScene);
        void DrawMeshes(const AZ::RPI::FeatureProcessor::RenderPacket& process);
        void RebuildDrawPackets(AZ::RPI::Scene& scene);

        static constexpr float GridSpacing{ 1.0f };
        static constexpr int32_t GridSize{ 64 }; // number of terrain quads (vertices are m_gridSize + 1)
        static constexpr float GridMeters{ GridSpacing * GridSize };
        static constexpr uint32_t MaxMaterialsPerSector = 4;

    private:
        
        struct VertexPosition
        {
            float m_posx;
            float m_posy;
        };

        struct PatchData
        {
            AZStd::vector<VertexPosition> m_positions;
            AZStd::vector<uint16_t> m_indices;
        };
        
        struct SectorData
        {
            AZStd::fixed_vector<AZ::RPI::MeshDrawPacket, AZ::RPI::ModelLodAsset::LodCountMax> m_drawPackets;
            AZStd::fixed_vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>, AZ::RPI::ModelLodAsset::LodCountMax> m_srgs; // Hold on to refs so it's not dropped
            AZ::Aabb m_aabb;
        };
        
        struct ShaderTerrainData // Must align with struct in Object Srg
        {
            AZStd::array<float, 2> m_xyTranslation{ 0.0f, 0.0f };
            float m_xyScale{ 1.0f };
        };

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> CreateBufferAsset(
            const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName);

        void InitializeTerrainPatch(uint16_t gridSize, PatchData& patchdata);
        bool InitializePatchModel();
        
        template<typename Callback>
        void ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback);

        AZStd::vector<SectorData> m_sectorData;
        AZ::Data::Instance<AZ::RPI::Model> m_patchModel;
        
        AZ::Aabb m_worldBounds{ AZ::Aabb::CreateNull() };
        float m_sampleSpacing = 1.0f;

        bool m_isInitialized{ false };
        bool m_rebuildSectors{ true };

    };
}
