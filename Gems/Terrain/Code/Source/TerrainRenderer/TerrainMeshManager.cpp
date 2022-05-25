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
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/RenderCommon.h>

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
        InitializeCommonSectorData();

        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        m_handleGlobalShaderOptionUpdate = AZ::RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler
        {
            [this](const AZ::Name&, AZ::RPI::ShaderOptionValue) { m_rebuildDrawPackets = true; }
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

            // Queue the load of the material's shaders now since they'll be needed later.
            for (auto& shaderItem : m_materialInstance->GetShaderCollection())
            {
                AZ::Data::Asset<AZ::RPI::ShaderAsset> shaderAsset = shaderItem.GetShaderAsset();
                if (!shaderAsset.IsReady())
                {
                    shaderAsset.QueueLoad();
                }
            }

            m_rebuildDrawPackets = true;
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

    void TerrainMeshManager::OnRenderPipelineAdded([[maybe_unused]] AZ::RPI::RenderPipelinePtr pipeline)
    {
        m_rebuildDrawPackets = true;
    }

    void TerrainMeshManager::OnRenderPipelinePassesChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline)
    {
        m_rebuildDrawPackets = true;
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
            RebuildSectors();
            m_rebuildSectors = false;
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
            StackData& stackData = m_sectorStack.at(i);

            auto [newStartCoordX, newStartCoordY] = [&]()
            {
                const float maxDistance = m_config.m_firstLodDistance * aznumeric_cast<float>(1 << i);
                const float gridMeters = (GridSize * m_sampleSpacing) * (1 << i);
                const int32_t startCoordX = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetX() - maxDistance) / gridMeters));
                const int32_t startCoordY = aznumeric_cast<int32_t>(AZStd::floorf((newPosition.GetY() - maxDistance) / gridMeters));

                // If the start coord for the stack is different, then some of the sectors will need to be updated.
                // There's 1 sector of wiggle room, so make sure we've moving the lod's start coord by as little as possible.

                const int32_t newStartCoordX = startCoordX > stackData.m_startCoordX + 1 ? startCoordX - 1 :
                    (startCoordX < stackData.m_startCoordX ? startCoordX : stackData.m_startCoordX);
                const int32_t newStartCoordY = startCoordY > stackData.m_startCoordY + 1 ? startCoordY - 1 :
                    (startCoordY < stackData.m_startCoordY ? startCoordY : stackData.m_startCoordY);

                return AZStd::pair(newStartCoordX, newStartCoordY);
            }();

            if (stackData.m_startCoordX != newStartCoordX || stackData.m_startCoordY != newStartCoordY)
            {
                stackData.m_startCoordX = newStartCoordX;
                stackData.m_startCoordY = newStartCoordY;

                const uint32_t firstSectorIndexX = (m_1dSectorCount + (newStartCoordX % m_1dSectorCount)) % m_1dSectorCount;
                const uint32_t firstSectorIndexY = (m_1dSectorCount + (newStartCoordY % m_1dSectorCount)) % m_1dSectorCount;

                for (uint32_t xOffset = 0; xOffset < m_1dSectorCount; ++xOffset)
                {
                    for (uint32_t yOffset = 0; yOffset < m_1dSectorCount; ++yOffset)
                    {
                        // Sectors use toroidal addressing to avoid needing to update any more than necessary.

                        const uint32_t sectorIndexX = (firstSectorIndexX + xOffset) % m_1dSectorCount;
                        const uint32_t sectorIndexY = (firstSectorIndexY + yOffset) % m_1dSectorCount;
                        const uint32_t sectorIndex = sectorIndexY * m_1dSectorCount + sectorIndexX;

                        const int32_t worldX = newStartCoordX + xOffset;
                        const int32_t worldY = newStartCoordY + yOffset;

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

    AZ::RHI::StreamBufferView TerrainMeshManager::CreateStreamBufferView(AZ::Data::Instance<AZ::RPI::Buffer>& buffer)
    {
        return
        {
            *buffer->GetRHIBuffer(),
            0,
            aznumeric_cast<uint32_t>(buffer->GetBufferSize()),
            AZ::RHI::GetFormatSize(buffer->GetBufferViewDescriptor().m_elementFormat)
        };
    }

    void TerrainMeshManager::BuildDrawPacket(StackSectorData& sector)
    {
        AZ::RHI::DrawPacketBuilder drawPacketBuilder;
        drawPacketBuilder.Begin(nullptr);
        drawPacketBuilder.SetDrawArguments(AZ::RHI::DrawIndexed(1, 0, 0, m_indexBuffer->GetBufferViewDescriptor().m_elementCount, 0));
        drawPacketBuilder.SetIndexBufferView(m_indexBufferView);
        drawPacketBuilder.AddShaderResourceGroup(sector.m_srg->GetRHIShaderResourceGroup());
        drawPacketBuilder.AddShaderResourceGroup(m_materialInstance->GetRHIShaderResourceGroup());

        sector.m_perDrawSrgs.clear();

        for (CachedDrawData& drawData : m_cachedDrawData)
        {
            AZ::Data::Instance<AZ::RPI::Shader>& shader = drawData.m_shader;

            AZ::RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = drawData.m_drawListTag;
            drawRequest.m_pipelineState = drawData.m_pipelineState;
            drawRequest.m_streamBufferViews = sector.m_streamBufferViews;
            drawRequest.m_stencilRef = AZ::Render::StencilRefs::UseDiffuseGIPass | AZ::Render::StencilRefs::UseIBLSpecularPass;

            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg;
            if (drawData.m_drawSrgLayout)
            {
                // If the DrawSrg exists we must create and bind it, otherwise the CommandList will fail validation for SRG being null
                drawSrg = AZ::RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawData.m_drawSrgLayout->GetName());
                if (!drawData.m_shaderVariant.IsFullyBaked() && drawData.m_drawSrgLayout->HasShaderVariantKeyFallbackEntry())
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

        sector.m_rhiDrawPacket = drawPacketBuilder.End();

    }

    void TerrainMeshManager::RebuildSectors()
    {
        const float gridMeters = GridSize * m_sampleSpacing;

        const auto& materialAsset = m_materialInstance->GetAsset();
        const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

        // Calculate the largest potential number of sectors needed per dimension at any stack level.
        const float firstLodDiameter = m_config.m_firstLodDistance * 2.0f;
        m_1dSectorCount = aznumeric_cast<uint32_t>(AZStd::ceilf(firstLodDiameter / gridMeters));
        // If the sector grid doesn't line up perfectly with the camera, it will cover part of a sector
        // along each boundary, so we need an extra sector to cover in those cases.
        m_1dSectorCount += 1;
        // Add one sector of wiggle room so to avoid thrashing updates when going back and forth over a boundary.
        m_1dSectorCount += 1;

        m_sectorStack.clear();

        const uint32_t stackCount = aznumeric_cast<uint32_t>(ceil(log2f(AZStd::GetMax(1.0f, m_config.m_renderDistance / m_config.m_firstLodDistance)) + 1.0f));
        m_sectorStack.reserve(stackCount);

        // Create all the sectors with uninitialized SRGs. The SRGs will be updated later by CheckStacksForUpdate().
        m_indexBufferView =
        {
            *m_indexBuffer->GetRHIBuffer(),
            0,
            aznumeric_cast<uint32_t>(m_indexBuffer->GetBufferSize()),
            AZ::RHI::IndexFormat::Uint16
        };

        for (uint32_t j = 0; j < stackCount; ++j)
        {
            m_sectorStack.push_back({});
            StackData& stackData = m_sectorStack.back();

            stackData.m_sectors.resize(m_1dSectorCount * m_1dSectorCount);

            for (uint32_t i = 0; i < stackData.m_sectors.size(); ++i)
            {
                StackSectorData& sector = stackData.m_sectors.at(i);

                sector.m_srg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());

                uint32_t vertexCount = (GridSize + 1) * (GridSize + 1);
                sector.m_heightsBuffer = CreateMeshBufferInstance(HeightFormat, vertexCount);
                sector.m_normalsBuffer = CreateMeshBufferInstance(NormalFormat, vertexCount);
                sector.m_streamBufferViews[0] = CreateStreamBufferView(m_xyPositionsBuffer);
                sector.m_streamBufferViews[1] = CreateStreamBufferView(sector.m_heightsBuffer);
                sector.m_streamBufferViews[2] = CreateStreamBufferView(sector.m_normalsBuffer);

                BuildDrawPacket(sector);
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
                        view->AddDrawPacket(sector.m_rhiDrawPacket.get());
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
        m_materialInstance->ApplyGlobalShaderOptions();
        m_cachedDrawData.clear();

        // Rebuild common draw packet data
        for (auto& shaderItem : m_materialInstance->GetShaderCollection())
        {
            if (!shaderItem.IsEnabled())
            {
                continue;
            }

            // Force load and cache shader instances.
            AZ::Data::Instance<AZ::RPI::Shader> shader = AZ::RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
            if (!shader)
            {
                AZ_Error("TerrainMeshManager", false, "Shader '%s'. Failed to find or create instance", shaderItem.GetShaderAsset()->GetName().GetCStr());
                continue;
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
                // drawListTag not found in this scene, so don't render this item
                return;
            }

            // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
            // (because FindVariantStableId treats unspecified options as a request specifically for a variant that doesn't specify those options)
            // [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default values for us, and remove this step here.
            AZ::RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
            shaderOptions.SetUnspecifiedToDefaultValues();

            const AZ::RPI::ShaderVariantId finalVariantId = shaderOptions.GetShaderVariantId();
            const AZ::RPI::ShaderVariant& variant = shader->GetVariant(finalVariantId);

            AZ::RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            variant.ConfigurePipelineState(pipelineStateDescriptor);

            AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "POSITION", 0 }, XYPositionFormat);
            layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "POSITION", 1 }, HeightFormat);
            layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "NORMAL" }, NormalFormat);
            pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

            m_parentScene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

            const AZ::RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
            if (!pipelineState)
            {
                AZ_Error("TerrainMeshManager", false, "Shader '%s'. Failed to acquire default pipeline state", shaderItem.GetShaderAsset()->GetName().GetCStr());
                return;
            }

            auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout(shader->GetSupervariantIndex());

            m_cachedDrawData.push_back({ shader, shaderOptions, pipelineState, drawListTag, drawSrgLayout, variant});
        }

        // Rebuild the draw packets themselves
        for (auto& stackData : m_sectorStack)
        {
            for (auto& sector : stackData.m_sectors)
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

    AZ::Data::Instance<AZ::RPI::Buffer> TerrainMeshManager::CreateMeshBufferInstance(AZ::RHI::Format format, uint64_t elementCount, const void* initialData)
    {
        AZ::RPI::CommonBufferDescriptor desc;
        desc.m_poolType = AZ::RPI::CommonBufferPoolType::StaticInputAssembly;
        desc.m_elementSize = AZ::RHI::GetFormatSize(format);
        desc.m_byteCount = desc.m_elementSize * elementCount;
        desc.m_elementFormat = format;
        desc.m_bufferData = initialData;

        return AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
    }

    void TerrainMeshManager::UpdateSectorBuffers(StackSectorData& sector, const AZStd::span<const HeightDataType> heights, const AZStd::span<const NormalDataType> normals)
    {
        sector.m_heightsBuffer->UpdateData(heights.data(), heights.size_bytes(), 0);
        sector.m_normalsBuffer->UpdateData(normals.data(), normals.size_bytes(), 0);
    }

    void TerrainMeshManager::InitializeCommonSectorData()
    {
        PatchData patchData;
        InitializeTerrainPatch(GridSize, patchData);

        m_xyPositionsBuffer = CreateMeshBufferInstance(XYPositionFormat, patchData.m_xyPositions.size(), patchData.m_xyPositions.data());
        m_indexBuffer = CreateMeshBufferInstance(AZ::RHI::Format::R16_UINT, patchData.m_indices.size(), patchData.m_indices.data());
    }

    void TerrainMeshManager::GatherMeshData(SectorDataRequest request, AZStd::vector<HeightDataType>& meshHeights, AZStd::vector<NormalDataType>& meshNormals, AZ::Aabb& meshAabb)
    {
        AZ::Vector3 aabbMin = AZ::Vector3(request.m_worldStartPosition.GetX(), request.m_worldStartPosition.GetY(), m_worldBounds.GetMin().GetZ());
        AZ::Vector3 aabbMax = aabbMin + AZ::Vector3(request.m_gridSize * request.m_vertexSpacing);

        // expand the bounds in order to calculate normals.
        AZ::Vector3 queryAabbMin = aabbMin - AZ::Vector3(request.m_vertexSpacing);
        AZ::Vector3 queryAabbMax = aabbMax + AZ::Vector3(request.m_vertexSpacing * 2.0f); // extra padding to catch the last vertex

        // pad the max by half a sample spacing to make sure it's inclusive of the last point.
        AZ::Aabb queryBounds = AZ::Aabb::CreateFromMinMax(queryAabbMin, queryAabbMax);

        auto samplerType = AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP;
        const AZ::Vector2 stepSize(request.m_vertexSpacing);

        AZStd::pair<size_t, size_t> numSamples;
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            numSamples, &AzFramework::Terrain::TerrainDataRequests::GetNumSamplesFromRegion,
            queryBounds, stepSize, samplerType);

        uint16_t vertexCount1d = (request.m_gridSize + 1); // grid size is length, need an extra vertex in each dimension to draw the final row / column of quads.
        uint16_t paddedVertexCount1d = vertexCount1d + 2; // extra row / column on each side for normals.

        if (numSamples.first != paddedVertexCount1d || numSamples.second != paddedVertexCount1d)
        {
            AZ_Assert(false, "Number of samples returned from GetNumSamplesFromRegion does not match expectations");
            return;
        }

        uint16_t meshVertexCount = vertexCount1d * vertexCount1d;
        uint16_t queryVertexCount = paddedVertexCount1d * paddedVertexCount1d;
        AZStd::vector<float> heights;
        heights.resize_no_construct(queryVertexCount);

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
        const float vertexSpacing2 = request.m_vertexSpacing * 2.0f;

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

        aabbMin.SetZ(minHeight);
        aabbMax.SetZ(maxHeight);
        meshAabb.Set(aabbMin, aabbMax);
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

                SectorDataRequest request;
                request.m_gridSize = GridSize;
                request.m_worldStartPosition = AZ::Vector2(sector.m_worldX * gridMeters, sector.m_worldY * gridMeters);
                request.m_vertexSpacing = gridMeters / GridSize;

                AZStd::vector<HeightDataType> meshHeights;
                AZStd::vector<NormalDataType> meshNormals;
                GatherMeshData(request, meshHeights, meshNormals, sector.m_aabb);

                UpdateSectorBuffers(sector, meshHeights, meshNormals);

                ShaderObjectData objectSrgData;
                objectSrgData.m_xyTranslation = { sector.m_aabb.GetMin().GetX(), sector.m_aabb.GetMin().GetY()};
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
        const AZ::Vector2 boundsMin2d = AZ::Vector2(bounds.GetMin());
        const AZ::Vector2 boundsMax2d = AZ::Vector2(bounds.GetMax());

        for (uint32_t lodLevel = 0; lodLevel < m_sectorStack.size(); ++lodLevel)
        {
            // Expand the bounds by the spacing of the lod since vertex normals are affected by neighbors.
            const AZ::Vector2 lodSpacing = AZ::Vector2(m_sampleSpacing * (1 << lodLevel));
            const AZ::Vector2 lodBoundsMin2d = boundsMin2d - lodSpacing;
            const AZ::Vector2 lodBoundsMax2d = boundsMax2d + lodSpacing;

            auto& stackData = m_sectorStack.at(lodLevel);
            for (StackSectorData& sectorData : stackData.m_sectors)
            {
                const AZ::Vector2 sectorAabbMin2D = AZ::Vector2(sectorData.m_aabb.GetMin());
                const AZ::Vector2 sectorAabbMax2D = AZ::Vector2(sectorData.m_aabb.GetMax());
                const bool overlaps = sectorAabbMin2D.IsLessEqualThan(lodBoundsMax2d) && sectorAabbMax2D.IsGreaterEqualThan(lodBoundsMin2d);
                if (overlaps)
                {
                    callback(sectorData, lodLevel);
                }
            }
        }
    }

}
