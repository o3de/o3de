/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainMeshManager.h>

#include <AzCore/Console/Console.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshCommon.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainMeshManagerName = "TerrainMeshManager";
    }

    AZ_CVAR(bool,
        r_debugTerrainLodLevels,
        false,
        [](const bool& value)
        {
            AZ::RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(AZ::Name{ "o_debugTerrainLodLevels" }, AZ::RPI::ShaderOptionValue{ value });
        },
        AZ::ConsoleFunctorFlags::Null,
        "Turns on debug coloring for terrain mesh lods."
    );

    AZ_CVAR(bool,
        r_debugTerrainAabbs,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Turns on debug aabbs for terrain sectors."
        );

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

        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        m_handleGlobalShaderOptionUpdate = AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler
        {
            [this](const AZ::Name&, AZ::RPI::ShaderOptionValue) { m_rebuildDrawPackets = true; }
        };
        AZ::RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);
        
        m_meshMovedFlag = m_parentScene->GetViewTagBitRegistry().AcquireTag(AZ::Render::MeshCommon::MeshMovedName);

        m_rayTracingFeatureProcessor = m_parentScene->GetFeatureProcessor<AZ::Render::RayTracingFeatureProcessorInterface>();
        m_rayTracingEnabled = (AZ::RHI::RHISystemInterface::Get()->GetRayTracingSupport() != AZ::RHI::MultiDevice::NoDevices) && m_rayTracingFeatureProcessor;

        m_isInitialized = true;
    }

    void TerrainMeshManager::SetConfiguration(const MeshConfiguration& config)
    {
        bool requireRebuild = m_config.CheckWouldRequireRebuild(config);

        m_config = config;

        if (requireRebuild)
        {
            m_rebuildSectors = true;
            OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
        }

        // This will trigger a draw packet rebuild later.
        AZ::RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(AZ::Name{ "o_useTerrainClod" }, AZ::RPI::ShaderOptionValue{ m_config.m_clodEnabled });
    }

    bool TerrainMeshManager::UpdateGridSize(float distanceToFirstLod)
    {
        float queryResolution = 1.0f;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        float quadsToFirstLod = distanceToFirstLod / queryResolution;
        uint32_t quadsPerSector = aznumeric_cast<uint32_t>(quadsToFirstLod / 4.0f);
        uint32_t gridSize = AZ::RHI::IsPowerOfTwo(quadsPerSector) ? quadsPerSector : (AZ::RHI::NextPowerOfTwo(quadsPerSector) >> 1);
        gridSize = AZStd::GetMin(gridSize, 128u); // x/y positions must be able to fix in 8 bits (256 is too large by 1)
        gridSize = AZStd::GetMax(gridSize, 8u); // make sure there's enough vertices to be worth drawing.

        if (gridSize != m_gridSize)
        {
            m_gridSize = aznumeric_cast<uint8_t>(gridSize);
            m_gridVerts1D = m_gridSize + 1;
            m_gridVerts2D = m_gridVerts1D * m_gridVerts1D;
            return true;
        }
        return false;
    }

    void TerrainMeshManager::SetMaterial(MaterialInstance materialInstance)
    {
        if (m_materialInstance != materialInstance || m_materialInstance->GetCurrentChangeId() != m_lastMaterialChangeId)
        {
            m_lastMaterialChangeId = materialInstance->GetCurrentChangeId();
            m_materialInstance = materialInstance;

            // Queue the load of the material's shaders now since they'll be needed later.
            m_materialInstance->ForAllShaderItems(
                [&](const AZ::Name&, const AZ::RPI::ShaderCollection::Item& shaderItem)
                {
                    AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = shaderItem.GetShaderAsset();
                    if (!shaderAsset.IsReady())
                    {
                        shaderAsset.QueueLoad();
                    }
                    return true;
                });

            m_rebuildDrawPackets = true;
        }
    }

    bool TerrainMeshManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainMeshManager::ClearSectorBuffers()
    {
        // RemoveRayTracedMeshes() needs to be called first since it uses pointers into the sector data stored in m_sectorLods.
        RemoveRayTracedMeshes();
        m_candidateSectors.clear();
        m_sectorsThatNeedSrgCompiled.clear();
        m_sectorLods.clear();
    }

    void TerrainMeshManager::Reset()
    {
        if (m_meshMovedFlag.IsValid())
        {
            m_parentScene->GetViewTagBitRegistry().ReleaseTag(m_meshMovedFlag);
        }
        ClearSectorBuffers();
        m_xyPositions.clear();
        m_cachedDrawData.clear();

        m_rebuildSectors = true;
    }

    void TerrainMeshManager::RemoveRayTracedMeshes()
    {
        AZ_Assert(m_rayTracedItems.empty() || !m_sectorLods.empty(),
            "RemoveRayTracedMeshes() is being called after the underlying sector data has been deleted. "
            "The pointers stored in it are no longer valid.");

        for (RayTracedItem& item : m_rayTracedItems)
        {
            if (auto& rtData = item.m_sector->m_rtData; rtData)
            {
                RtSector::MeshGroup& meshGroup = rtData->m_meshGroups.at(item.m_meshGroupIndex);
                meshGroup.m_isVisible = false;
                m_rayTracingFeatureProcessor->RemoveMesh(meshGroup.m_id);
            }
        }
        m_rayTracedItems.clear();
    }

    void TerrainMeshManager::Update(const AZ::RPI::ViewPtr mainView, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        if (m_rebuildDrawPackets)
        {
            // Rebuild the draw packets when the material or shaders change.
            RebuildDrawPackets();
            m_rebuildDrawPackets = false;
        }

        if (m_rebuildSectors)
        {
            // Rebuild the sectors when the configuration or terrain world changes
            CreateCommonBuffers();
            RebuildSectors();
            m_rebuildSectors = false;
        }

        ShaderMeshData meshData;
        mainView->GetCameraTransform().GetTranslation().StoreToFloat3(meshData.m_mainCameraPosition.data());
        meshData.m_firstLodDistance = m_config.m_firstLodDistance;
        meshData.m_rcpClodDistance = 1.0f / m_config.m_clodDistance;
        meshData.m_rcpGridSize = 1.0f / m_gridSize;
        meshData.m_gridToQuadScale = m_gridSize / 255.0f;
        terrainSrg->SetConstant(m_srgMeshDataIndex, meshData);
    }

    void TerrainMeshManager::CheckLodGridsForUpdate(AZ::Vector3 newPosition)
    {
        // lods of sectors that need updating, separated by LOD level.
        AZStd::vector<AZStd::vector<Sector*>> sectorsToUpdate(m_sectorLods.size());
        bool anySectorsUpdated = false;

        for (uint32_t lodLevel = 0; lodLevel < m_sectorLods.size(); ++lodLevel)
        {
            SectorLodGrid& lodGrid = m_sectorLods.at(lodLevel);

            // Figure out what the start coordinate should be for this lod level.
            const Vector2i newStartCoord = [&]()
            {
                const float maxDistance = m_config.m_firstLodDistance * aznumeric_cast<float>(1 << lodLevel);
                const float gridMeters = (m_gridSize * m_sampleSpacing) * (1 << lodLevel);
                const int32_t startCoordX = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetX() - maxDistance) / gridMeters));
                const int32_t startCoordY = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetY() - maxDistance) / gridMeters));

                // If the start coord for the lod level is different, then some of the sectors will need to be updated.
                // There's 1 sector of wiggle room, so make sure we've moving the lod's start coord by as little as possible.

                auto coordCheck = [&](int32_t newStartCoord, int32_t lodStartCoord) -> int32_t
                {
                    return
                        newStartCoord > lodStartCoord + 1 ? newStartCoord - 1 :
                        newStartCoord < lodStartCoord ? newStartCoord :
                        lodStartCoord;
                };

                return Vector2i(coordCheck(startCoordX, lodGrid.m_startCoord.m_x), coordCheck(startCoordY, lodGrid.m_startCoord.m_y));
            }();

            if (lodGrid.m_startCoord != newStartCoord)
            {
                lodGrid.m_startCoord = newStartCoord;

                const uint32_t firstSectorIndexX = (m_1dSectorCount + (newStartCoord.m_x % m_1dSectorCount)) % m_1dSectorCount;
                const uint32_t firstSectorIndexY = (m_1dSectorCount + (newStartCoord.m_y % m_1dSectorCount)) % m_1dSectorCount;

                for (uint32_t xOffset = 0; xOffset < m_1dSectorCount; ++xOffset)
                {
                    for (uint32_t yOffset = 0; yOffset < m_1dSectorCount; ++yOffset)
                    {
                        // Sectors use toroidal addressing to avoid needing to update any more than necessary.

                        const uint32_t sectorIndexX = (firstSectorIndexX + xOffset) % m_1dSectorCount;
                        const uint32_t sectorIndexY = (firstSectorIndexY + yOffset) % m_1dSectorCount;
                        const uint32_t sectorIndex = sectorIndexY * m_1dSectorCount + sectorIndexX;

                        const Vector2i worldCoord = newStartCoord + Vector2i(xOffset, yOffset);

                        Sector& sector = lodGrid.m_sectors.at(sectorIndex);

                        if (sector.m_worldCoord != worldCoord)
                        {
                            sector.m_worldCoord = worldCoord;
                            sectorsToUpdate.at(lodLevel).push_back(&sector);
                            anySectorsUpdated = true;
                        }
                    }
                }
            }
        }

        if (anySectorsUpdated)
        {
            ProcessSectorUpdates(sectorsToUpdate);
            return;
        }

    }

    AZ::RHI::StreamBufferView TerrainMeshManager::CreateStreamBufferView(AZ::Data::Instance<AZ::RPI::Buffer>& buffer, uint32_t offset)
    {
        return
        {
            *buffer->GetRHIBuffer(),
            offset,
            aznumeric_cast<uint32_t>(buffer->GetBufferSize()),
            buffer->GetBufferViewDescriptor().m_elementSize
        };
    }

    void TerrainMeshManager::BuildDrawPacket(Sector& sector)
    {
        AZ::RHI::DrawPacketBuilder drawPacketBuilder{AZ::RHI::MultiDevice::AllDevices};
        uint32_t indexCount = m_indexBuffer->GetBufferViewDescriptor().m_elementCount;
        sector.m_geometryView.SetDrawArguments(AZ::RHI::DrawIndexed(0, indexCount, 0));
        sector.m_geometryView.SetIndexBufferView(m_indexBufferView);

        drawPacketBuilder.Begin(nullptr);
        drawPacketBuilder.SetGeometryView(&sector.m_geometryView);
        drawPacketBuilder.AddShaderResourceGroup(sector.m_srg->GetRHIShaderResourceGroup());
        drawPacketBuilder.AddShaderResourceGroup(m_materialInstance->GetRHIShaderResourceGroup());

        sector.m_perDrawSrgs.clear();

        for (CachedDrawData& drawData : m_cachedDrawData)
        {
            AZ::Data::Instance<AZ::RPI::Shader>& shader = drawData.m_shader;

            AZ::RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = drawData.m_drawListTag;
            drawRequest.m_pipelineState = drawData.m_pipelineState;
            drawRequest.m_streamIndices = sector.m_geometryView.GetFullStreamBufferIndices();
            drawRequest.m_stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;

            if (drawData.m_materialPipelineName != AZ::RPI::MaterialPipelineNone)
            {
                AZ::RHI::DrawFilterTag pipelineTag = m_parentScene->GetDrawFilterTagRegistry()->AcquireTag(drawData.m_materialPipelineName);
                AZ_Assert(pipelineTag.IsValid(), "Could not acquire pipeline filter tag '%s'.", drawData.m_materialPipelineName.GetCStr());
                drawRequest.m_drawFilterMask = 1 << pipelineTag.GetIndex();
            }

            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg;
            if (drawData.m_drawSrgLayout)
            {
                // If the DrawSrg exists we must create and bind it, otherwise the CommandList will fail validation for SRG being null
                drawSrg = AZ::RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawData.m_drawSrgLayout->GetName());
                if (drawData.m_shaderVariant.UseKeyFallback() && drawData.m_drawSrgLayout->HasShaderVariantKeyFallbackEntry())
                {
                    drawSrg->SetShaderVariantKeyFallbackValue(drawData.m_shaderOptions.GetShaderVariantKeyFallbackValue());
                }
                drawSrg->Compile();
            }

            if (drawSrg)
            {
                drawRequest.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                sector.m_perDrawSrgs.push_back(drawSrg);
            }
            drawPacketBuilder.AddDrawItem(drawRequest);
        }

        AZ::RHI::DrawPacketBuilder commonQuadrantDrawPacketBuilder = drawPacketBuilder; // Copy of the draw packet builder to use later.
        sector.m_rhiDrawPacket = drawPacketBuilder.End();

        // Generate draw packets for each of the quadrants so they can be used to fill in places where the previous LOD didn't draw.
        // Due to z-ordered index buffer, no additional data is needed, just a different index offset and index count. Each quarter of
        // the index buffer perfectly corresponds to a quadrant of the sector in Z order (TL, TR, BL, BR).
        uint32_t lowerLodIndexCount = indexCount / 4;
        for (uint32_t i = 0; i < 4; ++i)
        {
            sector.m_quadrantGeometryViews[i] = sector.m_geometryView;
            sector.m_quadrantGeometryViews[i].SetDrawArguments( AZ::RHI::DrawIndexed(0, lowerLodIndexCount, lowerLodIndexCount * i) );

            AZ::RHI::DrawPacketBuilder quadrantDrawPacketBuilder = commonQuadrantDrawPacketBuilder;
            quadrantDrawPacketBuilder.SetGeometryView(&sector.m_quadrantGeometryViews[i]);
            sector.m_rhiDrawPacketQuadrant[i] = quadrantDrawPacketBuilder.End();
        }
    }

    void TerrainMeshManager::BuildRtSector(Sector& sector, uint32_t lodLevel)
    {
        RtSector& rtSector = *sector.m_rtData;

        AZStd::string positionName = AZStd::string::format("Terrain Positions-  Lod %u, Sector (%u, %u)", lodLevel, sector.m_worldCoord.m_x, sector.m_worldCoord.m_x);
        AZStd::string normalName = AZStd::string::format("Terrain Normals - Lod %u, Sector (%u, %u)", lodLevel, sector.m_worldCoord.m_x, sector.m_worldCoord.m_x);
        rtSector.m_positionsBuffer = CreateRayTracingMeshBufferInstance(AZ::RHI::Format::R32G32B32_FLOAT, m_gridVerts2D, nullptr, positionName.c_str());
        rtSector.m_normalsBuffer = CreateRayTracingMeshBufferInstance(AZ::RHI::Format::R32G32B32_FLOAT, m_gridVerts2D, nullptr, positionName.c_str());

        // setup the stream and shader buffer views
        AZ::RHI::Buffer& rhiPositionsBuffer = *rtSector.m_positionsBuffer->GetRHIBuffer();
        uint32_t positionsBufferByteCount = aznumeric_cast<uint32_t>(rhiPositionsBuffer.GetDescriptor().m_byteCount);
        AZ::RHI::Format positionsBufferFormat = rtSector.m_positionsBuffer->GetBufferViewDescriptor().m_elementFormat;
        uint32_t positionsBufferElementSize = AZ::RHI::GetFormatSize(positionsBufferFormat);
        AZ::RHI::StreamBufferView positionsVertexBufferView(rhiPositionsBuffer, 0, positionsBufferByteCount, positionsBufferElementSize);
        AZ::RHI::BufferViewDescriptor positionsBufferDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, positionsBufferByteCount);

        AZ::RHI::Buffer& rhiNormalsBuffer = *rtSector.m_normalsBuffer->GetRHIBuffer();
        uint32_t normalsBufferByteCount = aznumeric_cast<uint32_t>(rhiNormalsBuffer.GetDescriptor().m_byteCount);
        AZ::RHI::Format normalsBufferFormat = rtSector.m_normalsBuffer->GetBufferViewDescriptor().m_elementFormat;
        uint32_t normalsBufferElementSize = AZ::RHI::GetFormatSize(normalsBufferFormat);
        AZ::RHI::StreamBufferView normalsVertexBufferView(rhiNormalsBuffer, 0, normalsBufferByteCount, normalsBufferElementSize);
        AZ::RHI::BufferViewDescriptor normalsBufferDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, normalsBufferByteCount);

        AZ::RHI::Buffer& rhiIndexBuffer = *m_rtIndexBuffer->GetRHIBuffer();
        AZ::RHI::IndexFormat indexBufferFormat = AZ::RHI::IndexFormat::Uint32;

        uint32_t totalIndexBufferByteCount = aznumeric_cast<uint32_t>(rhiIndexBuffer.GetDescriptor().m_byteCount);
        uint32_t indexElementSize = AZ::RHI::GetIndexFormatSize(indexBufferFormat);

        // Create the ray tracing meshes. Each sector has 5 meshes which all share the same data - one mesh that covers the whole
        // sector, and 4 meshes that cover each quadrant of the sector.
        auto createMesh = [&](RtSector::MeshGroup& meshGroup, uint32_t indexBufferByteOffset, uint32_t indexBufferByteCount)
        {
            meshGroup.m_submeshVector.clear();
            AZ::Render::RayTracingFeatureProcessorInterface::SubMesh& subMesh = meshGroup.m_submeshVector.emplace_back();

            subMesh.m_positionFormat = positionsBufferFormat;
            subMesh.m_positionVertexBufferView = positionsVertexBufferView;
            subMesh.m_positionShaderBufferView = rhiPositionsBuffer.BuildBufferView(positionsBufferDescriptor);
            subMesh.m_normalFormat = normalsBufferFormat;
            subMesh.m_normalVertexBufferView = normalsVertexBufferView;
            subMesh.m_normalShaderBufferView = rhiNormalsBuffer.BuildBufferView(normalsBufferDescriptor);
            subMesh.m_indexBufferView = AZ::RHI::IndexBufferView(rhiIndexBuffer, indexBufferByteOffset, indexBufferByteCount, indexBufferFormat);
            subMesh.m_material.m_baseColor = AZ::Color::CreateFromVector3(AZ::Vector3(0.18f));

            AZ::RHI::BufferViewDescriptor indexBufferDescriptor;
            indexBufferDescriptor.m_elementOffset = indexBufferByteOffset / indexElementSize;
            indexBufferDescriptor.m_elementCount = indexBufferByteCount / indexElementSize;
            indexBufferDescriptor.m_elementSize = indexElementSize;
            indexBufferDescriptor.m_elementFormat = AZ::RHI::Format::R32_UINT;

            subMesh.m_indexShaderBufferView = rhiIndexBuffer.BuildBufferView(indexBufferDescriptor);

            meshGroup.m_mesh.m_assetId = AZ::Data::AssetId(meshGroup.m_id);
            float xyScale = (m_gridSize * m_sampleSpacing) * (1 << lodLevel);
            meshGroup.m_mesh.m_transform = AZ::Transform::CreateIdentity();
            meshGroup.m_mesh.m_nonUniformScale = AZ::Vector3(xyScale, xyScale, m_worldHeightBounds.m_max - m_worldHeightBounds.m_min);
        };

        createMesh(rtSector.m_meshGroups[0], 0, totalIndexBufferByteCount);

        uint32_t quarterCount = totalIndexBufferByteCount / 4;
        createMesh(rtSector.m_meshGroups[1], quarterCount * 0, quarterCount);
        createMesh(rtSector.m_meshGroups[2], quarterCount * 1, quarterCount);
        createMesh(rtSector.m_meshGroups[3], quarterCount * 2, quarterCount);
        createMesh(rtSector.m_meshGroups[4], quarterCount * 3, quarterCount);

    }

    void TerrainMeshManager::RebuildSectors()
    {
        const float gridMeters = m_gridSize * m_sampleSpacing;

        const auto& materialAsset = m_materialInstance->GetAsset();
        const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

        // Calculate the largest potential number of sectors needed per dimension at any lod level.
        const float firstLodDiameter = m_config.m_firstLodDistance * 2.0f;
        m_1dSectorCount = aznumeric_cast<uint32_t>(AZStd::ceilf(firstLodDiameter / gridMeters));
        // If the sector grid doesn't line up perfectly with the camera, it will cover part of a sector
        // along each boundary, so we need an extra sector to cover in those cases.
        m_1dSectorCount += 1;
        // Add one sector of wiggle room so to avoid thrashing updates when going back and forth over a boundary.
        m_1dSectorCount += 1;

        ClearSectorBuffers();

        const uint8_t lodCount = aznumeric_cast<uint8_t>(AZStd::ceilf(log2f(AZStd::GetMax(1.0f, m_config.m_renderDistance / m_config.m_firstLodDistance)) + 1.0f));
        m_sectorLods.reserve(lodCount);
        
        // Create all the sectors with uninitialized SRGs. The SRGs will be updated later by CheckLodGridsForUpdate().
        m_indexBufferView =
        {
            *m_indexBuffer->GetRHIBuffer(),
            0,
            aznumeric_cast<uint32_t>(m_indexBuffer->GetBufferSize()),
            AZ::RHI::IndexFormat::Uint16
        };

        for (uint8_t lodLevel = 0; lodLevel < lodCount; ++lodLevel)
        {
            m_sectorLods.push_back({});
            SectorLodGrid& lodGrid = m_sectorLods.back();

            lodGrid.m_sectors.resize(m_1dSectorCount * m_1dSectorCount);

            for (Sector& sector : lodGrid.m_sectors)
            {
                sector.m_srg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());

                sector.m_heightsNormalsBuffer = CreateMeshBufferInstance(sizeof(HeightNormalVertex), m_gridVerts2D);

                sector.m_geometryView.ClearStreamBufferViews();

                AZStd::vector<AZ::RHI::StreamBufferView>& streamBufferViews = sector.m_geometryView.GetStreamBufferViews();
                streamBufferViews.resize(StreamIndex::Count);
                streamBufferViews[StreamIndex::XYPositions] = CreateStreamBufferView(m_xyPositionsBuffer);
                streamBufferViews[StreamIndex::Heights] = CreateStreamBufferView(sector.m_heightsNormalsBuffer);
                streamBufferViews[StreamIndex::Normals] = CreateStreamBufferView(sector.m_heightsNormalsBuffer, AZ::RHI::GetFormatSize(HeightFormat));

                if (m_config.m_clodEnabled)
                {
                    sector.m_lodHeightsNormalsBuffer = CreateMeshBufferInstance(sizeof(HeightNormalVertex), m_gridVerts2D);
                    streamBufferViews[StreamIndex::LodHeights] = CreateStreamBufferView(sector.m_lodHeightsNormalsBuffer);
                    streamBufferViews[StreamIndex::LodNormals] = CreateStreamBufferView(sector.m_lodHeightsNormalsBuffer, AZ::RHI::GetFormatSize(HeightFormat));
                }
                else
                {
                    streamBufferViews[StreamIndex::LodHeights] = CreateStreamBufferView(m_dummyLodHeightsNormalsBuffer);
                    streamBufferViews[StreamIndex::LodNormals] = CreateStreamBufferView(m_dummyLodHeightsNormalsBuffer, AZ::RHI::GetFormatSize(HeightFormat));
                }

                BuildDrawPacket(sector);

                if (m_rayTracingEnabled)
                {
                    sector.m_rtData = AZStd::make_unique<RtSector>();
                    BuildRtSector(sector, lodLevel);
                }
            }
        }
    }

    void TerrainMeshManager::DrawMeshes(const AZ::RPI::FeatureProcessor::RenderPacket& process, const AZ::RPI::ViewPtr mainView)
    {
        AZ::Vector3 mainCameraPosition = mainView->GetCameraTransform().GetTranslation();
        CheckLodGridsForUpdate(mainCameraPosition);

        for (Sector* sector : m_sectorsThatNeedSrgCompiled)
        {
            sector->m_srg->Compile();
            sector->m_isQueuedForSrgCompile = false;
        }
        m_sectorsThatNeedSrgCompiled.clear();

        // Only update candidate sectors if the camera has moved. This could probably be relaxed further, but is a good starting point.
        const float minMovedDistanceSq = m_sampleSpacing * m_sampleSpacing;
        bool terrainChanged = m_candidateSectors.empty(); // candidate sectors need to be recalculated any time the terrain changes
        if (terrainChanged || m_cameraPosition.GetDistanceSq(mainCameraPosition) > minMovedDistanceSq)
        {
            m_cameraPosition = mainCameraPosition;
            UpdateCandidateSectors();
        }

        const AZ::RPI::AuxGeomDrawPtr auxGeomPtr = r_debugTerrainAabbs ?
            AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_parentScene) :
            nullptr;

        // Compare view frustums against the list of candidate sectors and submit those sectors to draw.
        for (auto& view : process.m_views)
        {
            if (terrainChanged)
            {
                view->ApplyFlags(m_meshMovedFlag.GetIndex());
            }

            const AZ::Frustum viewFrustum = AZ::Frustum::CreateFromMatrixColumnMajor(view->GetWorldToClipMatrix());
            for (CandidateSector& candidateSector : m_candidateSectors)
            {
                if (candidateSector.m_rhiDrawPacket && AZ::ShapeIntersection::Overlaps(viewFrustum, candidateSector.m_aabb))
                {
                    view->AddDrawPacket(candidateSector.m_rhiDrawPacket);
                    if (auxGeomPtr && view == mainView)
                    {
                        auxGeomPtr->DrawAabb(candidateSector.m_aabb, AZ::Colors::Red, AZ::RPI::AuxGeomDraw::DrawStyle::Line);
                    }
                }
            }
        }

    }

    void TerrainMeshManager::SetRebuildDrawPackets()
    {
        m_rebuildDrawPackets = true;
    }

    void TerrainMeshManager::RebuildDrawPackets()
    {
        m_materialInstance->ApplyGlobalShaderOptions();
        m_cachedDrawData.clear();
        m_candidateSectors.clear();

        // Rebuild common draw packet data
        m_materialInstance->ForAllShaderItems(
            [&](const AZ::Name& materialPipelineName, const AZ::RPI::ShaderCollection::Item& shaderItem)
            {
                if (!shaderItem.IsEnabled())
                {
                    return true;
                }

                // Force load and cache shader instances.
                AZ::Data::Instance<AZ::RPI::Shader> shader = AZ::RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
                if (!shader)
                {
                    AZ_Error(
                        TerrainMeshManagerName,
                        false,
                        "Shader '%s'. Failed to find or create instance",
                        shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return true;
                }

                // Skip the shader item without creating the shader instance
                // if the mesh is not going to be rendered based on the draw tag
                AZ::RHI::RHISystemInterface* rhiSystem = AZ::RHI::RHISystemInterface::Get();
                AZ::RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

                // Use the explicit draw list override if exists.
                AZ::RHI::DrawListTag drawListTag = shaderItem.GetDrawListTagOverride();

                if (drawListTag.IsNull())
                {
                    drawListTag = drawListTagRegistry->FindTag(shaderItem.GetShaderAsset()->GetDrawListName());
                }

                if (!m_parentScene->HasOutputForPipelineState(drawListTag))
                {
                    // drawListTag not found in this scene, so skip this item
                    return true;
                }

                // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
                // (because FindVariantStableId treats unspecified options as a request specifically for a variant that doesn't specify those
                // options) [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default values for us,
                // and remove this step here.
                AZ::RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
                shaderOptions.SetUnspecifiedToDefaultValues();

                const AZ::RPI::ShaderVariantId finalVariantId = shaderOptions.GetShaderVariantId();
                const AZ::RPI::ShaderVariant& variant = shader->GetVariant(finalVariantId);

                AZ::RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
                variant.ConfigurePipelineState(pipelineStateDescriptor, shaderOptions);

                AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "POSITION", 0 }, XYPositionFormat);
                layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "POSITION", 1 }, HeightFormat)->Padding(2);
                layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "NORMAL", 0 }, NormalFormat)->Padding(2);
                layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "POSITION", 2 }, HeightFormat)->Padding(2);
                layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "NORMAL", 1 }, NormalFormat)->Padding(2);
                pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

                m_parentScene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

                const AZ::RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
                if (!pipelineState)
                {
                    AZ_Error(
                        TerrainMeshManagerName,
                        false,
                        "Shader '%s'. Failed to acquire default pipeline state",
                        shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return true;
                }

                auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout(shader->GetSupervariantIndex());

                m_cachedDrawData.push_back({ shader, shaderOptions, pipelineState, drawListTag, drawSrgLayout, variant, materialPipelineName });
                return true;
            });

        // Rebuild the draw packets themselves
        for (auto& lodGrid : m_sectorLods)
        {
            for (auto& sector : lodGrid.m_sectors)
            {
                BuildDrawPacket(sector);
            }
        }
    }

    void TerrainMeshManager::OnTerrainDataCreateEnd()
    {
        OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
    }

    void TerrainMeshManager::OnTerrainDataDestroyBegin()
    {
        ClearSectorBuffers();
        m_rebuildSectors = true;
    }

    void TerrainMeshManager::OnTerrainDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion, TerrainDataChangedMask dataChangedMask)
    {
        if ((dataChangedMask & (TerrainDataChangedMask::HeightData | TerrainDataChangedMask::Settings)) != TerrainDataChangedMask::None)
        {
            AzFramework::Terrain::FloatRange heightBounds = AzFramework::Terrain::FloatRange::CreateNull();
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                heightBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightBounds);

            float queryResolution = 1.0f;
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

            bool gridSizeChanged = UpdateGridSize(m_config.m_firstLodDistance);

            // Sectors need to be rebuilt when certain settings change.
            m_rebuildSectors = m_rebuildSectors || (m_sampleSpacing != queryResolution) || (heightBounds != m_worldHeightBounds) || gridSizeChanged;

            m_worldHeightBounds = heightBounds;
            m_sampleSpacing = queryResolution;

            if (dirtyRegion.IsValid())
            {
                if (!m_rebuildSectors)
                {
                    // Rebuild any sectors in the dirty region if they aren't all being rebuilt
                    AZStd::vector<AZStd::vector<Sector*>> sectorsToUpdate(m_sectorLods.size());
                    ForOverlappingSectors(dirtyRegion,
                        [&sectorsToUpdate](Sector& sectorData, uint32_t lodLevel)
                        {
                            sectorsToUpdate.at(lodLevel).push_back(&sectorData);
                        }
                    );
                    if (!sectorsToUpdate.empty())
                    {
                        ProcessSectorUpdates(sectorsToUpdate);
                    }
                }
            }
        }
    }

    void TerrainMeshManager::CreateCommonBuffers()
    {
        // This function initializes positions and indices that are common to all terrain sectors. The indices are laid out
        // using a z-order curve (Morton code) which helps triangles which are close in space to also be close in the index
        // buffer. This in turn increases the probability that previously processed vertices will be in the vertex cache.

        // Generate x and y coordinates using Moser-de Bruijn sequences, so the final z-order position can be found quickly by interleaving.
        AZ_Assert(m_gridSize < AZStd::numeric_limits<uint8_t>::max(),
            "The following equation to generate z-order indices requires the number to be 8 or fewer bits.");

        AZStd::vector<uint16_t> zOrderX(m_gridSize);
        AZStd::vector<uint16_t> zOrderY(m_gridSize);

        for (uint16_t i = 0; i < m_gridSize; ++i)
        {
            // This will take any 8 bit number and put 0's in between each bit. For instance 0b1011 becomes 0b1000101.
            uint16_t value = ((i * 0x0101010101010101ULL & 0x8040201008040201ULL) * 0x0102040810204081ULL >> 49) & 0x5555;
            zOrderX.at(i) = value;
            zOrderY.at(i) = value << 1;
        }

        AZStd::vector<uint16_t> indices;
        indices.resize_no_construct(m_gridSize * m_gridSize * 6); // total number of quads, 2 triangles with 6 indices per quad.

        // Create the indices for a mesh patch in z-order for vertex cache optimization.
        for (uint16_t y = 0; y < m_gridSize; ++y)
        {
            for (uint16_t x = 0; x < m_gridSize; ++x)
            {
                uint32_t quadOrder = (zOrderX[x] | zOrderY[y]); // Interleave the x and y arrays from above for a final z-order index.
                quadOrder *= 6; // 6 indices per quad (2 triangles, 3 vertices each)

                const uint16_t topLeft = y * m_gridVerts1D + x;
                const uint16_t topRight = topLeft + 1;
                const uint16_t bottomLeft = topLeft + m_gridVerts1D;
                const uint16_t bottomRight = bottomLeft + 1;

                indices.at(quadOrder + 0) = topLeft;
                indices.at(quadOrder + 1) = topRight;
                indices.at(quadOrder + 2) = bottomLeft;
                indices.at(quadOrder + 3) = bottomLeft;
                indices.at(quadOrder + 4) = topRight;
                indices.at(quadOrder + 5) = bottomRight;
            }
        }

        // Infer the vertex order from the indices for cache efficient vertex buffer reads. Create a table that
        // can quickly map from a linear order (y * m_gridVerts1D + x) to the order dictated by the indices. Update
        // the index buffer to point directly to these new indices.
        constexpr uint16_t VertexNotSet = 0xFFFF;
        m_vertexOrder = AZStd::vector<uint16_t>(m_gridVerts2D, VertexNotSet);
        uint16_t vertex = 0;
        for (uint16_t& index : indices)
        {
            if (m_vertexOrder.at(index) == VertexNotSet)
            {
                // This is the first time this vertex has been seen in the index buffer, add it to the vertex order mapper.
                m_vertexOrder.at(index) = vertex;
                index = vertex;
                ++vertex;
            }
            else
            {
                // This vertex has already been added, so just update the index buffer to point to it.
                index = m_vertexOrder.at(index);
            }
        }

        m_indexBuffer = CreateMeshBufferInstance(
            AZ::RHI::GetFormatSize(AZ::RHI::Format::R16_UINT),
            aznumeric_cast<uint32_t>(indices.size()),
            indices.data());

        if (m_rayTracingEnabled)
        {
            // Generate a 32 bit index buffer for ray tracing by copying and transforming the 16 bit index buffer.
            AZStd::vector<uint32_t> rtIndices;
            rtIndices.resize_no_construct(indices.size());
            AZStd::transform(indices.begin(), indices.end(), rtIndices.begin(),
                [](uint16_t value)
                {
                    return static_cast<uint32_t>(value);
                }
            );
            m_rtIndexBuffer = CreateMeshBufferInstance(
                AZ::RHI::GetFormatSize(AZ::RHI::Format::R32_UINT),
                aznumeric_cast<uint32_t>(rtIndices.size()),
                rtIndices.data());
        }

        // Create x/y positions. These are the same for all sectors since they're in local space.
        m_xyPositions.resize_no_construct(m_gridVerts2D);
        for (uint8_t y = 0; y < m_gridVerts1D; ++y)
        {
            for (uint8_t x = 0; x < m_gridVerts1D; ++x)
            {
                uint16_t zOrderCoord = m_vertexOrder.at(y * m_gridVerts1D + x);
                m_xyPositions.at(zOrderCoord) = { x, y };
            }
        }

        m_xyPositionsBuffer = CreateMeshBufferInstance(
            AZ::RHI::GetFormatSize(XYPositionFormat),
            aznumeric_cast<uint32_t>(m_xyPositions.size()),
            m_xyPositions.data());

        m_dummyLodHeightsNormalsBuffer = CreateMeshBufferInstance(sizeof(HeightNormalVertex), m_gridVerts2D, nullptr);
    }

    void TerrainMeshManager::UpdateSectorBuffers(Sector& sector, const AZStd::span<const HeightNormalVertex> heightsNormals)
    {
        sector.m_heightsNormalsBuffer->UpdateData(heightsNormals.data(), heightsNormals.size_bytes());

        if (sector.m_rtData)
        {
            // While heightsNormals is in the exact format the terrain shader expects for optimum efficiency, for
            // ray tracing it needs to be a more conventional layout. So here we generate more traditional R32G32B32
            // data from the highly compressed HeightNormalVertex.

            struct RtVert
            {
                float x;
                float y;
                float z;
            };

            AZStd::vector<RtVert> rtPositions(heightsNormals.size());
            AZStd::vector<RtVert> rtNormals(heightsNormals.size());

            AZ_Assert(heightsNormals.size() == m_gridVerts2D, "Unexpected number of vertices.");

            constexpr float maxHeight = static_cast<float>(AZStd::numeric_limits<HeightDataType>::max());
            constexpr float maxNormal = static_cast<float>(AZStd::numeric_limits<NormalDataType>::max());

            for (uint32_t i = 0; i < heightsNormals.size(); ++i)
            {
                const HeightNormalVertex& heightNormal = heightsNormals[i];
                XYPosition xyPosition = m_xyPositions.at(i);
                float xyPositionMax = static_cast<float>(m_gridSize);

                rtPositions.at(i) =
                {
                    xyPosition.m_posx / xyPositionMax,
                    xyPosition.m_posy / xyPositionMax,
                    heightNormal.m_height == NoTerrainVertexHeight ?
                        0.0f :
                        heightNormal.m_height / maxHeight,
                };

                float normalX = heightNormal.m_normal.first / maxNormal;
                float normalY = heightNormal.m_normal.second / maxNormal;

                // It's a little unfortunate to use a sqrt to decode a normal which used a sqrt to encode in the
                // first place, but this avoids branching around ray tracing in GatherMeshData(). It also helps ensure
                // the ray traced normal lines up with the compressed one used in forward pass.
                float normalZ = sqrt(AZStd::GetMax(0.0f, 1.0f - normalX * normalX - normalY * normalY));

                rtNormals.at(i) = { normalX, normalY, normalZ };
            }

            sector.m_rtData->m_positionsBuffer->UpdateData(rtPositions.data(), rtPositions.size() * sizeof(RtVert));
            sector.m_rtData->m_normalsBuffer->UpdateData(rtNormals.data(), rtNormals.size() * sizeof(RtVert));

            // If the mesh is currently visible, it must be removed and re-added to update its data.
            for (RtSector::MeshGroup& meshGroup : sector.m_rtData->m_meshGroups)
            {
                if (meshGroup.m_isVisible)
                {
                    AZ::Vector3 translation = sector.m_aabb.GetMin();
                    translation.SetZ(m_worldHeightBounds.m_min);
                    meshGroup.m_mesh.m_transform = AZ::Transform::CreateTranslation(translation);
                    m_rayTracingFeatureProcessor->RemoveMesh(meshGroup.m_id);
                    m_rayTracingFeatureProcessor->AddMesh(meshGroup.m_id, meshGroup.m_mesh, meshGroup.m_submeshVector);
                }
            }
            
        }
    }

    void TerrainMeshManager::UpdateSectorLodBuffers(Sector& sector,
        const AZStd::span<const HeightNormalVertex> originalHeightsNormals,
        const AZStd::span<const HeightNormalVertex> lodHeightsNormals)
    {
        // Store the height and normal information for the next lod level in each vertex for continuous LOD.
        AZStd::vector<HeightNormalVertex> clodHeightNormals;
        clodHeightNormals.resize_no_construct(m_gridVerts2D);

        const uint16_t lodGridVerts1D = (m_gridVerts1D >> 1) + 1;

        for (uint16_t yPos = 0; yPos < m_gridVerts1D; ++yPos)
        {
            for (uint16_t xPos = 0; xPos < m_gridVerts1D; ++xPos)
            {
                uint16_t index = yPos * m_gridVerts1D + xPos;
                uint16_t lodIndex1 = (yPos / 2) * lodGridVerts1D + (xPos / 2);
                uint16_t lodIndex2 = lodIndex1;

                if (xPos % 2 == 1)
                {
                    // x position is between two vertices in the row
                    ++lodIndex1;
                }
                if (yPos % 2 == 1)
                {
                    // y position is between two vertices in the column
                    lodIndex2 += lodGridVerts1D;
                }

                const uint16_t zOrderIndex = m_vertexOrder.at(index);

                if (lodHeightsNormals[lodIndex1].m_height == NoTerrainVertexHeight || lodHeightsNormals[lodIndex2].m_height == NoTerrainVertexHeight)
                {
                    // One of the neighboring vertices has no data, so use the original height and normal
                    clodHeightNormals[zOrderIndex] = originalHeightsNormals[zOrderIndex];
                }
                else
                {
                    clodHeightNormals[zOrderIndex] =
                    {
                        HeightDataType((lodHeightsNormals[lodIndex1].m_height + lodHeightsNormals[lodIndex2].m_height) / 2),
                        NormalXYDataType(
                        {
                            int8_t((lodHeightsNormals[lodIndex1].m_normal.first + lodHeightsNormals[lodIndex2].m_normal.first) / 2),
                            int8_t((lodHeightsNormals[lodIndex1].m_normal.second + lodHeightsNormals[lodIndex2].m_normal.second) / 2)
                        })
                    };
                }
            }
        }

        sector.m_lodHeightsNormalsBuffer->UpdateData(clodHeightNormals.data(), clodHeightNormals.size() * sizeof(HeightNormalVertex), 0);
    }

    void TerrainMeshManager::GatherMeshData(SectorDataRequest request, AZStd::vector<HeightNormalVertex>& meshHeightsNormals, AZ::Aabb& meshAabb, bool& terrainExistsAnywhere)
    {
        const AZ::Vector2 stepSize(request.m_vertexSpacing);

        const uint16_t querySamplesX = request.m_samplesX + 2; // extra row / column on each side for normals.
        const uint16_t querySamplesY = request.m_samplesY + 2; // extra row / column on each side for normals.
        const uint16_t querySamplesCount = querySamplesX * querySamplesY;
        const uint16_t outputSamplesCount = request.m_samplesX * request.m_samplesY;

        AZStd::vector<float> heights;
        heights.resize_no_construct(querySamplesCount);

        meshHeightsNormals.resize_no_construct(outputSamplesCount);

        auto perPositionCallback = [this, &heights, querySamplesX, &terrainExistsAnywhere]
        (size_t xIndex, size_t yIndex, const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
        {
            static constexpr float HeightDoesNotExistValue = -1.0f;
            const float height = surfacePoint.m_position.GetZ() - m_worldHeightBounds.m_min;
            heights.at(yIndex * querySamplesX + xIndex) = terrainExists ? height : HeightDoesNotExistValue;
            terrainExistsAnywhere = terrainExistsAnywhere || terrainExists;
        };

        AzFramework::Terrain::TerrainQueryRegion queryRegion(
            request.m_worldStartPosition - stepSize, querySamplesX, querySamplesY, stepSize);

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequests::QueryRegion,
            queryRegion,
            AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights,
            perPositionCallback,
            request.m_samplerType);

        if (!terrainExistsAnywhere)
        {
            // No height data, so just return
            return;
        }

        float zExtents = (m_worldHeightBounds.m_max - m_worldHeightBounds.m_min);
        const float rcpWorldZ = 1.0f / zExtents;
        const float vertexSpacing2 = request.m_vertexSpacing * 2.0f;

        // initialize min/max heights to the max/min possible values so they're immediately updated when a valid point is found.
        float minHeight = zExtents;
        float maxHeight = 0.0f;

        // float versions of int max to make sure a int->float conversion doesn't happen at each loop iteration.
        constexpr float MaxHeightHalf = float(AZStd::numeric_limits<HeightDataType>::max() / 2);
        constexpr float MaxNormal = AZStd::numeric_limits<NormalDataType>::max();

        for (uint16_t y = 0; y < request.m_samplesY; ++y)
        {
            const uint16_t queryY = y + 1;

            for (uint16_t x = 0; x < request.m_samplesX; ++x)
            {
                const uint16_t queryX = x + 1;
                const uint16_t queryCoord = queryY * querySamplesX + queryX;

                uint16_t coord = y * request.m_samplesX + x;
                coord = request.m_useVertexOrderRemap ? m_vertexOrder.at(coord) : coord;

                const float height = heights.at(queryCoord);
                if (height < 0.0f)
                {
                    // Primary terrain height is limited to every-other bit, and clod heights can be in-between or the same
                    // as any of the primary heights. This leaves the max value as the single value that is never used by a
                    // legitimate height.
                    meshHeightsNormals.at(coord).m_height = NoTerrainVertexHeight;
                    continue;
                }

                const float clampedHeight = AZ::GetClamp(height * rcpWorldZ, 0.0f, 1.0f);

                // For continuous LOD, it needs to be possible to create a height that's exactly in between any other height, so scale
                // and quantize to half the height, then multiply by 2, ensuring there's always an in-between value available.
                const HeightDataType quantizedHeight = aznumeric_cast<HeightDataType>(clampedHeight * MaxHeightHalf + 0.5f); // always positive, so just add 0.5 to round.
                meshHeightsNormals.at(coord).m_height = quantizedHeight * 2;

                if (minHeight > height)
                {
                    minHeight = height;
                }
                else if (maxHeight < height)
                {
                    maxHeight = height;
                }

                auto getSlope = [&](float height1, float height2)
                {
                    if (height1 < 0.0f)
                    {
                        if (height2 < 0.0f)
                        {
                            // Assume no slope if the left and right vertices both don't exist.
                            return 0.0f;
                        }
                        else
                        {
                            return (height - height2) / request.m_vertexSpacing;
                        }
                    }
                    else
                    {
                        if (height2 < 0.0f)
                        {
                            return (height1 - height) / request.m_vertexSpacing;
                        }
                        else
                        {
                            return (height1 - height2) / vertexSpacing2;
                        }
                    }
                };

                const float leftHeight = heights.at(queryCoord - 1);
                const float rightHeight = heights.at(queryCoord + 1);
                const float xSlope = getSlope(leftHeight, rightHeight);
                const float normalX = xSlope / sqrt(xSlope * xSlope + 1); // sin(arctan(xSlope)

                const float upHeight = heights.at(queryCoord - querySamplesX);
                const float downHeight = heights.at(queryCoord + querySamplesX);
                const float ySlope = getSlope(upHeight, downHeight);
                const float normalY = ySlope / sqrt(ySlope * ySlope + 1); // sin(arctan(ySlope)

                meshHeightsNormals.at(coord).m_normal =
                {
                    aznumeric_cast<NormalDataType>(AZStd::lround(normalX * MaxNormal)),
                    aznumeric_cast<NormalDataType>(AZStd::lround(normalY * MaxNormal)),
                };
            }
        }

        if (maxHeight < minHeight)
        {
            // All height samples were invalid, so set the aabb to null.
            meshAabb.SetNull();
        }
        else
        {
            float width = (request.m_samplesX - 1) * request.m_vertexSpacing;
            float height = (request.m_samplesY - 1) * request.m_vertexSpacing;
            AZ::Vector3 aabbMin = AZ::Vector3(request.m_worldStartPosition.GetX(), request.m_worldStartPosition.GetY(), m_worldHeightBounds.m_min + minHeight);
            AZ::Vector3 aabbMax = AZ::Vector3(aabbMin.GetX() + width, aabbMin.GetY() + height, m_worldHeightBounds.m_min + maxHeight);
            meshAabb.Set(aabbMin, aabbMax);
        }
    }

    void TerrainMeshManager::ProcessSectorUpdates(AZStd::vector<AZStd::vector<Sector*>>& sectorUpdates)
    {
        AZ::JobCompletion jobCompletion;

        for (uint32_t lodLevel = 0; lodLevel < sectorUpdates.size(); ++lodLevel)
        {
            auto& sectors = sectorUpdates.at(lodLevel);
            if (sectors.empty())
            {
                continue;
            }

            for (Sector* sector : sectors)
            {
                const float gridMeters = (m_gridSize * m_sampleSpacing) * (1 << lodLevel);

                const auto jobLambda = [this, sector, gridMeters]() -> void
                {
                    AZStd::vector<HeightNormalVertex> meshHeightsNormals;

                    {
                        SectorDataRequest request;
                        request.m_samplesX = m_gridVerts1D;
                        request.m_samplesY = m_gridVerts1D;
                        request.m_worldStartPosition = sector->m_worldCoord.ToVector2() * gridMeters;
                        request.m_vertexSpacing = gridMeters / m_gridSize;
                        request.m_useVertexOrderRemap = true;

                        GatherMeshData(request, meshHeightsNormals, sector->m_aabb, sector->m_hasData);
                        if (sector->m_hasData)
                        {
                            UpdateSectorBuffers(*sector, meshHeightsNormals);
                        }

                        // Create AABBs for each quadrant for cases where this LOD needs to fill in a gap in a lower LOD.
                        CreateAabbQuadrants(sector->m_aabb, sector->m_quadrantAabbs);
                    }

                    if (m_config.m_clodEnabled && sector->m_hasData)
                    {
                        SectorDataRequest request;
                        uint16_t m_gridSizeNextLod = (m_gridSize >> 1);
                        request.m_samplesX = m_gridSizeNextLod + 1;
                        request.m_samplesY = m_gridSizeNextLod + 1;
                        request.m_worldStartPosition = sector->m_worldCoord.ToVector2() * gridMeters;
                        request.m_vertexSpacing = gridMeters / m_gridSizeNextLod;

                        AZ::Aabb dummyAabb = AZ::Aabb::CreateNull(); // Don't update the sector aabb based on only the clod vertices.
                        bool terrainExists = false;
                        AZStd::vector<HeightNormalVertex> meshLodHeightsNormals;
                        GatherMeshData(request, meshLodHeightsNormals, dummyAabb, terrainExists);
                        if (!terrainExists)
                        {
                            // It's unlikely but possible for the higher lod to have data and the lower lod to not. In that case 
                            // meshLodHeights will be empty, so fill it with values that represent "no data".
                            HeightNormalVertex defaultValue = { NoTerrainVertexHeight, NormalXYDataType(NormalDataType(0), NormalDataType(0)) };
                            AZStd::fill(meshLodHeightsNormals.begin(), meshLodHeightsNormals.end(), defaultValue);
                        }
                        UpdateSectorLodBuffers(*sector, meshHeightsNormals, meshLodHeightsNormals);
                    }
                };

                ShaderObjectData objectSrgData;
                objectSrgData.m_xyTranslation = { sector->m_worldCoord.m_x * gridMeters, sector->m_worldCoord.m_y * gridMeters };
                objectSrgData.m_xyScale = gridMeters * (aznumeric_cast<float>(AZStd::numeric_limits<uint8_t>::max()) / m_gridSize);
                objectSrgData.m_lodLevel = lodLevel;
                objectSrgData.m_rcpLodLevel = 1.0f / (lodLevel + 1);
                sector->m_srg->SetConstant(m_patchDataIndex, objectSrgData);
                if (!sector->m_isQueuedForSrgCompile)
                {
                    m_sectorsThatNeedSrgCompiled.push_back(sector);
                }
                sector->m_hasData = false; // mark the terrain as not having data for now. Once a job runs if it actually has data it'll flip to true.

                // Check against the area of terrain that could appear in this sector for any terrain areas. If none exist then skip updating the mesh.
                bool hasTerrain = false;
                AZ::Vector3 minAabb = AZ::Vector3(sector->m_worldCoord.m_x * gridMeters, sector->m_worldCoord.m_y * gridMeters, m_worldHeightBounds.m_min);
                AZ::Aabb sectorBounds = AZ::Aabb::CreateFromMinMax(minAabb,
                    minAabb + AZ::Vector3(gridMeters, gridMeters, m_worldHeightBounds.m_max - m_worldHeightBounds.m_min));
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    hasTerrain, &AzFramework::Terrain::TerrainDataRequests::TerrainAreaExistsInBounds, sectorBounds);

                if (hasTerrain)
                {
                    AZ::Job* executeGroupJob = aznew AZ::JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                    executeGroupJob->SetDependent(&jobCompletion);
                    executeGroupJob->Start();
                }
            }
        }
        jobCompletion.StartAndWaitForCompletion();
        m_candidateSectors.clear(); // Force recalculation of candidate sectors since AABBs could have changed.
    }

    void TerrainMeshManager::UpdateCandidateSectors()
    {
        // Gather a list of all sectors that could render based on their status, lod, and camera position.

        float maxDistanceSq = m_config.m_firstLodDistance * m_config.m_firstLodDistance;
        uint32_t nextLodSectorCount = m_1dSectorCount * 2; // The number of this lod's sectors that would fit into the next lod's space.
        AZStd::vector<bool> previousSelectedSectors;

        m_candidateSectors.clear();

        AZStd::vector<RayTracedItem> newRayTraceItems;
        if (m_rayTracingEnabled)
        {
            newRayTraceItems.reserve(m_sectorLods.size() * m_1dSectorCount * m_1dSectorCount);
        }

        for (uint32_t lodLevel = 0; lodLevel < m_sectorLods.size(); ++lodLevel)
        {
            auto& lodGrid = m_sectorLods.at(lodLevel);

            // Each sector in an LOD is half the size of a sector in the next LOD in each direction, so 4 sectors
            // in this LOD equal one sector in the next LOD. Construct a grid of bools for each sector in this
            // LOD that covers the entire space of the next LOD, and mark everything to false. As sectors in
            // this LOD are drawn, mark appropriate locations in the grid as true. When processing the next
            // LOD, each of the next LOD's sectors will look up the 4 entries in this that represent quadrants
            // of that sector to determine whether to draw nothing, specific quadrants, or the entire sector.

            AZStd::vector<bool> selectedSectors(nextLodSectorCount * nextLodSectorCount, false);

            Vector2i selectedSectorStartCoord{ 0, 0 };
            if (lodLevel == m_sectorLods.size() - 1)
            {
                // There is no next lod, so just use this one's start coord to avoid lots of checks in the for loop.
                selectedSectorStartCoord = m_sectorLods.at(lodLevel).m_startCoord;
            }
            else
            {
                // This is the start coord of the next LOD in the current LOD's scale.
                selectedSectorStartCoord = m_sectorLods.at(lodLevel + 1).m_startCoord * 2;
            }

            for (uint32_t sectorIndex = 0; sectorIndex < lodGrid.m_sectors.size(); ++sectorIndex)
            {
                Sector& sector = lodGrid.m_sectors.at(sectorIndex);
                Vector2i selectedCoord = sector.m_worldCoord - selectedSectorStartCoord;
                uint32_t selectedIndex = selectedCoord.m_y * nextLodSectorCount + selectedCoord.m_x;

                if (!sector.m_hasData)
                {
                    selectedSectors.at(selectedIndex) = true; // Terrain just doesn't exist here, so mark as "selected" so another LOD doesn't try to draw here.
                    continue;
                }

                const float aabbMinDistanceSq = sector.m_aabb.GetDistanceSq(m_cameraPosition);
                if (aabbMinDistanceSq < maxDistanceSq)
                {
                    selectedSectors.at(selectedIndex) = true;

                    if (lodLevel == 0)
                    {
                        // Since this is the first lod, no previous lod to check, so just draw.
                        m_candidateSectors.push_back({ sector.m_aabb, sector.m_rhiDrawPacket.get() });
                        if (sector.m_rtData)
                        {
                            newRayTraceItems.push_back({ &sector, 0, lodLevel });
                        }
                        continue;
                    }

                    Vector2i previousCoord = (sector.m_worldCoord - lodGrid.m_startCoord) * 2;
                    uint32_t previousDrawnIndex = previousCoord.m_y * nextLodSectorCount + previousCoord.m_x;

                    // Check the 4 sectors in the previous LOD that are covered by this sector.
                    uint8_t coveredByHigherLod =
                        (uint8_t(previousSelectedSectors.at(previousDrawnIndex)) << 0) | // Top left
                        (uint8_t(previousSelectedSectors.at(previousDrawnIndex + 1)) << 1) | // Top right
                        (uint8_t(previousSelectedSectors.at(previousDrawnIndex + nextLodSectorCount)) << 2) | // Bottom left
                        (uint8_t(previousSelectedSectors.at(previousDrawnIndex + nextLodSectorCount + 1)) << 3);  // Bottom right

                    if (coveredByHigherLod == 0b1111)
                    {
                        continue; // Completely covered by previous LOD, so do nothing
                    }
                    if (coveredByHigherLod == 0b0000)
                    {
                        // Not covered at all by previous LOD, so the draw entire sector
                        m_candidateSectors.push_back({ sector.m_aabb, sector.m_rhiDrawPacket.get() });
                        if (sector.m_rtData)
                        {
                            newRayTraceItems.push_back({ &sector, 0, lodLevel });
                        }
                    }
                    else
                    {
                        // Partially covered by previous LOD. Draw only missing quadrants
                        for (uint8_t i = 0; i < 4; ++i)
                        {
                            if ((coveredByHigherLod & 0b0001) == 0b0000)
                            {
                                m_candidateSectors.push_back({ sector.m_quadrantAabbs.at(i), sector.m_rhiDrawPacketQuadrant.at(i).get() });
                                if (sector.m_rtData)
                                {
                                    newRayTraceItems.push_back({ &sector, i + 1u, lodLevel });
                                }
                            }
                            coveredByHigherLod >>= 1;
                        }
                    }
                }
            }

            maxDistanceSq = maxDistanceSq * 4.0f; // Double the distance with squared distances is * 2^2.
            previousSelectedSectors = AZStd::move(selectedSectors);
        }

        if (m_rayTracingEnabled)
        {
            // Compare the sorted new list to the old list to figure out which ray traced sectors need to be
            // added or removed.

            auto getMeshGroup = [](auto& item) -> auto&
            {
                return item.m_sector->m_rtData->m_meshGroups[item.m_meshGroupIndex];
            };

            AZStd::sort(newRayTraceItems.begin(), newRayTraceItems.end(),
                [&getMeshGroup](const RayTracedItem& value1, const RayTracedItem& value2) -> bool
                {
                    return getMeshGroup(value1).m_id < getMeshGroup(value2).m_id;
                }
            );

            auto prevIt = m_rayTracedItems.begin();
            auto newIt = newRayTraceItems.begin();

            auto addMesh = [&](RayTracedItem& item, RtSector::MeshGroup& meshGroup)
            {
                const float gridMeters = (m_gridSize * m_sampleSpacing) * (1 << item.m_lodLevel);
                AZ::Vector3 translation = AZ::Vector3(item.m_sector->m_worldCoord.m_x * gridMeters, item.m_sector->m_worldCoord.m_y * gridMeters, m_worldHeightBounds.m_min);
                meshGroup.m_mesh.m_transform = AZ::Transform::CreateTranslation(translation);
                meshGroup.m_isVisible = true;
                m_rayTracingFeatureProcessor->AddMesh(meshGroup.m_id, meshGroup.m_mesh, meshGroup.m_submeshVector);
            };

            auto removeMesh = [&](RtSector::MeshGroup& meshGroup)
            {
                meshGroup.m_isVisible = false;
                m_rayTracingFeatureProcessor->RemoveMesh(meshGroup.m_id);
            };

            // Since the two lists are sorted, we can easily compare them and figure out which items need
            // to be removed or added. If a uuid shows up in the old list first, then it must not be in the new
            // list, so it needs to be removed, then only the old list iterator is incremented. Similarly if a
            // uuid shows up in the new list first then it must not be in the old list, so it needs to be added.
            // Finally if the uuids match, they're in both lists, and therefore both iterators can be incremented.
            while (prevIt < m_rayTracedItems.end() && newIt < newRayTraceItems.end())
            {
                RtSector::MeshGroup& prevMeshGroup = getMeshGroup(*prevIt);
                RtSector::MeshGroup& newMeshGroup = getMeshGroup(*newIt);
                if (prevMeshGroup.m_id < newMeshGroup.m_id)
                {
                    removeMesh(prevMeshGroup);
                    ++prevIt;
                }
                else if (prevMeshGroup.m_id > newMeshGroup.m_id)
                {
                    addMesh(*newIt, newMeshGroup);
                    ++newIt;
                }
                else
                {
                    ++prevIt;
                    ++newIt;
                }
            }

            // Since the above loop stops when either iterator is done, remaining items in the other iterator need to be handled here.
            while (prevIt < m_rayTracedItems.end())
            {
                removeMesh(getMeshGroup(*prevIt));
                ++prevIt;
            }
            while (newIt < newRayTraceItems.end())
            {
                addMesh(*newIt, getMeshGroup(*newIt));
                ++newIt;
            }

            m_rayTracedItems = AZStd::move(newRayTraceItems);
        }
    }

    void TerrainMeshManager::CreateAabbQuadrants(const AZ::Aabb& aabb, AZStd::span<AZ::Aabb, 4> quadrantAabb)
    {
        // Create 4 AABBs for each quadrant on the xy plane.
        if (aabb.IsValid())
        {
            float centerX = aabb.GetCenter().GetX();
            float centerY = aabb.GetCenter().GetY();

            quadrantAabb[0] = AZ::Aabb::CreateFromMinMax(
                aabb.GetMin(),
                AZ::Vector3(centerX, centerY, aabb.GetMax().GetZ())
            );

            float halfExtentX = aabb.GetXExtent() * 0.5f;
            float halfExtentY = aabb.GetYExtent() * 0.5f;

            quadrantAabb[1] = quadrantAabb[0].GetTranslated(AZ::Vector3(halfExtentX, 0.0f, 0.0f));
            quadrantAabb[2] = quadrantAabb[0].GetTranslated(AZ::Vector3(0.0f, halfExtentY, 0.0f));
            quadrantAabb[3] = quadrantAabb[0].GetTranslated(AZ::Vector3(halfExtentX, halfExtentY, 0.0f));
        }
        else
        {
            AZStd::fill(quadrantAabb.begin(), quadrantAabb.end(), AZ::Aabb::CreateNull());
        }
    }

    template<typename Callback>
    void TerrainMeshManager::ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback)
    {
        const AZ::Vector2 boundsMin2d = AZ::Vector2(bounds.GetMin());
        const AZ::Vector2 boundsMax2d = AZ::Vector2(bounds.GetMax());

        for (uint32_t lodLevel = 0; lodLevel < m_sectorLods.size(); ++lodLevel)
        {
            // Expand the bounds by the spacing of the lod since vertex normals are affected by neighbors.
            // The bounds needs to be 2x what's expected because clod also encodes information about the normals
            // for the next lod level in the current lod level (which has vertices spaced 2x as far apart)
            const AZ::Vector2 lodSpacing = AZ::Vector2(m_sampleSpacing * (1 << lodLevel) * 2.0f);
            const AZ::Vector2 lodBoundsMin2d = boundsMin2d - lodSpacing;
            const AZ::Vector2 lodBoundsMax2d = boundsMax2d + lodSpacing;
            const float gridMeters = (m_gridSize * m_sampleSpacing) * (1 << lodLevel);

            auto& lodGrid = m_sectorLods.at(lodLevel);
            for (Sector& sector : lodGrid.m_sectors)
            {
                const AZ::Vector2 sectorAabbMin2D = sector.m_worldCoord.ToVector2() * gridMeters;
                const AZ::Vector2 sectorAabbMax2D = sectorAabbMin2D + AZ::Vector2(gridMeters);

                const bool overlaps = sectorAabbMin2D.IsLessEqualThan(lodBoundsMax2d) && sectorAabbMax2D.IsGreaterEqualThan(lodBoundsMin2d);
                if (overlaps)
                {
                    callback(sector, lodLevel);
                }
            }
        }
    }

    AZ::Data::Instance<AZ::RPI::Buffer> TerrainMeshManager::CreateMeshBufferInstance(uint32_t elementSize, uint32_t elementCount, const void* initialData, const char* name)
    {
        AZ::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = AZ::RPI::CommonBufferPoolType::StaticInputAssembly;
        desc.m_elementSize = elementSize;
        desc.m_byteCount = desc.m_elementSize * elementCount;
        desc.m_bufferData = initialData;

        if (name != nullptr)
        {
            desc.m_bufferName = name;
        }

        return AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
    }

    AZ::Data::Instance<AZ::RPI::Buffer> TerrainMeshManager::CreateRayTracingMeshBufferInstance(AZ::RHI::Format elementFormat, uint32_t elementCount, const void* initialData, const char* name)
    {
        AZ::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = AZ::RPI::CommonBufferPoolType::DynamicInputAssembly;
        desc.m_elementSize = AZ::RHI::GetFormatSize(elementFormat);
        desc.m_byteCount = desc.m_elementSize * elementCount;
        desc.m_bufferData = initialData;
        desc.m_elementFormat = elementFormat;

        if (name != nullptr)
        {
            desc.m_bufferName = name;
        }

        return AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
    }

}
