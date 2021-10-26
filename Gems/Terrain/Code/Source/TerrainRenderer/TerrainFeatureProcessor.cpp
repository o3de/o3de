/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>

#include <AzCore/Console/Console.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/math.h>

#include <Atom/Utils/Utils.h>

#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Buffer/BufferSystem.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

#include <Atom/Feature/RenderCommon.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] const char* TerrainFPName = "TerrainFeatureProcessor";
        const char* TerrainHeightmapChars = "TerrainHeightmap";
        const char* TerrainDetailChars = "TerrainDetail";
    }

    namespace MaterialInputs
    {
        // Terrain material
        static const char* const HeightmapImage("settings.heightmapImage");
        static const char* const DetailMaterialIdImage("settings.detailMaterialIdImage");
        static const char* const DetailCenter("settings.detailMaterialIdCenter");
        static const char* const DetailAabb("settings.detailAabb");
        static const char* const DetailHalfPixelUv("settings.detailHalfPixelUv");
    }

    namespace DetailMaterialInputs
    {
        static const char* const BaseColorMap("baseColor.textureMap");
        static const char* const BaseColorUseTexture("baseColor.useTexture");
        static const char* const BaseColorFactor("baseColor.factor");
        static const char* const BaseColorBlendMode("baseColor.textureBlendMode");
        static const char* const MetallicMap("metallic.textureMap");
        static const char* const MetallicUseTexture("metallic.useTexture");
        static const char* const MetallicFactor("metallic.factor");
        static const char* const RoughnessMap("roughness.textureMap");
        static const char* const RoughnessUseTexture("roughness.useTexture");
        static const char* const RoughnessFactor("roughness.factor");
        static const char* const RoughnessUpperBound("roughness.lowerBound");
        static const char* const RoughnessLowerBound("roughness.upperBound");
        static const char* const SpecularF0Map("specularF0.textureMap");
        static const char* const SpecularF0UseTexture("specularF0.useTexture");
        static const char* const SpecularF0Factor("specularF0.factor");
        static const char* const NormalMap("normal.textureMap");
        static const char* const NormalUseTexture("normal.useTexture");
        static const char* const NormalFactor("normal.factor");
        static const char* const NormalFlipX("normal.flipX");
        static const char* const NormalFlipY("normal.flipY");
        static const char* const DiffuseOcclusionMap("occlusion.diffuseTextureMap");
        static const char* const DiffuseOcclusionUseTexture("occlusion.diffuseUseTexture");
        static const char* const DiffuseOcclusionFactor("occlusion.diffuseFactor");
        static const char* const HeightMap("parallax.textureMap");
        static const char* const HeightUseTexture("parallax.useTexture");
        static const char* const HeightFactor("parallax.factor");
        static const char* const HeightOffset("parallax.offset");
        static const char* const HeightBlendFactor("parallax.blendFactor");
    }

    namespace ShaderInputs
    {
        static const char* const ModelToWorld("m_modelToWorld");
        static const char* const TerrainData("m_terrainData");
        static const char* const MacroMaterialData("m_macroMaterialData");
        static const char* const MacroMaterialCount("m_macroMaterialCount");
        static const char* const MacroColorMap("m_macroColorMap");
        static const char* const MacroNormalMap("m_macroNormalMap");
    }
    
    AZ_CVAR(bool,
        r_terrainDebugDetailMaterials,
        false,
        [](const bool& value)
        {
            AZ::RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(AZ::Name{ "o_debugDetailMaterialIds" }, AZ::RPI::ShaderOptionValue{ value });
        },
        AZ::ConsoleFunctorFlags::Null,
        "Turns on debugging for detail material ids for terrain."
    );


    void TerrainFeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainFeatureProcessor, AZ::RPI::FeatureProcessor>()
                ->Version(0)
                ;
        }
    }

    void TerrainFeatureProcessor::Activate()
    {
        Initialize();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
        
        m_handleGlobalShaderOptionUpdate = AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler
        {
            [this](const AZ::Name&, AZ::RPI::ShaderOptionValue) { m_forceRebuildDrawPackets = true; }
        };
        AZ::RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);
    }

    void TerrainFeatureProcessor::Initialize()
    {
        // Load the terrain material asynchronously
        const AZStd::string materialFilePath = "Materials/Terrain/DefaultPbrTerrain.azmaterial";
        m_materialAssetLoader = AZStd::make_unique<AZ::RPI::AssetUtils::AsyncAssetLoader>();
        *m_materialAssetLoader = AZ::RPI::AssetUtils::AsyncAssetLoader::Create<AZ::RPI::MaterialAsset>(materialFilePath, 0u,
            [&](AZ::Data::Asset<AZ::Data::AssetData> assetData, bool success) -> void
            {
                const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset = static_cast<AZ::Data::Asset<AZ::RPI::MaterialAsset>>(assetData);
                if (success)
                {
                    m_materialInstance = AZ::RPI::Material::FindOrCreate(assetData);
                    AZ::RPI::MaterialReloadNotificationBus::Handler::BusConnect(materialAsset->GetId());
                    if (!materialAsset->GetObjectSrgLayout())
                    {
                        AZ_Error("TerrainFeatureProcessor", false, "No per-object ShaderResourceGroup found on terrain material.");
                    }
                    else
                    {
                        PrepareMaterialData();
                    }
                }
            }
        );
        if (!InitializePatchModel())
        {
            AZ_Error(TerrainFPName, false, "Failed to create Terrain render buffers!");
            return;
        }
        OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
    }

    void TerrainFeatureProcessor::Deactivate()
    {
        TerrainMacroMaterialNotificationBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AZ::RPI::MaterialReloadNotificationBus::Handler::BusDisconnect();

        m_patchModel = {};
        m_areaData = {};
        m_dirtyRegion = AZ::Aabb::CreateNull();
        m_sectorData.clear();
        m_macroMaterials.Clear();
        m_materialAssetLoader = {};
        m_materialInstance = {};
    }

    void TerrainFeatureProcessor::Render(const AZ::RPI::FeatureProcessor::RenderPacket& packet)
    {
        ProcessSurfaces(packet);
    }

    void TerrainFeatureProcessor::OnTerrainDataDestroyBegin()
    {
        m_areaData = {};
    }
    
    void TerrainFeatureProcessor::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & (TerrainDataChangedMask::HeightData | TerrainDataChangedMask::Settings)) != 0)
        {
            TerrainHeightOrSettingsUpdated(dirtyRegion);
        }
        if ((dataChangedMask & TerrainDataChangedMask::SurfaceData) != 0)
        {
            TerrainSurfaceDataUpdated(dirtyRegion);
        }
    }

    void TerrainFeatureProcessor::TerrainHeightOrSettingsUpdated(const AZ::Aabb& dirtyRegion)
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        const AZ::Aabb& regionToUpdate = dirtyRegion.IsValid() ? dirtyRegion : worldBounds;

        m_dirtyRegion.AddAabb(regionToUpdate);
        m_dirtyRegion.Clamp(worldBounds);

        const AZ::Transform transform = AZ::Transform::CreateTranslation(worldBounds.GetCenter());
        
        AZ::Vector2 queryResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        // Sectors need to be rebuilt if the world bounds change in the x/y, or the sample spacing changes.
        m_areaData.m_rebuildSectors = m_areaData.m_rebuildSectors ||
            m_areaData.m_terrainBounds.GetMin().GetX() != worldBounds.GetMin().GetX() ||
            m_areaData.m_terrainBounds.GetMin().GetY() != worldBounds.GetMin().GetY() ||
            m_areaData.m_terrainBounds.GetMax().GetX() != worldBounds.GetMax().GetX() ||
            m_areaData.m_terrainBounds.GetMax().GetY() != worldBounds.GetMax().GetY() ||
            m_areaData.m_sampleSpacing != queryResolution.GetX();

        m_areaData.m_transform = transform;
        m_areaData.m_terrainBounds = worldBounds;
        m_areaData.m_heightmapImageWidth = aznumeric_cast<uint32_t>(worldBounds.GetXExtent() / queryResolution.GetX());
        m_areaData.m_heightmapImageHeight = aznumeric_cast<uint32_t>(worldBounds.GetYExtent() / queryResolution.GetY());
        m_areaData.m_updateWidth = aznumeric_cast<uint32_t>(m_dirtyRegion.GetXExtent() / queryResolution.GetX());
        m_areaData.m_updateHeight = aznumeric_cast<uint32_t>(m_dirtyRegion.GetYExtent() / queryResolution.GetY());
        // Currently query resolution is multidimensional but the rendering system only supports this changing in one dimension.
        m_areaData.m_sampleSpacing = queryResolution.GetX();
        m_areaData.m_heightmapUpdated = true;
    }

    void TerrainFeatureProcessor::TerrainSurfaceDataUpdated(const AZ::Aabb& dirtyRegion)
    {
        m_dirtyDetailRegion.AddAabb(dirtyRegion);
    }

    void TerrainFeatureProcessor::OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        MacroMaterialData& materialData = FindOrCreateByEntityId(entityId, m_macroMaterials);

        UpdateMacroMaterialData(materialData, newMaterialData);

        // Update all sectors in region.
        ForOverlappingSectors(materialData.m_bounds,
            [&](SectorData& sectorData) {
                if (sectorData.m_macroMaterials.size() < sectorData.m_macroMaterials.max_size())
                {
                    sectorData.m_macroMaterials.push_back(m_macroMaterials.GetIndexForData(&materialData));
                }
            }
        );
    }

    void TerrainFeatureProcessor::OnTerrainMacroMaterialChanged(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        MacroMaterialData& data = FindOrCreateByEntityId(entityId, m_macroMaterials);
        UpdateMacroMaterialData(data, newMaterialData);
    }
    
    void TerrainFeatureProcessor::OnTerrainMacroMaterialRegionChanged(
        AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion)
    {
        MacroMaterialData& materialData = FindOrCreateByEntityId(entityId, m_macroMaterials);
        for (SectorData& sectorData : m_sectorData)
        {
            bool overlapsOld = sectorData.m_aabb.Overlaps(materialData.m_bounds);
            bool overlapsNew = sectorData.m_aabb.Overlaps(newRegion);
            if (overlapsOld && !overlapsNew)
            {
                // Remove the macro material from this sector
                for (uint16_t& idx : sectorData.m_macroMaterials)
                {
                    if (m_macroMaterials.GetData(idx).m_entityId == entityId)
                    {
                        idx = sectorData.m_macroMaterials.back();
                        sectorData.m_macroMaterials.pop_back();
                    }
                }
            }
            else if (overlapsNew && !overlapsOld)
            {
                // Add the macro material to this sector
                if (sectorData.m_macroMaterials.size() < MaxMaterialsPerSector)
                {
                    sectorData.m_macroMaterials.push_back(m_macroMaterials.GetIndexForData(&materialData));
                }
            }
        }
        m_areaData.m_macroMaterialsUpdated = true;
        materialData.m_bounds = newRegion;
    }

    void TerrainFeatureProcessor::OnTerrainMacroMaterialDestroyed(AZ::EntityId entityId)
    {
        const MacroMaterialData* materialData = FindByEntityId(entityId, m_macroMaterials);

        if (materialData)
        {
            uint16_t destroyedMaterialIndex = m_macroMaterials.GetIndexForData(materialData);
            ForOverlappingSectors(materialData->m_bounds,
                [&](SectorData& sectorData) {
                for (uint16_t& idx : sectorData.m_macroMaterials)
                {
                    if (idx == destroyedMaterialIndex)
                    {
                        idx = sectorData.m_macroMaterials.back();
                        sectorData.m_macroMaterials.pop_back();
                    }
                }
            });
        }
        
        m_areaData.m_macroMaterialsUpdated = true;
        RemoveByEntityId(entityId, m_macroMaterials);
    }

    void TerrainFeatureProcessor::OnTerrainSurfaceMaterialMappingCreated(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);

        // Validate that the surface tag is new
        for (DetailMaterialSurface& surface : materialRegion.m_materialsForSurfaces)
        {
            if (surface.m_surfaceTag == surfaceTag)
            {
                AZ_Error(TerrainFPName, false, "Already have a surface material mapping for this surface tag.");
                return;
            }
        }

        uint16_t detailMaterialId = CreateOrUpdateDetailMaterial(material);
        materialRegion.m_materialsForSurfaces.push_back({ surfaceTag, detailMaterialId });
        m_dirtyDetailRegion.AddAabb(materialRegion.m_region);
    }

    void TerrainFeatureProcessor::OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);

        for (DetailMaterialSurface& surface : materialRegion.m_materialsForSurfaces)
        {
            if (surface.m_surfaceTag == surfaceTag)
            {
                if (surface.m_surfaceTag != materialRegion.m_materialsForSurfaces.back().m_surfaceTag)
                {
                    AZStd::swap(surface, materialRegion.m_materialsForSurfaces.back());
                }
                materialRegion.m_materialsForSurfaces.pop_back();
                m_dirtyDetailRegion.AddAabb(materialRegion.m_region);
                return;
            }
        }
        AZ_Error(TerrainFPName, false, "Could not find surface tag to destroy for OnTerrainSurfaceMaterialMappingDestroyed().");
    }

    void TerrainFeatureProcessor::OnTerrainSurfaceMaterialMappingChanged(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);

        bool found = false;
        uint16_t materialId = CreateOrUpdateDetailMaterial(material);
        for (DetailMaterialSurface& surface : materialRegion.m_materialsForSurfaces)
        {
            if (surface.m_surfaceTag == surfaceTag)
            {
                found = true;
                surface.m_detailMaterialId = materialId;
                break;
            }
        }

        if (!found)
        {
            materialRegion.m_materialsForSurfaces.push_back({ surfaceTag, materialId });
        }
        m_dirtyDetailRegion.AddAabb(materialRegion.m_region);
    }

    void TerrainFeatureProcessor::OnTerrainSurfaceMaterialMappingRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);
        materialRegion.m_region = newRegion;
        m_dirtyDetailRegion.AddAabb(oldRegion);
        m_dirtyDetailRegion.AddAabb(newRegion);
    }

    uint16_t TerrainFeatureProcessor::CreateOrUpdateDetailMaterial(MaterialInstance material)
    {
        static constexpr uint16_t InvalidDetailMaterial = 0xFFFF;
        uint16_t detailMaterialId = InvalidDetailMaterial;

        for (DetailMaterialData& detailMaterial : m_detailMaterials.GetDataVector())
        {
            if (detailMaterial.m_assetId == material->GetAssetId())
            {
                UpdateDetailMaterialData(detailMaterial, material);
                detailMaterialId = m_detailMaterials.GetIndexForData(&detailMaterial);
                break;
            }
        }

        if (detailMaterialId == InvalidDetailMaterial)
        {
            detailMaterialId = m_detailMaterials.GetFreeSlotIndex();
            UpdateDetailMaterialData(m_detailMaterials.GetData(detailMaterialId), material);
        }
        return detailMaterialId;
    }

    void TerrainFeatureProcessor::UpdateDetailMaterialData(DetailMaterialData& materialData, MaterialInstance material)
    {
        if (materialData.m_materialChangeId != material->GetCurrentChangeId())
        {
            materialData = DetailMaterialData();
            DetailTextureFlags& flags = materialData.m_properties.m_flags;
            materialData.m_materialChangeId = material->GetCurrentChangeId();
            materialData.m_assetId = material->GetAssetId();
            
            auto getIndex = [&](const char* const indexName) -> AZ::RPI::MaterialPropertyIndex
            {
                const AZ::RPI::MaterialPropertyIndex index = material->FindPropertyIndex(AZ::Name(indexName));
                AZ_Warning(TerrainFPName, index.IsValid(), "Failed to find shader input constant %s.", indexName);
                return index;
            };

            auto applyProperty = [&](const char* const indexName, auto& ref) -> void
            {
                const auto index = getIndex(indexName);
                if (index.IsValid())
                {
                    using TypeRefRemoved = AZStd::remove_cvref_t<decltype(ref)>;
                    ref = material->GetPropertyValue(index).GetValue<TypeRefRemoved>();
                }
            };
            
            auto applyFlag = [&](const char* const indexName, DetailTextureFlags flagToSet) -> void
            {
                const auto index = getIndex(indexName);
                if (index.IsValid())
                {
                    bool flagValue = material->GetPropertyValue(index).GetValue<bool>();
                    flags = DetailTextureFlags(flagValue ? flags | flagToSet : flags);
                }
            };

            auto getEnumName = [&](const char* const indexName) -> const AZStd::string_view
            {
                const auto index = getIndex(indexName);
                if (index.IsValid())
                {
                    uint32_t enumIndex = material->GetPropertyValue(index).GetValue<uint32_t>();
                    const AZ::Name& enumName = material->GetMaterialPropertiesLayout()->GetPropertyDescriptor(index)->GetEnumName(enumIndex);
                    return enumName.GetStringView();
                }
                return "";
            };

            using namespace DetailMaterialInputs;
            applyProperty(BaseColorMap, materialData.m_colorImage);
            applyFlag(BaseColorUseTexture, DetailTextureFlags::UseTextureBaseColor);
            applyProperty(BaseColorFactor, materialData.m_properties.m_baseColorFactor);

            const AZStd::string_view& blendModeString = getEnumName(BaseColorBlendMode);
            if (blendModeString == "Multiply")
            {
                flags = DetailTextureFlags(flags | DetailTextureFlags::BlendModeMultiply);
            }
            else if (blendModeString == "LinearLight")
            {
                flags = DetailTextureFlags(flags | DetailTextureFlags::BlendModeLinearLight);
            }
            else if (blendModeString == "Lerp")
            {
                flags = DetailTextureFlags(flags | DetailTextureFlags::BlendModeLerp);
            }
            else if (blendModeString == "Overlay")
            {
                flags = DetailTextureFlags(flags | DetailTextureFlags::BlendModeOverlay);
            }
            
            applyProperty(MetallicMap, materialData.m_metalnessImage);
            applyFlag(MetallicUseTexture, DetailTextureFlags::UseTextureMetallic);
            applyProperty(MetallicFactor, materialData.m_properties.m_metalFactor);
            
            applyProperty(RoughnessMap, materialData.m_roughnessImage);
            applyFlag(RoughnessUseTexture, DetailTextureFlags::UseTextureRoughness);

            if ((flags & DetailTextureFlags::UseTextureRoughness) > 0)
            {
                float lowerBound = 0.0;
                float upperBound = 1.0;
                applyProperty(RoughnessLowerBound, lowerBound);
                applyProperty(RoughnessUpperBound, upperBound);
                materialData.m_properties.m_roughnessBias = lowerBound;
                materialData.m_properties.m_roughnessScale = upperBound - lowerBound;
            }
            else
            {
                materialData.m_properties.m_roughnessBias = 0.0;
                applyProperty(RoughnessFactor, materialData.m_properties.m_roughnessScale);
            }
            
            applyProperty(SpecularF0Map, materialData.m_specularF0Image);
            applyFlag(SpecularF0UseTexture, DetailTextureFlags::UseTextureSpecularF0);
            applyProperty(SpecularF0Factor, materialData.m_properties.m_specularF0Factor);
            
            applyProperty(NormalMap, materialData.m_normalImage);
            applyFlag(NormalUseTexture, DetailTextureFlags::UseTextureNormal);
            applyProperty(NormalFactor, materialData.m_properties.m_normalFactor);
            applyFlag(NormalFlipX, DetailTextureFlags::FlipNormalX);
            applyFlag(NormalFlipY, DetailTextureFlags::FlipNormalY);
            
            applyProperty(DiffuseOcclusionMap, materialData.m_occlusionImage);
            applyFlag(DiffuseOcclusionUseTexture, DetailTextureFlags::UseTextureOcclusion);
            applyProperty(DiffuseOcclusionFactor, materialData.m_properties.m_occlusionFactor);
            
            applyProperty(HeightMap, materialData.m_heightImage);
            applyFlag(HeightUseTexture, DetailTextureFlags::UseTextureHeight);
            applyProperty(HeightFactor, materialData.m_properties.m_heightFactor);
            applyProperty(HeightOffset, materialData.m_properties.m_heightOffset);
            applyProperty(HeightBlendFactor, materialData.m_properties.m_heightBlendFactor);

        }
    }

    void TerrainFeatureProcessor::CheckUpdateDetailTexture(const Aabb2i& newBounds, const Vector2i& newCenter)
    {
        if (!m_detailTextureImage)
        {
            // If the m_detailTextureImage doesn't exist, create it and populate the entire texture

            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, DetailTextureSize, DetailTextureSize, AZ::RHI::Format::R8G8B8A8_UINT
            );
            const AZ::Name TerrainDetailName = AZ::Name(TerrainDetailChars);
            m_detailTextureImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainDetailName, nullptr, nullptr);
            AZ_Error(TerrainFPName, m_detailTextureImage, "Failed to initialize the detail texture image.");
            
            UpdateDetailTexture(newBounds, newBounds, newCenter);
        }
        else
        {
            // If the new bounds of the detail texture are different than the old bounds, then the edges of the texture need to be updated.

            int32_t offsetX = m_detailTextureBounds.m_min.m_x - newBounds.m_min.m_x;

            // Horizontal edge update
            if (newBounds.m_min.m_x != m_detailTextureBounds.m_min.m_x)
            {
                Aabb2i updateBounds;
                if (newBounds.m_min.m_x < m_detailTextureBounds.m_min.m_x)
                {
                    updateBounds.m_min.m_x = newBounds.m_min.m_x;
                    updateBounds.m_max.m_x = m_detailTextureBounds.m_min.m_x;
                }
                else
                {
                    updateBounds.m_min.m_x = m_detailTextureBounds.m_max.m_x;
                    updateBounds.m_max.m_x = newBounds.m_max.m_x;
                }
                updateBounds.m_min.m_y = newBounds.m_min.m_y;
                updateBounds.m_max.m_y = newBounds.m_max.m_y;
                UpdateDetailTexture(updateBounds, newBounds, newCenter);
            }

            // Vertical edge update
            if (newBounds.m_min.m_y != m_detailTextureBounds.m_min.m_y)
            {
                Aabb2i updateBounds;
                // Don't update areas that have already been updated in the horizontal update.
                updateBounds.m_min.m_x = newBounds.m_min.m_x + AZ::GetMax(0, offsetX);
                updateBounds.m_max.m_x = newBounds.m_max.m_x + AZ::GetMin(0, offsetX);
                if (newBounds.m_min.m_y < m_detailTextureBounds.m_min.m_y)
                {
                    updateBounds.m_min.m_y = newBounds.m_min.m_y;
                    updateBounds.m_max.m_y = m_detailTextureBounds.m_min.m_y;
                }
                else
                {
                    updateBounds.m_min.m_y = m_detailTextureBounds.m_max.m_y;
                    updateBounds.m_max.m_y = newBounds.m_max.m_y;
                }
                UpdateDetailTexture(updateBounds, newBounds, newCenter);
            }

            if (m_dirtyDetailRegion.IsValid())
            {
                // If any regions are marked as dirty, then they should be updated.

                AZ::Vector3 currentMin = AZ::Vector3(newBounds.m_min.m_x * DetailTextureScale, newBounds.m_min.m_y * DetailTextureScale, -0.5f);
                AZ::Vector3 currentMax = AZ::Vector3(newBounds.m_max.m_x * DetailTextureScale, newBounds.m_max.m_y * DetailTextureScale, 0.5f);
                AZ::Aabb detailTextureCoverage = AZ::Aabb::CreateFromMinMax(currentMin, currentMax);
                AZ::Vector3 previousMin = AZ::Vector3(m_detailTextureBounds.m_min.m_x * DetailTextureScale, m_detailTextureBounds.m_min.m_y * DetailTextureScale, -0.5f);
                AZ::Vector3 previousMax = AZ::Vector3(m_detailTextureBounds.m_max.m_x * DetailTextureScale, m_detailTextureBounds.m_max.m_y * DetailTextureScale, 0.5f);
                AZ::Aabb previousCoverage = AZ::Aabb::CreateFromMinMax(previousMin, previousMax);

                // Area of texture not already updated by camera movement above.
                AZ::Aabb clampedCoverage = previousCoverage.GetClamped(detailTextureCoverage);

                // Clamp the dirty region to the area of the detail texture that is visible and not already updated.
                clampedCoverage.Clamp(m_dirtyDetailRegion);

                if (clampedCoverage.IsValid())
                {
                    Aabb2i updateBounds;
                    updateBounds.m_min.m_x = aznumeric_cast<int32_t>(AZStd::roundf(clampedCoverage.GetMin().GetX() / DetailTextureScale));
                    updateBounds.m_min.m_y = aznumeric_cast<int32_t>(AZStd::roundf(clampedCoverage.GetMin().GetY() / DetailTextureScale));
                    updateBounds.m_max.m_x = aznumeric_cast<int32_t>(AZStd::roundf(clampedCoverage.GetMax().GetX() / DetailTextureScale));
                    updateBounds.m_max.m_y = aznumeric_cast<int32_t>(AZStd::roundf(clampedCoverage.GetMax().GetY() / DetailTextureScale));
                    if (updateBounds.m_min.m_x < updateBounds.m_max.m_x && updateBounds.m_min.m_y < updateBounds.m_max.m_y)
                    {
                        UpdateDetailTexture(updateBounds, newBounds, newCenter);
                    }
                }
            }
        }

    }

    uint8_t TerrainFeatureProcessor::CalculateUpdateRegions(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel,
        AZStd::array<Aabb2i, 4>& textureSpaceAreas, AZStd::array<Aabb2i, 4>& scaledWorldSpaceAreas)
    {
        Vector2i centerOffset = { centerPixel.m_x - DetailTextureSizeHalf, centerPixel.m_y - DetailTextureSizeHalf };

        int32_t quadrantXOffset = centerPixel.m_x < DetailTextureSizeHalf ? DetailTextureSize : -DetailTextureSize;
        int32_t quadrantYOffset = centerPixel.m_y < DetailTextureSizeHalf ? DetailTextureSize : -DetailTextureSize;

        uint8_t numQuadrants = 0;

        // For each of the 4 quadrants:
        auto calculateQuadrant = [&](Vector2i quadrantOffset)
        {
            Aabb2i offsetUpdateArea = updateArea + centerOffset + quadrantOffset;
            Aabb2i updateSectionBounds = textureBounds.GetClamped(offsetUpdateArea);
            if (updateSectionBounds.IsValid())
            {
                textureSpaceAreas[numQuadrants] = updateSectionBounds - textureBounds.m_min;
                scaledWorldSpaceAreas[numQuadrants] = updateSectionBounds - centerOffset - quadrantOffset;
                ++numQuadrants;
            }
        };

        calculateQuadrant({ 0, 0 });
        calculateQuadrant({ quadrantXOffset, 0 });
        calculateQuadrant({ 0, quadrantYOffset });
        calculateQuadrant({ quadrantXOffset, quadrantYOffset });

        return numQuadrants;
    }

    void TerrainFeatureProcessor::UpdateDetailTexture(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel)
    {
        if (!m_detailTextureImage)
        {
            return;
        }

        struct DetailMaterialPixel
        {
            uint8_t m_material1{ 255 };
            uint8_t m_material2{ 255 };
            uint8_t m_blend{ 0 }; // 0 = full weight on material1, 255 = full weight on material2
            uint8_t m_padding{ 0 };
        };

        // Because the center of the detail texture may be offset, each update area may actually need to be split into
        // up to 4 separate update areas in each sector of the quadrant.
        AZStd::array<Aabb2i, 4> textureSpaceAreas;
        AZStd::array<Aabb2i, 4> scaledWorldSpaceAreas;
        uint8_t updateAreaCount = CalculateUpdateRegions(updateArea, textureBounds, centerPixel, textureSpaceAreas, scaledWorldSpaceAreas);

        // Pull the data for each area updated and use it to construct an update for the detail material id texture.
        for (uint8_t i = 0; i < updateAreaCount; ++i)
        {
            const Aabb2i& quadrantTextureArea = textureSpaceAreas[i];
            const Aabb2i& quadrantWorldArea = scaledWorldSpaceAreas[i];

            AZStd::vector<DetailMaterialPixel> pixels;
            pixels.resize((quadrantWorldArea.m_max.m_x - quadrantWorldArea.m_min.m_x) * (quadrantWorldArea.m_max.m_y - quadrantWorldArea.m_min.m_y));
            uint32_t index = 0;

            for (int yPos = quadrantWorldArea.m_min.m_y; yPos < quadrantWorldArea.m_max.m_y; ++yPos)
            {
                for (int xPos = quadrantWorldArea.m_min.m_x; xPos < quadrantWorldArea.m_max.m_x; ++xPos)
                {
                    AZ::Vector2 position = AZ::Vector2(xPos * DetailTextureScale, yPos * DetailTextureScale);
                    AzFramework::SurfaceData::SurfaceTagWeightList surfaceWeights;
                    AzFramework::Terrain::TerrainDataRequestBus::Broadcast(&AzFramework::Terrain::TerrainDataRequests::GetSurfaceWeightsFromVector2, position, surfaceWeights, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, nullptr);

                    // Store the top two surface weights in the texture with m_blend storing the relative weight.
                    bool isFirstMaterial = true;
                    float firstWeight = 0.0f;
                    for (const auto& surfaceTagWeight : surfaceWeights)
                    {
                        if (surfaceTagWeight.m_weight > 0.0f)
                        {
                            AZ::Crc32 surfaceType = surfaceTagWeight.m_surfaceType;
                            uint16_t materialId = GetDetailMaterialForSurfaceTypeAndPosition(surfaceType, position);
                            if (materialId != m_detailMaterials.NoFreeSlot && materialId < 255)
                            {
                                if (isFirstMaterial)
                                {
                                    pixels.at(index).m_material1 = aznumeric_cast<uint8_t>(materialId);
                                    firstWeight = surfaceTagWeight.m_weight;
                                    // m_blend only needs to be calculated is material 2 is found, otherwise the initial value of 0 is correct.
                                    isFirstMaterial = false;
                                }
                                else
                                {
                                    pixels.at(index).m_material2 = aznumeric_cast<uint8_t>(materialId);
                                    float totalWeight = firstWeight + surfaceTagWeight.m_weight;
                                    float blendWeight = 1.0f - (firstWeight / totalWeight);
                                    pixels.at(index).m_blend = aznumeric_cast<uint8_t>(AZStd::round(blendWeight * 255.0f));
                                    break;
                                }
                            }
                        }
                        else
                        {
                            break; // since the list is ordered, no other materials are in the list with positive weights.
                        }
                    }
                    ++index;
                }
            }

            const int32_t left = quadrantTextureArea.m_min.m_x;
            const int32_t top = quadrantTextureArea.m_min.m_y;
            const int32_t width = quadrantTextureArea.m_max.m_x - quadrantTextureArea.m_min.m_x;
            const int32_t height = quadrantTextureArea.m_max.m_y - quadrantTextureArea.m_min.m_y;

            AZ::RHI::ImageUpdateRequest imageUpdateRequest;
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = aznumeric_cast<uint32_t>(left);
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = aznumeric_cast<uint32_t>(top);
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = width * sizeof(DetailMaterialPixel);
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = width * height * sizeof(DetailMaterialPixel);
            imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = height;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_width = width;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_height = height;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_depth = 1;
            imageUpdateRequest.m_sourceData = pixels.data();
            imageUpdateRequest.m_image = m_detailTextureImage->GetRHIImage();

            m_detailTextureImage->UpdateImageContents(imageUpdateRequest);
        }
    }
    
    uint16_t TerrainFeatureProcessor::GetDetailMaterialForSurfaceTypeAndPosition(AZ::Crc32 surfaceType, const AZ::Vector2& position)
    {
        for (const auto& materialRegion : m_detailMaterialRegions.GetDataVector())
        {
            if (materialRegion.m_region.Contains(AZ::Vector3(position.GetX(), position.GetY(), 0.0f)))
            {
                for (const auto& materialSurface : materialRegion.m_materialsForSurfaces)
                {
                    if (materialSurface.m_surfaceTag == surfaceType)
                    {
                        return materialSurface.m_detailMaterialId;
                    }
                }
            }
        }
        return m_detailMaterials.NoFreeSlot;
    }

    void TerrainFeatureProcessor::UpdateTerrainData()
    {
        uint32_t width = m_areaData.m_updateWidth;
        uint32_t height = m_areaData.m_updateHeight;
        const AZ::Aabb& worldBounds = m_areaData.m_terrainBounds;
        const float queryResolution = m_areaData.m_sampleSpacing;

        const AZ::RHI::Size worldSize = AZ::RHI::Size(m_areaData.m_heightmapImageWidth, m_areaData.m_heightmapImageHeight, 1);

        if (!m_areaData.m_heightmapImage || m_areaData.m_heightmapImage->GetDescriptor().m_size != worldSize)
        {
            // World size changed, so the whole world needs updating.
            width = worldSize.m_width;
            height = worldSize.m_height;
            m_dirtyRegion = worldBounds;

            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, width, height, AZ::RHI::Format::R16_UNORM
            );

            const AZ::Name TerrainHeightmapName = AZ::Name(TerrainHeightmapChars);
            m_areaData.m_heightmapImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainHeightmapName, nullptr, nullptr);
            AZ_Error(TerrainFPName, m_areaData.m_heightmapImage, "Failed to initialize the heightmap image.");
        }

        AZStd::vector<uint16_t> pixels;
        pixels.reserve(width * height);

        {
            // Block other threads from accessing the surface data bus while we are in GetHeightFromFloats (which may call into the SurfaceData bus).
            // We lock our surface data mutex *before* checking / setting "isRequestInProgress" so that we prevent race conditions
            // that create false detection of cyclic dependencies when multiple requests occur on different threads simultaneously.
            // (One case where this was previously able to occur was in rapid updating of the Preview widget on the
            // GradientSurfaceDataComponent in the Editor when moving the threshold sliders back and forth rapidly)

            auto& surfaceDataContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
            typename SurfaceData::SurfaceDataSystemRequestBus::Context::DispatchLockGuard scopeLock(surfaceDataContext.m_contextMutex);

            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    bool terrainExists = true;
                    float terrainHeight = 0.0f;
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        terrainHeight, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats,
                        (x * queryResolution) + m_dirtyRegion.GetMin().GetX(),
                        (y * queryResolution) + m_dirtyRegion.GetMin().GetY(),
                        AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT,
                        &terrainExists);

                    const float clampedHeight = AZ::GetClamp((terrainHeight - worldBounds.GetMin().GetZ()) / worldBounds.GetExtents().GetZ(), 0.0f, 1.0f);
                    const float expandedHeight = AZStd::roundf(clampedHeight * AZStd::numeric_limits<uint16_t>::max());
                    const uint16_t uint16Height = aznumeric_cast<uint16_t>(expandedHeight);

                    pixels.push_back(uint16Height);
                }
            }
        }

        if (m_areaData.m_heightmapImage)
        {
            const float left = (m_dirtyRegion.GetMin().GetX() - worldBounds.GetMin().GetX()) / queryResolution;
            const float top = (m_dirtyRegion.GetMin().GetY() - worldBounds.GetMin().GetY()) / queryResolution;
            AZ::RHI::ImageUpdateRequest imageUpdateRequest;
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = aznumeric_cast<uint32_t>(left);
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = aznumeric_cast<uint32_t>(top);
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = width * sizeof(uint16_t);
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = width * height * sizeof(uint16_t);
            imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = height;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_width = width;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_height = height;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_depth = 1;
            imageUpdateRequest.m_sourceData = pixels.data();
            imageUpdateRequest.m_image = m_areaData.m_heightmapImage->GetRHIImage();

            m_areaData.m_heightmapImage->UpdateImageContents(imageUpdateRequest);
        }
        
        m_dirtyRegion = AZ::Aabb::CreateNull();
    }

    void TerrainFeatureProcessor::PrepareMaterialData()
    {   
        const auto layout = m_materialInstance->GetAsset()->GetObjectSrgLayout();

        m_modelToWorldIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::ModelToWorld));
        AZ_Error(TerrainFPName, m_modelToWorldIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::ModelToWorld);

        m_terrainDataIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::TerrainData));
        AZ_Error(TerrainFPName, m_terrainDataIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::TerrainData);

        m_macroMaterialDataIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::MacroMaterialData));
        AZ_Error(TerrainFPName, m_macroMaterialDataIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::MacroMaterialData);
        
        m_macroMaterialCountIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::MacroMaterialCount));
        AZ_Error(TerrainFPName, m_macroMaterialCountIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::MacroMaterialCount);

        m_macroColorMapIndex = layout->FindShaderInputImageIndex(AZ::Name(ShaderInputs::MacroColorMap));
        AZ_Error(TerrainFPName, m_macroColorMapIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::MacroColorMap);

        m_macroNormalMapIndex = layout->FindShaderInputImageIndex(AZ::Name(ShaderInputs::MacroNormalMap));
        AZ_Error(TerrainFPName, m_macroNormalMapIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::MacroNormalMap);
        
        m_heightmapPropertyIndex = m_materialInstance->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(MaterialInputs::HeightmapImage));
        AZ_Error(TerrainFPName, m_heightmapPropertyIndex.IsValid(), "Failed to find material input constant %s.", MaterialInputs::HeightmapImage);
        
        m_detailMaterialIdPropertyIndex = m_materialInstance->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(MaterialInputs::DetailMaterialIdImage));
        AZ_Error(TerrainFPName, m_detailMaterialIdPropertyIndex.IsValid(), "Failed to find material input constant %s.", MaterialInputs::DetailMaterialIdImage);
        
        m_detailCenterPropertyIndex = m_materialInstance->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(MaterialInputs::DetailCenter));
        AZ_Error(TerrainFPName, m_detailCenterPropertyIndex.IsValid(), "Failed to find material input constant %s.", MaterialInputs::DetailCenter);

        m_detailAabbPropertyIndex = m_materialInstance->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(MaterialInputs::DetailAabb));
        AZ_Error(TerrainFPName, m_detailAabbPropertyIndex.IsValid(), "Failed to find material input constant %s.", MaterialInputs::DetailAabb);
        
        m_detailHalfPixelUvPropertyIndex = m_materialInstance->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(MaterialInputs::DetailHalfPixelUv));
        AZ_Error(TerrainFPName, m_detailHalfPixelUvPropertyIndex.IsValid(), "Failed to find material input constant %s.", MaterialInputs::DetailHalfPixelUv);

        // Find any macro materials that have already been created.
        TerrainMacroMaterialRequestBus::EnumerateHandlers(
            [&](TerrainMacroMaterialRequests* handler)
            {
                MacroMaterialData macroMaterial = handler->GetTerrainMacroMaterialData();
                AZ::EntityId entityId = *(Terrain::TerrainMacroMaterialRequestBus::GetCurrentBusId());
                OnTerrainMacroMaterialCreated(entityId, macroMaterial);
                return true;
            }
        );
        TerrainMacroMaterialNotificationBus::Handler::BusConnect();
        
        // Find any detail material areas that have already been created.
        TerrainAreaMaterialRequestBus::EnumerateHandlers(
            [&](TerrainAreaMaterialRequests* handler)
            {
                const AZ::Aabb& bounds = handler->GetTerrainSurfaceMaterialRegion();
                const AZStd::vector<TerrainSurfaceMaterialMapping> materialMappings = handler->GetSurfaceMaterialMappings();
                AZ::EntityId entityId = *(Terrain::TerrainAreaMaterialRequestBus::GetCurrentBusId());
                
                DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);
                materialRegion.m_region = bounds;

                for (const auto& materialMapping : materialMappings)
                {
                    if (materialMapping.m_materialInstance)
                    {
                        OnTerrainSurfaceMaterialMappingCreated(entityId, materialMapping.m_surfaceTag, materialMapping.m_materialInstance);
                    }
                }
                return true;
            }
        );
        TerrainAreaMaterialNotificationBus::Handler::BusConnect();

    }

    void TerrainFeatureProcessor::UpdateMacroMaterialData(MacroMaterialData& macroMaterialData, const MacroMaterialData& newMaterialData)
    {
        macroMaterialData = newMaterialData;

        if (macroMaterialData.m_bounds.IsValid())
        {
            m_areaData.m_macroMaterialsUpdated = true;
        }
    }

    void TerrainFeatureProcessor::ProcessSurfaces(const FeatureProcessor::RenderPacket& process)
    {
        AZ_PROFILE_FUNCTION(AzRender);
        
        const AZ::Aabb& terrainBounds = m_areaData.m_terrainBounds;

        if (!terrainBounds.IsValid())
        {
            return;
        }

        if (m_materialInstance && m_materialInstance->CanCompile())
        {
            if (m_areaData.m_rebuildSectors)
            {
                // Something about the whole world changed, so the sectors need to be rebuilt

                m_areaData.m_rebuildSectors = false;

                m_sectorData.clear();
                const float xFirstPatchStart = terrainBounds.GetMin().GetX() - fmod(terrainBounds.GetMin().GetX(), GridMeters);
                const float xLastPatchStart = terrainBounds.GetMax().GetX() - fmod(terrainBounds.GetMax().GetX(), GridMeters);
                const float yFirstPatchStart = terrainBounds.GetMin().GetY() - fmod(terrainBounds.GetMin().GetY(), GridMeters);
                const float yLastPatchStart = terrainBounds.GetMax().GetY() - fmod(terrainBounds.GetMax().GetY(), GridMeters);
            
                const auto& materialAsset = m_materialInstance->GetAsset();
                const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

                for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += GridMeters)
                {
                    for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += GridMeters)
                    {
                        auto objectSrg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                        if (!objectSrg)
                        {
                            AZ_Warning("TerrainFeatureProcessor", false, "Failed to create a new shader resource group, skipping.");
                            continue;
                        }
                    
                        m_sectorData.push_back();
                        SectorData& sectorData = m_sectorData.back();

                        for (auto& lod : m_patchModel->GetLods())
                        {
                            AZ::RPI::ModelLod& modelLod = *lod.get();
                            sectorData.m_drawPackets.emplace_back(modelLod, 0, m_materialInstance, objectSrg);
                            AZ::RPI::MeshDrawPacket& drawPacket = sectorData.m_drawPackets.back();

                            // set the shader option to select forward pass IBL specular if necessary
                            if (!drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ false }))
                            {
                                AZ_Warning("MeshDrawPacket", false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                            }
                            const uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                            drawPacket.SetStencilRef(stencilRef);
                            drawPacket.Update(*GetParentScene(), true);
                        }

                        sectorData.m_aabb =
                            AZ::Aabb::CreateFromMinMax(
                                AZ::Vector3(xPatch, yPatch, terrainBounds.GetMin().GetZ()),
                                AZ::Vector3(xPatch + GridMeters, yPatch + GridMeters, terrainBounds.GetMax().GetZ())
                            );
                        sectorData.m_srg = objectSrg;
                    }
                }

                if (m_areaData.m_macroMaterialsUpdated)
                {
                    // sectors were rebuilt, so any cached macro material data needs to be regenerated
                    for (SectorData& sectorData : m_sectorData)
                    {
                        for (MacroMaterialData& macroMaterialData : m_macroMaterials.GetDataVector())
                        {
                            if (macroMaterialData.m_bounds.Overlaps(sectorData.m_aabb))
                            {
                                sectorData.m_macroMaterials.push_back(m_macroMaterials.GetIndexForData(&macroMaterialData));
                                if (sectorData.m_macroMaterials.size() == MaxMaterialsPerSector)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else if (m_forceRebuildDrawPackets)
            {
                for (auto& sectorData : m_sectorData)
                {
                    for (auto& drawPacket : sectorData.m_drawPackets)
                    {
                        drawPacket.Update(*GetParentScene(), true);
                    }
                }
            }
            m_forceRebuildDrawPackets = false;

            if (m_areaData.m_heightmapUpdated)
            {
                UpdateTerrainData();
                
                const AZ::Data::Instance<AZ::RPI::Image> heightmapImage = m_areaData.m_heightmapImage; // cast StreamingImage to Image
                m_materialInstance->SetPropertyValue(m_heightmapPropertyIndex, heightmapImage);
            }
            
            AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();
            for (auto& view : process.m_views)
            {
                if ((view->GetUsageFlags() & AZ::RPI::View::UsageFlags::UsageCamera) > 0)
                {
                    cameraPosition = view->GetCameraTransform().GetTranslation();
                    break;
                }
            }

            if (m_dirtyDetailRegion.IsValid() || !cameraPosition.IsClose(m_previousCameraPosition))
            {
                int32_t newDetailTexturePosX = aznumeric_cast<int32_t>(AZStd::roundf(cameraPosition.GetX() / DetailTextureScale));
                int32_t newDetailTexturePosY = aznumeric_cast<int32_t>(AZStd::roundf(cameraPosition.GetY() / DetailTextureScale));
                
                Aabb2i newBounds;
                newBounds.m_min.m_x = newDetailTexturePosX - DetailTextureSizeHalf;
                newBounds.m_min.m_y = newDetailTexturePosY - DetailTextureSizeHalf;
                newBounds.m_max.m_x = newDetailTexturePosX + DetailTextureSizeHalf;
                newBounds.m_max.m_y = newDetailTexturePosY + DetailTextureSizeHalf;

                // Use modulo to find the center point in texture space. Care must be taken so negative values are
                // handled appropriately (ie, we want -1 % 1024 to equal 1023, not -1)
                Vector2i newCenter;
                newCenter.m_x = (DetailTextureSize + (newDetailTexturePosX % DetailTextureSize)) % DetailTextureSize;
                newCenter.m_y = (DetailTextureSize + (newDetailTexturePosY % DetailTextureSize)) % DetailTextureSize;

                CheckUpdateDetailTexture(newBounds, newCenter);
                
                m_detailTextureBounds = newBounds;
                m_dirtyDetailRegion = AZ::Aabb::CreateNull();

                m_previousCameraPosition = cameraPosition;
                const AZ::Data::Instance<AZ::RPI::Image> detailTextureImage = m_detailTextureImage; // cast StreamingImage to Image
                m_materialInstance->SetPropertyValue(m_detailMaterialIdPropertyIndex, detailTextureImage);

                AZ::Vector4 detailAabb = AZ::Vector4(
                    m_detailTextureBounds.m_min.m_x * DetailTextureScale,
                    m_detailTextureBounds.m_min.m_y * DetailTextureScale,
                    m_detailTextureBounds.m_max.m_x * DetailTextureScale,
                    m_detailTextureBounds.m_max.m_y * DetailTextureScale
                );
                m_materialInstance->SetPropertyValue(m_detailAabbPropertyIndex, detailAabb);
                m_materialInstance->SetPropertyValue(m_detailHalfPixelUvPropertyIndex, 0.5f / DetailTextureSize);

                AZ::Vector2 detailUvOffset = AZ::Vector2(float(newCenter.m_x) / DetailTextureSize, float(newCenter.m_y) / DetailTextureSize);
                m_materialInstance->SetPropertyValue(m_detailCenterPropertyIndex, detailUvOffset);
            }

            if (m_areaData.m_heightmapUpdated || m_areaData.m_macroMaterialsUpdated)
            {
                // Currently when anything in the heightmap changes we're updating all the srgs, but this could probably
                // be optimized to only update the srgs that changed.

                m_areaData.m_heightmapUpdated = false;
                m_areaData.m_macroMaterialsUpdated = false;

                for (SectorData& sectorData : m_sectorData)
                {
                    ShaderTerrainData terrainDataForSrg;

                    const float xPatch = sectorData.m_aabb.GetMin().GetX();
                    const float yPatch = sectorData.m_aabb.GetMin().GetY();

                    terrainDataForSrg.m_uvMin = {
                        (xPatch - terrainBounds.GetMin().GetX()) / terrainBounds.GetXExtent(),
                        (yPatch - terrainBounds.GetMin().GetY()) / terrainBounds.GetYExtent()
                    };
                    
                    terrainDataForSrg.m_uvMax = {
                        ((xPatch + GridMeters) - terrainBounds.GetMin().GetX()) / terrainBounds.GetXExtent(),
                        ((yPatch + GridMeters) - terrainBounds.GetMin().GetY()) / terrainBounds.GetYExtent()
                    };

                    terrainDataForSrg.m_uvStep =
                    {
                        1.0f / m_areaData.m_heightmapImageWidth,
                        1.0f / m_areaData.m_heightmapImageHeight,
                    };

                    AZ::Transform transform = m_areaData.m_transform;
                    transform.SetTranslation(xPatch, yPatch, m_areaData.m_transform.GetTranslation().GetZ());

                    terrainDataForSrg.m_sampleSpacing = m_areaData.m_sampleSpacing;
                    terrainDataForSrg.m_heightScale = terrainBounds.GetZExtent();

                    sectorData.m_srg->SetConstant(m_terrainDataIndex, terrainDataForSrg);

                    AZStd::array<ShaderMacroMaterialData, MaxMaterialsPerSector> macroMaterialData;
                    for (uint32_t i = 0; i < sectorData.m_macroMaterials.size(); ++i)
                    {
                        const MacroMaterialData& materialData = m_macroMaterials.GetData(sectorData.m_macroMaterials.at(i));
                        ShaderMacroMaterialData& shaderData = macroMaterialData.at(i);
                        const AZ::Aabb& materialBounds = materialData.m_bounds;

                        // Use reverse coordinates (1 - y) for the y direction so that the lower left corner of the macro material images
                        // map to the lower left corner in world space.  This will match up with the height uv coordinate mapping.
                        shaderData.m_uvMin = {
                            (xPatch - materialBounds.GetMin().GetX()) / materialBounds.GetXExtent(),
                            1.0f - ((yPatch - materialBounds.GetMin().GetY()) / materialBounds.GetYExtent())
                        };
                        shaderData.m_uvMax = {
                            ((xPatch + GridMeters) - materialBounds.GetMin().GetX()) / materialBounds.GetXExtent(),
                            1.0f - (((yPatch + GridMeters) - materialBounds.GetMin().GetY()) / materialBounds.GetYExtent())
                        };
                        shaderData.m_normalFactor = materialData.m_normalFactor;
                        shaderData.m_flipNormalX = materialData.m_normalFlipX;
                        shaderData.m_flipNormalY = materialData.m_normalFlipY;

                        const AZ::RHI::ImageView* colorImageView = materialData.m_colorImage ? materialData.m_colorImage->GetImageView() : nullptr;
                        sectorData.m_srg->SetImageView(m_macroColorMapIndex, colorImageView, i);
                        
                        const AZ::RHI::ImageView* normalImageView = materialData.m_normalImage ? materialData.m_normalImage->GetImageView() : nullptr;
                        sectorData.m_srg->SetImageView(m_macroNormalMapIndex, normalImageView, i);

                        // set flags for which images are used.
                        shaderData.m_mapsInUse = (colorImageView ? ColorImageUsed : 0) | (normalImageView ? NormalImageUsed : 0);
                    }

                    sectorData.m_srg->SetConstantArray(m_macroMaterialDataIndex, macroMaterialData);
                    sectorData.m_srg->SetConstant(m_macroMaterialCountIndex, aznumeric_cast<uint32_t>(sectorData.m_macroMaterials.size()));

                    const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(transform);
                    sectorData.m_srg->SetConstant(m_modelToWorldIndex, matrix3x4);

                    sectorData.m_srg->Compile();
                }
            }
        }

        for (auto& sectorData : m_sectorData)
        {
            uint8_t lodChoice = AZ::RPI::ModelLodAsset::LodCountMax;

            // Go through all cameras and choose an LOD based on the closest camera.
            for (auto& view : process.m_views)
            {
                if ((view->GetUsageFlags() & AZ::RPI::View::UsageFlags::UsageCamera) > 0)
                {
                    const AZ::Vector3 cameraPosition = view->GetCameraTransform().GetTranslation();
                    const AZ::Vector2 cameraPositionXY = AZ::Vector2(cameraPosition.GetX(), cameraPosition.GetY());
                    const AZ::Vector2 sectorCenterXY = AZ::Vector2(sectorData.m_aabb.GetCenter().GetX(), sectorData.m_aabb.GetCenter().GetY());

                    const float sectorDistance = sectorCenterXY.GetDistance(cameraPositionXY);

                    // This will be configurable later
                    const float minDistanceForLod0 = (GridMeters * 4.0f);

                    // For every distance doubling beyond a minDistanceForLod0, we only need half the mesh density. Each LOD
                    // is exactly half the resolution of the last.
                    const float lodForCamera = floorf(AZ::GetMax(0.0f, log2f(sectorDistance / minDistanceForLod0)));

                    // All cameras should render the same LOD so effects like shadows are consistent.
                    lodChoice = AZ::GetMin(lodChoice, aznumeric_cast<uint8_t>(lodForCamera));
                }
            }

            // Add the correct LOD draw packet for visible sectors.
            for (auto& view : process.m_views)
            {
                AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
                if (viewFrustum.IntersectAabb(sectorData.m_aabb) != AZ::IntersectResult::Exterior)
                {
                    const uint8_t lodToRender = AZ::GetMin(lodChoice, aznumeric_cast<uint8_t>(sectorData.m_drawPackets.size() - 1));
                    view->AddDrawPacket(sectorData.m_drawPackets.at(lodToRender).GetRHIDrawPacket());
                }
            }
        }

        if (m_materialInstance)
        {
            m_materialInstance->Compile();
        }
    }

    void TerrainFeatureProcessor::InitializeTerrainPatch(uint16_t gridSize, float gridSpacing, PatchData& patchdata)
    {
        patchdata.m_positions.clear();
        patchdata.m_uvs.clear();
        patchdata.m_indices.clear();

        const uint16_t gridVertices = gridSize + 1; // For m_gridSize quads, (m_gridSize + 1) vertices are needed.
        const size_t size = gridVertices * gridVertices;

        patchdata.m_positions.reserve(size);
        patchdata.m_uvs.reserve(size);

        for (uint16_t y = 0; y < gridVertices; ++y)
        {
            for (uint16_t x = 0; x < gridVertices; ++x)
            {
                patchdata.m_positions.push_back({ aznumeric_cast<float>(x) * gridSpacing, aznumeric_cast<float>(y) * gridSpacing });
                patchdata.m_uvs.push_back({ aznumeric_cast<float>(x) / gridSize, aznumeric_cast<float>(y) / gridSize });
            }
        }

        patchdata.m_indices.reserve(gridSize * gridSize * 6); // total number of quads, 2 triangles with 6 indices per quad.
        
        for (uint16_t y = 0; y < gridSize; ++y)
        {
            for (uint16_t x = 0; x < gridSize; ++x)
            {
                const uint16_t topLeft = y * gridVertices + x;
                const uint16_t topRight = topLeft + 1;
                const uint16_t bottomLeft = (y + 1) * gridVertices + x;
                const uint16_t bottomRight = bottomLeft + 1;

                patchdata.m_indices.emplace_back(topLeft);
                patchdata.m_indices.emplace_back(topRight);
                patchdata.m_indices.emplace_back(bottomLeft);
                patchdata.m_indices.emplace_back(bottomLeft);
                patchdata.m_indices.emplace_back(topRight);
                patchdata.m_indices.emplace_back(bottomRight);
            }
        }
    }
    
    AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> TerrainFeatureProcessor::CreateBufferAsset(
        const void* data, const AZ::RHI::BufferViewDescriptor& bufferViewDescriptor, const AZStd::string& bufferName)
    {
        AZ::RPI::BufferAssetCreator creator;
        creator.Begin(AZ::Uuid::CreateRandom());

        AZ::RHI::BufferDescriptor bufferDescriptor;
        bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
        bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

        creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
        creator.SetBufferViewDescriptor(bufferViewDescriptor);
        creator.SetUseCommonPool(AZ::RPI::CommonBufferPoolType::StaticInputAssembly);

        AZ::Data::Asset<AZ::RPI::BufferAsset> bufferAsset;
        if (creator.End(bufferAsset))
        {
            bufferAsset.SetHint(bufferName);
            return AZ::Success(bufferAsset);
        }

        return AZ::Failure();
    }

    bool TerrainFeatureProcessor::InitializePatchModel()
    {
        AZ::RPI::ModelAssetCreator modelAssetCreator;
        modelAssetCreator.Begin(AZ::Uuid::CreateRandom());

        uint16_t gridSize = GridSize;
        float gridSpacing = GridSpacing;

        for (uint32_t i = 0; i < AZ::RPI::ModelLodAsset::LodCountMax && gridSize > 0; ++i)
        {
            PatchData patchData;
            InitializeTerrainPatch(gridSize, gridSpacing, patchData);

            const auto positionBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_positions.size()), AZ::RHI::Format::R32G32_FLOAT);
            const auto positionsOutcome = CreateBufferAsset(patchData.m_positions.data(), positionBufferViewDesc, "TerrainPatchPositions");
        
            const auto uvBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_uvs.size()), AZ::RHI::Format::R32G32_FLOAT);
            const auto uvsOutcome = CreateBufferAsset(patchData.m_uvs.data(), uvBufferViewDesc, "TerrainPatchUvs");
        
            const auto indexBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_indices.size()), AZ::RHI::Format::R16_UINT);
            const auto indicesOutcome = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

            if (!positionsOutcome.IsSuccess() || !uvsOutcome.IsSuccess() || !indicesOutcome.IsSuccess())
            {
                AZ_Error(TerrainFPName, false, "Failed to create GPU buffers for Terrain");
                return false;
            }
            
            AZ::RPI::ModelLodAssetCreator modelLodAssetCreator;
            modelLodAssetCreator.Begin(AZ::Uuid::CreateRandom());

            modelLodAssetCreator.BeginMesh();
            modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "POSITION" }, AZ::Name(), {positionsOutcome.GetValue(), positionBufferViewDesc});
            modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "UV" }, AZ::Name(), {uvsOutcome.GetValue(), uvBufferViewDesc});
            modelLodAssetCreator.SetMeshIndexBuffer({indicesOutcome.GetValue(), indexBufferViewDesc});

            AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0, 0.0, 0.0), AZ::Vector3(GridMeters, GridMeters, 0.0));
            modelLodAssetCreator.SetMeshAabb(AZStd::move(aabb));
            modelLodAssetCreator.SetMeshName(AZ::Name("Terrain Patch"));
            modelLodAssetCreator.EndMesh();

            AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset;
            modelLodAssetCreator.End(modelLodAsset);
        
            modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));

            gridSize = gridSize / 2;
            gridSpacing *= 2.0f;
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        bool success = modelAssetCreator.End(modelAsset);

        m_patchModel = AZ::RPI::Model::FindOrCreate(modelAsset);

        return success;
    }
    
    void TerrainFeatureProcessor::OnMaterialReinitialized([[maybe_unused]] const MaterialInstance& material)
    {
        for (auto& sectorData : m_sectorData)
        {
            for (auto& drawPacket : sectorData.m_drawPackets)
            {
                drawPacket.Update(*GetParentScene());
            }
        }
    }

    void TerrainFeatureProcessor::SetWorldSize([[maybe_unused]] AZ::Vector2 sizeInMeters)
    {
        // This will control the max rendering size. Actual terrain size can be much
        // larger but this will limit how much is rendered.
    }
    
    template <typename T>
    T* TerrainFeatureProcessor::FindByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<T>& container)
    {
        for (T& data : container.GetDataVector())
        {
            if (data.m_entityId == entityId)
            {
                return &data;
            }
        }
        return nullptr;
    }
    
    template <typename T>
    T& TerrainFeatureProcessor::FindOrCreateByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<T>& container)
    {
        T* dataPtr = FindByEntityId(entityId, container);
        if (dataPtr != nullptr)
        {
            return *dataPtr;
        }

        const uint16_t slotId = container.GetFreeSlotIndex();
        AZ_Assert(slotId != AZ::Render::IndexedDataVector<T>::NoFreeSlot, "Ran out of indices");

        T& data = container.GetData(slotId);
        data.m_entityId = entityId;
        return data;
    }
    
    template <typename T>
    void TerrainFeatureProcessor::RemoveByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<T>& container)
    {
        for (T& data : container.GetDataVector())
        {
            if (data.m_entityId == entityId)
            {
                container.RemoveData(&data);
                return;
            }
        }
        AZ_Assert(false, "Entity Id not found in container.")
    }
    
    template<typename Callback>
    void TerrainFeatureProcessor::ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback)
    {
        for (SectorData& sectorData : m_sectorData)
        {
            if (sectorData.m_aabb.Overlaps(bounds))
            {
                callback(sectorData);
            }
        }
    }
    
    auto TerrainFeatureProcessor::Vector2i::operator+(const Vector2i& rhs) const -> Vector2i
    {
        Vector2i offsetPoint = *this;
        offsetPoint += rhs;
        return offsetPoint;
    }
    
    auto TerrainFeatureProcessor::Vector2i::operator+=(const Vector2i& rhs) -> Vector2i&
    {
        m_x += rhs.m_x;
        m_y += rhs.m_y;
        return *this;
    }

    auto TerrainFeatureProcessor::Vector2i::operator-(const Vector2i& rhs) const -> Vector2i
    {
        return *this + -rhs;
    }
    
    auto TerrainFeatureProcessor::Vector2i::operator-=(const Vector2i& rhs) -> Vector2i&
    {
        return *this += -rhs;
    }
    
    auto TerrainFeatureProcessor::Vector2i::operator-() const -> Vector2i
    {
        return {-m_x, -m_y};
    }

    auto TerrainFeatureProcessor::Aabb2i::operator+(const Vector2i& rhs) const -> Aabb2i
    {
        return { m_min + rhs, m_max + rhs };
    }

    auto TerrainFeatureProcessor::Aabb2i::operator-(const Vector2i& rhs) const -> Aabb2i
    {
        return *this + -rhs;
    }

    auto TerrainFeatureProcessor::Aabb2i::GetClamped(Aabb2i rhs) const -> Aabb2i
    {
        Aabb2i ret;
        ret.m_min.m_x = AZ::GetMax(m_min.m_x, rhs.m_min.m_x);
        ret.m_min.m_y = AZ::GetMax(m_min.m_y, rhs.m_min.m_y);
        ret.m_max.m_x = AZ::GetMin(m_max.m_x, rhs.m_max.m_x);
        ret.m_max.m_y = AZ::GetMin(m_max.m_y, rhs.m_max.m_y);
        return ret;
    }

    bool TerrainFeatureProcessor::Aabb2i::IsValid() const
    {
        // Intentionally strict, equal min/max not valid.
        return m_min.m_x < m_max.m_x && m_min.m_y < m_max.m_y;
    }

}
