/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainDetailMaterialManager.h>
#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>

#include <AzCore/Console/Console.h>
#include <AzCore/std/parallel/semaphore.h>

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>

#include <Atom/Utils/MaterialUtils.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainDetailMaterialManagerName = "TerrainDetailMaterialManager";
        static const char* TerrainDetailChars = "TerrainDetail";
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
        static const char* const ParallaxHeightFactor("parallax.factor");
        static const char* const ParallaxHeightOffset("parallax.offset");
        static const char* const TerrainSettingsOverrideParallax("terrain.overrideParallaxSettings");
        static const char* const TerrainHeightFactor("terrain.heightScale");
        static const char* const TerrainHeightOffset("terrain.heightOffset");
        static const char* const HeightBlendFactor("terrain.blendFactor");
        static const char* const HeightWeightClampFactor("terrain.weightClampFactor");
        static const char* const UvCenter("uv.center");
        static const char* const UvScale("uv.scale");
        static const char* const UvTileU("uv.tileU");
        static const char* const UvTileV("uv.tileV");
        static const char* const UvOffsetU("uv.offsetU");
        static const char* const UvOffsetV("uv.offsetV");
        static const char* const UvRotateDegrees("uv.rotateDegrees");
    }
    
    namespace TerrainSrgInputs
    {
        static const char* const DetailMaterialIdImage("m_detailMaterialIdImage");
        static const char* const DetailMaterialData("m_detailMaterialData");
        static const char* const DetailMaterialScale("m_detailMaterialIdScale");
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
    
    AZ_CVAR(bool,
        r_terrainDebugDetailImageUpdates,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Turns on debugging for detail material update regions for terrain."
    );

    void TerrainDetailMaterialManager::Initialize(
        const AZStd::shared_ptr<AZ::Render::BindlessImageArrayHandler>& bindlessImageHandler,
        const AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg,
        const AZ::Data::Instance<AZ::RPI::Material>& terrainMaterial)
    {
        AZ_Error(TerrainDetailMaterialManagerName, bindlessImageHandler, "bindlessImageHandler must not be null.");
        AZ_Error(TerrainDetailMaterialManagerName, terrainSrg, "terrainSrg must not be null.");
        AZ_Error(TerrainDetailMaterialManagerName, !m_isInitialized, "Already initialized.");

        if (!bindlessImageHandler || !terrainSrg || m_isInitialized)
        {
            return;
        }

        m_terrainMaterial = terrainMaterial;
        UpdateTerrainMaterial();

        InitializePassthroughDetailMaterial();
        InitializeTextureParams();

        if (UpdateSrgIndices(terrainSrg))
        {
            m_bindlessImageHandler = bindlessImageHandler;
            
            // Find any detail material areas that have already been created.
            TerrainAreaMaterialRequestBus::EnumerateHandlers(
                [&](TerrainAreaMaterialRequests* handler)
                {
                    const AZ::Aabb& bounds = handler->GetTerrainSurfaceMaterialRegion();
                    const AZStd::vector<TerrainSurfaceMaterialMapping> materialMappings = handler->GetSurfaceMaterialMappings();
                    AZ::EntityId entityId = *(Terrain::TerrainAreaMaterialRequestBus::GetCurrentBusId());
                
                    DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);
                    materialRegion.m_region = bounds;

                    if (handler->GetDefaultMaterial().m_materialInstance)
                    {
                        OnTerrainDefaultSurfaceMaterialCreated(entityId, handler->GetDefaultMaterial().m_materialInstance);
                    }

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
            
            AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

            OnTerrainDataChanged(worldBounds, TerrainDataChangedMask::SurfaceData);
            AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

            m_isInitialized = true;
        }
    }
    
    bool TerrainDetailMaterialManager::UpdateSrgIndices(const AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        const AZ::RHI::ShaderResourceGroupLayout* terrainSrgLayout = terrainSrg->GetLayout();
            
        m_detailMaterialIdPropertyIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailMaterialIdImage));
        AZ_Error(TerrainDetailMaterialManagerName, m_detailMaterialIdPropertyIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::DetailMaterialIdImage);
        
        m_detailScalePropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailMaterialScale));
        AZ_Error(TerrainDetailMaterialManagerName, m_detailScalePropertyIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::DetailMaterialScale);

        // Set up the gpu buffer for detail material data
        AZ::Render::GpuBufferHandler::Descriptor desc;
        desc.m_bufferName = "Detail Material Data";
        desc.m_bufferSrgName = TerrainSrgInputs::DetailMaterialData;
        desc.m_elementSize = sizeof(DetailMaterialShaderData);
        desc.m_srgLayout = terrainSrgLayout;
        m_detailMaterialDataBuffer = AZ::Render::GpuBufferHandler(desc);

        bool IndicesValid =
            m_detailMaterialIdPropertyIndex.IsValid() &&
            m_detailScalePropertyIndex.IsValid();

        m_detailImageNeedsUpdate = true;
        m_detailMaterialBufferNeedsUpdate = true;

        return IndicesValid && m_detailMaterialDataBuffer.IsValid();
    }
    
    void TerrainDetailMaterialManager::RemoveAllImages()
    {   
        for (const DetailMaterialData& materialData: m_detailMaterials.GetDataVector())
        {
            DetailMaterialShaderData& shaderData = m_detailMaterialShaderData.GetElement(materialData.m_detailMaterialBufferIndex);

            auto checkRemoveImage = [&](uint16_t index)
            {
                if (index != 0xFFFF)
                {
                    m_bindlessImageHandler->RemoveBindlessImage(index);
                }
            };
            
            checkRemoveImage(shaderData.m_colorImageIndex);
            checkRemoveImage(shaderData.m_normalImageIndex);
            checkRemoveImage(shaderData.m_roughnessImageIndex);
            checkRemoveImage(shaderData.m_metalnessImageIndex);
            checkRemoveImage(shaderData.m_specularF0ImageIndex);
            checkRemoveImage(shaderData.m_occlusionImageIndex);
            checkRemoveImage(shaderData.m_heightImageIndex);
        }
    }

    bool TerrainDetailMaterialManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainDetailMaterialManager::Reset()
    {
        RemoveAllImages();

        m_detailTextureImage = {};
        m_detailMaterials.Clear();
        m_detailMaterialRegions.Clear();
        m_detailMaterialShaderData.Clear();
        m_detailMaterialDataBuffer.Release();

        m_dirtyDetailRegion = AZ::Aabb::CreateNull();

        m_detailMaterialBufferNeedsUpdate = false;
        m_detailImageNeedsUpdate = false;

        TerrainAreaMaterialNotificationBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();

        m_isInitialized = false;
    }

    void TerrainDetailMaterialManager::Update(const AZ::Vector3& cameraPosition, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        if (m_terrainMaterial->NeedsCompile())
        {
            UpdateTerrainMaterial();
        }

        if (m_detailMaterialBufferNeedsUpdate)
        {
            m_detailMaterialBufferNeedsUpdate = false;
            m_detailMaterialDataBuffer.UpdateBuffer(m_detailMaterialShaderData.GetRawData(), aznumeric_cast<uint32_t>(m_detailMaterialShaderData.GetSize()));
        }

        CheckUpdateDetailTexture(cameraPosition);
        
        if (m_detailImageNeedsUpdate)
        {
            terrainSrg->SetConstant(m_detailScalePropertyIndex, 1.0f / m_detailTextureScale);
            terrainSrg->SetImage(m_detailMaterialIdPropertyIndex, m_detailTextureImage);

            m_detailMaterialDataBuffer.UpdateSrg(terrainSrg.get());

            m_detailImageNeedsUpdate = false;
        }

    }
    
    void TerrainDetailMaterialManager::InitializeTextureParams()
    {   
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            m_detailTextureScale, &AzFramework::Terrain::TerrainDataRequests::GetTerrainSurfaceDataQueryResolution);

        // Texture size needs to be twice the render distance because the camera is positioned in the middle of the texture.
        m_detailTextureSize = lroundf(m_config.m_renderDistance / m_detailTextureScale) * 2;

        ClipmapBoundsDescriptor desc;
        desc.m_clipmapUpdateMultiple = 1;
        desc.m_clipmapToWorldScale = m_detailTextureScale;
        desc.m_size = m_detailTextureSize;

        // Initialize world space to a value that won't match the initial camera position.
        desc.m_worldSpaceCenter = AZ::Vector2(AZStd::numeric_limits<float>::max(), 0.0f);
        m_detailMaterialIdBounds = ClipmapBounds(desc);

        m_detailTextureImage = {}; // Force the image to rebuild
        m_detailImageNeedsUpdate = true;
    }
    
    void TerrainDetailMaterialManager::UpdateTerrainMaterial()
    {
        AZ::RPI::MaterialPropertyIndex detailTextureMultiplierIndex = m_terrainMaterial->FindPropertyIndex(AZ::Name("settings.detailTextureMultiplier"));
        AZ::RPI::MaterialPropertyIndex detailTextureFadeDistanceIndex = m_terrainMaterial->FindPropertyIndex(AZ::Name("settings.detailFadeDistance"));
        AZ::RPI::MaterialPropertyIndex detailTextureFadeLengthIndex = m_terrainMaterial->FindPropertyIndex(AZ::Name("settings.detailFadeLength"));

        AZ_Assert(detailTextureMultiplierIndex.IsValid(), "Terrain Feature Processor unable to find settings.detailTextureMultiplier in the terrain material.");
        AZ_Assert(detailTextureFadeDistanceIndex.IsValid(), "Terrain Feature Processor unable to find settings.detailFadeDistance in the terrain material.");
        AZ_Assert(detailTextureFadeLengthIndex.IsValid(), "Terrain Feature Processor unable to find settings.detailFadeLength in the terrain material.");

        m_terrainMaterial->SetPropertyValue(detailTextureMultiplierIndex, m_config.m_scale);
        m_terrainMaterial->SetPropertyValue(detailTextureFadeDistanceIndex, AZStd::GetMax<float>(0.0f, m_config.m_renderDistance - m_config.m_fadeDistance));
        m_terrainMaterial->SetPropertyValue(detailTextureFadeLengthIndex, m_config.m_fadeDistance);
    }

    void TerrainDetailMaterialManager::SetDetailMaterialConfiguration(const DetailMaterialConfiguration& config)
    {
        m_config = config;
        
        AZ::RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(
            AZ::Name{ "o_terrainUseHeightBasedBlending" },
            AZ::RPI::ShaderOptionValue{ m_config.m_useHeightBasedBlending }
        );

        if (IsInitialized())
        {
            UpdateTerrainMaterial();
            InitializeTextureParams();
        }
    }

    void TerrainDetailMaterialManager::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & TerrainDataChangedMask::SurfaceData) != 0)
        {
            m_dirtyDetailRegion.AddAabb(dirtyRegion);
        }
        if ((dataChangedMask & TerrainDataChangedMask::Settings) != 0)
        {
            InitializeTextureParams();
        }
    }

    bool TerrainDetailMaterialManager::ForSurfaceTag(DetailMaterialListRegion& materialRegion,
        SurfaceData::SurfaceTag surfaceTag, DefaultMaterialSurfaceCallback callback)
    {
        for (DetailMaterialSurface& surface : materialRegion.m_materialsForSurfaces)
        {
            if (surface.m_surfaceTag == surfaceTag)
            {
                callback(surface);
                return true;
            }
        }
        return false;
    }
    
    void TerrainDetailMaterialManager::OnTerrainDefaultSurfaceMaterialCreated(AZ::EntityId entityId, MaterialInstance material)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainDefaultSurfaceMaterialCreated() called for region that doesn't exist.");
            return;
        }
        AZ_Error("TerrainDetailMaterialManager", materialRegion->m_defaultDetailMaterialId == InvalidDetailMaterialId,
            "Default detail material created but was already set for this region.");

        materialRegion->m_defaultDetailMaterialId = CreateOrUpdateDetailMaterial(material);
        m_detailMaterials.GetData(materialRegion->m_defaultDetailMaterialId).m_refCount++;
        m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
    }

    void TerrainDetailMaterialManager::OnTerrainDefaultSurfaceMaterialDestroyed(AZ::EntityId entityId)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainDefaultSurfaceMaterialDestroyed() called for region that doesn't exist.");
            return;
        }
        if (materialRegion->m_defaultDetailMaterialId == InvalidDetailMaterialId)
        {
            AZ_Assert(false, "OnTerrainDefaultSurfaceMaterialDestroyed() called for a region without a default material");
            return;
        }

        CheckDetailMaterialForDeletion(materialRegion->m_defaultDetailMaterialId);
        materialRegion->m_defaultDetailMaterialId = InvalidDetailMaterialId;
        m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
    }

    void TerrainDetailMaterialManager::OnTerrainDefaultSurfaceMaterialChanged(AZ::EntityId entityId, MaterialInstance newMaterial)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainDefaultSurfaceMaterialChanged() called for region that doesn't exist.");
            return;
        }

        // Update existing entry or create a new material entry
        uint16_t materialId = CreateOrUpdateDetailMaterial(newMaterial);
        if (materialRegion->m_defaultDetailMaterialId != materialId)
        {
            m_detailMaterials.GetData(materialId).m_refCount++;
            CheckDetailMaterialForDeletion(materialRegion->m_defaultDetailMaterialId);
            materialRegion->m_defaultDetailMaterialId = materialId;
        }
        m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
    }

    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingCreated(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainSurfaceMaterialMappingCreated() called for region that doesn't exist.");
            return;
        }

        // Validate that the surface tag is new
        ForSurfaceTag(*materialRegion, surfaceTag, [](DetailMaterialSurface&)
        {
            AZ_Error(TerrainDetailMaterialManagerName, false, "Already have a surface material mapping for this surface tag.");
        });

        uint16_t detailMaterialId = CreateOrUpdateDetailMaterial(material);
        materialRegion->m_materialsForSurfaces.push_back({ surfaceTag, detailMaterialId });
        m_detailMaterials.GetData(detailMaterialId).m_refCount++;
        m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
    }
    
    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainSurfaceMaterialMappingDestroyed() called for region that doesn't exist.");
            return;
        }
        
        [[maybe_unused]] bool found = ForSurfaceTag(*materialRegion, surfaceTag,
            [&](DetailMaterialSurface& surface)
        {
            CheckDetailMaterialForDeletion(surface.m_detailMaterialId);
            
            if (surface.m_surfaceTag != materialRegion->m_materialsForSurfaces.back().m_surfaceTag)
            {
                AZStd::swap(surface, materialRegion->m_materialsForSurfaces.back());
            }
            materialRegion->m_materialsForSurfaces.pop_back();
            m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
        });

        AZ_Error(TerrainDetailMaterialManagerName, found, "Could not find surface tag to destroy for OnTerrainSurfaceMaterialMappingDestroyed().");
    }
    
    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingMaterialChanged(
        AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainSurfaceMaterialMappingMaterialChanged() called for region that doesn't exist.");
            return;
        }

        // Update existing entry or create a new material entry
        uint16_t materialId = CreateOrUpdateDetailMaterial(material);
        
        [[maybe_unused]] bool found = ForSurfaceTag(*materialRegion, surfaceTag,
            [&](DetailMaterialSurface& surface)
        {
            if (surface.m_detailMaterialId != materialId)
            {
                // Updated material was a different asset than the old material, decrement ref count and
                // delete if no other surface tags are using it.
                m_detailMaterials.GetData(materialId).m_refCount++;
                CheckDetailMaterialForDeletion(surface.m_detailMaterialId);
                surface.m_detailMaterialId = materialId;
            }
            m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
        });

        AZ_Assert(found, "OnTerrainSurfaceMaterialMappingMaterialChanged() called for tag that doesn't exist.");
    }

    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingTagChanged(
        AZ::EntityId entityId, SurfaceData::SurfaceTag oldTag,  SurfaceData::SurfaceTag newTag)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainSurfaceMaterialMappingTagChanged() called for region that doesn't exist.");
            return;
        }
        
        [[maybe_unused]] bool found = ForSurfaceTag(*materialRegion, oldTag,
            [&](DetailMaterialSurface& surface)
        {
            surface.m_surfaceTag = newTag;
            m_dirtyDetailRegion.AddAabb(materialRegion->m_region);
        });
        AZ_Assert(found, "OnTerrainSurfaceMaterialMappingTagChanged() called for tag that doesn't exist.");
    }
    
    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingRegionCreated(AZ::EntityId entityId, const AZ::Aabb& region)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);
        materialRegion.m_region = region;
        if (materialRegion.HasMaterials())
        {
            m_dirtyDetailRegion.AddAabb(region);
        }
    }
    
    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingRegionDestroyed(AZ::EntityId entityId, const AZ::Aabb& oldRegion)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainSurfaceMaterialMappingRegionDestroyed() called for region that doesn't exist.");
            return;
        }
        
        if (materialRegion->HasMaterials())
        {
            m_dirtyDetailRegion.AddAabb(oldRegion);
        }

        m_detailMaterialRegions.RemoveData(materialRegion);
    }

    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion)
    {
        DetailMaterialListRegion* materialRegion = FindByEntityId(entityId, m_detailMaterialRegions);
        if (materialRegion == nullptr)
        {
            AZ_Assert(false, "OnTerrainSurfaceMaterialMappingRegionChanged() called for region that doesn't exist.");
            return;
        }
        materialRegion->m_region = newRegion;
        
        if (materialRegion->HasMaterials())
        {
            m_dirtyDetailRegion.AddAabb(oldRegion);
            m_dirtyDetailRegion.AddAabb(newRegion);
        }
    }

    void TerrainDetailMaterialManager::CheckDetailMaterialForDeletion(uint16_t detailMaterialId)
    {
        auto& detailMaterialData = m_detailMaterials.GetData(detailMaterialId);
        if (--detailMaterialData.m_refCount == 0)
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
                if (imageIndex != InvalidImageIndex)
                {
                    m_bindlessImageHandler->RemoveBindlessImage(imageIndex);
                }
            }

            m_detailMaterialShaderData.Release(bufferIndex);
            m_detailMaterials.RemoveIndex(detailMaterialId);

            m_detailMaterialBufferNeedsUpdate = true;
        }
    }
    
    uint16_t TerrainDetailMaterialManager::CreateOrUpdateDetailMaterial(MaterialInstance material)
    {
        static constexpr uint16_t InvalidDetailMaterial = 0xFFFF;
        uint16_t detailMaterialId = InvalidDetailMaterial;

        for (const auto& detailMaterialData : m_detailMaterials.GetDataVector())
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
    
    void TerrainDetailMaterialManager::UpdateDetailMaterialData(uint16_t detailMaterialIndex, MaterialInstance material)
    {
        DetailMaterialData& materialData = m_detailMaterials.GetData(detailMaterialIndex);
        materialData.m_assetId = material->GetAssetId();
        
        DetailMaterialShaderData& shaderData = m_detailMaterialShaderData.GetElement(materialData.m_detailMaterialBufferIndex);
        shaderData = DetailMaterialShaderData();

        DetailTextureFlags& flags = shaderData.m_flags;
        flags = DetailTextureFlags::None;
            
        auto getIndex = [&](const char* const indexName) -> AZ::RPI::MaterialPropertyIndex
        {
            const AZ::RPI::MaterialPropertyIndex index = material->FindPropertyIndex(AZ::Name(indexName));
            AZ_Warning(TerrainDetailMaterialManagerName, index.IsValid(), "Failed to find shader input constant %s.", indexName);
            return index;
        };

        auto applyProperty = [&](const char* const indexName, auto& ref) -> void
        {
            const auto index = getIndex(indexName);
            if (index.IsValid())
            {
                // GetValue<T>() expects the actual type, not a reference type, so the reference needs to be removed.
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
                if (imageIndex == InvalidImageIndex)
                {
                    imageIndex = m_bindlessImageHandler->AppendBindlessImage(ref->GetImageView());
                }
                else
                {
                    m_bindlessImageHandler->UpdateBindlessImage(imageIndex, ref->GetImageView());
                }
            }
            else if (imageIndex != InvalidImageIndex)
            {
                m_bindlessImageHandler->RemoveBindlessImage(imageIndex);
                imageIndex = InvalidImageIndex;
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

        const auto baseColorIndex = getIndex(BaseColorColor);
        if (baseColorIndex.IsValid())
        {
            const AZ::Color baseColor = material->GetPropertyValue(baseColorIndex).GetValue<AZ::Color>();
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

        bool terrainSettingsOverrideParallax = false;
        applyProperty(TerrainSettingsOverrideParallax, terrainSettingsOverrideParallax);

        if (terrainSettingsOverrideParallax)
        {
            applyProperty(TerrainHeightFactor, shaderData.m_heightFactor);
            applyProperty(TerrainHeightOffset, shaderData.m_heightOffset);
        }
        else
        {
            // Parallax ranges from 0 to 0.1, so multiply by 10 to be in the 0-1 range.
            applyProperty(ParallaxHeightFactor, shaderData.m_heightFactor);
            shaderData.m_heightFactor *= 10.0f;
            applyProperty(ParallaxHeightOffset, shaderData.m_heightOffset);
            shaderData.m_heightOffset *= 10.0f;
        }
        applyProperty(HeightBlendFactor, shaderData.m_heightBlendFactor);
        applyProperty(HeightWeightClampFactor, shaderData.m_heightWeightClampFactor);
        shaderData.m_heightWeightClampFactor = 1.0f / AZStd::GetMax(0.0001f, shaderData.m_heightWeightClampFactor);

        AZ::Render::UvTransformDescriptor transformDescriptor;
        applyProperty(UvCenter, transformDescriptor.m_center);
        applyProperty(UvScale, transformDescriptor.m_scale);
        applyProperty(UvTileU, transformDescriptor.m_scaleX);
        applyProperty(UvTileV, transformDescriptor.m_scaleY);
        applyProperty(UvOffsetU, transformDescriptor.m_translateX);
        applyProperty(UvOffsetV, transformDescriptor.m_translateY);
        applyProperty(UvRotateDegrees, transformDescriptor.m_rotateDegrees);

        AZStd::array<AZ::Render::TransformType, 3> order =
        {
            AZ::Render::TransformType::Rotate,
            AZ::Render::TransformType::Translate,
            AZ::Render::TransformType::Scale,
        };

        AZ::Matrix3x3 uvTransformMatrix = AZ::Render::CreateUvTransformMatrix(transformDescriptor, order);
        uvTransformMatrix.GetRow(0).StoreToFloat3(&shaderData.m_uvTransform[0]);
        uvTransformMatrix.GetRow(1).StoreToFloat3(&shaderData.m_uvTransform[4]);
        uvTransformMatrix.GetRow(2).StoreToFloat3(&shaderData.m_uvTransform[8]);

        // Store a hash of the matrix in element in an unused portion for quick comparisons in the shader
        size_t hash64 = 0;
        for (float value : shaderData.m_uvTransform)
        {
            AZStd::hash_combine(hash64, value);
        }
        uint32_t hash32 = uint32_t((hash64 ^ (hash64 >> 32)) & 0xFFFFFFFF);
        shaderData.m_uvTransform[3] = *reinterpret_cast<float*>(&hash32);

        m_detailMaterialBufferNeedsUpdate = true;
    }
    
    void TerrainDetailMaterialManager::CheckUpdateDetailTexture(const AZ::Vector3& cameraPosition)
    {
        
        AZ::Aabb untouchedRegion = AZ::Aabb::CreateNull();
        ClipmapBoundsRegionList edgeUpdatedRegions =
            m_detailMaterialIdBounds.UpdateCenter(AZ::Vector2(cameraPosition.GetX(), cameraPosition.GetY()), &untouchedRegion);
        
        if (!m_detailTextureImage)
        {
            // If the m_detailTextureImage doesn't exist, create it and populate the entire texture

            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, m_detailTextureSize, m_detailTextureSize, AZ::RHI::Format::R8G8B8A8_UINT
            );
            const AZ::Name TerrainDetailName = AZ::Name(TerrainDetailChars);
            m_detailTextureImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainDetailName, nullptr, nullptr);
            AZ_Error(TerrainDetailMaterialManagerName, m_detailTextureImage, "Failed to initialize the detail texture image.");

            ClipmapBoundsRegionList updateRegions = m_detailMaterialIdBounds.TransformRegion(m_detailMaterialIdBounds.GetWorldBounds());
            for (const auto& region : updateRegions)
            {
                UpdateDetailTexture(region.m_worldAabb, region.m_localAabb);
            }
        }
        else
        {
            // Update the edge regions
            for (const auto& region : edgeUpdatedRegions)
            {
                UpdateDetailTexture(region.m_worldAabb, region.m_localAabb);
            }

            if (m_dirtyDetailRegion.IsValid())
            {
                m_dirtyDetailRegion = m_dirtyDetailRegion.GetClamped(untouchedRegion);
                if (m_dirtyDetailRegion.IsValid())
                {
                    ClipmapBoundsRegionList updateRegions = m_detailMaterialIdBounds.TransformRegion(m_dirtyDetailRegion);
                    for (const auto& region : updateRegions)
                    {
                        UpdateDetailTexture(region.m_worldAabb, region.m_localAabb);
                    }
                }
                m_dirtyDetailRegion = AZ::Aabb::CreateNull();
            }
        }
    }
    
    void TerrainDetailMaterialManager::UpdateDetailTexture(const AZ::Aabb& worldUpdateAabb, const Aabb2i& textureUpdateAabb)
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
        
        const int32_t width = textureUpdateAabb.m_max.m_x - textureUpdateAabb.m_min.m_x;
        const int32_t height = textureUpdateAabb.m_max.m_y - textureUpdateAabb.m_min.m_y;

        AZStd::vector<DetailMaterialPixel> pixels(width * height);

        auto perPositionCallback = [this, &pixels, &width](
            size_t xIndex, size_t yIndex,
            const AzFramework::SurfaceData::SurfacePoint& surfacePoint,
            [[maybe_unused]] bool terrainExists)
        {
            // Store the top two surface weights in the texture with m_blend storing the relative weight.
            DetailMaterialPixel& pixel = pixels.at(yIndex * width + xIndex);
            uint32_t foundMaterials = 0;
            float firstWeight = 0.0f;
            float secondWeight = 0.0f;

            AZ::Vector2 position(surfacePoint.m_position.GetX(), surfacePoint.m_position.GetY());
            const DetailMaterialListRegion* region = FindRegionForPosition(position);

            if (region == nullptr)
            {
                pixel.m_material1 = m_passthroughMaterialId;
                return;
            }

            for (const auto& surfaceTagWeight : surfacePoint.m_surfaceTags)
            {
                if (surfaceTagWeight.m_weight > 0.0f)
                {
                    AZ::Crc32 surfaceType = surfaceTagWeight.m_surfaceType;
                    uint16_t materialId = GetDetailMaterialForSurfaceType(*region, surfaceType);
                    if (materialId < 255)
                    {
                        if (foundMaterials == 0)
                        {
                            // Found first material. Save its weight to calculate blend later.
                            ++foundMaterials;
                            pixel.m_material1 = aznumeric_cast<uint8_t>(materialId);
                            firstWeight = surfaceTagWeight.m_weight;
                        }
                        else if (materialId == pixel.m_material1)
                        {
                            // Same material as the first material, just add the weights.
                            firstWeight += surfaceTagWeight.m_weight;
                        }
                        else if (foundMaterials == 1)
                        {
                            // Found second material. Save its weight to calculate blend later.
                            ++foundMaterials;
                            secondWeight += surfaceTagWeight.m_weight;
                            pixel.m_material2 = aznumeric_cast<uint8_t>(materialId);
                        }
                        else if (materialId == pixel.m_material2)
                        {
                            // Same material as the second material, just add the weights.
                            secondWeight += surfaceTagWeight.m_weight;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            
            if (foundMaterials == 0)
            {
                // No materials found, so use the default material.
                uint8_t defaultMaterial = region->m_defaultDetailMaterialId == InvalidDetailMaterialId ? m_passthroughMaterialId :
                    aznumeric_cast<uint8_t>(m_detailMaterials.GetData(region->m_defaultDetailMaterialId).m_detailMaterialBufferIndex);
                pixel.m_material1 = defaultMaterial;
            }
            else if (foundMaterials == 2)
            {
                float totalWeight = firstWeight + secondWeight;
                float blendWeight = 1.0f - (firstWeight / totalWeight);
                pixel.m_blend = aznumeric_cast<uint8_t>(blendWeight * 255.0f + 0.5f);
            }
        };
            
        AZ::Vector2 stepSize(m_detailTextureScale);

        AZStd::binary_semaphore wait;

        AZStd::shared_ptr<AzFramework::Terrain::QueryAsyncParams> asyncParams
            = AZStd::make_shared<AzFramework::Terrain::QueryAsyncParams>();
        asyncParams->m_desiredNumberOfJobs = AzFramework::Terrain::QueryAsyncParams::UseMaxJobs;
        asyncParams->m_minPositionsPerJob = 4 * m_detailTextureSize; // do at least 4 rows per job.
        asyncParams->m_completionCallback = [&wait](AZStd::shared_ptr<AzFramework::Terrain::TerrainJobContext>)
        {
            wait.release();
        };

        AzFramework::Terrain::TerrainQueryRegion queryRegion(worldUpdateAabb.GetMin(), width, height, stepSize);
        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequests::QueryRegionAsync,
            queryRegion,
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::SurfaceData,
            perPositionCallback,
            AzFramework::Terrain::TerrainDataRequests::Sampler::DEFAULT,
            asyncParams);

        wait.acquire();

        const int32_t left = textureUpdateAabb.m_min.m_x;
        const int32_t top = textureUpdateAabb.m_min.m_y;

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
    
    uint16_t TerrainDetailMaterialManager::GetDetailMaterialForSurfaceType(const DetailMaterialListRegion& materialRegion, AZ::Crc32 surfaceType) const
    {
        for (const auto& materialSurface : materialRegion.m_materialsForSurfaces)
        {
            if (materialSurface.m_surfaceTag == surfaceType)
            {
                return m_detailMaterials.GetData(materialSurface.m_detailMaterialId).m_detailMaterialBufferIndex;
            }
        }
        return InvalidDetailMaterialId;
    }
    
    auto TerrainDetailMaterialManager::FindRegionForPosition(const AZ::Vector2& position) const -> const DetailMaterialListRegion*
    {
        for (const auto& materialRegion : m_detailMaterialRegions.GetDataVector())
        {
            if (SurfaceData::AabbContains2D(materialRegion.m_region, position))
            {
                return &materialRegion;
            }
        }
        return nullptr;
    }

    void TerrainDetailMaterialManager::InitializePassthroughDetailMaterial()
    {
        m_passthroughMaterialId = aznumeric_cast<uint8_t>(m_detailMaterialShaderData.Reserve());
        DetailMaterialShaderData& materialShaderData = m_detailMaterialShaderData.GetElement(m_passthroughMaterialId);
        // Material defaults to white (1.0, 1.0, 1.0), set the blend mode to multiply so it passes through to the macro material.
        materialShaderData.m_flags = DetailTextureFlags::BlendModeMultiply;
    }

    auto TerrainDetailMaterialManager::FindByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<DetailMaterialListRegion>& container)
        -> DetailMaterialListRegion*
    {
        for (DetailMaterialListRegion& data : container.GetDataVector())
        {
            if (data.m_entityId == entityId)
            {
                return &data;
            }
        }
        return nullptr;
    }
    
    auto TerrainDetailMaterialManager::FindOrCreateByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<DetailMaterialListRegion>& container)
        -> DetailMaterialListRegion&
    {
        DetailMaterialListRegion* dataPtr = FindByEntityId(entityId, container);
        if (dataPtr != nullptr)
        {
            return *dataPtr;
        }

        const uint16_t slotId = container.GetFreeSlotIndex();
        AZ_Assert(slotId != AZ::Render::IndexedDataVector<TerrainDetailMaterialManager>::NoFreeSlot, "Ran out of indices");

        DetailMaterialListRegion& data = container.GetData(slotId);
        data.m_entityId = entityId;
        return data;
    }
    
    void TerrainDetailMaterialManager::RemoveByEntityId(AZ::EntityId entityId, AZ::Render::IndexedDataVector<DetailMaterialListRegion>& container)
    {
        for (DetailMaterialListRegion& data : container.GetDataVector())
        {
            if (data.m_entityId == entityId)
            {
                container.RemoveData(&data);
                return;
            }
        }
        AZ_Assert(false, "Entity Id not found in container.")
    }

}
