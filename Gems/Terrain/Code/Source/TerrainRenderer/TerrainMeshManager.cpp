/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainMeshManager.h>

#include <AzCore/Math/Frustum.h>

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

#include <Atom/Feature/RenderCommon.h>

#include <TerrainRenderer/Aabb2i.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainMeshManagerName = "TerrainMeshManager";
    }

    namespace ShaderInputs
    {
        static const char* const PatchData("m_patchData");
    }

    TerrainMeshManager::TerrainMeshManager()
    {   
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
    }

    TerrainMeshManager::~TerrainMeshManager()
    {   
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
    }

    void TerrainMeshManager::Initialize()
    {
        if (!InitializePatchModel())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create Terrain render buffers!");
            return;
        }

        if (!InitializeSectorModel())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create Terrain render buffers!");
            return;
        }

        OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);

        m_isInitialized = true;
    }

    void TerrainMeshManager::SetConfiguration(const MeshConfiguration& config)
    {
        if (config != m_config)
        {
            m_config = config;
            m_rebuildSectors = true;
            OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
        }
    }

    bool TerrainMeshManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainMeshManager::Reset()
    {
        m_patchModel = {};
        m_sectorData.clear();
        m_rebuildSectors = true;
        m_isInitialized = false;
    }

    void TerrainMeshManager::CheckStackForUpdate(AZ::Vector3 newPosition)
    {
        for (uint32_t i = 0; i < m_sectorStack.size(); ++i)
        {
            const float gridMeters = (GridSize * m_sampleSpacing) * (2 << i);
            int32_t startCoordX = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetX() - m_config.m_firstLodDistance) / gridMeters)) * 2;
            int32_t startCoordY = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetY() - m_config.m_firstLodDistance) / gridMeters)) * 2;

            StackData& stackData = m_sectorStack.back();
            if (stackData.m_startCoordX != startCoordX || stackData.m_startCoordY != startCoordY)
            {
                // Sectors need updating
                uint32_t oldFirstIndexX = (stackData.m_1dSectorCount + (stackData.m_startCoordX % stackData.m_1dSectorCount)) % stackData.m_1dSectorCount;
                uint32_t oldFirstIndexY = (stackData.m_1dSectorCount + (stackData.m_startCoordY % stackData.m_1dSectorCount)) % stackData.m_1dSectorCount;

                uint32_t newFirstIndexX = (stackData.m_1dSectorCount + (startCoordX % stackData.m_1dSectorCount)) % stackData.m_1dSectorCount;
                uint32_t newFirstIndexY = (stackData.m_1dSectorCount + (startCoordY % stackData.m_1dSectorCount)) % stackData.m_1dSectorCount;

                int32_t count = 0;

                for (uint32_t j = 0; j < stackData.m_sectors.size(); ++j)
                {
                    // Local coordinates of sector from 0 to stackData.m_1dSectorCount
                    uint32_t localX = j % stackData.m_1dSectorCount;
                    uint32_t localY = j / stackData.m_1dSectorCount;

                    uint32_t stackX = (localX + newFirstIndexX) % stackData.m_1dSectorCount;
                    uint32_t stackY = (localY + newFirstIndexY) % stackData.m_1dSectorCount;
                    
                    // World sector coordinates
                    uint32_t oldLocalX = ((stackX - oldFirstIndexX + stackData.m_1dSectorCount) % stackData.m_1dSectorCount);
                    uint32_t oldLocalY = ((stackY - oldFirstIndexY + stackData.m_1dSectorCount) % stackData.m_1dSectorCount);

                    int32_t oldWorldCoordX = stackData.m_startCoordX + oldLocalX;
                    int32_t oldWorldCoordY = stackData.m_startCoordY + oldLocalY;

                    int32_t newWorldCoordX = startCoordX + localX;
                    int32_t newWorldCoordY = startCoordY + localY;

                    if (oldWorldCoordX == newWorldCoordX && oldWorldCoordY == newWorldCoordY)
                    {
                        continue; // This sector hasn't changed
                    }

                    StackSectorData& sector = stackData.m_sectors.at(j);

                    AZ::Vector3 min = AZ::Vector3(newWorldCoordX * gridMeters, newWorldCoordY * gridMeters, m_worldBounds.GetMin().GetZ());
                    AZ::Vector3 max = min + AZ::Vector3(gridMeters, gridMeters, m_worldBounds.GetMax().GetZ());
                    sector.m_aabb = AZ::Aabb::CreateFromMinMax(min, max);

                    ShaderTerrainData objectSrgData;
                    objectSrgData.m_xyTranslation = { min.GetX(), min.GetY() };
                    objectSrgData.m_xyScale = gridMeters;

                    sector.m_srg->SetConstant(m_patchDataIndex, objectSrgData);
                    sector.m_srg->Compile();

                    //sector.m_drawPacket.Update(parentScene, true);

                    ++count;
                }
                if (count > 0)
                {
                    AZ_Warning("Terrain Mesh", false, "Found %i sectors to rebuild", count);
                }
                else
                {
                    AZ_Warning("Terrain Mesh", false, "DID NOT FIND SECTORS TO REBUILD...");
                }

                stackData.m_startCoordX = startCoordX;
                stackData.m_startCoordY = startCoordY;
            }
        }

    }

    bool TerrainMeshManager::CheckRebuildSurfaces(const AZ::Vector3& position, MaterialInstance materialInstance, AZ::RPI::Scene& parentScene)
    {
        const float gridMeters = GridSize * m_sampleSpacing;

        if (m_rebuildSectors)
        {
            m_rebuildSectors = false;

            // Tile sectors

            m_sectorData.clear();

            const auto layout = materialInstance->GetAsset()->GetObjectSrgLayout();

            m_patchDataIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::PatchData));
            AZ_Error(TerrainMeshManagerName, m_patchDataIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::PatchData);

            const float xFirstPatchStart = AZStd::floorf(m_worldBounds.GetMin().GetX() / gridMeters) * gridMeters;
            const float xLastPatchStart = AZStd::floorf(m_worldBounds.GetMax().GetX() / gridMeters) * gridMeters;
            const float yFirstPatchStart = AZStd::floorf(m_worldBounds.GetMin().GetY() / gridMeters) * gridMeters;
            const float yLastPatchStart = AZStd::floorf(m_worldBounds.GetMax().GetY() / gridMeters) * gridMeters;

            const auto& materialAsset = materialInstance->GetAsset();
            const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

            for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += gridMeters)
            {
                for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += gridMeters)
                {
                    ShaderTerrainData objectSrgData;
                    objectSrgData.m_xyTranslation = { xPatch, yPatch };

                    m_sectorData.push_back();
                    SectorData& sectorData = m_sectorData.back();

                    for (auto& lod : m_patchModel->GetLods())
                    {
                        objectSrgData.m_xyScale = gridMeters;

                        auto objectSrg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                        if (!objectSrg)
                        {
                            AZ_WarningOnce(TerrainMeshManagerName, false, "Failed to create a new shader resource group, skipping.");
                            continue;
                        }
                        objectSrg->SetConstant(m_patchDataIndex, objectSrgData);
                        objectSrg->Compile();

                        AZ::RPI::ModelLod& modelLod = *lod.get();
                        sectorData.m_drawPackets.emplace_back(modelLod, 0, materialInstance, objectSrg);
                        AZ::RPI::MeshDrawPacket& drawPacket = sectorData.m_drawPackets.back();

                        sectorData.m_srgs.emplace_back(objectSrg);

                        // set the shader option to select forward pass IBL specular if necessary
                        if (!drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ false }))
                        {
                            AZ_Warning(TerrainMeshManagerName, false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                        }
                        const uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                        drawPacket.SetStencilRef(stencilRef);
                        drawPacket.Update(parentScene, true);
                    }

                    sectorData.m_aabb =
                        AZ::Aabb::CreateFromMinMax(
                            AZ::Vector3(xPatch, yPatch, m_worldBounds.GetMin().GetZ()),
                            AZ::Vector3(xPatch + gridMeters, yPatch + gridMeters, m_worldBounds.GetMax().GetZ())
                        );
                }
            }

            // Stack sectors:
            {
                m_previousCameraPosition = position;
                m_sectorStack.push_back({});
                StackData& stackData = m_sectorStack.back();

                // Calculate the largest potential number of sectors needed per dimension at any stack level.
                stackData.m_1dSectorCount = 2 * aznumeric_cast<uint32_t>(AZStd::ceilf((m_config.m_firstLodDistance + gridMeters) / gridMeters));
                stackData.m_sectors.resize(stackData.m_1dSectorCount * stackData.m_1dSectorCount);

                float gridMeters2 = gridMeters * 2.0f;
                stackData.m_startCoordX = aznumeric_cast<int32_t>(AZStd::floorf((position.GetX() - m_config.m_firstLodDistance) / gridMeters2)) * 2;
                stackData.m_startCoordY = aznumeric_cast<int32_t>(AZStd::floorf((position.GetY() - m_config.m_firstLodDistance) / gridMeters2)) * 2;

                uint32_t firstIndexX = (stackData.m_1dSectorCount + (stackData.m_startCoordX % stackData.m_1dSectorCount)) % stackData.m_1dSectorCount;
                uint32_t firstIndexY = (stackData.m_1dSectorCount + (stackData.m_startCoordY % stackData.m_1dSectorCount)) % stackData.m_1dSectorCount;

                for (uint32_t i = 0; i < stackData.m_sectors.size(); ++i)
                {
                    // Local coordinates of 0 to stackData.m_1dSectorCount
                    uint32_t localX = i % stackData.m_1dSectorCount;
                    uint32_t localY = i / stackData.m_1dSectorCount;

                    // Actual coordinates inside m_sectors based on the offset first index in x and y
                    uint32_t stackX = (localX + firstIndexX) % stackData.m_1dSectorCount;
                    uint32_t stackY = (localY + firstIndexY) % stackData.m_1dSectorCount;

                    // World sector coordinates
                    int32_t worldCoordX = stackData.m_startCoordX + localX;
                    int32_t worldCoordY = stackData.m_startCoordY + localY;

                    uint32_t index = stackY * stackData.m_1dSectorCount + stackX;
                    StackSectorData& sector = stackData.m_sectors.at(index);

                    // Aabb
                    AZ::Vector3 min = AZ::Vector3(worldCoordX * gridMeters, worldCoordY * gridMeters, m_worldBounds.GetMin().GetZ());
                    AZ::Vector3 max = min + AZ::Vector3(gridMeters, gridMeters, m_worldBounds.GetMax().GetZ());
                    sector.m_aabb = AZ::Aabb::CreateFromMinMax(min, max);

                    // Srg
                    ShaderTerrainData objectSrgData;
                    objectSrgData.m_xyTranslation = { min.GetX(), min.GetY() };
                    objectSrgData.m_xyScale = gridMeters;

                    sector.m_srg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                    if (!sector.m_srg)
                    {
                        AZ_WarningOnce(TerrainMeshManagerName, false, "Failed to create a new shader resource group, skipping.");
                        continue;
                    }
                    sector.m_srg->SetConstant(m_patchDataIndex, objectSrgData);
                    sector.m_srg->Compile();

                    // Draw packet
                    AZ::RPI::ModelLod& modelLod = *m_sectorModel->GetLods().begin()->get();
                    sector.m_drawPacket = AZ::RPI::MeshDrawPacket(modelLod, 0, materialInstance, sector.m_srg);

                    // set the shader option to select forward pass IBL specular if necessary
                    if (!sector.m_drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ false }))
                    {
                        AZ_Warning(TerrainMeshManagerName, false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                    }
                    const uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                    sector.m_drawPacket.SetStencilRef(stencilRef);
                    sector.m_drawPacket.Update(parentScene, true);
                }
            }

        }




        return true;
    }

    void TerrainMeshManager::DrawMeshes(const AZ::RPI::FeatureProcessor::RenderPacket& process)
    {
        
        /*
        const float gridMeters = GridSize * m_sampleSpacing;
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
                    const float minDistanceForLod0 = (gridMeters * 4.0f);

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
        */

        AZ::Vector3 cameraPosition = AZ::Vector3::CreateZero();

        for (auto& view : process.m_views)
        {
            if ((view->GetUsageFlags() & AZ::RPI::View::UsageCamera) > 0)
            {
                cameraPosition = view->GetCameraTransform().GetTranslation();
                CheckStackForUpdate(cameraPosition);
                break;
            }
        }

        for (auto& view : process.m_views)
        {
            for (auto& sectorStack : m_sectorStack)
            {
                for (auto& sector : sectorStack.m_sectors)
                {
                    AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
                    if (sector.m_aabb.GetDistanceSq(cameraPosition) < m_config.m_firstLodDistance * m_config.m_firstLodDistance &&
                        sector.m_drawPacket.GetRHIDrawPacket() &&
                        viewFrustum.IntersectAabb(sector.m_aabb) != AZ::IntersectResult::Exterior)
                    {
                        view->AddDrawPacket(sector.m_drawPacket.GetRHIDrawPacket());
                    }
                }
            }
        }
    }

    void TerrainMeshManager::RebuildDrawPackets(AZ::RPI::Scene& scene)
    {
        for (auto& sectorData : m_sectorData)
        {
            for (auto& drawPacket : sectorData.m_drawPackets)
            {
                drawPacket.Update(scene, true);
            }
        }
    }

    void TerrainMeshManager::OnTerrainDataCreateEnd()
    {
        Initialize();
    }

    void TerrainMeshManager::OnTerrainDataDestroyBegin()
    {
        Reset();
    }

    void TerrainMeshManager::OnTerrainDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & (TerrainDataChangedMask::HeightData | TerrainDataChangedMask::Settings)) != 0)
        {
            AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

            float queryResolution = 1.0f;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);
            // Currently query resolution is multidimensional but the rendering system only supports this changing in one dimension.

            // Sectors need to be rebuilt if the world bounds change in the x/y, or the sample spacing changes.
            m_rebuildSectors = m_rebuildSectors ||
                m_worldBounds.GetMin().GetX() != worldBounds.GetMin().GetX() ||
                m_worldBounds.GetMin().GetY() != worldBounds.GetMin().GetY() ||
                m_worldBounds.GetMax().GetX() != worldBounds.GetMax().GetX() ||
                m_worldBounds.GetMax().GetY() != worldBounds.GetMax().GetY() ||
                m_sampleSpacing != queryResolution;

            m_worldBounds = worldBounds;
            m_sampleSpacing = queryResolution;
        }
    }

    void TerrainMeshManager::InitializeTerrainPatch(uint16_t gridSize, PatchData& patchdata)
    {
        patchdata.m_positions.clear();
        patchdata.m_indices.clear();

        const uint16_t gridVertices = gridSize + 1; // For m_gridSize quads, (m_gridSize + 1) vertices are needed.
        const size_t size = gridVertices * gridVertices;

        patchdata.m_positions.reserve(size);

        for (uint16_t y = 0; y < gridVertices; ++y)
        {
            for (uint16_t x = 0; x < gridVertices; ++x)
            {
                patchdata.m_positions.push_back({ aznumeric_cast<float>(x) / gridSize, aznumeric_cast<float>(y) / gridSize });
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

    AZ::Outcome<AZ::Data::Asset<AZ::RPI::BufferAsset>> TerrainMeshManager::CreateBufferAsset(
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

    bool TerrainMeshManager::CreateLod(AZ::RPI::ModelAssetCreator& modelAssetCreator, const PatchData& patchData)
    {
        const auto positionBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_positions.size()), AZ::RHI::Format::R32G32_FLOAT);
        const auto positionsOutcome = CreateBufferAsset(patchData.m_positions.data(), positionBufferViewDesc, "TerrainPatchPositions");

        const auto indexBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_indices.size()), AZ::RHI::Format::R16_UINT);
        const auto indicesOutcome = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

        if (!positionsOutcome.IsSuccess() || !indicesOutcome.IsSuccess())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create GPU buffers for Terrain");
            return false;
        }

        AZ::RPI::ModelLodAssetCreator modelLodAssetCreator;
        modelLodAssetCreator.Begin(AZ::Uuid::CreateRandom());

        modelLodAssetCreator.BeginMesh();
        modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "POSITION" }, AZ::Name(), { positionsOutcome.GetValue(), positionBufferViewDesc });
        modelLodAssetCreator.SetMeshIndexBuffer({ indicesOutcome.GetValue(), indexBufferViewDesc });

        AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0, 0.0, 0.0), AZ::Vector3(GridSize, GridSize, 0.0));
        modelLodAssetCreator.SetMeshAabb(AZStd::move(aabb));
        modelLodAssetCreator.SetMeshName(AZ::Name("Terrain Patch"));
        modelLodAssetCreator.EndMesh();

        AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset;
        modelLodAssetCreator.End(modelLodAsset);

        modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
        return true;
    }

    bool TerrainMeshManager::InitializePatchModel()
    {
        AZ::RPI::ModelAssetCreator modelAssetCreator;
        modelAssetCreator.Begin(AZ::Uuid::CreateRandom());

        uint16_t gridSize = GridSize;

        for (uint32_t i = 0; i < AZ::RPI::ModelLodAsset::LodCountMax && gridSize > 0; ++i)
        {
            PatchData patchData;
            InitializeTerrainPatch(gridSize, patchData);

            if (!CreateLod(modelAssetCreator, patchData))
            {
                return false;
            }
            gridSize = gridSize / 2;
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        if (!modelAssetCreator.End(modelAsset))
        {
            return false;
        }

        m_patchModel = AZ::RPI::Model::FindOrCreate(modelAsset);

        return m_patchModel != nullptr;
    }

    bool TerrainMeshManager::InitializeSectorModel()
    {
        AZ::RPI::ModelAssetCreator modelAssetCreator;
        modelAssetCreator.Begin(AZ::Uuid::CreateRandom());

        PatchData patchData;
        InitializeTerrainPatch(GridSize, patchData);

        if (!CreateLod(modelAssetCreator, patchData))
        {
            return false;
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        if (!modelAssetCreator.End(modelAsset))
        {
            return false;
        }

        m_sectorModel = AZ::RPI::Model::FindOrCreate(modelAsset);
        return m_sectorModel != nullptr;
    }



    template<typename Callback>
    void TerrainMeshManager::ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback)
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
