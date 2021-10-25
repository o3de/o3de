/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainFeatureProcessor.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/math.h>
#include <AzCore/Math/Frustum.h>

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
    }

    namespace MaterialInputs
    {
        // Terrain material
        static const char* const HeightmapImage("settings.heightmapImage");
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
        if ((dataChangedMask & (TerrainDataChangedMask::HeightData | TerrainDataChangedMask::Settings)) == 0)
        {
            return;
        }

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
    
    void TerrainFeatureProcessor::OnTerrainMacroMaterialCreated(AZ::EntityId entityId, const MacroMaterialData& newMaterialData)
    {
        MacroMaterialData& materialData = FindOrCreateMacroMaterial(entityId);

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
        MacroMaterialData& data = FindOrCreateMacroMaterial(entityId);
        UpdateMacroMaterialData(data, newMaterialData);
    }
    
    void TerrainFeatureProcessor::OnTerrainMacroMaterialRegionChanged(
        AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& oldRegion, const AZ::Aabb& newRegion)
    {
        MacroMaterialData& materialData = FindOrCreateMacroMaterial(entityId);
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
        MacroMaterialData* materialData = FindMacroMaterial(entityId);

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
        RemoveMacroMaterial(entityId);
    }

    void TerrainFeatureProcessor::UpdateTerrainData()
    {
        static const AZ::Name TerrainHeightmapName = AZ::Name(TerrainHeightmapChars);

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
            m_areaData.m_heightmapImage = AZ::RPI::AttachmentImage::Create(*imagePool.get(), imageDescriptor, TerrainHeightmapName, nullptr, nullptr);
            AZ_Error(TerrainFPName, m_areaData.m_heightmapImage, "Failed to initialize the heightmap image!");
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

            if (m_areaData.m_heightmapUpdated)
            {
                UpdateTerrainData();

                const AZ::Data::Instance<AZ::RPI::Image> heightmapImage = m_areaData.m_heightmapImage;
                m_materialInstance->SetPropertyValue(m_heightmapPropertyIndex, heightmapImage);
                m_materialInstance->Compile();
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
    
    MacroMaterialData* TerrainFeatureProcessor::FindMacroMaterial(AZ::EntityId entityId)
    {
        for (MacroMaterialData& data : m_macroMaterials.GetDataVector())
        {
            if (data.m_entityId == entityId)
            {
                return &data;
            }
        }
        return nullptr;
    }

    MacroMaterialData& TerrainFeatureProcessor::FindOrCreateMacroMaterial(AZ::EntityId entityId)
    {
        MacroMaterialData* dataPtr = FindMacroMaterial(entityId);
        if (dataPtr != nullptr)
        {
            return *dataPtr;
        }

        const uint16_t slotId = m_macroMaterials.GetFreeSlotIndex();
        AZ_Assert(slotId != m_macroMaterials.NoFreeSlot, "Ran out of indices for macro materials");

        MacroMaterialData& data = m_macroMaterials.GetData(slotId);
        data.m_entityId = entityId;
        return data;
    }

    void TerrainFeatureProcessor::RemoveMacroMaterial(AZ::EntityId entityId)
    {
        for (MacroMaterialData& data : m_macroMaterials.GetDataVector())
        {
            if (data.m_entityId == entityId)
            {
                m_macroMaterials.RemoveData(&data);
                return;
            }
        }
        AZ_Assert(false, "Entity Id not found in m_macroMaterials.")
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
}
