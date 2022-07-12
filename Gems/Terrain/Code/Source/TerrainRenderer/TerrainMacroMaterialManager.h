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

#include <TerrainRenderer/BindlessImageArrayHandler.h>
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

        void Initialize(
            const AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler>& bindlessImageHandler,
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        void Reset();
        bool IsInitialized();
        bool UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);
        void SetRenderDistance(float distance);

        void Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg);

    private:

        static constexpr auto InvalidImageIndex = AZ::Render::BindlessImageArrayHandler::InvalidImageIndex;
        static constexpr float MacroMaterialGridSize = 64.0f;
        static constexpr uint16_t MacroMaterialsPerTile = 4;
        static constexpr uint16_t InvalidMacroMaterialRef = AZStd::numeric_limits<uint16_t>::max();

        using MacroMaterialRefs = AZStd::array<uint16_t, MacroMaterialsPerTile>;

        static constexpr MacroMaterialRefs DefaultRefs
        {
            InvalidMacroMaterialRef, InvalidMacroMaterialRef, InvalidMacroMaterialRef, InvalidMacroMaterialRef
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
        };
        static_assert(sizeof(MacroMaterialShaderData) % 16 == 0, "MacroMaterialShaderData must be 16 byte aligned.");

        struct MacroMaterialData2D
        {
            MacroMaterialData2D() = default;
            MacroMaterialData2D(const MacroMaterialData& data);

            bool Overlaps(const AZ::Vector2& min, const AZ::Vector2& max);

            AZ::Vector2 m_minBounds{ AZ::Vector2::CreateZero() };
            AZ::Vector2 m_maxBounds{ AZ::Vector2::CreateZero() };

            bool m_normalFlipX{ false };
            bool m_normalFlipY{ false };
            float m_normalFactor{ 0.0f };
        };

        struct MacroMaterial
        {
            MacroMaterialData2D m_data;
            uint16_t m_colorIndex{ InvalidImageIndex };
            uint16_t m_normalIndex{ InvalidImageIndex };
            uint16_t m_materialRef{ InvalidMacroMaterialRef };
        };

        struct MacroMaterialGridShaderData
        {
            uint32_t m_tileCount1D; // How many x/y tiles along edge of grid. Grid is always square, so total number of tiles is this value squared.
            float m_tileSize; // Size of a tile in meters.
        };

        // TerrainMacroMaterialNotificationBus overrides...
        void OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& material) override;
        void OnTerrainMacroMaterialRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId) override;
        
        void UpdateMacroMaterialShaderEntry(MacroMaterialShaderData& shaderData, const MacroMaterial& macroMaterialData);
        void RemoveMacroMaterialShaderEntry(uint16_t shaderDataIdx, MacroMaterialRefs& materialRefs);

        template<typename Callback>
        void ForMacroMaterialsInRegion(const ClipmapBoundsRegion& region, Callback callback);
        template<typename Callback>
        void ForMacroMaterialsInBounds(const AZ::Vector2& minBounds, const AZ::Vector2& maxBounds, Callback callback);

        void RemoveAllImages();
        void RemoveImagesForMaterial(const MacroMaterial& macroMaterial);

        // Macro materials stored in a grid of (MacroMaterialGridCount * MacroMaterialGridCount) where each tile in the grid covers
        // an area of (MacroMaterialGridSize * MacroMaterialGridSize) and each tile can hold MacroMaterialsPerTile macro materials
        AZ::Render::SparseVector<MacroMaterialShaderData> m_materialShaderData; // Info about the macro material itself.
        AZStd::vector<MacroMaterialRefs> m_materialRefGridShaderData; // A grid of macro material references that covers the world.

        AZStd::map<AZ::EntityId, MacroMaterial> m_macroMaterials; // Used for looking up macro materials by entity id when the data isn't provided by a bus.

        int32_t m_tiles1D{ 0 };
        ClipmapBounds m_macroMaterialTileBounds;

        AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler> m_bindlessImageHandler;
        AZ::Render::GpuBufferHandler m_materialDataBuffer;
        AZ::Render::GpuBufferHandler m_materialRefGridDataBuffer;

        AZ::RHI::ShaderInputNameIndex m_macroMaterialGridIndex = "m_macroMaterialGridInfo";

        bool m_terrainSizeChanged{ false };
        bool m_bufferNeedsUpdate{ false };
        bool m_isInitialized{ false };

    };
}
