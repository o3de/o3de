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

    TerrainMacroMaterialManager::MacroMaterialData2D::MacroMaterialData2D(const MacroMaterialData& data)
        : m_minBounds(data.m_bounds.GetMin())
        , m_maxBounds(data.m_bounds.GetMax())
        , m_normalFlipX(data.m_normalFlipX)
        , m_normalFlipY(data.m_normalFlipY)
        , m_normalFactor(data.m_normalFactor)
    {
    }

    bool TerrainMacroMaterialManager::MacroMaterialData2D::Overlaps(const AZ::Vector2& min, const AZ::Vector2& max)
    {
        return m_minBounds.IsLessThan(max) && m_maxBounds.IsGreaterThan(min);
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

        m_materialShaderData.Clear();
        m_materialRefGridShaderData.clear();

        RemoveAllImages();
        m_macroMaterials.clear();
        
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
            !m_macroMaterials.contains(entityId),
            "OnTerrainMacroMaterialCreated called for a macro material that already exists. This indicates that either the bus is incorrectly sending out "
            "OnCreated announcements for existing materials, or the terrain feature processor isn't properly cleaning up macro materials.");

        AZ_Assert(m_materialShaderData.GetSize() < AZStd::numeric_limits<uint16_t>::max(), "No more room for terrain macro materials.")

        MacroMaterial& macroMaterial = m_macroMaterials[entityId];
        macroMaterial.m_data = newMaterialData;
        if (newMaterialData.m_colorImage)
        {
            macroMaterial.m_colorIndex = m_bindlessImageHandler->AppendBindlessImage(newMaterialData.m_colorImage->GetImageView());
        }
        if (newMaterialData.m_normalImage)
        {
            macroMaterial.m_normalIndex = m_bindlessImageHandler->AppendBindlessImage(newMaterialData.m_normalImage->GetImageView());
        }

        macroMaterial.m_materialRef = aznumeric_cast<uint16_t>(m_materialShaderData.Reserve());
        UpdateMacroMaterialShaderEntry(m_materialShaderData.GetElement(macroMaterial.m_materialRef), macroMaterial);

        ForMacroMaterialsInBounds(macroMaterial.m_data.m_minBounds, macroMaterial.m_data.m_maxBounds,
            [&](uint32_t idx, [[maybe_unused]] const AZ::Vector2& corner)
            {
                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                AddMacroMaterialShaderEntry(macroMaterial.m_materialRef, materialRefList);
            }
        );

        m_bufferNeedsUpdate = true;
    }

    void TerrainMacroMaterialManager::OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        // If terrainSizeChanged, everything will rebuild later, so don't do any work here.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            m_macroMaterials.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        MacroMaterial& macroMaterial = m_macroMaterials[entityId];
        macroMaterial.m_data = newMaterialData;

        auto UpdateImageIndex = [&](uint16_t& indexRef, const AZ::Data::Instance<AZ::RPI::Image>& imageView)
        {
            if (indexRef)
            {
                if (imageView)
                {
                    m_bindlessImageHandler->UpdateBindlessImage(indexRef, imageView->GetImageView());
                }
                else
                {
                    m_bindlessImageHandler->RemoveBindlessImage(indexRef);
                    indexRef = 0xFFFF;
                }
            }
            else if (imageView)
            {
                indexRef = m_bindlessImageHandler->AppendBindlessImage(imageView->GetImageView());
            }
        };

        if (macroMaterial.m_colorIndex != 0xFFFF)
        {
            UpdateImageIndex(macroMaterial.m_colorIndex, newMaterialData.m_colorImage);
        }

        if (macroMaterial.m_normalIndex != 0xFFFF)
        {
            UpdateImageIndex(macroMaterial.m_normalIndex, newMaterialData.m_normalImage);
        }

        UpdateMacroMaterialShaderEntry(m_materialShaderData.GetElement(macroMaterial.m_materialRef), macroMaterial);
        m_bufferNeedsUpdate = true;
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
            m_macroMaterials.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        MacroMaterial& macroMaterial = m_macroMaterials[entityId];
        macroMaterial.m_data.m_minBounds = AZ::Vector2(newRegion.GetMin());
        macroMaterial.m_data.m_maxBounds = AZ::Vector2(newRegion.GetMax());

        UpdateMacroMaterialShaderEntry(m_materialShaderData.GetElement(macroMaterial.m_materialRef), macroMaterial);

        AZ::Aabb changedRegion = oldRegion;
        changedRegion.AddAabb(newRegion);

        ForMacroMaterialsInBounds(AZ::Vector2(changedRegion.GetMin()), AZ::Vector2(changedRegion.GetMax()),
            [&](uint32_t idx, const AZ::Vector2& tileMin)
            {
                AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
                bool overlapsNew =
                    tileMin.IsLessThan(macroMaterial.m_data.m_maxBounds) &&
                    tileMax.IsGreaterThan(macroMaterial.m_data.m_minBounds);

                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                for (uint16_t refIdx = 0; refIdx < MacroMaterialsPerTile; ++refIdx)
                {
                    if (materialRefList.at(refIdx) == macroMaterial.m_materialRef)
                    {
                        if (!overlapsNew)
                        {
                            // Remove material from region it no longer overlaps
                            RemoveMacroMaterialShaderEntry(refIdx, materialRefList);
                        }
                        break;
                    }
                    else if (materialRefList.at(refIdx) == InvalidMacroMaterialRef)
                    {
                        if (overlapsNew)
                        {
                            // Add material to region that it now overlaps but used to not.
                            materialRefList.at(refIdx) = macroMaterial.m_materialRef;
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
            m_macroMaterials.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        const MacroMaterial& macroMaterial = m_macroMaterials[entityId];
        
        ForMacroMaterialsInBounds(macroMaterial.m_data.m_minBounds, macroMaterial.m_data.m_maxBounds,
            [&](uint32_t idx, [[maybe_unused]] const AZ::Vector2& corner)
            {
                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                for (uint16_t refIdx = 0; refIdx < MacroMaterialsPerTile; ++refIdx)
                {
                    if (materialRefList.at(refIdx) == macroMaterial.m_materialRef)
                    {
                        RemoveMacroMaterialShaderEntry(refIdx, materialRefList);
                        break;
                    }
                }
            }
        );
        
        if (macroMaterial.m_colorIndex != 0xFFFF)
        {
            m_bindlessImageHandler->RemoveBindlessImage(macroMaterial.m_colorIndex);
        }
        if (macroMaterial.m_normalIndex != 0xFFFF)
        {
            m_bindlessImageHandler->RemoveBindlessImage(macroMaterial.m_normalIndex);
        }
        m_materialShaderData.Release(macroMaterial.m_materialRef);
        m_macroMaterials.erase(entityId);
        m_bufferNeedsUpdate = true;
    }
    
    void TerrainMacroMaterialManager::UpdateMacroMaterialShaderEntry(MacroMaterialShaderData& macroMaterialShaderData, const MacroMaterial& macroMaterial)
    {
        macroMaterialShaderData.m_flags = (MacroMaterialShaderFlags)(
            (macroMaterial.m_data.m_normalFlipX ? MacroMaterialShaderFlags::FlipMacroNormalX : 0) |
            (macroMaterial.m_data.m_normalFlipY ? MacroMaterialShaderFlags::FlipMacroNormalY : 0)
        );

        macroMaterialShaderData.m_normalFactor = macroMaterial.m_data.m_normalFactor;
        macroMaterial.m_data.m_minBounds.StoreToFloat2(macroMaterialShaderData.m_boundsMin.data());
        macroMaterial.m_data.m_maxBounds.StoreToFloat2(macroMaterialShaderData.m_boundsMax.data());
        macroMaterialShaderData.m_colorMapId = macroMaterial.m_colorIndex;
        macroMaterialShaderData.m_normalMapId = macroMaterial.m_normalIndex;
    }

    void TerrainMacroMaterialManager::AddMacroMaterialShaderEntry(uint16_t materialRef, MacroMaterialRefs& materialRefs)
    {
        for (auto& ref : materialRefs)
        {
            if (ref == InvalidMacroMaterialRef)
            {
                ref = materialRef;
                return;
            }
        }
    }

    void TerrainMacroMaterialManager::RemoveMacroMaterialShaderEntry(uint16_t shaderDataIdx, MacroMaterialRefs& materialRefs)
    {
        // Remove the macro material entry from this tile by copying the remaining entries on top.
        for (++shaderDataIdx; shaderDataIdx < MacroMaterialsPerTile; ++shaderDataIdx)
        {
            materialRefs.at(shaderDataIdx - 1) = materialRefs.at(shaderDataIdx);
        }
        // Disable the last entry.
        materialRefs.at(MacroMaterialsPerTile - 1) = InvalidMacroMaterialRef;
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
            m_macroMaterials.clear();
            m_materialShaderData.Clear();
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
                AZStd::vector<MacroMaterial> affectedMaterials;
                affectedMaterials.reserve(AZStd::GetMin(m_macroMaterials.size(), size_t(128)));
                AZ::Vector2 regionMin = AZ::Vector2(updateRegion.m_worldAabb.GetMin());
                AZ::Vector2 regionMax = AZ::Vector2(updateRegion.m_worldAabb.GetMax());

                // Do a coarse check of which materials might affect this region's tiles by gathering all
                // macro materials that overlap the region. This should reduce the number of checks that need
                // to be done per-tile.

                for (auto& [entityId, macroMaterial] : m_macroMaterials)
                {
                    if (macroMaterial.m_data.Overlaps(regionMin, regionMax))
                    {
                        affectedMaterials.push_back(macroMaterial);
                    }
                }

                // Check the list of macro materials against all the tiles in this region.

                ForMacroMaterialsInRegion(updateRegion,
                    [&](uint32_t idx, const AZ::Vector2& tileMin)
                    {
                        AZ::Vector2 tileMax = tileMin + AZ::Vector2(MacroMaterialGridSize);
                        MacroMaterialRefs& refs = m_materialRefGridShaderData.at(idx);
                        refs = DefaultRefs; // clear out current refs

                        for (MacroMaterial macroMaterial : affectedMaterials)
                        {
                            if (macroMaterial.m_data.Overlaps(tileMin, tileMax))
                            {
                                AddMacroMaterialShaderEntry(macroMaterial.m_materialRef, refs);
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
            m_materialDataBuffer.UpdateBuffer(m_materialShaderData.GetRawData(), aznumeric_cast<uint32_t>(m_materialShaderData.GetSize()));
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
        for (const auto& [entity, macroMaterial] : m_macroMaterials)
        {
            RemoveImagesForMaterial(macroMaterial);
        }
    }

    void TerrainMacroMaterialManager::RemoveImagesForMaterial(const MacroMaterial& macroMaterial)
    {
        if (macroMaterial.m_colorIndex != 0xFFFF)
        {
            m_bindlessImageHandler->RemoveBindlessImage(macroMaterial.m_colorIndex);
        }
        if (macroMaterial.m_normalIndex != 0xFFFF)
        {
            m_bindlessImageHandler->RemoveBindlessImage(macroMaterial.m_normalIndex);
        }
    }

}
