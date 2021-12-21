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

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <SurfaceData/SurfaceDataSystemRequestBus.h>

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
        static const char* const HeightFactor("parallax.factor");
        static const char* const HeightOffset("parallax.offset");
        static const char* const HeightBlendFactor("parallax.blendFactor");
    }
    
    namespace TerrainSrgInputs
    {
        static const char* const DetailMaterialIdImage("m_detailMaterialIdImage");
        static const char* const DetailMaterialData("m_detailMaterialData");
        static const char* const DetailMaterialIdImageCenter("m_detailMaterialIdImageCenter");
        static const char* const DetailHalfPixelUv("m_detailHalfPixelUv");
        static const char* const DetailAabb("m_detailAabb");
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
        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        AZ_Error(TerrainDetailMaterialManagerName, bindlessImageHandler, "bindlessImageHandler must not be null.");
        AZ_Error(TerrainDetailMaterialManagerName, terrainSrg, "terrainSrg must not be null.");
        AZ_Error(TerrainDetailMaterialManagerName, !m_isInitialized, "Already initialized.");

        if (!bindlessImageHandler || !terrainSrg || m_isInitialized)
        {
            return;
        }
        
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
    
    bool TerrainDetailMaterialManager::UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        const AZ::RHI::ShaderResourceGroupLayout* terrainSrgLayout = terrainSrg->GetLayout();
            
        m_detailMaterialIdPropertyIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailMaterialIdImage));
        AZ_Error(TerrainDetailMaterialManagerName, m_detailMaterialIdPropertyIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::DetailMaterialIdImage);
        
        m_detailCenterPropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailMaterialIdImageCenter));
        AZ_Error(TerrainDetailMaterialManagerName, m_detailCenterPropertyIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::DetailMaterialIdImageCenter);

        m_detailHalfPixelUvPropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailHalfPixelUv));
        AZ_Error(TerrainDetailMaterialManagerName, m_detailHalfPixelUvPropertyIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::DetailHalfPixelUv);
        
        m_detailAabbPropertyIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::DetailAabb));
        AZ_Error(TerrainDetailMaterialManagerName, m_detailAabbPropertyIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::DetailAabb);

        // Set up the gpu buffer for detail material data
        AZ::Render::GpuBufferHandler::Descriptor desc;
        desc.m_bufferName = "Detail Material Data";
        desc.m_bufferSrgName = TerrainSrgInputs::DetailMaterialData;
        desc.m_elementSize = sizeof(DetailMaterialShaderData);
        desc.m_srgLayout = terrainSrgLayout;
        m_detailMaterialDataBuffer = AZ::Render::GpuBufferHandler(desc);

        bool IndicesValid =
            m_detailMaterialIdPropertyIndex.IsValid() &&
            m_detailCenterPropertyIndex.IsValid() &&
            m_detailHalfPixelUvPropertyIndex.IsValid() &&
            m_detailAabbPropertyIndex.IsValid();

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
        m_bindlessImageHandler.reset();

        m_detailTextureImage = {};
        m_detailMaterials.Clear();
        m_detailMaterialRegions.Clear();
        m_detailMaterialShaderData.Clear();
        m_detailMaterialDataBuffer.Release();

        m_dirtyDetailRegion = AZ::Aabb::CreateNull();
        m_previousCameraPosition = AZ::Vector3(AZStd::numeric_limits<float>::max(), 0.0, 0.0);
        m_detailTextureBounds = {};
        m_detailTextureCenter = {};

        m_detailMaterialBufferNeedsUpdate = false;
        m_detailImageNeedsUpdate = false;

        TerrainAreaMaterialNotificationBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();

        m_isInitialized = false;
    }

    void TerrainDetailMaterialManager::Update(const AZ::Vector3& cameraPosition, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        if (m_detailMaterialBufferNeedsUpdate)
        {
            m_detailMaterialBufferNeedsUpdate = false;
            m_detailMaterialDataBuffer.UpdateBuffer(m_detailMaterialShaderData.GetRawData(), aznumeric_cast<uint32_t>(m_detailMaterialShaderData.GetSize()));
        }
        
        if (m_dirtyDetailRegion.IsValid() || !cameraPosition.IsClose(m_previousCameraPosition) || m_detailImageNeedsUpdate)
        {
            if (r_terrainDebugDetailImageUpdates)
            {
                AZ_Printf("TerrainDetailMaterialManager", "Previous Camera: (%f, %f, %f) New Cameara: (%f, %f, %f)",
                    m_previousCameraPosition.GetX(), m_previousCameraPosition.GetY(), m_previousCameraPosition.GetZ(),
                    cameraPosition.GetX(), cameraPosition.GetY(), cameraPosition.GetZ());
            }
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

            terrainSrg->SetConstant(m_detailAabbPropertyIndex, detailAabb);
            terrainSrg->SetConstant(m_detailHalfPixelUvPropertyIndex, 0.5f / DetailTextureSize);
            terrainSrg->SetConstant(m_detailCenterPropertyIndex, detailUvOffset);
            terrainSrg->SetImage(m_detailMaterialIdPropertyIndex, m_detailTextureImage);

            m_detailMaterialDataBuffer.UpdateSrg(terrainSrg.get());
        }
        
        m_detailImageNeedsUpdate = false;
    }

    void TerrainDetailMaterialManager::OnTerrainDataChanged(const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & TerrainDataChangedMask::SurfaceData) != 0)
        {
            m_dirtyDetailRegion.AddAabb(dirtyRegion);
        }
    }
    
    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingCreated(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);

        // Validate that the surface tag is new
        for (DetailMaterialSurface& surface : materialRegion.m_materialsForSurfaces)
        {
            if (surface.m_surfaceTag == surfaceTag)
            {
                AZ_Error(TerrainDetailMaterialManagerName, false, "Already have a surface material mapping for this surface tag.");
                return;
            }
        }

        uint16_t detailMaterialId = CreateOrUpdateDetailMaterial(material);
        materialRegion.m_materialsForSurfaces.push_back({ surfaceTag, detailMaterialId });
        m_detailMaterials.GetData(detailMaterialId).refCount++;
        m_dirtyDetailRegion.AddAabb(materialRegion.m_region);
    }
    
    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingDestroyed(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag)
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
        AZ_Error(TerrainDetailMaterialManagerName, false, "Could not find surface tag to destroy for OnTerrainSurfaceMaterialMappingDestroyed().");
    }

    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingChanged(AZ::EntityId entityId, SurfaceData::SurfaceTag surfaceTag, MaterialInstance material)
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

    void TerrainDetailMaterialManager::OnTerrainSurfaceMaterialMappingRegionChanged(AZ::EntityId entityId, const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion)
    {
        DetailMaterialListRegion& materialRegion = FindOrCreateByEntityId(entityId, m_detailMaterialRegions);
        materialRegion.m_region = newRegion;
        m_dirtyDetailRegion.AddAabb(oldRegion);
        m_dirtyDetailRegion.AddAabb(newRegion);
    }

    void TerrainDetailMaterialManager::CheckDetailMaterialForDeletion(uint16_t detailMaterialId)
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
    
    void TerrainDetailMaterialManager::UpdateDetailMaterialData(uint16_t detailMaterialIndex, MaterialInstance material)
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
            AZ_Warning(TerrainDetailMaterialManagerName, index.IsValid(), "Failed to find shader input constant %s.", indexName);
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

        m_detailMaterialBufferNeedsUpdate = true;
    }
    
    void TerrainDetailMaterialManager::CheckUpdateDetailTexture(const Aabb2i& newBounds, const Vector2i& newCenter)
    {
        if (r_terrainDebugDetailImageUpdates)
        {
            AZ_Printf("TerrainDetailMaterialManager", "Old Bounds: m(%i, %i)M(%i, %i) New Bounds: m(%i, %i)M(%i, %i)",
                m_detailTextureBounds.m_min.m_x, m_detailTextureBounds.m_min.m_y, m_detailTextureBounds.m_max.m_x, m_detailTextureBounds.m_max.m_y,
                newBounds.m_min.m_x, newBounds.m_min.m_y, newBounds.m_max.m_x, newBounds.m_max.m_y
            );
        }
        if (!m_detailTextureImage)
        {
            // If the m_detailTextureImage doesn't exist, create it and populate the entire texture

            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
            AZ::RHI::ImageDescriptor imageDescriptor = AZ::RHI::ImageDescriptor::Create2D(
                AZ::RHI::ImageBindFlags::ShaderRead, DetailTextureSize, DetailTextureSize, AZ::RHI::Format::R8G8B8A8_UINT
            );
            const AZ::Name TerrainDetailName = AZ::Name(TerrainDetailChars);
            m_detailTextureImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainDetailName, nullptr, nullptr);
            AZ_Error(TerrainDetailMaterialManagerName, m_detailTextureImage, "Failed to initialize the detail texture image.");
            
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
                
                if (r_terrainDebugDetailImageUpdates)
                {
                    AZ_Printf("TerrainDetailMaterialManager", "Updating horizontal edge: m(%i, %i)M(%i, %i)",
                        updateBounds.m_min.m_x, updateBounds.m_min.m_y, updateBounds.m_max.m_x, updateBounds.m_max.m_y);
                }
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
                
                if (r_terrainDebugDetailImageUpdates)
                {
                    AZ_Printf("TerrainDetailMaterialManager", "Updating vertical edge: m(%i, %i)M(%i, %i)",
                        updateBounds.m_min.m_x, updateBounds.m_min.m_y, updateBounds.m_max.m_x, updateBounds.m_max.m_y);
                }
                UpdateDetailTexture(updateBounds, newBounds, newCenter);
            }

            if (m_dirtyDetailRegion.IsValid())
            {
                if (r_terrainDebugDetailImageUpdates)
                {
                    AZ_Printf("TerrainDetailMaterialManager", "m_dirtyDetailRegion: m(%f, %f)M(%f, %f)",
                        m_dirtyDetailRegion.GetMin().GetX(), m_dirtyDetailRegion.GetMin().GetY(), m_dirtyDetailRegion.GetMax().GetX(), m_dirtyDetailRegion.GetMax().GetY());
                }
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
                        
                        if (r_terrainDebugDetailImageUpdates)
                        {
                            AZ_Printf("TerrainDetailMaterialManager", "Updating dirty region: m(%i, %i)M(%i, %i)",
                                updateBounds.m_min.m_x, updateBounds.m_min.m_y, updateBounds.m_max.m_x, updateBounds.m_max.m_y);
                        }
                        UpdateDetailTexture(updateBounds, newBounds, newCenter);
                    }
                }
            }
        }
    }
    
    void TerrainDetailMaterialManager::UpdateDetailTexture(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel)
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

        if (updateAreaCount > 0)
        {
            m_detailImageNeedsUpdate = true;
        }

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
    
    uint8_t TerrainDetailMaterialManager::CalculateUpdateRegions(const Aabb2i& updateArea, const Aabb2i& textureBounds, const Vector2i& centerPixel,
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

    uint16_t TerrainDetailMaterialManager::GetDetailMaterialForSurfaceTypeAndPosition(AZ::Crc32 surfaceType, const AZ::Vector2& position)
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
