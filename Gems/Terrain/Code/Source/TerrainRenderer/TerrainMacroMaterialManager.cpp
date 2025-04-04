/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainMacroMaterialManager.h>
#include <Atom/RHI/RHISystemInterface.h>
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

    void TerrainMacroMaterialManager::Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        AZ_Error(TerrainMacroMaterialManagerName, terrainSrg, "terrainSrg must not be null.");
        AZ_Error(TerrainMacroMaterialManagerName, !m_isInitialized, "Already initialized.");

        if (!terrainSrg || m_isInitialized)
        {
            return;
        }
        
        auto deviceCount = AZ::RHI::RHISystemInterface::Get()->GetDeviceCount();

        for (auto deviceIndex{0}; deviceIndex < deviceCount; ++deviceIndex)
        {
            if (terrainSrg->GetRHIShaderResourceGroup()->IsDeviceSet(deviceIndex))
            {
                m_materialShaderData[deviceIndex] = {};
            }
        }

        if (UpdateSrgIndices(terrainSrg))
        {
            TerrainMacroMaterialNotificationBus::Handler::BusConnect();
            
            m_terrainSizeChanged = true;
            m_isInitialized = true;
        }
    }
    
    void TerrainMacroMaterialManager::Reset()
    {
        m_isInitialized = false;

        RemoveAllImages();

        m_materialDataBuffer = {};
        m_materialRefGridDataBuffer = {};

        for (auto& [deviceIndex, data] : m_materialShaderData)
        {
            data.clear();
        }

        m_materialPriorityData.Clear();
        m_materialRefGridShaderData.clear();
        m_entityToMaterialHandle.clear();
        
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
        desc.m_elementSize = sizeof(TileMaterials);
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
            !m_entityToMaterialHandle.contains(entityId),
            "OnTerrainMacroMaterialCreated called for a macro material that already exists. This indicates that either the bus is incorrectly sending out "
            "OnCreated announcements for existing materials, or the terrain feature processor isn't properly cleaning up macro materials.");

        AZ_Assert(m_materialPriorityData.GetSize() < AZStd::numeric_limits<uint16_t>::max(), "No more room for terrain macro materials.");

        MaterialHandle materialHandle = MaterialHandle(m_materialPriorityData.Reserve());

        for (auto& [deviceIndex, data] : m_materialShaderData)
        {
            data.resize(m_materialPriorityData.GetSize());
        }

        m_entityToMaterialHandle[entityId] = materialHandle;

        UpdateMacroMaterialShaderEntry(materialHandle, newMaterialData);

        ForMacroMaterialsInBounds(newMaterialData.m_bounds,
            [&](TileHandle tileHandle, [[maybe_unused]] const AZ::Vector2& corner)
            {
                AddMacroMaterialToTile(materialHandle, tileHandle);
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
            m_entityToMaterialHandle.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        MaterialHandle materialHandle = m_entityToMaterialHandle[entityId];
        UpdateMacroMaterialShaderEntry(materialHandle, newMaterialData);
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
            m_entityToMaterialHandle.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");

        MaterialHandle materialHandle = m_entityToMaterialHandle[entityId];
        const AZ::Vector2 boundsMin = AZ::Vector2(newRegion.GetMin());
        const AZ::Vector2 boundsMax = AZ::Vector2(newRegion.GetMax());

        for (auto& [deviceIndex, data] : m_materialShaderData)
        {
            MacroMaterialShaderData& shaderData = data.at(materialHandle.GetIndex());
            boundsMin.StoreToFloat2(shaderData.m_boundsMin.data());
            boundsMax.StoreToFloat2(shaderData.m_boundsMax.data());
        }

        AZ::Aabb changedRegion = oldRegion;
        changedRegion.AddAabb(newRegion);

        ForMacroMaterialsInBounds(changedRegion,
            [&](TileHandle tileHandle, const AZ::Vector2& tileMin)
            {
                AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
                bool overlapsNew =
                    tileMin.IsLessThan(boundsMax) &&
                    tileMax.IsGreaterThan(boundsMin);

                if (overlapsNew)
                {
                    AddMacroMaterialToTile(materialHandle, tileHandle);
                }
                else
                {
                    RemoveMacroMaterialFromTile(materialHandle, tileHandle, tileMin);
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
            m_entityToMaterialHandle.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");

        MaterialHandle materialHandle = m_entityToMaterialHandle[entityId];
        MacroMaterialShaderData& shaderData = m_materialShaderData.begin()->second.at(materialHandle.GetIndex());
        
        ForMacroMaterialsInBounds(shaderData.m_boundsMin, shaderData.m_boundsMax,
            [&](TileHandle tileHandle, [[maybe_unused]] const AZ::Vector2& corner)
            {
                RemoveMacroMaterialFromTile(materialHandle, tileHandle, corner);
            }
        );
        
        m_materialPriorityData.Release(materialHandle.GetIndex());
        m_entityToMaterialHandle.erase(entityId);
        m_bufferNeedsUpdate = true;
    }
    
    void TerrainMacroMaterialManager::UpdateMacroMaterialShaderEntry(MaterialHandle materialHandle, const MacroMaterialData& macroMaterialData)
    {
        auto UpdateImageIndex = [&](uint32_t& indexRef, const AZ::Data::Instance<AZ::RPI::Image>& imageView, int deviceIndex)
        {
            indexRef = imageView ? imageView->GetImageView()->GetDeviceImageView(deviceIndex)->GetBindlessReadIndex() : InvalidImageIndex;
        };

        for (auto& [deviceIndex, data] : m_materialShaderData)
        {
            MacroMaterialShaderData& shaderData = data.at(materialHandle.GetIndex());

            shaderData.m_flags = (MacroMaterialShaderFlags)(
                (macroMaterialData.m_normalFlipX ? MacroMaterialShaderFlags::FlipMacroNormalX : 0) |
                (macroMaterialData.m_normalFlipY ? MacroMaterialShaderFlags::FlipMacroNormalY : 0)
            );

            shaderData.m_normalFactor = macroMaterialData.m_normalFactor;
            AZ::Vector2(macroMaterialData.m_bounds.GetMin()).StoreToFloat2(shaderData.m_boundsMin.data());
            AZ::Vector2(macroMaterialData.m_bounds.GetMax()).StoreToFloat2(shaderData.m_boundsMax.data());

            UpdateImageIndex(shaderData.m_colorMapId, macroMaterialData.m_colorImage, deviceIndex);
            UpdateImageIndex(shaderData.m_normalMapId, macroMaterialData.m_normalImage, deviceIndex);
        }

        MacroMaterialPriority& priority = m_materialPriorityData.GetElement(materialHandle.GetIndex());
        priority.m_priority = macroMaterialData.m_priority;
        priority.m_hash = uint32_t(AZ::u64(macroMaterialData.m_entityId) >> 32) ^ uint32_t(AZ::u64(macroMaterialData.m_entityId) & 0xFFFFFFFF);

        m_bufferNeedsUpdate = true;
    }

    void TerrainMacroMaterialManager::AddMacroMaterialToTile(MaterialHandle newMaterialHandle, TileHandle tileHandle)
    {
        TileMaterials& tileMaterials = m_materialRefGridShaderData.at(tileHandle.GetIndex());

        MacroMaterialPriority& newPriority = m_materialPriorityData.GetElement(newMaterialHandle.GetIndex());

        for (uint16_t materialIndex = 0; materialIndex < MacroMaterialsPerTile; ++materialIndex)
        {
            MaterialHandle& materialHandle = tileMaterials.at(materialIndex);
            if (materialHandle == MaterialHandle::Null)
            {
                // Empty spot, just add the material
                materialHandle = newMaterialHandle;
                return;
            }
            else if (materialHandle == newMaterialHandle)
            {
                return;
            }
            else
            {
                // Check the priority. If the new material's priority is greater, insert.
                MacroMaterialPriority& priority = m_materialPriorityData.GetElement(materialHandle.GetIndex());
                if (newPriority > priority)
                {
                    MaterialHandle temphandle = newMaterialHandle;
                    for (; materialIndex < MacroMaterialsPerTile; ++materialIndex)
                    {
                        MaterialHandle& materialHandle2 = tileMaterials.at(materialIndex);
                        AZStd::swap(materialHandle2, temphandle);
                    }
                    return;
                }
            }
        }
    }

    void TerrainMacroMaterialManager::RemoveMacroMaterialFromTile(MaterialHandle materialHandleToRemove, TileHandle tileHandle, const AZ::Vector2& tileMin)
    {
        TileMaterials& tileMaterials = m_materialRefGridShaderData.at(tileHandle.GetIndex());

        for (uint16_t materialIndex = 0; materialIndex < MacroMaterialsPerTile; ++materialIndex)
        {
            if (tileMaterials.at(materialIndex) == materialHandleToRemove)
            {
                // Remove the macro material entry from this tile by copying the remaining entries on top.
                for (++materialIndex; materialIndex < MacroMaterialsPerTile; ++materialIndex)
                {
                    tileMaterials.at(materialIndex - 1) = tileMaterials.at(materialIndex);
                }
                break;
            }
        }

        // Disable or replace the last entry.
        MaterialHandle& lastEntry = tileMaterials.at(MacroMaterialsPerTile - 1);
        if (lastEntry != MaterialHandle::Null)
        {
            lastEntry = MaterialHandle::Null;
            MacroMaterialPriority lastPriority;

            // Check all the macro materials to see if any overlap this tile. Since the tile was full, when a macro material
            // was removed, there may be a macro material that can be placed in the empty spot.

            // Create a list of macro materials to ignore when searching for possible entries to add at the end.
            // This is basically all the materials except the last one, plus the material currently being removed.
            AZStd::array<MaterialHandle, MacroMaterialsPerTile> alreadyUsedMaterials;
            AZStd::copy(tileMaterials.begin(), tileMaterials.end(), alreadyUsedMaterials.begin());
            alreadyUsedMaterials.at(MacroMaterialsPerTile - 1) = materialHandleToRemove;

            AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
            for (auto& [entityId, materialHandle] : m_entityToMaterialHandle)
            {
                if (AZStd::find(alreadyUsedMaterials.begin(), alreadyUsedMaterials.end(), materialHandle) != nullptr)
                {
                    continue;
                }

                MacroMaterialShaderData& shaderData = m_materialShaderData.begin()->second.at(materialHandle.GetIndex());
                MacroMaterialPriority priority = m_materialPriorityData.GetElement(materialHandle.GetIndex());

                if (shaderData.Overlaps(tileMin, tileMax) && (lastEntry == MaterialHandle::Null || priority > lastPriority))
                {
                    lastEntry = materialHandle;
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
                TileHandle tileHandle = TileHandle(local.m_y * m_tiles1D + local.m_x);
                const AZ::Vector2 corner = regionCorner + AZ::Vector2(x * MacroMaterialGridSize, y * MacroMaterialGridSize);
                callback(tileHandle, corner);
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
            m_entityToMaterialHandle.clear();
            m_materialPriorityData.Clear();
            m_materialRefGridShaderData.clear();

            for (auto& [deviceIndex, data] : m_materialShaderData)
            {
                data.clear();
            }

            const uint32_t macroMaterialTileCount = m_tiles1D * m_tiles1D;
            
            m_materialRefGridShaderData.resize(macroMaterialTileCount);
            AZStd::fill(m_materialRefGridShaderData.begin(), m_materialRefGridShaderData.end(), DefaultTileMaterials);

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
                AZStd::vector<MaterialHandle> affectedMaterials;
                affectedMaterials.reserve(AZStd::GetMin(m_entityToMaterialHandle.size(), size_t(128)));
                AZ::Vector2 regionMin = AZ::Vector2(updateRegion.m_worldAabb.GetMin());
                AZ::Vector2 regionMax = AZ::Vector2(updateRegion.m_worldAabb.GetMax());

                // Do a coarse check of which materials might affect this region's tiles by gathering all
                // macro materials that overlap the region. This should reduce the number of checks that need
                // to be done per-tile.

                for (auto& [entityId, materialHandle] : m_entityToMaterialHandle)
                {
                    MacroMaterialShaderData& shaderData = m_materialShaderData.begin()->second.at(materialHandle.GetIndex());

                    if (shaderData.Overlaps(regionMin, regionMax))
                    {
                        affectedMaterials.push_back(materialHandle);
                    }
                }

                // Check the list of macro materials against all the tiles in this region.

                ForMacroMaterialsInRegion(updateRegion,
                    [&](TileHandle tileHandle, const AZ::Vector2& tileMin)
                    {
                        AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
                        m_materialRefGridShaderData.at(tileHandle.GetIndex()) = DefaultTileMaterials; // clear out current materials

                        for (MaterialHandle materialHandle : affectedMaterials)
                        {
                            MacroMaterialShaderData& shaderData = m_materialShaderData.begin()->second.at(materialHandle.GetIndex());
                            if (shaderData.Overlaps(tileMin, tileMax))
                            {
                                AddMacroMaterialToTile(materialHandle, tileHandle);
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
            AZStd::unordered_map<int, const void*> rawData;

            for (auto& [deviceIndex, data] : m_materialShaderData)
            {
                rawData[deviceIndex] = data.data();
            }

            m_materialDataBuffer.UpdateBuffer(rawData, aznumeric_cast<uint32_t>(m_materialPriorityData.GetSize()));
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
        for (const auto& [entity, materialRef] : m_entityToMaterialHandle)
        {
            RemoveImagesForMaterial(materialRef);
        }
    }

    void TerrainMacroMaterialManager::RemoveImagesForMaterial(MaterialHandle materialHandle)
    {
        for (auto& [deviceIndex, data] : m_materialShaderData)
        {
            MacroMaterialShaderData& shaderData = data.at(materialHandle.GetIndex());
            shaderData.m_colorMapId = InvalidImageIndex;
            shaderData.m_normalMapId = InvalidImageIndex;
        }
    }

}
