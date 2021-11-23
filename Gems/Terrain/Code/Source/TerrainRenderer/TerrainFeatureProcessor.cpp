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
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>

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

    namespace ViewSrgInputs
    {
        static const char* const HeightmapImage("m_heightmapImage");
    }

    namespace TerrainSrgInputs
    {
        static const char* const DetailMaterialIdImage("m_detailMaterialIdImage");
        static const char* const DetailMaterialData("m_detailMaterialData");
        static const char* const DetailMaterialIdImageCenter("m_detailMaterialIdImageCenter");
        static const char* const DetailHalfPixelUv("m_detailHalfPixelUv");
        static const char* const DetailAabb("m_detailAabb");
        static const char* const DetailTextures("m_detailTextures");
    }

    namespace DetailMaterialInputs
    {
        static const char* const BaseColorColor("baseColor.color");
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
        static const char* const RoughnessLowerBound("roughness.lowerBound");
        static const char* const RoughnessUpperBound("roughness.upperBound");
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
        EnableSceneNotification();
        CacheForwardPass();

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
        // Load indices for the View Srg.

        auto viewSrgLayout = AZ::RPI::RPISystemInterface::Get()->GetViewSrgLayout();
        
        m_heightmapPropertyIndex = viewSrgLayout->FindShaderInputImageIndex(AZ::Name(ViewSrgInputs::HeightmapImage));
        AZ_Error(TerrainFPName, m_heightmapPropertyIndex.IsValid(), "Failed to find view srg input constant %s.", ViewSrgInputs::HeightmapImage);
        
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
        
        DisableSceneNotification();

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
        
        AZ::Vector2 queryResolution2D = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            queryResolution2D, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);
        // Currently query resolution is multidimensional but the rendering system only supports this changing in one dimension.
        float queryResolution = queryResolution2D.GetX();

        // Sectors need to be rebuilt if the world bounds change in the x/y, or the sample spacing changes.
        m_areaData.m_rebuildSectors = m_areaData.m_rebuildSectors ||
            m_areaData.m_terrainBounds.GetMin().GetX() != worldBounds.GetMin().GetX() ||
            m_areaData.m_terrainBounds.GetMin().GetY() != worldBounds.GetMin().GetY() ||
            m_areaData.m_terrainBounds.GetMax().GetX() != worldBounds.GetMax().GetX() ||
            m_areaData.m_terrainBounds.GetMax().GetY() != worldBounds.GetMax().GetY() ||
            m_areaData.m_sampleSpacing != queryResolution;

        m_areaData.m_transform = transform;
        m_areaData.m_terrainBounds = worldBounds;
        m_areaData.m_sampleSpacing = queryResolution;
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
        m_detailMaterials.GetData(detailMaterialId).refCount++;
        m_dirtyDetailRegion.AddAabb(materialRegion.m_region);
    }

    void TerrainFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline)
    {
        CacheForwardPass();
    }

    void TerrainFeatureProcessor::CheckDetailMaterialForDeletion(uint16_t detailMaterialId)
    {
        auto& detailMaterialData = m_detailMaterials.GetData(detailMaterialId);
        if (--detailMaterialData.refCount == 0)
        {
            uint16_t bufferIndex = detailMaterialData.m_detailMaterialBufferIndex;
            DetailMaterialShaderData& shaderData = m_detailMaterialShaderData.GetElement(bufferIndex);

            for (uint16_t imageIndex :
                {
                    shaderData.m_colorImageIndex,
                    shaderData.m_normalImageIndex,
                    shaderData.m_roughnessImageIndex,
                    shaderData.m_metalnessImageIndex,
                    shaderData.m_specularF0ImageIndex,
                    shaderData.m_occlusionImageIndex,
                    shaderData.m_heightImageIndex
                })
            {
                if (imageIndex != InvalidDetailImageIndex)
                {
                    m_detailImageViews.at(imageIndex) = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Magenta)->GetImageView();
                    m_detailImageViewFreeList.push_back(imageIndex);
                    m_detailImagesNeedUpdate = true;
                }
            }

            m_detailMaterialShaderData.Release(bufferIndex);
            m_detailMaterials.RemoveIndex(detailMaterialId);
        }
    }

    void TerrainFeatureProcessor::OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);

        for (DetailMaterialSurface& surface : materialRegion.m_materialsForSurfaces)
        {
            if (surface.m_surfaceTag == surfaceTag)
            {
                CheckDetailMaterialForDeletion(surface.m_detailMaterialId);

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
                if (surface.m_detailMaterialId != materialId)
                {
                    ++m_detailMaterials.GetData(materialId).refCount;
                    CheckDetailMaterialForDeletion(surface.m_detailMaterialId);
                    surface.m_detailMaterialId = materialId;
                }
                break;
            }
        }

        if (!found)
        {
            ++m_detailMaterials.GetData(materialId).refCount;
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

        for (auto& detailMaterialData : m_detailMaterials.GetDataVector())
        {
            if (detailMaterialData.m_assetId == material->GetAssetId())
            {
                detailMaterialId = m_detailMaterials.GetIndexForData(&detailMaterialData);
                UpdateDetailMaterialData(detailMaterialId, material);
                break;
            }
        }

        AZ_Assert(m_detailMaterialShaderData.GetSize() < 0xFF, "Only 255 detail materials supported.");

        if (detailMaterialId == InvalidDetailMaterial && m_detailMaterialShaderData.GetSize() < 0xFF)
        {
            detailMaterialId = m_detailMaterials.GetFreeSlotIndex();
            auto& detailMaterialData = m_detailMaterials.GetData(detailMaterialId);
            detailMaterialData.m_detailMaterialBufferIndex = aznumeric_cast<uint16_t>(m_detailMaterialShaderData.Reserve());
            UpdateDetailMaterialData(detailMaterialId, material);
        }
        return detailMaterialId;
    }

    void TerrainFeatureProcessor::UpdateDetailMaterialData(uint16_t detailMaterialIndex, MaterialInstance material)
    {
        DetailMaterialData& materialData = m_detailMaterials.GetData(detailMaterialIndex);
        DetailMaterialShaderData& shaderData = m_detailMaterialShaderData.GetElement(materialData.m_detailMaterialBufferIndex);

        if (materialData.m_materialChangeId == material->GetCurrentChangeId())
        {
            return; // material hasn't changed, nothing to do
        }

        materialData.m_materialChangeId = material->GetCurrentChangeId();
        materialData.m_assetId = material->GetAssetId();
            
        DetailTextureFlags& flags = shaderData.m_flags;
            
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
                // GetValue<T>() expects the actaul type, not a reference type, so the reference needs to be removed.
                using TypeRefRemoved = AZStd::remove_cvref_t<decltype(ref)>;
                ref = material->GetPropertyValue(index).GetValue<TypeRefRemoved>();
            }
        };

        auto applyImage = [&](const char* const indexName, AZ::Data::Instance<AZ::RPI::Image>& ref, const char* const usingFlagName, DetailTextureFlags flagToSet, uint16_t& imageIndex) -> void
        {
            // Determine if an image exists and if its using flag allows it to be used.
            const auto index = getIndex(indexName);
            const auto useTextureIndex = getIndex(usingFlagName);
            bool useTextureValue = true;
            if (useTextureIndex.IsValid())
            {
                useTextureValue = material->GetPropertyValue(useTextureIndex).GetValue<bool>();
            }
            if (index.IsValid() && useTextureValue)
            {
                ref = material->GetPropertyValue(index).GetValue<AZ::Data::Instance<AZ::RPI::Image>>();
            }
            useTextureValue = useTextureValue && ref;
            flags = DetailTextureFlags(useTextureValue ? (flags | flagToSet) : (flags & ~flagToSet));

            // Update queues to add/remove textures depending on if the image is used
            if (ref)
            {
                if (imageIndex == InvalidDetailImageIndex)
                {
                    if (m_detailImageViewFreeList.size() > 0)
                    {
                        imageIndex = m_detailImageViewFreeList.back();
                        m_detailImageViewFreeList.pop_back();
                    }
                    else
                    {
                        imageIndex = aznumeric_cast<uint16_t>(m_detailImageViews.size());
                        m_detailImageViews.push_back();
                    }
                }
                m_detailImageViews.at(imageIndex) = ref->GetImageView();
                m_detailImagesNeedUpdate = true;
            }
            else if (imageIndex != InvalidDetailImageIndex)
            {
                m_detailImageViews.at(imageIndex) = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::Magenta)->GetImageView();
                m_detailImageViewFreeList.push_back(imageIndex);
                m_detailImagesNeedUpdate = true;
                imageIndex = InvalidDetailImageIndex;
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
        applyImage(BaseColorMap, materialData.m_colorImage, BaseColorUseTexture, DetailTextureFlags::UseTextureBaseColor, shaderData.m_colorImageIndex);
        applyProperty(BaseColorFactor, shaderData.m_baseColorFactor);

        const auto index = getIndex(BaseColorColor);
        if (index.IsValid())
        {
            AZ::Color baseColor = material->GetPropertyValue(index).GetValue<AZ::Color>();
            shaderData.m_baseColorRed = baseColor.GetR();
            shaderData.m_baseColorGreen = baseColor.GetG();
            shaderData.m_baseColorBlue = baseColor.GetB();
        }

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
            
        applyImage(MetallicMap, materialData.m_metalnessImage, MetallicUseTexture, DetailTextureFlags::UseTextureMetallic, shaderData.m_metalnessImageIndex);
        applyProperty(MetallicFactor, shaderData.m_metalFactor);
            
        applyImage(RoughnessMap, materialData.m_roughnessImage, RoughnessUseTexture, DetailTextureFlags::UseTextureRoughness, shaderData.m_roughnessImageIndex);

        if ((flags & DetailTextureFlags::UseTextureRoughness) > 0)
        {
            float lowerBound = 0.0;
            float upperBound = 1.0;
            applyProperty(RoughnessLowerBound, lowerBound);
            applyProperty(RoughnessUpperBound, upperBound);
            shaderData.m_roughnessBias = lowerBound;
            shaderData.m_roughnessScale = upperBound - lowerBound;
        }
        else
        {
            shaderData.m_roughnessBias = 0.0;
            applyProperty(RoughnessFactor, shaderData.m_roughnessScale);
        }
            
        applyImage(SpecularF0Map, materialData.m_specularF0Image, SpecularF0UseTexture, DetailTextureFlags::UseTextureSpecularF0, shaderData.m_specularF0ImageIndex);
        applyProperty(SpecularF0Factor, shaderData.m_specularF0Factor);
            
        applyImage(NormalMap, materialData.m_normalImage, NormalUseTexture, DetailTextureFlags::UseTextureNormal, shaderData.m_normalImageIndex);
        applyProperty(NormalFactor, shaderData.m_normalFactor);
        applyFlag(NormalFlipX, DetailTextureFlags::FlipNormalX);
        applyFlag(NormalFlipY, DetailTextureFlags::FlipNormalY);
            
        applyImage(DiffuseOcclusionMap, materialData.m_occlusionImage, DiffuseOcclusionUseTexture, DetailTextureFlags::UseTextureOcclusion, shaderData.m_occlusionImageIndex);
        applyProperty(DiffuseOcclusionFactor, shaderData.m_occlusionFactor);
            
        applyImage(HeightMap, materialData.m_heightImage, HeightUseTexture, DetailTextureFlags::UseTextureHeight, shaderData.m_heightImageIndex);
        applyProperty(HeightFactor, shaderData.m_heightFactor);
        applyProperty(HeightOffset, shaderData.m_heightOffset);
        applyProperty(HeightBlendFactor, shaderData.m_heightBlendFactor);

        m_updateDetailMaterialBuffer = true;
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
                        return m_detailMaterials.GetData(materialSurface.m_detailMaterialId).m_detailMaterialBufferIndex;
                    }
                }
            }
        }
        return m_detailMaterials.NoFreeSlot;
    }

    void TerrainFeatureProcessor::UpdateTerrainData()
    {

        const float queryResolution = m_areaData.m_sampleSpacing;
        const AZ::Aabb& worldBounds = m_areaData.m_terrainBounds;

        int32_t heightmapImageXStart = aznumeric_cast<int32_t>(AZStd::ceilf(worldBounds.GetMin().GetX() / queryResolution));
        int32_t heightmapImageXEnd = aznumeric_cast<int32_t>(AZStd::floorf(worldBounds.GetMax().GetX() / queryResolution)) + 1;
        int32_t heightmapImageYStart = aznumeric_cast<int32_t>(AZStd::ceilf(worldBounds.GetMin().GetY() / queryResolution));
        int32_t heightmapImageYEnd = aznumeric_cast<int32_t>(AZStd::floorf(worldBounds.GetMax().GetY() / queryResolution)) + 1;
        uint32_t heightmapImageWidth = heightmapImageXEnd - heightmapImageXStart;
        uint32_t heightmapImageHeight = heightmapImageYEnd - heightmapImageYStart;

        const AZ::RHI::Size heightmapSize = AZ::RHI::Size(heightmapImageWidth, heightmapImageHeight, 1);

        if (!m_areaData.m_heightmapImage || m_areaData.m_heightmapImage->GetDescriptor().m_size != heightmapSize)
        {
            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, heightmapSize.m_width, heightmapSize.m_height, AZ::RHI::Format::R16_UNORM
            );

            const AZ::Name TerrainHeightmapName = AZ::Name(TerrainHeightmapChars);
            m_areaData.m_heightmapImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainHeightmapName, nullptr, nullptr);
            AZ_Error(TerrainFPName, m_areaData.m_heightmapImage, "Failed to initialize the heightmap image.");
            
            // World size changed, so the whole height map needs updating.
            m_dirtyRegion = worldBounds;
            m_imagesNeedUpdate = true;
        }
        
        int32_t xStart = aznumeric_cast<int32_t>(AZStd::ceilf(m_dirtyRegion.GetMin().GetX() / queryResolution));
        int32_t xEnd = aznumeric_cast<int32_t>(AZStd::floorf(m_dirtyRegion.GetMax().GetX() / queryResolution)) + 1;
        int32_t yStart = aznumeric_cast<int32_t>(AZStd::ceilf(m_dirtyRegion.GetMin().GetY() / queryResolution));
        int32_t yEnd = aznumeric_cast<int32_t>(AZStd::floorf(m_dirtyRegion.GetMax().GetY() / queryResolution)) + 1;
        uint32_t updateWidth = xEnd - xStart;
        uint32_t updateHeight = yEnd - yStart;

        AZStd::vector<uint16_t> pixels;
        pixels.reserve(updateWidth * updateHeight);

        {
            // Block other threads from accessing the surface data bus while we are in GetHeightFromFloats (which may call into the SurfaceData bus).
            // We lock our surface data mutex *before* checking / setting "isRequestInProgress" so that we prevent race conditions
            // that create false detection of cyclic dependencies when multiple requests occur on different threads simultaneously.
            // (One case where this was previously able to occur was in rapid updating of the Preview widget on the
            // GradientSurfaceDataComponent in the Editor when moving the threshold sliders back and forth rapidly)

            auto& surfaceDataContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
            typename SurfaceData::SurfaceDataSystemRequestBus::Context::DispatchLockGuard scopeLock(surfaceDataContext.m_contextMutex);

            for (int32_t y = yStart; y < yEnd; y++)
            {
                for (int32_t x = xStart; x < xEnd; x++)
                {
                    bool terrainExists = true;
                    float terrainHeight = 0.0f;
                    float xPos = x * queryResolution;
                    float yPos = y * queryResolution;
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        terrainHeight, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats,
                        xPos, yPos, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);

                    const float clampedHeight = AZ::GetClamp((terrainHeight - worldBounds.GetMin().GetZ()) / worldBounds.GetExtents().GetZ(), 0.0f, 1.0f);
                    const float expandedHeight = AZStd::roundf(clampedHeight * AZStd::numeric_limits<uint16_t>::max());
                    const uint16_t uint16Height = aznumeric_cast<uint16_t>(expandedHeight);

                    pixels.push_back(uint16Height);
                }
            }
        }

        if (m_areaData.m_heightmapImage)
        {
            constexpr uint32_t BytesPerPixel = sizeof(uint16_t);
            const float left = xStart - (worldBounds.GetMin().GetX() / queryResolution);
            const float top = yStart - (worldBounds.GetMin().GetY() / queryResolution);

            AZ::RHI::ImageUpdateRequest imageUpdateRequest;
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = aznumeric_cast<uint32_t>(left);
            imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = aznumeric_cast<uint32_t>(top);
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerRow = updateWidth * BytesPerPixel;
            imageUpdateRequest.m_sourceSubresourceLayout.m_bytesPerImage = updateWidth * updateHeight * BytesPerPixel;
            imageUpdateRequest.m_sourceSubresourceLayout.m_rowCount = updateHeight;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_width = updateWidth;
            imageUpdateRequest.m_sourceSubresourceLayout.m_size.m_height = updateHeight;
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

        m_terrainSrg = {};
        for (auto& shaderItem : m_materialInstance->GetShaderCollection())
        {
            if (shaderItem.GetShaderAsset()->GetDrawListName() == AZ::Name("forward"))
            {
                const auto& shaderAsset = shaderItem.GetShaderAsset();
                m_terrainSrg = AZ::RPI::ShaderResourceGroup::Create(shaderItem.GetShaderAsset(), shaderAsset->GetSupervariantIndex(AZ::Name()), AZ::Name{"TerrainSrg"});
                AZ_Error(TerrainFPName, m_terrainSrg, "Failed to create Terrain shader resource group");
                break;
            }
        }

        AZ_Error(TerrainFPName, m_terrainSrg, "Terrain Srg not found on any shader in the terrain material");

        if (m_terrainSrg)
        {
            const AZ::RHI::ShaderResourceGroupLayout* terrainSrgLayout = m_terrainSrg->GetLayout();

            m_detailMaterialIdPropertyIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailMaterialIdImage));
            AZ_Error(TerrainFPName, m_detailMaterialIdPropertyIndex.IsValid(), "Failed to find view srg input constant %s.", TerrainSrgInputs::DetailMaterialIdImage);
        
            m_detailCenterPropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailMaterialIdImageCenter));
            AZ_Error(TerrainFPName, m_detailCenterPropertyIndex.IsValid(), "Failed to find view srg input constant %s.", TerrainSrgInputs::DetailMaterialIdImageCenter);

            m_detailHalfPixelUvPropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailHalfPixelUv));
            AZ_Error(TerrainFPName, m_detailHalfPixelUvPropertyIndex.IsValid(), "Failed to find view srg input constant %s.", TerrainSrgInputs::DetailHalfPixelUv);
        
            m_detailAabbPropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailAabb));
            AZ_Error(TerrainFPName, m_detailAabbPropertyIndex.IsValid(), "Failed to find view srg input constant %s.", TerrainSrgInputs::DetailAabb);

            m_detailTexturesIndex = terrainSrgLayout->FindShaderInputImageUnboundedArrayIndex(AZ::Name(TerrainSrgInputs::DetailTextures));
            AZ_Error(TerrainFPName, m_detailTexturesIndex.IsValid(), "Failed to find view srg input constant %s.", TerrainSrgInputs::DetailTextures);

            // Set up the gpu buffer for detail material data
            AZ::Render::GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "Detail Material Data";
            desc.m_bufferSrgName = TerrainSrgInputs::DetailMaterialData;
            desc.m_elementSize = sizeof(DetailMaterialShaderData);
            desc.m_srgLayout = terrainSrgLayout;
            m_detailMaterialDataBuffer = AZ::Render::GpuBufferHandler(desc);
        }

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
                const float xFirstPatchStart = AZStd::floorf(terrainBounds.GetMin().GetX() / GridMeters) * GridMeters;
                const float xLastPatchStart = AZStd::floorf(terrainBounds.GetMax().GetX() / GridMeters) * GridMeters;
                const float yFirstPatchStart = AZStd::floorf(terrainBounds.GetMin().GetY() / GridMeters) * GridMeters;
                const float yLastPatchStart = AZStd::floorf(terrainBounds.GetMax().GetY() / GridMeters) * GridMeters;
            
                const auto& materialAsset = m_materialInstance->GetAsset();
                const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

                for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += GridMeters)
                {
                    for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += GridMeters)
                    {
                        auto objectSrg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                        if (!objectSrg)
                        {
                            AZ_Warning(TerrainFPName, false, "Failed to create a new shader resource group, skipping.");
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
                                AZ_Warning(TerrainFPName, false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
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
            }
            
            if (m_updateDetailMaterialBuffer)
            {
                m_updateDetailMaterialBuffer = false;
                m_detailMaterialDataBuffer.UpdateBuffer(m_detailMaterialShaderData.GetRawData(), aznumeric_cast<uint32_t>(m_detailMaterialShaderData.GetSize()));
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

            if (m_dirtyDetailRegion.IsValid() || !cameraPosition.IsClose(m_previousCameraPosition) || m_detailImagesNeedUpdate)
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

                AZ::Vector4 detailAabb = AZ::Vector4(
                    m_detailTextureBounds.m_min.m_x * DetailTextureScale,
                    m_detailTextureBounds.m_min.m_y * DetailTextureScale,
                    m_detailTextureBounds.m_max.m_x * DetailTextureScale,
                    m_detailTextureBounds.m_max.m_y * DetailTextureScale
                );
                AZ::Vector2 detailUvOffset = AZ::Vector2(float(newCenter.m_x) / DetailTextureSize, float(newCenter.m_y) / DetailTextureSize);

                if (m_terrainSrg)
                {
                    m_terrainSrg->SetConstant(m_detailAabbPropertyIndex, detailAabb);
                    m_terrainSrg->SetConstant(m_detailHalfPixelUvPropertyIndex, 0.5f / DetailTextureSize);
                    m_terrainSrg->SetConstant(m_detailCenterPropertyIndex, detailUvOffset);

                    m_detailMaterialDataBuffer.UpdateSrg(m_terrainSrg.get());
                }
            }

            if (m_areaData.m_heightmapUpdated || m_areaData.m_macroMaterialsUpdated)
            {
                // Currently when anything in the heightmap changes we're updating all the srgs, but this could probably
                // be optimized to only update the srgs that changed.

                m_areaData.m_heightmapUpdated = false;
                m_areaData.m_macroMaterialsUpdated = false;

                AZStd::array<float, 2> uvStep =
                {
                    1.0f / aznumeric_cast<uint32_t>(m_areaData.m_terrainBounds.GetXExtent() / m_areaData.m_sampleSpacing),
                    1.0f / aznumeric_cast<uint32_t>(m_areaData.m_terrainBounds.GetYExtent() / m_areaData.m_sampleSpacing),
                };

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

                    terrainDataForSrg.m_uvStep = uvStep;

                    AZ::Transform transform = m_areaData.m_transform;
                    transform.SetTranslation(xPatch, yPatch, m_areaData.m_transform.GetTranslation().GetZ());

                    terrainDataForSrg.m_sampleSpacing = m_areaData.m_sampleSpacing;
                    terrainDataForSrg.m_heightScale = terrainBounds.GetZExtent();

                    sectorData.m_srg->SetConstant(m_terrainDataIndex, terrainDataForSrg);

                    AZStd::array<ShaderMacroMaterialData, MaxMaterialsPerSector> macroMaterialData;

                    uint32_t i = 0;
                    for (; i < sectorData.m_macroMaterials.size(); ++i)
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
                    for (; i < sectorData.m_macroMaterials.capacity(); ++i)
                    {
                        sectorData.m_srg->SetImageView(m_macroColorMapIndex, nullptr, i);
                        sectorData.m_srg->SetImageView(m_macroNormalMapIndex, nullptr, i);
                    }

                    sectorData.m_srg->SetConstantArray(m_macroMaterialDataIndex, macroMaterialData);
                    sectorData.m_srg->SetConstant(m_macroMaterialCountIndex, aznumeric_cast<uint32_t>(sectorData.m_macroMaterials.size()));

                    const AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(transform);
                    sectorData.m_srg->SetConstant(m_modelToWorldIndex, matrix3x4);

                    sectorData.m_srg->Compile();
                }
            }

            // Currently there seems to be a bug in unbounded image arrays where flickering can occur if this isn't updated every frame.
            if (m_terrainSrg/* && m_detailImagesUpdated*/)
            {
                AZStd::array_view<const AZ::RHI::ImageView*> imageViews(m_detailImageViews.data(), m_detailImageViews.size());
                [[maybe_unused]] bool result = m_terrainSrg->SetImageViewUnboundedArray(m_detailTexturesIndex, imageViews);
                AZ_Error(TerrainFPName, result, "Failed to set image view unbounded array into shader resource group.");
                m_detailImagesNeedUpdate = false;
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
                    const float lodForCamera = AZStd::floorf(AZ::GetMax(0.0f, log2f(sectorDistance / minDistanceForLod0)));

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

        if (m_detailTextureImage && m_areaData.m_heightmapImage && m_imagesNeedUpdate)
        {
            m_imagesNeedUpdate = false;
            for (auto& view : process.m_views)
            {
                auto viewSrg = view->GetShaderResourceGroup();
                viewSrg->SetImage(m_heightmapPropertyIndex, m_areaData.m_heightmapImage);
            }
            if (m_terrainSrg)
            {
                m_terrainSrg->SetImage(m_detailMaterialIdPropertyIndex, m_detailTextureImage);
            }
        }

        if (m_materialInstance)
        {
            m_materialInstance->Compile();
        }

        if (m_terrainSrg && m_forwardPass)
        {
            m_terrainSrg->Compile();
            m_forwardPass->BindSrg(m_terrainSrg->GetRHIShaderResourceGroup());
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
        PrepareMaterialData();
        for (auto& sectorData : m_sectorData)
        {
            for (auto& drawPacket : sectorData.m_drawPackets)
            {
                drawPacket.Update(*GetParentScene());
            }
        }
        m_imagesNeedUpdate = true;
        m_detailImagesNeedUpdate = true;
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

    void TerrainFeatureProcessor::CacheForwardPass()
    {
        auto rasterPassFilter = AZ::RPI::PassFilter::CreateWithPassClass<AZ::RPI::RasterPass>();
        rasterPassFilter.SetOwnerScene(GetParentScene());
        AZ::RHI::RHISystemInterface* rhiSystem = AZ::RHI::RHISystemInterface::Get();
        AZ::RHI::DrawListTag forwardTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(AZ::Name("forward"));
        AZ::RPI::PassSystemInterface::Get()->ForEachPass(rasterPassFilter,
            [&](AZ::RPI::Pass* pass) -> AZ::RPI::PassFilterExecutionFlow
            {
                auto* rasterPass = azrtti_cast<AZ::RPI::RasterPass*>(pass);
                    
                if (rasterPass && rasterPass->GetDrawListTag() == forwardTag)
                {
                    m_forwardPass = rasterPass;
                    return AZ::RPI::PassFilterExecutionFlow::StopVisitingPasses;
                }
                return AZ::RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            }
        );
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
