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
#include <Atom/RPI.Public/Base.h>

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/SparseVector.h>

#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <TerrainRenderer/Vector2i.h>
#include <TerrainRenderer/ClipmapBounds.h>

namespace Terrain
{
    class TerrainMacroMaterialManager
        : private TerrainMacroMaterialNotificationBus::Handler
    {
    public:
        
        TerrainMacroMaterialManager() = default;
        ~TerrainMacroMaterialManager() = default;

        void Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        void Reset();
        bool IsInitialized();
        bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        void SetRenderDistance(float distance);

        void Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

    private:

        static constexpr auto InvalidImageIndex = AZStd::numeric_limits<uint32_t>::max();
        static constexpr float MacroMaterialGridSize = 64.0f;
        static constexpr uint16_t MacroMaterialsPerTile = 4;

        using MaterialHandle = AZ::RHI::Handle<uint16_t, class Material>;
        using TileHandle = AZ::RHI::Handle<uint32_t, class Tile>;
        using TileMaterials = AZStd::array<MaterialHandle, MacroMaterialsPerTile>;

        static constexpr TileMaterials DefaultTileMaterials
        {
            MaterialHandle(), MaterialHandle(), MaterialHandle(), MaterialHandle()
        };

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

            bool Overlaps(const AZ::Vector2& min, const AZ::Vector2& max) const;

        };
        static_assert(sizeof(MacroMaterialShaderData) % 16 == 0, "MacroMaterialShaderData must be 16 byte aligned.");

        struct MacroMaterialPriority
        {
            int32_t m_priority{ 0 };
            uint32_t m_hash{ 0 };

            bool operator>(MacroMaterialPriority& other) const
            {
                return m_priority > other.m_priority || (m_priority == other.m_priority && m_hash > other.m_hash);
            }
        };

        struct alignas(16) MacroMaterialGridShaderData
        {
            uint32_t m_tileCount1D; // How many x/y tiles along edge of grid. Grid is always square, so total number of tiles is this value squared.
            float m_tileSize; // Size of a tile in meters.
        };

        // TerrainMacroMaterialNotificationBus overrides...
        void OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId) override;
        
        void UpdateMacroMaterialShaderEntry(MaterialHandle materialHandle, const MacroMaterialData& macroMaterialData);
        void AddMacroMaterialToTile(MaterialHandle materialHandle, TileHandle tileHandle);
        void RemoveMacroMaterialFromTile(MaterialHandle materialHandle, TileHandle tileHandle, const AZ::Vector2& tileMin);

        template<typename Callback>
        void ForMacroMaterialsInRegion(const ClipmapBoundsRegion& region, Callback callback);

        template<typename Callback>
        void ForMacroMaterialsInBounds(const AZ::Vector2& minBounds, const AZ::Vector2& maxBounds, Callback callback);
        template<typename Callback>
        void ForMacroMaterialsInBounds(const AZ::Aabb& bounds, Callback callback);
        template<typename Callback>
        void ForMacroMaterialsInBounds(const AZStd::array<float, 2>& minBounds, const AZStd::array<float, 2>& maxBounds, Callback callback);

        void RemoveAllImages();
        void RemoveImagesForMaterial(MaterialHandle materialHandle);

        // Macro materials stored in a grid of (MacroMaterialGridCount * MacroMaterialGridCount) where each tile in the grid covers
        // an area of (MacroMaterialGridSize * MacroMaterialGridSize) and each tile can hold MacroMaterialsPerTile macro materials
        AZ::Render::SparseVector<MacroMaterialPriority> m_materialPriorityData; // Priority info about the macro material itself.
        AZStd::unordered_map<int, AZStd::vector<MacroMaterialShaderData>> m_materialShaderData; // Shader info about the macro material itself.
        AZStd::vector<TileMaterials> m_materialRefGridShaderData; // A grid of macro material references that covers the world.

        AZStd::map<AZ::EntityId, MaterialHandle> m_entityToMaterialHandle; // Used for looking up macro materials by entity id when the data isn't provided by a bus.

        int32_t m_tiles1D{ 0 };
        ClipmapBounds m_macroMaterialTileBounds;

        AZ::Render::GpuBufferHandler m_materialDataBuffer;
        AZ::Render::GpuBufferHandler m_materialRefGridDataBuffer;

        AZ::RHI::ShaderInputNameIndex m_macroMaterialGridIndex = "m_macroMaterialGridInfo";

        bool m_terrainSizeChanged{ false };
        bool m_bufferNeedsUpdate{ false };
        bool m_isInitialized{ false };

    };
}
