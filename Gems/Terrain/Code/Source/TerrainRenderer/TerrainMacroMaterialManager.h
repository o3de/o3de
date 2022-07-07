/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <TerrainRenderer/BindlessImageArrayHandler.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <Atom/Feature/Utils/SparseVector.h>

namespace Terrain
{
    class TerrainMacroMaterialManager
        : private TerrainMacroMaterialNotificationBus::Handler
        , private AzFramework::Terrain::TerrainDataNotificationBus::Handler
    {
    public:
        
        TerrainMacroMaterialManager() = default;
        ~TerrainMacroMaterialManager() = default;

        void Initialize(
            const AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler>& bindlessImageHandler,
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        void Reset();
        bool IsInitialized();
        bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        
        void Update(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

    private:

        static constexpr auto InvalidImageIndex = AZ::Render::BindlessImageArrayHandler::InvalidImageIndex;
        static constexpr float MacroMaterialGridSize = 64.0f;
        static constexpr uint16_t MacroMaterialsPerTile = 4;
        static constexpr uint16_t InvalidMacroMaterialRef = AZStd::numeric_limits<uint16_t>::max();

        using MacroMaterialRefs = AZStd::array<uint16_t, MacroMaterialsPerTile>;

        enum MacroMaterialShaderFlags : uint32_t
        {
            FlipMacroNormalX = 0b0000'0000'0000'0000'0000'0000'0000'0001,
            FlipMacroNormalY = 0b0000'0000'0000'0000'0000'0000'0000'0010,
        };

        struct MacroMaterialShaderData
        {
            MacroMaterialShaderFlags m_flags;
            uint32_t m_colorMapId{InvalidImageIndex};
            uint32_t m_normalMapId{InvalidImageIndex};
            float m_normalFactor;

            // macro material bounds in world space
            AZStd::array<float, 2> m_boundsMin{ 0.0f, 0.0f };
            AZStd::array<float, 2> m_boundsMax{ 0.0f, 0.0f };
        };
        static_assert(sizeof(MacroMaterialShaderData) % 16 == 0, "MacroMaterialShaderData must be 16 byte aligned.");

        struct MacroMaterial
        {
            MacroMaterialData m_data;
            uint16_t m_colorIndex{ InvalidImageIndex };
            uint16_t m_normalIndex{ InvalidImageIndex };
            uint16_t m_materialRef{ InvalidMacroMaterialRef };
        };

        struct MacroMaterialGridShaderData
        {
            uint32_t m_resolution; // How many x/y tiles in grid. x & y stored in 16 bits each. Total number of entries in m_macroMaterialData will be x * y
            float m_tileSize; // Size of a tile in meters.
            AZStd::array<float, 2> m_offset; // x/y offset of min x/y corner of grid.
        };
        static_assert(sizeof(MacroMaterialGridShaderData) % 16 == 0, "MacroMaterialGridShaderData must be 16 byte aligned.");
        
        // AzFramework::Terrain::TerrainDataNotificationBus overrides...
        void OnTerrainDataChanged(const AZ::Aabb& dirtyRegion [[maybe_unused]], TerrainDataChangedMask dataChangedMask) override;

        // TerrainMacroMaterialNotificationBus overrides...
        void OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId) override;
        
        void UpdateMacroMaterialShaderEntry(MacroMaterialShaderData& shaderData, const MacroMaterial& macroMaterialData);
        void RemoveMacroMaterialShaderEntry(uint16_t shaderDataIdx, MacroMaterialRefs& materialRefs);

        template<typename Callback>
        void ForMacroMaterialsInBounds(const AZ::Aabb& bounds, Callback callback);

        void RemoveAllImages();
        void RemoveImagesForMaterial(const MacroMaterial& macroMaterial);

        AZ::Aabb m_terrainBounds{ AZ::Aabb::CreateNull() };

        // Macro materials stored in a grid of (MacroMaterialGridCount * MacroMaterialGridCount) where each tile in the grid covers
        // an area of (MacroMaterialGridSize * MacroMaterialGridSize) and each tile can hold MacroMaterialsPerTile macro materials
        AZ::Render::SparseVector<MacroMaterialShaderData> m_materialShaderData; // Info about the macro material itself.
        AZStd::vector<MacroMaterialRefs> m_materialRefGridShaderData; // A grid of macro material references that covers the world.

        AZStd::map<AZ::EntityId, MacroMaterial> m_macroMaterials; // Used for looking up macro materials by entity id when the data isn't provided by a bus.
        uint16_t m_tilesX{ 0 };
        uint16_t m_tilesY{ 0 };

        AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler> m_bindlessImageHandler;
        AZ::Render::GpuBufferHandler m_materialDataBuffer;
        AZ::Render::GpuBufferHandler m_materialRefGridDataBuffer;

        AZ::RHI::ShaderInputNameIndex m_macroMaterialGridIndex = "m_macroMaterialGridInfo";

        bool m_terrainSizeChanged{ false };
        bool m_bufferNeedsUpdate{ false };
        bool m_isInitialized{ false };

    };
}
