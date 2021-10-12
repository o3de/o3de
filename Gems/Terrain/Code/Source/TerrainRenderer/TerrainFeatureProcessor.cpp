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
        static const char* const HeightmapImage("settings.heightmapImage");
    }

    namespace ShaderInputs
    {
        static const char* const ModelToWorld("m_modelToWorld");
        static const char* const TerrainData("m_terrainData");
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
        m_areaData = {};
        m_dirtyRegion = AZ::Aabb::CreateNull();
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
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        AZ::RPI::MaterialReloadNotificationBus::Handler::BusDisconnect();

        m_patchModel = {};
        m_areaData = {};
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
        if (dataChangedMask != TerrainDataChangedMask::HeightData && dataChangedMask != TerrainDataChangedMask::Settings)
        {
            return;
        }

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        const AZ::Aabb& regionToUpdate = dirtyRegion.IsValid() ? dirtyRegion : worldBounds;

        m_dirtyRegion.AddAabb(regionToUpdate);
        m_dirtyRegion.Clamp(worldBounds);

        AZ::Transform transform = AZ::Transform::CreateTranslation(worldBounds.GetCenter());
        
        AZ::Vector2 queryResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        m_areaData.m_transform = transform;
        m_areaData.m_heightScale = worldBounds.GetZExtent();
        m_areaData.m_terrainBounds = worldBounds;
        m_areaData.m_heightmapImageWidth = aznumeric_cast<uint32_t>(worldBounds.GetXExtent() / queryResolution.GetX());
        m_areaData.m_heightmapImageHeight = aznumeric_cast<uint32_t>(worldBounds.GetYExtent() / queryResolution.GetY());
        m_areaData.m_updateWidth = aznumeric_cast<uint32_t>(m_dirtyRegion.GetXExtent() / queryResolution.GetX());
        m_areaData.m_updateHeight = aznumeric_cast<uint32_t>(m_dirtyRegion.GetYExtent() / queryResolution.GetY());
        // Currently query resolution is multidimensional but the rendering system only supports this changing in one dimension.
        m_areaData.m_sampleSpacing = queryResolution.GetX();
        m_areaData.m_propertiesDirty = true;
    }

    void TerrainFeatureProcessor::UpdateTerrainData()
    {
        static const AZ::Name TerrainHeightmapName = AZ::Name(TerrainHeightmapChars);

        uint32_t width = m_areaData.m_updateWidth;
        uint32_t height = m_areaData.m_updateHeight;
        const AZ::Aabb& worldBounds = m_areaData.m_terrainBounds;
        float queryResolution = m_areaData.m_sampleSpacing;

        AZ::RHI::Size worldSize = AZ::RHI::Size(m_areaData.m_heightmapImageWidth, m_areaData.m_heightmapImageHeight, 1);

        if (!m_areaData.m_heightmapImage || m_areaData.m_heightmapImage->GetDescriptor().m_size != worldSize)
        {
            // World size changed, so the whole world needs updating.
            width = worldSize.m_width;
            height = worldSize.m_height;
            m_dirtyRegion = worldBounds;

            AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
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

                    float clampedHeight = AZ::GetClamp((terrainHeight - worldBounds.GetMin().GetZ()) / worldBounds.GetExtents().GetZ(), 0.0f, 1.0f);
                    float expandedHeight = AZStd::roundf(clampedHeight * AZStd::numeric_limits<uint16_t>::max());
                    uint16_t uint16Height = aznumeric_cast<uint16_t>(expandedHeight);

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

    void TerrainFeatureProcessor::ProcessSurfaces(const FeatureProcessor::RenderPacket& process)
    {
        AZ_PROFILE_FUNCTION(AzRender);

        if (!m_areaData.m_terrainBounds.IsValid())
        {
            return;
        }
        
        if (m_areaData.m_propertiesDirty && m_materialInstance && m_materialInstance->CanCompile())
        {
            UpdateTerrainData();

            m_areaData.m_propertiesDirty = false;
            m_sectorData.clear();

            AZ::RPI::MaterialPropertyIndex heightmapPropertyIndex =
                m_materialInstance->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(MaterialInputs::HeightmapImage));
            AZ_Error(TerrainFPName, heightmapPropertyIndex.IsValid(), "Failed to find material input constant %s.", MaterialInputs::HeightmapImage);
            AZ::Data::Instance<AZ::RPI::Image> heightmapImage = m_areaData.m_heightmapImage;
            m_materialInstance->SetPropertyValue(heightmapPropertyIndex, heightmapImage);
            m_materialInstance->Compile();

            const auto layout = m_materialInstance->GetAsset()->GetObjectSrgLayout();

            m_modelToWorldIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::ModelToWorld));
            AZ_Error(TerrainFPName, m_modelToWorldIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::ModelToWorld);

            m_terrainDataIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::TerrainData));
            AZ_Error(TerrainFPName, m_terrainDataIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::TerrainData);

            float xFirstPatchStart =
                m_areaData.m_terrainBounds.GetMin().GetX() - fmod(m_areaData.m_terrainBounds.GetMin().GetX(), GridMeters);
            float xLastPatchStart = m_areaData.m_terrainBounds.GetMax().GetX() - fmod(m_areaData.m_terrainBounds.GetMax().GetX(), GridMeters);
            float yFirstPatchStart =
                m_areaData.m_terrainBounds.GetMin().GetY() - fmod(m_areaData.m_terrainBounds.GetMin().GetY(), GridMeters);
            float yLastPatchStart = m_areaData.m_terrainBounds.GetMax().GetY() - fmod(m_areaData.m_terrainBounds.GetMax().GetY(), GridMeters);

            for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += GridMeters)
            {
                for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += GridMeters)
                {
                    const auto& materialAsset = m_materialInstance->GetAsset();
                    auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
                    auto objectSrg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                    if (!objectSrg)
                    {
                        AZ_Warning("TerrainFeatureProcessor", false, "Failed to create a new shader resource group, skipping.");
                        continue;
                    }

                    { // Update SRG
                        
                        AZStd::array<float, 2> uvMin = { 0.0f, 0.0f };
                        AZStd::array<float, 2> uvMax = { 1.0f, 1.0f };

                        uvMin[0] = (float)((xPatch - m_areaData.m_terrainBounds.GetMin().GetX()) / m_areaData.m_terrainBounds.GetXExtent());
                        uvMin[1] = (float)((yPatch - m_areaData.m_terrainBounds.GetMin().GetY()) / m_areaData.m_terrainBounds.GetYExtent());

                        uvMax[0] =
                            (float)(((xPatch + GridMeters) - m_areaData.m_terrainBounds.GetMin().GetX()) / m_areaData.m_terrainBounds.GetXExtent());
                        uvMax[1] =
                            (float)(((yPatch + GridMeters) - m_areaData.m_terrainBounds.GetMin().GetY()) / m_areaData.m_terrainBounds.GetYExtent());

                        AZStd::array<float, 2> uvStep =
                        {
                            1.0f / m_areaData.m_heightmapImageWidth, 1.0f / m_areaData.m_heightmapImageHeight,
                        };

                        AZ::Transform transform = m_areaData.m_transform;
                        transform.SetTranslation(xPatch, yPatch, m_areaData.m_transform.GetTranslation().GetZ());

                        AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(transform);

                        objectSrg->SetConstant(m_modelToWorldIndex, matrix3x4);

                        ShaderTerrainData terrainDataForSrg;
                        terrainDataForSrg.m_sampleSpacing = m_areaData.m_sampleSpacing;
                        terrainDataForSrg.m_heightScale = m_areaData.m_heightScale;
                        terrainDataForSrg.m_uvMin = uvMin;
                        terrainDataForSrg.m_uvMax = uvMax;
                        terrainDataForSrg.m_uvStep = uvStep;
                        objectSrg->SetConstant(m_terrainDataIndex, terrainDataForSrg);

                        objectSrg->Compile();
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
                        uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                        drawPacket.SetStencilRef(stencilRef);
                        drawPacket.Update(*GetParentScene(), true);
                    }

                    sectorData.m_aabb =
                        AZ::Aabb::CreateFromMinMax(
                            AZ::Vector3(xPatch, yPatch, m_areaData.m_terrainBounds.GetMin().GetZ()),
                            AZ::Vector3(xPatch + GridMeters, yPatch + GridMeters, m_areaData.m_terrainBounds.GetMax().GetZ())
                        );
                    sectorData.m_srg = objectSrg;
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
                    AZ::Vector3 cameraPosition = view->GetCameraTransform().GetTranslation();
                    AZ::Vector2 cameraPositionXY = AZ::Vector2(cameraPosition.GetX(), cameraPosition.GetY());
                    AZ::Vector2 sectorCenterXY = AZ::Vector2(sectorData.m_aabb.GetCenter().GetX(), sectorData.m_aabb.GetCenter().GetY());

                    float sectorDistance = sectorCenterXY.GetDistance(cameraPositionXY);
                    float lodForCamera = floorf(AZ::GetMax(0.0f, log2f(sectorDistance / (GridMeters * 4.0f))));
                    lodChoice = AZ::GetMin(lodChoice, aznumeric_cast<uint8_t>(lodForCamera));
                }
            }

            // Add the correct LOD draw packet for visible sectors.
            for (auto& view : process.m_views)
            {
                AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
                if (viewFrustum.IntersectAabb(sectorData.m_aabb) != AZ::IntersectResult::Exterior)
                {
                    uint8_t lodToRender = AZ::GetMin(lodChoice, aznumeric_cast<uint8_t>(sectorData.m_drawPackets.size() - 1));
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

        uint16_t gridVertices = gridSize + 1; // For m_gridSize quads, (m_gridSize + 1) vertices are needed.
        size_t size = gridVertices * gridVertices;
        size *= size;

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
                uint16_t topLeft = y * gridVertices + x;
                uint16_t topRight = topLeft + 1;
                uint16_t bottomLeft = (y + 1) * gridVertices + x;
                uint16_t bottomRight = bottomLeft + 1;

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

            auto positionBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_positions.size()), AZ::RHI::Format::R32G32_FLOAT);
            auto positionsOutcome = CreateBufferAsset(patchData.m_positions.data(), positionBufferViewDesc, "TerrainPatchPositions");
        
            auto uvBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_uvs.size()), AZ::RHI::Format::R32G32_FLOAT);
            auto uvsOutcome = CreateBufferAsset(patchData.m_uvs.data(), uvBufferViewDesc, "TerrainPatchUvs");
        
            auto indexBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_indices.size()), AZ::RHI::Format::R16_UINT);
            auto indicesOutcome = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

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
    
    void TerrainFeatureProcessor::OnMaterialReinitialized([[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Material>& material)
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
}
