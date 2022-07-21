/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainMacroMaterialManager.h>
#include <Atom/RPI.Public/View.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainMacroMaterialManagerName = "TerrainMacroMaterialManager";
    }

    namespace TerrainSrgInputs
    {
        static const char* const MacroMaterialData("m_macroMaterialData");
        static const char* const MacroMaterialGridRefs("m_macroMaterialGridRefs");
    }

    bool TerrainMacroMaterialManager::MacroMaterialShaderData::Overlaps(const AZ::Vector2& min, const AZ::Vector2& max) const
    {
        return AZ::Vector2::CreateFromFloat2(m_boundsMin.data()).IsLessThan(max) &&
            AZ::Vector2::CreateFromFloat2(m_boundsMax.data()).IsGreaterThan(min);
    }

    void TerrainMacroMaterialManager::Initialize(
        const AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler>& bindlessImageHandler,
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        AZ_Error(TerrainMacroMaterialManagerName, bindlessImageHandler, "bindlessImageHandler must not be null.");
        AZ_Error(TerrainMacroMaterialManagerName, terrainSrg, "terrainSrg must not be null.");
        AZ_Error(TerrainMacroMaterialManagerName, !m_isInitialized, "Already initialized.");

        if (!bindlessImageHandler || !terrainSrg || m_isInitialized)
        {
            return;
        }
        
        if (UpdateSrgIndices(terrainSrg))
        {
            m_bindlessImageHandler = bindlessImageHandler;
            TerrainMacroMaterialNotificationBus::Handler::BusConnect();
            
            m_terrainSizeChanged = true;
            m_isInitialized = true;
        }
    }
    
    void TerrainMacroMaterialManager::Reset()
    {
        m_isInitialized = false;

        m_materialDataBuffer = {};
        m_materialRefGridDataBuffer = {};

        m_materialData.Clear();
        m_materialRefGridShaderData.clear();

        RemoveAllImages();
        m_entityToRef.clear();
        
        m_bindlessImageHandler = {};

        TerrainMacroMaterialNotificationBus::Handler::BusDisconnect();
    }
    
    bool TerrainMacroMaterialManager::IsInitialized()
    {
        return m_isInitialized;
    }
    
    bool TerrainMacroMaterialManager::UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        const AZ::RHI::ShaderResourceGroupLayout* terrainSrgLayout = terrainSrg->GetLayout();
        
        AZ::Render::GpuBufferHandler::Descriptor desc;
        desc.m_srgLayout = terrainSrgLayout;

        // Set up the gpu buffer for macro material data
        desc.m_bufferName = "Macro Material Data";
        desc.m_bufferSrgName = TerrainSrgInputs::MacroMaterialData;
        desc.m_elementSize = sizeof(MacroMaterialShaderData);
        m_materialDataBuffer = AZ::Render::GpuBufferHandler(desc);

        desc.m_bufferName = "Macro Material Ref Grid";
        desc.m_bufferSrgName = TerrainSrgInputs::MacroMaterialGridRefs;
        desc.m_elementSize = sizeof(MacroMaterialRefs);
        m_materialRefGridDataBuffer = AZ::Render::GpuBufferHandler(desc);

        m_bufferNeedsUpdate = true;

        return m_materialDataBuffer.IsValid() && m_materialRefGridDataBuffer.IsValid();
    }

    void TerrainMacroMaterialManager::OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        // If terrainSizeChanged, everything will rebuild later, so don't do any work here.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            !m_entityToRef.contains(entityId),
            "OnTerrainMacroMaterialCreated called for a macro material that already exists. This indicates that either the bus is incorrectly sending out "
            "OnCreated announcements for existing materials, or the terrain feature processor isn't properly cleaning up macro materials.");

        AZ_Assert(m_materialData.GetSize() < AZStd::numeric_limits<uint16_t>::max(), "No more room for terrain macro materials.");

        uint16_t materialRef = aznumeric_cast<uint16_t>(m_materialData.Reserve());
        m_entityToRef[entityId] = materialRef;

        UpdateMacroMaterialShaderEntry(materialRef, newMaterialData);

        ForMacroMaterialsInBounds(newMaterialData.m_bounds,
            [&](uint32_t idx, [[maybe_unused]] const AZ::Vector2& corner)
            {
                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                AddMacroMaterialToTile(materialRef, materialRefList);
            }
        );
    }

    void TerrainMacroMaterialManager::OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        // If terrainSizeChanged, everything will rebuild later, so don't do any work here.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            m_entityToRef.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        uint16_t materialRef = m_entityToRef[entityId];
        UpdateMacroMaterialShaderEntry(materialRef, newMaterialData);
    }
    
    void TerrainMacroMaterialManager::OnTerrainMacroMaterialRegionChanged(
        AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion)
    {
        // If terrainSizeChanged, everything will rebuild later, so don't do any work here.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            m_entityToRef.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");

        const uint16_t materialRef = m_entityToRef[entityId];
        MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);
        const AZ::Vector2 boundsMin = AZ::Vector2(newRegion.GetMin());
        const AZ::Vector2 boundsMax = AZ::Vector2(newRegion.GetMax());

        boundsMin.StoreToFloat2(shaderData.m_boundsMin.data());
        boundsMax.StoreToFloat2(shaderData.m_boundsMax.data());

        AZ::Aabb changedRegion = oldRegion;
        changedRegion.AddAabb(newRegion);

        ForMacroMaterialsInBounds(changedRegion,
            [&](uint32_t idx, const AZ::Vector2& tileMin)
            {
                AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
                bool overlapsNew =
                    tileMin.IsLessThan(boundsMax) &&
                    tileMax.IsGreaterThan(boundsMin);

                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                for (uint16_t refIdx = 0; refIdx < MacroMaterialsPerTile; ++refIdx)
                {
                    if (materialRefList.at(refIdx) == materialRef)
                    {
                        if (!overlapsNew)
                        {
                            // Remove material from region it no longer overlaps
                            RemoveMacroMaterialFromTile(refIdx, materialRefList, tileMin);
                        }
                        break;
                    }
                    else if (materialRefList.at(refIdx) == InvalidMacroMaterialRef)
                    {
                        if (overlapsNew)
                        {
                            // Add material to region that it now overlaps but used to not.
                            materialRefList.at(refIdx) = materialRef;
                        }
                        break;
                    }
                }
            }
        );
        
        m_bufferNeedsUpdate = true;
    }

    void TerrainMacroMaterialManager::OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId)
    {
        // If terrainSizeChanged, everything will rebuild later, so don't do any work here.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            m_entityToRef.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");

        const uint16_t materialRef = m_entityToRef[entityId];
        MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);
        
        ForMacroMaterialsInBounds(shaderData.m_boundsMin, shaderData.m_boundsMax,
            [&](uint32_t idx, [[maybe_unused]] const AZ::Vector2& corner)
            {
                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                for (uint16_t refIdx = 0; refIdx < MacroMaterialsPerTile; ++refIdx)
                {
                    if (materialRefList.at(refIdx) == materialRef)
                    {
                        RemoveMacroMaterialFromTile(refIdx, materialRefList, corner);
                        break;
                    }
                }
            }
        );
        
        m_bindlessImageHandler->RemoveBindlessImage(aznumeric_cast<uint16_t>(shaderData.m_colorMapId));
        m_bindlessImageHandler->RemoveBindlessImage(aznumeric_cast<uint16_t>(shaderData.m_normalMapId));

        m_materialData.Release(materialRef);
        m_entityToRef.erase(entityId);
        m_bufferNeedsUpdate = true;
    }
    
    void TerrainMacroMaterialManager::UpdateMacroMaterialShaderEntry(uint16_t materialRef, const MacroMaterialData& macroMaterialData)
    {
        MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);

        shaderData.m_flags = (MacroMaterialShaderFlags)(
            (macroMaterialData.m_normalFlipX ? MacroMaterialShaderFlags::FlipMacroNormalX : 0) |
            (macroMaterialData.m_normalFlipY ? MacroMaterialShaderFlags::FlipMacroNormalY : 0)
        );

        shaderData.m_normalFactor = macroMaterialData.m_normalFactor;
        AZ::Vector2(macroMaterialData.m_bounds.GetMin()).StoreToFloat2(shaderData.m_boundsMin.data());
        AZ::Vector2(macroMaterialData.m_bounds.GetMax()).StoreToFloat2(shaderData.m_boundsMax.data());

        auto UpdateImageIndex = [&](uint32_t& indexRef, const AZ::Data::Instance<AZ::RPI::Image>& imageView)
        {
            if (indexRef != InvalidImageIndex)
            {
                if (imageView)
                {
                    m_bindlessImageHandler->UpdateBindlessImage(aznumeric_cast<uint16_t>(indexRef), imageView->GetImageView());
                }
                else
                {
                    m_bindlessImageHandler->RemoveBindlessImage(aznumeric_cast<uint16_t>(indexRef));
                    indexRef = InvalidImageIndex;
                }
            }
            else if (imageView)
            {
                indexRef = m_bindlessImageHandler->AppendBindlessImage(imageView->GetImageView());
            }
        };

        UpdateImageIndex(shaderData.m_colorMapId, macroMaterialData.m_colorImage);
        UpdateImageIndex(shaderData.m_normalMapId, macroMaterialData.m_normalImage);

        MacroMaterialMetaData& metaData = m_materialData.GetElement<1>(materialRef);
        metaData.m_priority = macroMaterialData.m_priority;

        m_bufferNeedsUpdate = true;
    }

    void TerrainMacroMaterialManager::AddMacroMaterialToTile(uint16_t newMaterialRef, MacroMaterialRefs& materialRefs)
    {
        int32_t newPriority = m_materialData.GetElement<1>(newMaterialRef).m_priority;

        for (uint16_t refIdx = 0; refIdx < MacroMaterialsPerTile; ++refIdx)
        {
            uint16_t& ref = materialRefs.at(refIdx);
            if (ref == InvalidMacroMaterialRef)
            {
                // Empty spot, just add the material
                ref = newMaterialRef;
                return;
            }
            else
            {
                // Check the priority. If it's less than the material being added, insert.
                int32_t refPriority = m_materialData.GetElement<1>(ref).m_priority;
                if (refPriority < newPriority)
                {
                    uint16_t tempRef = newMaterialRef;
                    for (; refIdx < MacroMaterialsPerTile; ++refIdx)
                    {
                        uint16_t& ref2 = materialRefs.at(refIdx);
                        AZStd::swap(ref2, tempRef);
                    }
                    return;
                }
            }
        }
    }

    void TerrainMacroMaterialManager::RemoveMacroMaterialFromTile(uint16_t shaderDataIdx, MacroMaterialRefs& materialRefs, const AZ::Vector2& tileMin)
    {
        // Remove the macro material entry from this tile by copying the remaining entries on top.
        for (++shaderDataIdx; shaderDataIdx < MacroMaterialsPerTile; ++shaderDataIdx)
        {
            materialRefs.at(shaderDataIdx - 1) = materialRefs.at(shaderDataIdx);
        }

        // Disable the last entry.
        uint16_t& lastEntry = materialRefs.at(MacroMaterialsPerTile - 1);
        if (lastEntry != InvalidMacroMaterialRef)
        {
            lastEntry = InvalidMacroMaterialRef;
            int32_t lastPriority = 0;

            // Check all the macro materials to see if any overlap this tile. Since the tile was full, when a macro material
            // was removed, there may be a macro material that can be placed in the empty spot.

            AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
            for (auto& [entityId, materialRef] : m_entityToRef)
            {
                MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);
                int32_t priority = m_materialData.GetElement<1>(materialRef).m_priority;

                if (shaderData.Overlaps(tileMin, tileMax) && (lastEntry == InvalidMacroMaterialRef || lastPriority < priority))
                {
                    lastEntry = materialRef;
                    lastPriority = priority;
                }
            }
        }
    }

    template<typename Callback>
    void TerrainMacroMaterialManager::ForMacroMaterialsInRegion(const ClipmapBoundsRegion& region, Callback callback)
    {
        AZ::Vector2 regionCorner = AZ::Vector2(region.m_worldAabb.GetMin());
        Vector2i extents = region.m_localAabb.m_max - region.m_localAabb.m_min;

        for (int32_t y = 0; y < extents.m_y; ++y)
        {
            for (int32_t x = 0; x < extents.m_x; ++x)
            {
                const Vector2i local = region.m_localAabb.m_min + Vector2i(x, y);
                uint32_t idx = (local.m_y * m_tiles1D + local.m_x);
                const AZ::Vector2 corner = regionCorner + AZ::Vector2(x * MacroMaterialGridSize, y * MacroMaterialGridSize);
                callback(idx, corner);
            }
        }
    }

    template<typename Callback>
    void TerrainMacroMaterialManager::ForMacroMaterialsInBounds(const AZ::Vector2& minBounds, const AZ::Vector2& maxBounds, Callback callback)
    {
        auto updateRegionsList = m_macroMaterialTileBounds.TransformRegion(minBounds, maxBounds);

        for (const auto& region : updateRegionsList)
        {
            ForMacroMaterialsInRegion(region, callback);
        }
    }

    template<typename Callback>
    void TerrainMacroMaterialManager::ForMacroMaterialsInBounds(const AZ::Aabb& bounds, Callback callback)
    {
        ForMacroMaterialsInBounds(AZ::Vector2(bounds.GetMin()), AZ::Vector2(bounds.GetMax()), callback);
    }

    template<typename Callback>
    void TerrainMacroMaterialManager::ForMacroMaterialsInBounds(const AZStd::array<float, 2>& minBounds, const AZStd::array<float, 2>& maxBounds, Callback callback)
    {
        ForMacroMaterialsInBounds(AZ::Vector2::CreateFromFloat2(minBounds.data()), AZ::Vector2::CreateFromFloat2(maxBounds.data()), callback);
    }

    void TerrainMacroMaterialManager::SetRenderDistance(float distance)
    {
        uint16_t newTiles1D = aznumeric_cast<uint16_t>(AZStd::ceilf((distance) / MacroMaterialGridSize)) + 1;
        newTiles1D *= 2; // distance is radius, grid covers diameter.
        if (newTiles1D != m_tiles1D)
        {
            m_tiles1D = newTiles1D;
            m_terrainSizeChanged = true;
        }
    }

    void TerrainMacroMaterialManager::Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        AZ::Vector3 mainCameraPosition = mainView->GetCameraTransform().GetTranslation();

        if (m_terrainSizeChanged)
        {
            m_terrainSizeChanged = false;
            m_bufferNeedsUpdate = true;

            ClipmapBoundsDescriptor desc;
            desc.m_clipmapToWorldScale = MacroMaterialGridSize;
            desc.m_clipmapUpdateMultiple = 1;
            desc.m_size = m_tiles1D;
            desc.m_worldSpaceCenter = AZ::Vector2(mainCameraPosition);

            m_macroMaterialTileBounds = ClipmapBounds(desc);

            // Rebuild the macro material tiles from scratch when the world size changes. This could be made more efficient
            // but is fine for now since world resizes are rare.

            RemoveAllImages();
            m_entityToRef.clear();
            m_materialData.Clear();
            m_materialRefGridShaderData.clear();

            const uint32_t macroMaterialTileCount = m_tiles1D * m_tiles1D;
            
            m_materialRefGridShaderData.resize(macroMaterialTileCount);
            AZStd::fill(m_materialRefGridShaderData.begin(), m_materialRefGridShaderData.end(), DefaultRefs);

            TerrainMacroMaterialRequestBus::EnumerateHandlers(
                [&](TerrainMacroMaterialRequests* handler)
                {
                    MacroMaterialData macroMaterial = handler->GetTerrainMacroMaterialData();
                    AZ::EntityId entityId = *(Terrain::TerrainMacroMaterialRequestBus::GetCurrentBusId());
                    OnTerrainMacroMaterialCreated(entityId, macroMaterial);
                    return true;
                }
            );
        }
        else
        {
            auto updateRegionList = m_macroMaterialTileBounds.UpdateCenter(AZ::Vector2(mainCameraPosition));
            for (const auto& updateRegion : updateRegionList)
            {
                AZStd::vector<uint16_t> affectedMaterials;
                affectedMaterials.reserve(AZStd::GetMin(m_entityToRef.size(), 128ull));
                AZ::Vector2 regionMin = AZ::Vector2(updateRegion.m_worldAabb.GetMin());
                AZ::Vector2 regionMax = AZ::Vector2(updateRegion.m_worldAabb.GetMax());

                // Do a coarse check of which materials might affect this region's tiles by gathering all
                // macro materials that overlap the region. This should reduce the number of checks that need
                // to be done per-tile.

                for (auto& [entityId, materialRef] : m_entityToRef)
                {
                    MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);

                    if (shaderData.Overlaps(regionMin, regionMax))
                    {
                        affectedMaterials.push_back(materialRef);
                    }
                }

                // Check the list of macro materials against all the tiles in this region.

                ForMacroMaterialsInRegion(updateRegion,
                    [&](uint32_t idx, const AZ::Vector2& tileMin)
                    {
                        AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
                        MacroMaterialRefs& refs = m_materialRefGridShaderData.at(idx);
                        refs = DefaultRefs; // clear out current refs

                        for (uint16_t materialRef : affectedMaterials)
                        {
                            MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);
                            if (shaderData.Overlaps(tileMin, tileMax))
                            {
                                AddMacroMaterialToTile(materialRef, refs);
                            }
                        }
                    }
                );

                m_bufferNeedsUpdate = true;
            }
        }

        if (m_bufferNeedsUpdate && terrainSrg)
        {
            m_bufferNeedsUpdate = false;
            m_materialDataBuffer.UpdateBuffer(m_materialData.GetRawData<0>(), aznumeric_cast<uint32_t>(m_materialData.GetSize()));
            m_materialRefGridDataBuffer.UpdateBuffer(m_materialRefGridShaderData.data(), aznumeric_cast<uint32_t>(m_materialRefGridShaderData.size()));

            MacroMaterialGridShaderData macroMaterialGridShaderData;
            macroMaterialGridShaderData.m_tileCount1D = m_tiles1D;
            macroMaterialGridShaderData.m_tileSize = MacroMaterialGridSize;

            m_materialDataBuffer.UpdateSrg(terrainSrg.get());
            m_materialRefGridDataBuffer.UpdateSrg(terrainSrg.get());
            terrainSrg->SetConstant(m_macroMaterialGridIndex, macroMaterialGridShaderData);
        }
    }

    void TerrainMacroMaterialManager::RemoveAllImages()
    {   
        for (const auto& [entity, materialRef] : m_entityToRef)
        {
            RemoveImagesForMaterial(materialRef);
        }
    }

    void TerrainMacroMaterialManager::RemoveImagesForMaterial(const uint16_t materialRef)
    {
        MacroMaterialShaderData& shaderData = m_materialData.GetElement<0>(materialRef);
        m_bindlessImageHandler->RemoveBindlessImage(aznumeric_cast<uint16_t>(shaderData.m_colorMapId));
        m_bindlessImageHandler->RemoveBindlessImage(aznumeric_cast<uint16_t>(shaderData.m_normalMapId));
    }

}
