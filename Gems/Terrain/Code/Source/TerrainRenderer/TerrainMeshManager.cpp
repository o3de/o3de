/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainMeshManager.h>

#include <AzCore/Math/Frustum.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>

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
    }

    TerrainMeshManager::~TerrainMeshManager()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
    }

    void TerrainMeshManager::Initialize(AZ::RPI::Scene& parentScene)
    {
        m_parentScene = &parentScene;

        bool success = true;
        success = success && InitializeCommonSectorData();
        success = success && InitializeDefaultSectorModel();
        if (!success)
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create Terrain render buffers!");
            return;
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        m_handleGlobalShaderOptionUpdate = AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler
        {
            [this](const AZ::Name&, AZ::RPI::ShaderOptionValue) { m_forceRebuildDrawPackets = true; }
        };
        AZ::RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);

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

    void TerrainMeshManager::SetMaterial(MaterialInstance materialInstance)
    {
        if (m_materialInstance != materialInstance || m_materialInstance->GetCurrentChangeId() != m_lastMaterialChangeId)
        {
            m_lastMaterialChangeId = materialInstance->GetCurrentChangeId();
            m_materialInstance = materialInstance;
            m_forceRebuildDrawPackets = true;
        }
    }

    bool TerrainMeshManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainMeshManager::Reset()
    {
        m_sectorStack.clear();
        m_rebuildSectors = true;
    }

    void TerrainMeshManager::Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        if (m_rebuildSectors)
        {
            RebuildSectors();
            m_rebuildSectors = false;
            m_forceRebuildDrawPackets = false;
        }
        else if (m_forceRebuildDrawPackets)
        {
            // The draw packets may need to be forcibly rebuilt in cases like a shader reload.
            RebuildDrawPackets();
        }

        ShaderMeshData meshData;
        mainView->GetCameraTransform().GetTranslation().StoreToFloat3(meshData.m_mainCameraPosition.data());
        meshData.m_firstLodDistance = m_config.m_firstLodDistance;
        terrainSrg->SetConstant(m_srgMeshDataIndex, meshData);
    }

    void TerrainMeshManager::CheckStacksForUpdate(AZ::Vector3 newPosition)
    {
        AZStd::vector<SectorUpdateContext> sectorsToUpdate;

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

                            sectorsToUpdate.push_back({ i , &sector });
                        }
                    }
                }
            }
        }

        if (!sectorsToUpdate.empty())
        {
            ProcessSectorUpdates(sectorsToUpdate);
            return;
        }

    }

    void TerrainMeshManager::RebuildSectors()
    {
        const float gridMeters = GridSize * m_sampleSpacing;

        const auto& materialAsset = m_materialInstance->GetAsset();
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
                sector.m_model = m_defaultSectorModel;
                sector.m_srg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                AZ::RPI::ModelLod& modelLod = *m_defaultSectorModel->GetLods().begin()->get();
                sector.m_drawPacket = AZ::RPI::MeshDrawPacket(modelLod, 0, m_materialInstance, sector.m_srg);

                // set the shader option to select forward pass IBL specular if necessary
                if (!sector.m_drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ false }))
                {
                    AZ_Warning(TerrainMeshManagerName, false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                }
                const uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                sector.m_drawPacket.SetStencilRef(stencilRef);
                sector.m_drawPacket.Update(*m_parentScene, true);
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

    void TerrainMeshManager::RebuildDrawPackets()
    {
        for (auto& stackData : m_sectorStack)
        {
            for (auto& sector : stackData.m_sectors)
            {
                sector.m_drawPacket.Update(*m_parentScene, true);
            }
        }
    }

    void TerrainMeshManager::OnTerrainDataCreateEnd()
    {
        OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
    }

    void TerrainMeshManager::OnTerrainDataDestroyBegin()
    {
        m_sectorStack.clear();
        m_rebuildSectors = true;
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

            if (!m_rebuildSectors)
            {
                // Rebuild any sectors in the dirty region if they aren't all being rebuilt
                AZStd::vector<SectorUpdateContext> sectorsToUpdate;
                ForOverlappingSectors(dirtyRegion,
                    [&sectorsToUpdate](StackSectorData& sectorData, uint32_t lodLevel)
                    {
                        sectorsToUpdate.push_back({ lodLevel, &sectorData });
                    }
                );
                if (!sectorsToUpdate.empty())
                {
                    ProcessSectorUpdates(sectorsToUpdate);
                }
            }
        }
    }

    void TerrainMeshManager::InitializeTerrainPatch(uint16_t gridSize, PatchData& patchdata)
    {
        patchdata.m_xyPositions.clear();
        patchdata.m_indices.clear();

        const uint16_t gridVertices = gridSize + 1; // For m_gridSize quads, (m_gridSize + 1) vertices are needed.
        const size_t size = gridVertices * gridVertices;

        patchdata.m_xyPositions.reserve(size);

        for (uint16_t y = 0; y < gridVertices; ++y)
        {
            for (uint16_t x = 0; x < gridVertices; ++x)
            {
                patchdata.m_xyPositions.push_back({ aznumeric_cast<float>(x) / gridSize, aznumeric_cast<float>(y) / gridSize });
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

    bool TerrainMeshManager::CreateLod(AZ::RPI::ModelAssetCreator& modelAssetCreator, const AZ::RPI::BufferAssetView& zPositions, const AZ::RPI::BufferAssetView& normals)
    {
        AZ::RPI::ModelLodAssetCreator modelLodAssetCreator;
        modelLodAssetCreator.Begin(AZ::Uuid::CreateRandom());

        modelLodAssetCreator.BeginMesh();
        modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "POSITION", 0 }, AZ::Name(), m_sectorXyPositionsBufferAssetView);
        modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "POSITION", 1 }, AZ::Name(), zPositions);
        modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "NORMAL" }, AZ::Name(), normals);
        modelLodAssetCreator.SetMeshIndexBuffer(m_sectorIndicesBufferAssetView);

        AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0, 0.0, 0.0), AZ::Vector3(GridSize, GridSize, 0.0));
        modelLodAssetCreator.SetMeshAabb(AZStd::move(aabb));
        modelLodAssetCreator.SetMeshName(AZ::Name("Terrain Patch"));
        modelLodAssetCreator.EndMesh();

        AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset;
        modelLodAssetCreator.End(modelLodAsset);

        modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
        return true;
    }

    bool TerrainMeshManager::InitializeCommonSectorData()
    {
        PatchData patchData;
        InitializeTerrainPatch(GridSize, patchData);

        const auto xyPositionBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_xyPositions.size()), AZ::RHI::Format::R32G32_FLOAT);
        const auto xyPositionsOutcome = CreateBufferAsset(patchData.m_xyPositions.data(), xyPositionBufferViewDesc, "TerrainPatchXYPositions");

        AZStd::vector<uint16_t> zeros (patchData.m_xyPositions.size()); // uint16_t initialize to 0.
        const auto zPositionBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(zeros.size()), AZ::RHI::Format::R16_UNORM);
        const auto zPositionOutcome = CreateBufferAsset(zeros.data(), zPositionBufferViewDesc, "TerrainPatchZeroHeights");

        AZStd::vector<AZStd::pair<uint16_t, uint16_t>> normals(patchData.m_xyPositions.size()); // uint16_t initialize to 0.
        const auto normalsBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(zeros.size()), AZ::RHI::Format::R16G16_SNORM);
        const auto normalsOutcome = CreateBufferAsset(zeros.data(), normalsBufferViewDesc, "TerrainPatchZeroNormals");

        const auto indexBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(patchData.m_indices.size()), AZ::RHI::Format::R16_UINT);
        const auto indicesOutcome = CreateBufferAsset(patchData.m_indices.data(), indexBufferViewDesc, "TerrainPatchIndices");

        if (!xyPositionsOutcome.IsSuccess() || !indicesOutcome.IsSuccess() || !zPositionOutcome.IsSuccess() || !normalsOutcome.IsSuccess())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create GPU buffers for Terrain");
            return false;
        }

        m_sectorXyPositionsBufferAssetView = { xyPositionsOutcome.GetValue(), xyPositionBufferViewDesc };
        m_sectorZPositionsBufferAssetView = { zPositionOutcome.GetValue(), zPositionBufferViewDesc };
        m_sectorNormalsBufferAssetView = { normalsOutcome.GetValue(), normalsBufferViewDesc };
        m_sectorIndicesBufferAssetView = { indicesOutcome.GetValue(), indexBufferViewDesc };

        return true;
    }

    bool TerrainMeshManager::InitializeDefaultSectorModel()
    {
        m_defaultSectorModel = InitializeSectorModel(m_sectorZPositionsBufferAssetView, m_sectorNormalsBufferAssetView);
        return m_defaultSectorModel != nullptr;
    }

    AZ::Data::Instance<AZ::RPI::Model> TerrainMeshManager::InitializeSectorModel(uint16_t gridSize, const AZ::Vector2& worldStartPosition, float vertexSpacing, AZ::Aabb& modelAabb)
    {
        AZ::Vector3 aabbMin = AZ::Vector3(worldStartPosition.GetX(), worldStartPosition.GetY(), m_worldBounds.GetMin().GetZ());
        AZ::Vector3 aabbMax = aabbMin + AZ::Vector3(gridSize * vertexSpacing);

        // expand the bounds in order to calculate normals.
        AZ::Vector3 queryAabbMin = aabbMin - AZ::Vector3(vertexSpacing);
        AZ::Vector3 queryAabbMax = aabbMax + AZ::Vector3(2.0f * vertexSpacing); // extra padding to catch the last vertex

        // pad the max by half a sample spacing to make sure it's inclusive of the last point.
        AZ::Aabb queryBounds = AZ::Aabb::CreateFromMinMax(queryAabbMin, queryAabbMax);

        auto samplerType = AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP;
        const AZ::Vector2 stepSize(vertexSpacing);

        AZStd::pair<size_t, size_t> numSamples;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            numSamples, &AzFramework::Terrain::TerrainDataRequests::GetNumSamplesFromRegion,
            queryBounds, stepSize, samplerType);

        uint16_t vertexCount1d = (gridSize + 1); // grid size is length, need an extra vertex in each dimension to draw the final row / column of quads.
        uint16_t paddedVertexCount1d = vertexCount1d + 2; // extra row / column on each side for normals.

        if (numSamples.first != paddedVertexCount1d || numSamples.second != paddedVertexCount1d)
        {
            AZ_Assert(false, "Number of samples returned from GetNumSamplesFromRegion does not match expectations");
            return {};
        }

        uint16_t meshVertexCount = vertexCount1d * vertexCount1d;
        uint16_t queryVertexCount = paddedVertexCount1d * paddedVertexCount1d;
        AZStd::vector<float> heights;
        heights.resize_no_construct(queryVertexCount);

        AZStd::vector<uint16_t> meshHeights;
        AZStd::vector<AZStd::pair<uint16_t, uint16_t>> meshNormals;
        meshHeights.resize_no_construct(meshVertexCount);
        meshNormals.resize_no_construct(meshVertexCount);

        auto perPositionCallback = [this, &heights, paddedVertexCount1d]
            (size_t xIndex, size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
        {
            const float height = surfacePoint.m_position.GetZ() - m_worldBounds.GetMin().GetZ();
            heights.at(yIndex * paddedVertexCount1d + xIndex) = height;
        };

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromRegion,
            queryBounds, stepSize, perPositionCallback, samplerType);

        const float rcpWorldZ = 1.0f / m_worldBounds.GetExtents().GetZ();
        const float vertexSpacing2 = vertexSpacing * 2.0f;

        // initialize min/max heights to the first height
        float minHeight = heights.at(paddedVertexCount1d + 1);
        float maxHeight = heights.at(paddedVertexCount1d + 1);

        // float versions of int max to make sure a int->float conversion doesn't happen at each loop iteration.
        constexpr float maxUint16 = AZStd::numeric_limits<uint16_t>::max();
        constexpr float maxInt16 = AZStd::numeric_limits<int16_t>::max();

        for (uint16_t y = 0; y < vertexCount1d; ++y)
        {
            const uint16_t offsetY = y + 1;

            for (uint16_t x = 0; x < vertexCount1d; ++x)
            {
                const uint16_t offsetX = x + 1;
                const uint16_t offsetCoord = offsetY * paddedVertexCount1d + offsetX;
                const uint16_t coord = y * vertexCount1d + x;

                const float height = heights.at(offsetCoord);
                const float clampedHeight = AZ::GetClamp(height * rcpWorldZ, 0.0f, 1.0f);
                const uint16_t uint16Height = aznumeric_cast<uint16_t>(clampedHeight * maxUint16 + 0.5f); // always positive, so just add 0.5 to round.
                meshHeights.at(coord) = uint16Height;

                if (minHeight > height)
                {
                    minHeight = height;
                }
                else if (maxHeight < height)
                {
                    maxHeight = height;
                }

                const float leftHeight = heights.at(offsetCoord - 1);
                const float rightHeight = heights.at(offsetCoord + 1);
                const float xSlope = (leftHeight - rightHeight) / vertexSpacing2;
                const float normalX = xSlope / sqrt(xSlope * xSlope + 1); // sin(arctan(xSlope)

                const float upHeight = heights.at(offsetCoord - paddedVertexCount1d);
                const float downHeight = heights.at(offsetCoord + paddedVertexCount1d);
                const float ySlope = (upHeight - downHeight) / vertexSpacing2;
                const float normalY = ySlope / sqrt(ySlope * ySlope + 1); // sin(arctan(ySlope)

                meshNormals.at(coord) =
                {
                    aznumeric_cast<int16_t>(AZStd::lround(normalX * maxInt16)),
                    aznumeric_cast<int16_t>(AZStd::lround(normalY * maxInt16)),
                };
            }
        }
        const auto heightsBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(meshHeights.size()), AZ::RHI::Format::R16_UNORM);
        const auto heightsOutcome = CreateBufferAsset(meshHeights.data(), heightsBufferViewDesc, "TerrainPatchZPositions");

        if (!heightsOutcome.IsSuccess())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create GPU buffer of z positions for Terrain");
            return {};
        }

        const auto normalsBufferViewDesc = AZ::RHI::BufferViewDescriptor::CreateTyped(0, aznumeric_cast<uint32_t>(meshNormals.size()), AZ::RHI::Format::R16G16_SNORM);
        const auto normalsOutcome = CreateBufferAsset(meshNormals.data(), normalsBufferViewDesc, "TerrainPatchNormals");

        if (!normalsOutcome.IsSuccess())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create GPU buffers of normals for Terrain");
            return {};
        }

        aabbMin.SetZ(minHeight);
        aabbMax.SetZ(maxHeight);
        modelAabb.Set(aabbMin, aabbMax);

        return InitializeSectorModel({ heightsOutcome.GetValue(), heightsBufferViewDesc }, { normalsOutcome .GetValue(), normalsBufferViewDesc });
    }

    AZ::Data::Instance<AZ::RPI::Model> TerrainMeshManager::InitializeSectorModel(const AZ::RPI::BufferAssetView& heights, const AZ::RPI::BufferAssetView& normals)
    {
        AZ::RPI::ModelAssetCreator modelAssetCreator;
        modelAssetCreator.Begin(AZ::Uuid::CreateRandom());

        if (!CreateLod(modelAssetCreator, heights, normals))
        {
            return {};
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        if (!modelAssetCreator.End(modelAsset))
        {
            return {};
        }

        return AZ::RPI::Model::FindOrCreate(modelAsset);
    }

    void TerrainMeshManager::ProcessSectorUpdates(AZStd::span<SectorUpdateContext> sectorUpdates)
    {
        AZ::JobCompletion jobCompletion;
        for (SectorUpdateContext& updateContext : sectorUpdates)
        {
            const float gridMeters = (GridSize * m_sampleSpacing) * (1 << updateContext.m_lodLevel);

            const auto jobLambda = [this, updateContext, gridMeters]() -> void
            {
                auto& sector = *updateContext.m_sector;

                AZ::Aabb modelAabb = AZ::Aabb::CreateNull();
                sector.m_model = InitializeSectorModel(GridSize, AZ::Vector2(sector.m_worldX * gridMeters, sector.m_worldY * gridMeters), gridMeters / GridSize, modelAabb);
                sector.m_aabb = modelAabb;

                AZ::RPI::ModelLod& modelLod = *sector.m_model->GetLods().begin()->get();
                sector.m_drawPacket = AZ::RPI::MeshDrawPacket(modelLod, 0, m_materialInstance, sector.m_srg);

                // set the shader option to select forward pass IBL specular if necessary
                if (!sector.m_drawPacket.SetShaderOption(AZ::Name("o_meshUseForwardPassIBLSpecular"), AZ::RPI::ShaderOptionValue{ false }))
                {
                    AZ_Warning(TerrainMeshManagerName, false, "Failed to set o_meshUseForwardPassIBLSpecular on mesh draw packet");
                }
                const uint8_t stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;
                sector.m_drawPacket.SetStencilRef(stencilRef);
                sector.m_drawPacket.Update(*m_parentScene, true);

                ShaderObjectData objectSrgData;
                objectSrgData.m_xyTranslation = { modelAabb.GetMin().GetX(), modelAabb.GetMin().GetY()};
                objectSrgData.m_xyScale = gridMeters;
                objectSrgData.m_lodLevel = updateContext.m_lodLevel;

                sector.m_srg->SetConstant(m_patchDataIndex, objectSrgData);
                sector.m_srg->Compile();
            };

            AZ::Job* executeGroupJob = aznew AZ::JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
            executeGroupJob->SetDependent(&jobCompletion);
            executeGroupJob->Start();
        }
        jobCompletion.StartAndWaitForCompletion();

    }

    template<typename Callback>
    void TerrainMeshManager::ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback)
    {
        for (uint32_t lodLevel = 0; lodLevel < m_sectorStack.size(); ++lodLevel)
        {
            auto& stackData = m_sectorStack.at(lodLevel);
            for (StackSectorData& sectorData : stackData.m_sectors)
            {
                if (sectorData.m_aabb.Overlaps(bounds))
                {
                    callback(sectorData, lodLevel);
                }
            }
        }
    }

}
