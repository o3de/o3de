/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainMacroMaterialManager.h>

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

            OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::Settings);
            AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
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

        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
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

    void TerrainMacroMaterialManager::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion [[maybe_unused]], TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & TerrainDataChangedMask::Settings) != 0)
        {
            AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

            m_terrainSizeChanged = m_terrainSizeChanged || m_terrainBounds != worldBounds;
            m_terrainBounds = worldBounds;
        }
    }
    
    void TerrainMacroMaterialManager::OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        // If terrainSizeChanged is set, we're going to rebuild everything anyways, so don't do any work here. This early-out also
        // fixes order-of-activation issues when the following happens:
        // - macro material entity tries to register itself by calling OnTerrainMacroMaterialCreated
        // - TerrainMacroMaterialManager initializes
        // - macro material entity gets a change and calls OnTerrainMacroMaterialChanged (assert because not registered yet)
        // - TerrainMacroMaterialManager updates itself (manager enumerates the already-connected handlers and registers them)
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

        ForMacroMaterialsInBounds(newMaterialData.m_bounds,
            [&](uint32_t idx, [[maybe_unused]] const AZ::Vector2& corner)
            {
                MacroMaterialRefs& materialRefList = m_materialRefGridShaderData.at(idx);
                for (uint8_t offset = 0; offset < MacroMaterialsPerTile; ++offset)
                {
                    if (materialRefList.at(offset) == InvalidMacroMaterialRef)
                    {
                        materialRefList.at(offset) = macroMaterial.m_materialRef;
                        break;
                    }
                }
            }
        );

        m_bufferNeedsUpdate = true;
    }

    void TerrainMacroMaterialManager::OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        // If terrainSizeChanged is set, we're going to rebuild everything anyways, so don't do any work here. This early-out also
        // fixes order-of-activation issues.
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
        // If terrainSizeChanged is set, we're going to rebuild everything anyways, so don't do any work here. This early-out also
        // fixes order-of-activation issues.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            m_macroMaterials.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        MacroMaterial& macroMaterial = m_macroMaterials[entityId];
        macroMaterial.m_data.m_bounds = newRegion;

        UpdateMacroMaterialShaderEntry(m_materialShaderData.GetElement(macroMaterial.m_materialRef), macroMaterial);

        AZ::Aabb changedRegion = oldRegion;
        changedRegion.AddAabb(newRegion);

        ForMacroMaterialsInBounds(changedRegion,
            [&](uint32_t idx, const AZ::Vector2& corner)
            {
                AZ::Aabb tileAabb = AZ::Aabb::CreateFromMinMaxValues(
                    corner.GetX(), corner.GetY(), m_terrainBounds.GetMin().GetZ(),
                    corner.GetX() + MacroMaterialGridSize, corner.GetY() + MacroMaterialGridSize, m_terrainBounds.GetMax().GetZ());

                bool overlapsNew = tileAabb.Overlaps(newRegion);

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
        // If terrainSizeChanged is set, we're going to rebuild everything anyways, so don't do any work here. This early-out also
        // fixes order-of-activation issues.
        if (m_terrainSizeChanged)
        {
            return;
        }

        AZ_Assert(
            m_macroMaterials.contains(entityId),
            "OnTerrainMacroMaterialChanged called for a macro material that TerrainFeatureProcessor isn't tracking. This indicates that either the bus is sending out "
            "Changed announcements for materials that haven't had a OnCreated event sent, or the terrain feature processor isn't properly tracking macro materials.");
        
        const MacroMaterial& macroMaterial = m_macroMaterials[entityId];
        
        ForMacroMaterialsInBounds(macroMaterial.m_data.m_bounds,
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
        macroMaterialShaderData.m_boundsMin = { macroMaterial.m_data.m_bounds.GetMin().GetX(), macroMaterial.m_data.m_bounds.GetMin().GetY() };
        macroMaterialShaderData.m_boundsMax = { macroMaterial.m_data.m_bounds.GetMax().GetX(), macroMaterial.m_data.m_bounds.GetMax().GetY() };
        macroMaterialShaderData.m_colorMapId = macroMaterial.m_colorIndex;
        macroMaterialShaderData.m_normalMapId = macroMaterial.m_normalIndex;
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
    void TerrainMacroMaterialManager::ForMacroMaterialsInBounds(const AZ::Aabb& bounds, Callback callback)
    {
        // Get the macro material bounds relative to the terrain
        float yStart = bounds.GetMin().GetY() - m_terrainBounds.GetMin().GetY();
        float yEnd = bounds.GetMax().GetY() - m_terrainBounds.GetMin().GetY();
        float xStart = bounds.GetMin().GetX() - m_terrainBounds.GetMin().GetX();
        float xEnd = bounds.GetMax().GetX() - m_terrainBounds.GetMin().GetX();

        // Clamp the bounds to the terrain
        uint16_t yStartIdx = yStart > 0.0f ? uint16_t(yStart / MacroMaterialGridSize) : 0;
        uint16_t yEndIdx = yEnd > 0.0f ? AZStd::GetMin<uint16_t>(uint16_t(yEnd / MacroMaterialGridSize) + 1, m_tilesY) : 0;
        uint16_t xStartIdx = xStart > 0.0f ? uint16_t(xStart / MacroMaterialGridSize) : 0;
        uint16_t xEndIdx = xEnd > 0.0f ? AZStd::GetMin<uint16_t>(uint16_t(xEnd / MacroMaterialGridSize) + 1, m_tilesX) : 0;

        AZ::Vector2 gridCorner = AZ::Vector2(
            floor(m_terrainBounds.GetMin().GetX() / MacroMaterialGridSize) * MacroMaterialGridSize,
            floor(m_terrainBounds.GetMin().GetY() / MacroMaterialGridSize) * MacroMaterialGridSize);

        for (uint16_t y = yStartIdx; y < yEndIdx; ++y)
        {
            for (uint16_t x = xStartIdx; x < xEndIdx; ++x)
            {
                uint32_t idx = (y * m_tilesX + x);
                const AZ::Vector2 corner = gridCorner + AZ::Vector2(x * MacroMaterialGridSize, y * MacroMaterialGridSize);
                callback(idx, corner);
            }
        }
    }

    void TerrainMacroMaterialManager::Update(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        if (m_terrainSizeChanged)
        {
            m_terrainSizeChanged = false;

            // Rebuild the macro material tiles from scratch when the world size changes. This could be made more efficient
            // but is fine for now since world resizes are rare.

            RemoveAllImages();
            m_macroMaterials.clear();
            m_materialShaderData.Clear();
            m_materialRefGridShaderData.clear();
            
            m_tilesX = aznumeric_cast<uint16_t>(m_terrainBounds.GetXExtent() / MacroMaterialGridSize) + 1;
            m_tilesY = aznumeric_cast<uint16_t>(m_terrainBounds.GetYExtent() / MacroMaterialGridSize) + 1;
            const uint32_t macroMaterialTileCount = m_tilesX * m_tilesY;
            
            m_materialRefGridShaderData.resize(macroMaterialTileCount);
            MacroMaterialRefs defaultRefs =
            {
                InvalidMacroMaterialRef, InvalidMacroMaterialRef, InvalidMacroMaterialRef, InvalidMacroMaterialRef
            };
            AZStd::fill(m_materialRefGridShaderData.begin(), m_materialRefGridShaderData.end(), defaultRefs);

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

        if (m_bufferNeedsUpdate)
        {
            m_bufferNeedsUpdate = false;
            m_materialDataBuffer.UpdateBuffer(m_materialShaderData.GetRawData(), aznumeric_cast<uint32_t>(m_materialShaderData.GetSize()));
            m_materialRefGridDataBuffer.UpdateBuffer(m_materialRefGridShaderData.data(), aznumeric_cast<uint32_t>(m_materialRefGridShaderData.size()));

            MacroMaterialGridShaderData macroMaterialGridShaderData;
            macroMaterialGridShaderData.m_offset = { m_terrainBounds.GetMin().GetX(), m_terrainBounds.GetMin().GetY() };
            macroMaterialGridShaderData.m_resolution = (m_tilesX << 16) | m_tilesY;
            macroMaterialGridShaderData.m_tileSize = MacroMaterialGridSize;

            if (terrainSrg)
            {   
                m_materialDataBuffer.UpdateSrg(terrainSrg.get());
                m_materialRefGridDataBuffer.UpdateSrg(terrainSrg.get());
                terrainSrg->SetConstant(m_macroMaterialGridIndex, macroMaterialGridShaderData);
            }
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
