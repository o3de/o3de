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

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainMeshManagerName = "TerrainMeshManager";
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
        m_sectorModel = {};
        m_sectorStack.clear();
        m_rebuildSectors = true;
        m_isInitialized = false;
    }

    void TerrainMeshManager::Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg,
        MaterialInstance materialInstance, AZ::RPI::Scene& parentScene, bool forceRebuildDrawPackets)
    {
        if (m_rebuildSectors)
        {
            RebuildSectors(materialInstance, parentScene);
            m_rebuildSectors = false;
        }
        else if (forceRebuildDrawPackets)
        {
            // The draw packets may need to be forcibly rebuilt in cases like a shader reload.
            RebuildDrawPackets(parentScene);
        }

        ShaderMeshData meshData;
        mainView->GetCameraTransform().GetTranslation().StoreToFloat3(meshData.m_mainCameraPosition.data());
        meshData.m_firstLodDistance = m_config.m_firstLodDistance;
        terrainSrg->SetConstant(m_srgMeshDataIndex, meshData);
    }

    void TerrainMeshManager::CheckStacksForUpdate(AZ::Vector3 newPosition)
    {
        for (uint32_t i = 0; i < m_sectorStack.size(); ++i)
        {
            const float maxDistance = m_config.m_firstLodDistance * aznumeric_cast<float>(1 << i);
            const float gridMeters = (GridSize * m_sampleSpacing) * (1 << i);
            const int32_t startCoordX = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetX() - maxDistance) / gridMeters));
            const int32_t startCoordY = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetY() - maxDistance) / gridMeters));

            // If the start coord for the stack is different, then some of the sectors will need to be updated.
            StackData& stackData = m_sectorStack.at(i);
            if (stackData.m_startCoordX != startCoordX || stackData.m_startCoordY != startCoordY)
            {
                stackData.m_startCoordX = startCoordX;
                stackData.m_startCoordY = startCoordY;

                const uint32_t firstSectorIndexX = (m_1dSectorCount + (startCoordX % m_1dSectorCount)) % m_1dSectorCount;
                const uint32_t firstSectorIndexY = (m_1dSectorCount + (startCoordY % m_1dSectorCount)) % m_1dSectorCount;

                for (uint32_t xOffset = 0; xOffset < m_1dSectorCount; ++xOffset)
                {
                    for (uint32_t yOffset = 0; yOffset < m_1dSectorCount; ++yOffset)
                    {
                        // Sectors use toroidal addressing to avoid needing to update any more than necessary.

                        const uint32_t sectorIndexX = (firstSectorIndexX + xOffset) % m_1dSectorCount;
                        const uint32_t sectorIndexY = (firstSectorIndexY + yOffset) % m_1dSectorCount;
                        const uint32_t sectorIndex = sectorIndexY * m_1dSectorCount + sectorIndexX;

                        const int32_t worldX = startCoordX + xOffset;
                        const int32_t worldY = startCoordY + yOffset;

                        StackSectorData& sector = stackData.m_sectors.at(sectorIndex);

                        if (sector.m_worldX != worldX || sector.m_worldY != worldY)
                        {
                            sector.m_worldX = worldX;
                            sector.m_worldY = worldY;

                            const AZ::Vector3 min = AZ::Vector3(worldX * gridMeters, worldY * gridMeters, m_worldBounds.GetMin().GetZ());
                            const AZ::Vector3 max = min + AZ::Vector3(gridMeters, gridMeters, m_worldBounds.GetZExtent());
                            sector.m_aabb = AZ::Aabb::CreateFromMinMax(min, max);

                            ShaderObjectData objectSrgData;
                            objectSrgData.m_xyTranslation = { min.GetX(), min.GetY() };
                            objectSrgData.m_xyScale = gridMeters;
                            objectSrgData.m_lodLevel = i;

                            sector.m_srg->SetConstant(m_patchDataIndex, objectSrgData);
                            sector.m_srg->Compile();
                        }
                    }
                }
            }
        }

    }

    void TerrainMeshManager::RebuildSectors(MaterialInstance materialInstance, AZ::RPI::Scene& parentScene)
    {
        const float gridMeters = GridSize * m_sampleSpacing;

        const auto& materialAsset = materialInstance->GetAsset();
        const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

        // Calculate the largest potential number of sectors needed per dimension at any stack level.
        m_1dSectorCount = aznumeric_cast<uint32_t>(AZStd::ceilf((m_config.m_firstLodDistance + gridMeters) / gridMeters));
        m_1dSectorCount += 1; // Add one sector of wiggle room.
        m_1dSectorCount *= 2; // Lod Distance is radius, but we need diameter.

        m_sectorStack.clear();

        const uint32_t stackCount = aznumeric_cast<uint32_t>(ceil(log2f(AZStd::GetMax(1.0f, m_config.m_renderDistance / m_config.m_firstLodDistance)) + 1.0f));
        m_sectorStack.reserve(stackCount);

        // Create all the sectors with uninitialized SRGs. The SRGs will be updated later by CheckStacksForUpdate().

        for (uint32_t j = 0; j < stackCount; ++j)
        {
            m_sectorStack.push_back({});
            StackData& stackData = m_sectorStack.back();

            stackData.m_sectors.resize(m_1dSectorCount * m_1dSectorCount);

            for (uint32_t i = 0; i < stackData.m_sectors.size(); ++i)
            {
                StackSectorData& sector = stackData.m_sectors.at(i);
                sector.m_srg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
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

    void TerrainMeshManager::DrawMeshes(const AZ::RPI::FeatureProcessor::RenderPacket& process, const AZ::RPI::ViewPtr mainView)
    {
        AZ::Vector3 mainCameraPosition = mainView->GetCameraTransform().GetTranslation();
        CheckStacksForUpdate(mainCameraPosition);

        for (auto& view : process.m_views)
        {
            float minDistanceSq = 0.0f;
            float maxDistanceSq = m_config.m_firstLodDistance * m_config.m_firstLodDistance;

            const AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
            const AZ::Vector3 viewVector = viewFrustum.GetPlane(AZ::Frustum::PlaneId::Near).GetNormal();
            const AZ::Vector3 viewPosition = view->GetCameraTransform().GetTranslation();

            for (auto& sectorStack : m_sectorStack)
            {
                float radius = 0.0f;
                [[maybe_unused]] AZ::Vector3 center;
                sectorStack.m_sectors.at(0).m_aabb.GetAsSphere(center, radius);

                for (auto& sector : sectorStack.m_sectors)
                {
                    if (viewVector.Dot(viewPosition - sector.m_aabb.GetCenter()) < -radius || // Cheap check to eliminate sectors behind camera
                        viewFrustum.IntersectAabb(sector.m_aabb) == AZ::IntersectResult::Exterior || // Check against frustum
                        !sector.m_aabb.Overlaps(m_worldBounds)) // Check against world bounds
                    {
                        continue;
                    }

                    // Sector is in view, but only draw if it's in the correct LOD range.
                    const float aabbMinDistanceSq = sector.m_aabb.GetDistanceSq(mainCameraPosition);
                    const float aabbMaxDistanceSq = sector.m_aabb.GetMaxDistanceSq(mainCameraPosition);
                    if (aabbMaxDistanceSq > minDistanceSq && aabbMinDistanceSq <= maxDistanceSq)
                    {
                        view->AddDrawPacket(sector.m_drawPacket.GetRHIDrawPacket());
                    }
                }
                minDistanceSq = maxDistanceSq;
                maxDistanceSq = maxDistanceSq * 4.0f; // Double the distance with squared distances is * 2^2.
            }
        }

        m_previousCameraPosition = mainCameraPosition;
    }

    void TerrainMeshManager::RebuildDrawPackets(AZ::RPI::Scene& scene)
    {
        for (auto& stackData : m_sectorStack)
        {
            for (auto& sector : stackData.m_sectors)
            {
                sector.m_drawPacket.Update(scene, true);
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
        for (StackData& stackData : m_sectorStack)
        {
            for (StackSectorData& sectorData : stackData.m_sectors)
            {
                if (sectorData.m_aabb.Overlaps(bounds))
                {
                    callback(stackData);
                }
            }
        }
    }

}
