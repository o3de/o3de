/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>

namespace AZ::RPI
{
    namespace AssetUtils
    {
        class AsyncAssetLoader;
    }
    class Material;
    class Model;
    class StreamingImage;
}

namespace Terrain
{
    class TerrainFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
        , private AZ::RPI::MaterialReloadNotificationBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
        , private TerrainMacroMaterialNotificationBus::Handler
    {
    public:
        AZ_RTTI(TerrainFeatureProcessor, "{D7DAC1F9-4A9F-4D3C-80AE-99579BF8AB1C}", AZ::RPI::FeatureProcessor);
        AZ_DISABLE_COPY_MOVE(TerrainFeatureProcessor);
        AZ_FEATURE_PROCESSOR(TerrainFeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        TerrainFeatureProcessor() = default;
        ~TerrainFeatureProcessor() = default;

        // AZ::RPI::FeatureProcessor overrides...
        void Activate() override;
        void Deactivate() override;
        void Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet) override;

        void SetWorldSize(AZ::Vector2 sizeInMeters);

    private:
        
        using MaterialInstance = AZ::Data::Instance<AZ::RPI::Material>;
        static constexpr uint32_t MaxMaterialsPerSector = 4;

        enum MacroMaterialFlags
        {
            ColorImageUsed = 0b01,
            NormalImageUsed = 0b10,
        };

        struct ShaderTerrainData // Must align with struct in Object Srg
        {
            AZStd::array<float, 2> m_uvMin;
            AZStd::array<float, 2> m_uvMax;
            AZStd::array<float, 2> m_uvStep;
            float m_sampleSpacing;
            float m_heightScale;
        };

        struct ShaderMacroMaterialData
        {
            AZStd::array<float, 2> m_uvMin;
            AZStd::array<float, 2> m_uvMax;
            float m_normalFactor;
            uint32_t m_flipNormalX{ 0 }; // bool in shader
            uint32_t m_flipNormalY{ 0 }; // bool in shader
            uint32_t m_mapsInUse{ 0b00 }; // 0b01 = color, 0b10 = normal
        };

        struct VertexPosition
        {
            float m_posx;
            float m_posy;
        };

        struct VertexUv
        {
            float m_u;
            float m_v;
        };

        struct PatchData
        {
            AZStd::vector<VertexPosition> m_positions;
            AZStd::vector<VertexUv> m_uvs;
            AZStd::vector<uint16_t> m_indices;
        };
        
        struct SectorData
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_srg; // Hold on to ref so it's not dropped
            AZ::Aabb m_aabb;
            AZStd::fixed_vector<AZ::RPI::MeshDrawPacket, AZ::RPI::ModelLodAsset::LodCountMax> m_drawPackets;
            AZStd::fixed_vector<uint16_t, MaxMaterialsPerSector> m_macroMaterials;
        };

        // AZ::RPI::MaterialReloadNotificationBus::Handler overrides...
        void OnMaterialReinitialized(const MaterialInstance& material) override;

        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataDestroyBegin() override;
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask) override;

        // TerrainMacroMaterialNotificationBus overrides...
        void OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId) override;

        void Initialize();
        void InitializeTerrainPatch(uint16_t gridSize, float gridSpacing, PatchData& patchdata);
        bool InitializePatchModel();

        void UpdateTerrainData();
        void PrepareMaterialData();
        void UpdateMacroMaterialData(MacroMaterialData& macroMaterialData, const MacroMaterialData& newMaterialData);

        void ProcessSurfaces(const FeatureProcessor::RenderPacket& process);
        
        MacroMaterialData* FindMacroMaterial(AZ::EntityId entityId);
        MacroMaterialData& FindOrCreateMacroMaterial(AZ::EntityId entityId);
        void RemoveMacroMaterial(AZ::EntityId entityId);

        template<typename Callback>
        void ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback);

        AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> CreateBufferAsset(
            const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName);

        // System-level parameters
        static constexpr float GridSpacing{ 1.0f };
        static constexpr uint32_t GridSize{ 64 }; // number of terrain quads (vertices are m_gridSize + 1)
        static constexpr float GridMeters{ GridSpacing * GridSize };

        AZStd::unique_ptr<AZ::RPI::AssetUtils::AsyncAssetLoader> m_materialAssetLoader;
        MaterialInstance m_materialInstance;

        AZ::RHI::ShaderInputConstantIndex m_modelToWorldIndex;
        AZ::RHI::ShaderInputConstantIndex m_terrainDataIndex;
        AZ::RHI::ShaderInputConstantIndex m_macroMaterialDataIndex;
        AZ::RHI::ShaderInputConstantIndex m_macroMaterialCountIndex;
        AZ::RHI::ShaderInputImageIndex m_macroColorMapIndex;
        AZ::RHI::ShaderInputImageIndex m_macroNormalMapIndex;
        AZ::RPI::MaterialPropertyIndex m_heightmapPropertyIndex;

        AZ::Data::Instance<AZ::RPI::Model> m_patchModel;

        // Per-area data
        struct TerrainAreaData
        {
            AZ::Transform m_transform{ AZ::Transform::CreateIdentity() };
            AZ::Aabb m_terrainBounds{ AZ::Aabb::CreateNull() };
            AZ::Data::Instance<AZ::RPI::AttachmentImage> m_heightmapImage;
            uint32_t m_heightmapImageWidth{ 0 };
            uint32_t m_heightmapImageHeight{ 0 };
            uint32_t m_updateWidth{ 0 };
            uint32_t m_updateHeight{ 0 };
            float m_sampleSpacing{ 0.0f };
            bool m_heightmapUpdated{ true };
            bool m_macroMaterialsUpdated{ true };
            bool m_rebuildSectors{ true };
        };

        TerrainAreaData m_areaData;
        AZ::Aabb m_dirtyRegion{ AZ::Aabb::CreateNull() };

        AZStd::vector<SectorData> m_sectorData;

        AZ::Render::IndexedDataVector<MacroMaterialData> m_macroMaterials;
    };
}
