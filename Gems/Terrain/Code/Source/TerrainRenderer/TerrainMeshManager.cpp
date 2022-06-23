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
#include <RayTracing/RayTracingFeatureProcessor.h>

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
        if (m_config.CheckWouldRequireRebuild(config))
        {
            m_rebuildSectors = true;
            OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
        }
        m_config = config;

        // This will trigger a draw packet rebuild later.
        AZ::RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(AZ::Name{ "o_useTerrainClod" }, AZ::RPI::ShaderOptionValue{ m_config.m_clodEnabled });
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

        AZ::Render::RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_parentScene->GetFeatureProcessor<AZ::Render::RayTracingFeatureProcessor>();
        if (rayTracingFeatureProcessor)
        {
            rayTracingFeatureProcessor->RemoveMesh(m_rayTracingMeshUuid);
        }

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
        meshData.m_rcpClodDistance = 1.0f / m_config.m_clodDistance;
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

                sector.m_heightsNormalsBuffer = CreateMeshBufferInstance(sizeof(HeightNormalVertex), GridVerts2D);
                sector.m_streamBufferViews[StreamIndex::XYPositions] = CreateStreamBufferView(m_xyPositionsBuffer);
                sector.m_streamBufferViews[StreamIndex::Heights] = CreateStreamBufferView(sector.m_heightsNormalsBuffer);
                sector.m_streamBufferViews[StreamIndex::Normals] = CreateStreamBufferView(sector.m_heightsNormalsBuffer, AZ::RHI::GetFormatSize(HeightFormat));

                if (m_config.m_clodEnabled)
                {
                    sector.m_lodHeightsNormalsBuffer = CreateMeshBufferInstance(sizeof(HeightNormalVertex), GridVerts2D);
                    sector.m_streamBufferViews[StreamIndex::LodHeights] = CreateStreamBufferView(sector.m_lodHeightsNormalsBuffer);
                    sector.m_streamBufferViews[StreamIndex::LodNormals] = CreateStreamBufferView(sector.m_lodHeightsNormalsBuffer, AZ::RHI::GetFormatSize(HeightFormat));
                }
                else
                {
                    sector.m_streamBufferViews[StreamIndex::LodHeights] = CreateStreamBufferView(m_dummyLodHeightsNormalsBuffer);
                    sector.m_streamBufferViews[StreamIndex::LodNormals] = CreateStreamBufferView(m_dummyLodHeightsNormalsBuffer, AZ::RHI::GetFormatSize(HeightFormat));
                }

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
                    if (!sector.m_hasData || // No terrain areas exist in this sector or it's all empty.
                        viewVector.Dot(viewPosition - sector.m_aabb.GetCenter()) < -radius || // Cheap check to eliminate sectors behind camera
                        viewFrustum.IntersectAabb(sector.m_aabb) == AZ::IntersectResult::Exterior) // Check against frustum
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
                AZ_Error(TerrainMeshManagerName, false, "Shader '%s'. Failed to find or create instance", shaderItem.GetShaderAsset()->GetName().GetCStr());
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
            layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "NORMAL", 0 }, NormalFormat);
            layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "POSITION", 2 }, HeightFormat);
            layoutBuilder.AddBuffer()->Channel(AZ::RHI::ShaderSemantic{ "NORMAL", 1 }, NormalFormat);
            pipelineStateDescriptor.m_inputStreamLayout = layoutBuilder.End();

            m_parentScene->ConfigurePipelineState(drawListTag, pipelineStateDescriptor);

            const AZ::RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
            if (!pipelineState)
            {
                AZ_Error(TerrainMeshManagerName, false, "Shader '%s'. Failed to acquire default pipeline state", shaderItem.GetShaderAsset()->GetName().GetCStr());
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

            // Sectors need to be rebuilt if the sample spacing changes.
            m_rebuildSectors = m_rebuildSectors || (m_sampleSpacing != queryResolution);

            m_worldBounds = worldBounds;
            m_sampleSpacing = queryResolution;

            if (dirtyRegion.IsValid())
            {
                AZ::Aabb clampedDirtyRegion = dirtyRegion.GetClamped(m_worldBounds);
                if (!m_rebuildSectors)
                {
                    // Rebuild any sectors in the dirty region if they aren't all being rebuilt
                    AZStd::vector<SectorUpdateContext> sectorsToUpdate;
                    ForOverlappingSectors(clampedDirtyRegion,
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

                UpdateRaytracingData(clampedDirtyRegion);
            }
        }
    }

    void TerrainMeshManager::InitializeTerrainPatch(PatchData& patchdata)
    {
        // This function initializes positions and indices that are common to all terrain sectors. The indices are laid out
        // using a z-order curve (Morton code) which helps triangles which are close in space to also be close in the index
        // buffer. This in turn increases the probability that previously processed vertices will be in the vertex cache.

        patchdata.m_xyPositions.clear();
        patchdata.m_indices.clear();

        // Generate x and y coordinates using Moser-de Bruijn sequences, so the final z-order position can be found quickly by interleaving.
        static_assert(GridSize < AZStd::numeric_limits<uint8_t>::max(),
            "The following equation to generate z-order indices requires the number to be 8 or fewer bits.");

        AZStd::array<uint16_t, GridSize> zOrderX;
        AZStd::array<uint16_t, GridSize> zOrderY;

        for (uint16_t i = 0; i < GridSize; ++i)
        {
            // This will take any 8 bit number and put 0's in between each bit. For instance 0b1011 becomes 0b1000101.
            uint16_t value = ((i * 0x0101010101010101ULL & 0x8040201008040201ULL) * 0x0102040810204081ULL >> 49) & 0x5555;
            zOrderX.at(i) = value;
            zOrderY.at(i) = value << 1;
        }

        patchdata.m_indices.resize_no_construct(GridSize * GridSize * 6); // total number of quads, 2 triangles with 6 indices per quad.

        // Create the indices for a mesh patch in z-order for vertex cache optimization.
        for (uint16_t y = 0; y < GridSize; ++y)
        {
            for (uint16_t x = 0; x < GridSize; ++x)
            {
                uint16_t quadOrder = (zOrderX[x] | zOrderY[y]); // Interleave the x and y arrays from above for a final z-order index.
                quadOrder *= 6; // 6 indices per quad (2 triangles, 3 vertices each)

                const uint16_t topLeft = y * GridVerts1D + x;
                const uint16_t topRight = topLeft + 1;
                const uint16_t bottomLeft = topLeft + GridVerts1D;
                const uint16_t bottomRight = bottomLeft + 1;

                patchdata.m_indices.at(quadOrder + 0) = topLeft;
                patchdata.m_indices.at(quadOrder + 1) = topRight;
                patchdata.m_indices.at(quadOrder + 2) = bottomLeft;
                patchdata.m_indices.at(quadOrder + 3) = bottomLeft;
                patchdata.m_indices.at(quadOrder + 4) = topRight;
                patchdata.m_indices.at(quadOrder + 5) = bottomRight;
            }
        }

        // Infer the vertex order from the indices for cache efficient vertex buffer reads. Create a table that
        // can quickly map from a linear order (y * GridVerts1D + x) to the order dictated by the indices. Update
        // the index buffer to point directly to these new indices.
        constexpr uint16_t VertexNotSet = 0xFFFF;
        m_vertexOrder = AZStd::vector<uint16_t>(GridVerts2D, VertexNotSet);
        uint16_t vertex = 0;
        for (uint16_t& index : patchdata.m_indices)
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

        // Create x/y positions. These are the same for all sectors since they're in local space.
        patchdata.m_xyPositions.resize_no_construct(GridVerts2D);
        for (uint16_t y = 0; y < GridVerts1D; ++y)
        {
            for (uint16_t x = 0; x < GridVerts1D; ++x)
            {
                uint16_t zOrderCoord = m_vertexOrder.at(y * GridVerts1D + x);
                patchdata.m_xyPositions.at(zOrderCoord) = { aznumeric_cast<float>(x) / GridSize, aznumeric_cast<float>(y) / GridSize };
            }
        }

    }

    void TerrainMeshManager::UpdateSectorBuffers(StackSectorData& sector, const AZStd::span<const HeightNormalVertex> heightsNormals)
    {
        sector.m_heightsNormalsBuffer->UpdateData(heightsNormals.data(), heightsNormals.size_bytes());
    }

    void TerrainMeshManager::UpdateSectorLodBuffers(StackSectorData& sector,
        const AZStd::span<const HeightNormalVertex> originalHeightsNormals,
        const AZStd::span<const HeightNormalVertex> lodHeightsNormals)
    {
        // Store the height and normal information for the next lod level in each vertex for continuous LOD.
        AZStd::vector<HeightNormalVertex> clodHeightNormals;
        clodHeightNormals.resize_no_construct(GridVerts2D);

        constexpr uint16_t LodGridVerts1D = (GridVerts1D >> 1) + 1;

        for (uint16_t yPos = 0; yPos < GridVerts1D; ++yPos)
        {
            for (uint16_t xPos = 0; xPos < GridVerts1D; ++xPos)
            {
                uint16_t index = yPos * GridVerts1D + xPos;
                uint16_t lodIndex1 = (yPos / 2) * LodGridVerts1D + (xPos / 2);
                uint16_t lodIndex2 = lodIndex1;

                if (xPos % 2 == 1)
                {
                    // x position is between two vertices in the row
                    ++lodIndex1;
                }
                if (yPos % 2 == 1)
                {
                    // y position is between two vertices in the column
                    lodIndex2 += LodGridVerts1D;
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
                        NormalDataType(
                        {
                            int16_t((lodHeightsNormals[lodIndex1].m_normal.first + lodHeightsNormals[lodIndex2].m_normal.first) / 2),
                            int16_t((lodHeightsNormals[lodIndex1].m_normal.second + lodHeightsNormals[lodIndex2].m_normal.second) / 2)
                        })
                    };
                }
            }
        }

        sector.m_lodHeightsNormalsBuffer->UpdateData(clodHeightNormals.data(), clodHeightNormals.size() * sizeof(HeightNormalVertex), 0);
    }

    void TerrainMeshManager::InitializeCommonSectorData()
    {
        PatchData patchData;
        InitializeTerrainPatch(patchData);

        m_xyPositionsBuffer = CreateMeshBufferInstance(
            AZ::RHI::GetFormatSize(XYPositionFormat),
            aznumeric_cast<uint32_t>(patchData.m_xyPositions.size()),
            patchData.m_xyPositions.data());
        m_indexBuffer = CreateMeshBufferInstance(
            AZ::RHI::GetFormatSize(AZ::RHI::Format::R16_UINT),
            aznumeric_cast<uint32_t>(patchData.m_indices.size()),
            patchData.m_indices.data());

        m_dummyLodHeightsNormalsBuffer = CreateMeshBufferInstance(sizeof(HeightNormalVertex), GridVerts2D, nullptr);

        constexpr uint32_t rayTracingVertices1d = RayTracingQuads1D + 1; // need vertex for end cap
        constexpr uint32_t rayTracingTotalVertices = rayTracingVertices1d * rayTracingVertices1d;
        m_raytracingPositionsBuffer = CreateRayTracingMeshBufferInstance(AZ::RHI::Format::R32G32B32_FLOAT, rayTracingTotalVertices, nullptr, "TerrainRaytracingPositions");
        m_raytracingNormalsBuffer = CreateRayTracingMeshBufferInstance(AZ::RHI::Format::R32G32B32_FLOAT, rayTracingTotalVertices, nullptr, "TerrainRaytracingNormals");

        constexpr uint32_t rayTracingIndicesCount = RayTracingQuads1D * RayTracingQuads1D * 2 * 3; // 2 triangles per quad, 3 vertices per triangle
        AZStd::vector<uint32_t> raytracingIndices;
        raytracingIndices.reserve(rayTracingIndicesCount);

        for (uint32_t y = 0; y < RayTracingQuads1D; ++y)
        {
            for (uint32_t x = 0; x < RayTracingQuads1D; ++x)
            {
                const uint32_t topLeft = y * (RayTracingQuads1D + 1) + x;
                const uint32_t topRight = topLeft + 1;
                const uint32_t bottomLeft = (y + 1) * (RayTracingQuads1D + 1) + x;
                const uint32_t bottomRight = bottomLeft + 1;

                raytracingIndices.emplace_back(topLeft);
                raytracingIndices.emplace_back(topRight);
                raytracingIndices.emplace_back(bottomLeft);
                raytracingIndices.emplace_back(bottomLeft);
                raytracingIndices.emplace_back(topRight);
                raytracingIndices.emplace_back(bottomRight);
            }
        }

        m_raytracingIndexBuffer = CreateRayTracingMeshBufferInstance(AZ::RHI::Format::R32_UINT, rayTracingIndicesCount, raytracingIndices.data(), "TerrainRaytracingIndices");
        m_rayTracingMeshUuid = AZ::Uuid::CreateRandom();
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
            const float height = surfacePoint.m_position.GetZ() - m_worldBounds.GetMin().GetZ();
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

        const float rcpWorldZ = 1.0f / m_worldBounds.GetExtents().GetZ();
        const float vertexSpacing2 = request.m_vertexSpacing * 2.0f;

        // initialize min/max heights to the max/min possible values so they're immediately updated when a valid point is found.
        float minHeight = m_worldBounds.GetExtents().GetZ();
        float maxHeight = 0.0f;

        // float versions of int max to make sure a int->float conversion doesn't happen at each loop iteration.
        constexpr float maxUint15 = float(AZStd::numeric_limits<uint16_t>::max() / 2);
        constexpr float maxInt16 = AZStd::numeric_limits<int16_t>::max();

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

                // For continuous LOD, it needs to be possible to create a height that's exactly in between any other height, so scale to 15 bits
                // instead of 16, then multiply by 2, ensuring there's always an in-between value available.
                const uint16_t uint16Height = aznumeric_cast<uint16_t>(clampedHeight * maxUint15 + 0.5f); // always positive, so just add 0.5 to round.
                meshHeightsNormals.at(coord).m_height = uint16Height * 2;

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
                    aznumeric_cast<int16_t>(AZStd::lround(normalX * maxInt16)),
                    aznumeric_cast<int16_t>(AZStd::lround(normalY * maxInt16)),
                };
            }
        }

        float width = (request.m_samplesX - 1) * request.m_vertexSpacing;
        float height = (request.m_samplesY - 1) * request.m_vertexSpacing;
        AZ::Vector3 aabbMin = AZ::Vector3(request.m_worldStartPosition.GetX(), request.m_worldStartPosition.GetY(), m_worldBounds.GetMin().GetZ() + minHeight);
        AZ::Vector3 aabbMax = AZ::Vector3(aabbMin.GetX() + width, aabbMin.GetY() + height, m_worldBounds.GetMin().GetZ() + maxHeight);
        meshAabb.Set(aabbMin, aabbMax);
    }

    void TerrainMeshManager::ProcessSectorUpdates(AZStd::span<SectorUpdateContext> sectorUpdates)
    {
        AZ::JobCompletion jobCompletion;
        for (SectorUpdateContext& updateContext : sectorUpdates)
        {
            const float gridMeters = (GridSize * m_sampleSpacing) * (1 << updateContext.m_lodLevel);
            StackSectorData* sector = updateContext.m_sector;

            const auto jobLambda = [this, sector, gridMeters]() -> void
            {
                AZStd::vector<HeightNormalVertex> meshHeightsNormals;

                {
                    SectorDataRequest request;
                    request.m_samplesX = GridVerts1D;
                    request.m_samplesY = GridVerts1D;
                    request.m_worldStartPosition = AZ::Vector2(sector->m_worldX * gridMeters, sector->m_worldY * gridMeters);
                    request.m_vertexSpacing = gridMeters / GridSize;
                    request.m_useVertexOrderRemap = true;

                    GatherMeshData(request, meshHeightsNormals, sector->m_aabb, sector->m_hasData);
                    if (sector->m_hasData)
                    {
                        UpdateSectorBuffers(*sector, meshHeightsNormals);
                    }
                }

                if (m_config.m_clodEnabled && sector->m_hasData)
                {
                    SectorDataRequest request;
                    uint16_t gridSizeNextLod = (GridSize >> 1);
                    request.m_samplesX = gridSizeNextLod + 1;
                    request.m_samplesY = gridSizeNextLod + 1;
                    request.m_worldStartPosition = AZ::Vector2(sector->m_worldX * gridMeters, sector->m_worldY * gridMeters);
                    request.m_vertexSpacing = gridMeters / gridSizeNextLod;

                    AZ::Aabb dummyAabb = AZ::Aabb::CreateNull(); // Don't update the sector aabb based on only the clod vertices.
                    bool terrainExists = false;
                    AZStd::vector<HeightNormalVertex> meshLodHeightsNormals;
                    GatherMeshData(request, meshLodHeightsNormals, dummyAabb, terrainExists);
                    if (!terrainExists)
                    {
                        // It's unlikely but possible for the higher lod to have data and the lower lod to not. In that case 
                        // meshLodHeights will be empty, so fill it with values that represent "no data".
                        HeightNormalVertex defaultValue = { NoTerrainVertexHeight, NormalDataType(int16_t(0), int16_t(0)) };
                        AZStd::fill(meshLodHeightsNormals.begin(), meshLodHeightsNormals.end(), defaultValue);
                    }
                    UpdateSectorLodBuffers(*sector, meshHeightsNormals, meshLodHeightsNormals);
                }
            };

            ShaderObjectData objectSrgData;
            objectSrgData.m_xyTranslation = { sector->m_worldX * gridMeters, sector->m_worldY * gridMeters };
            objectSrgData.m_xyScale = gridMeters;
            objectSrgData.m_lodLevel = updateContext.m_lodLevel;
            objectSrgData.m_rcpLodLevel = 1.0f / (updateContext.m_lodLevel + 1);
            sector->m_srg->SetConstant(m_patchDataIndex, objectSrgData);
            sector->m_srg->Compile();

            // Check against the area of terrain that could appear in this sector for any terrain areas. If none exist then skip updating the mesh.
            bool hasTerrain = false;
            AZ::Vector3 minAabb = AZ::Vector3(sector->m_worldX * gridMeters, sector->m_worldY * gridMeters, m_worldBounds.GetMin().GetZ());
            AZ::Aabb sectorBounds = AZ::Aabb::CreateFromMinMax(minAabb, minAabb + AZ::Vector3(gridMeters, gridMeters, m_worldBounds.GetZExtent()));
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                hasTerrain, &AzFramework::Terrain::TerrainDataRequests::TerrainAreaExistsInBounds, sectorBounds);

            if (hasTerrain)
            {
                AZ::Job* executeGroupJob = aznew AZ::JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                executeGroupJob->SetDependent(&jobCompletion);
                executeGroupJob->Start();
            }
            else
            {
                sector->m_hasData = false;
            }

        }
        jobCompletion.StartAndWaitForCompletion();

    }

    void TerrainMeshManager::UpdateRaytracingData(const AZ::Aabb& bounds)
    {
        AZ::Render::RayTracingFeatureProcessor* rayTracingFeatureProcessor = m_parentScene->GetFeatureProcessor<AZ::Render::RayTracingFeatureProcessor>();
        if (!rayTracingFeatureProcessor)
        {
            return;
        }

        // remove existing mesh from the raytracing scene
        rayTracingFeatureProcessor->RemoveMesh(m_rayTracingMeshUuid);

        // build the new position and normal buffers
        SectorDataRequest request;
        request.m_worldStartPosition = AZ::Vector2(bounds.GetMin());
        request.m_vertexSpacing = AZStd::GetMax(m_worldBounds.GetXExtent(), m_worldBounds.GetYExtent()) / RayTracingQuads1D;
        request.m_samplesX = aznumeric_cast<uint16_t>(bounds.GetXExtent() / request.m_vertexSpacing) + 1;
        request.m_samplesY = aznumeric_cast<uint16_t>(bounds.GetYExtent() / request.m_vertexSpacing) + 1;
        request.m_samplerType = AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT;

        AZStd::vector<HeightNormalVertex> meshHeightsNormals;
        AZ::Aabb outAabb;
        bool terrainExistsAnywhere = false; // ignored by ray tracing for now
        GatherMeshData(request, meshHeightsNormals, outAabb, terrainExistsAnywhere);

        struct Position
        {
            float x;
            float y;
            float z;
        };

        struct Normal
        {
            float x;
            float y;
            float z;
        };

        Position* positions = reinterpret_cast<Position*>(m_raytracingPositionsBuffer->Map(m_raytracingPositionsBuffer->GetBufferSize(), 0));
        Normal* normals = reinterpret_cast<Normal*>(m_raytracingNormalsBuffer->Map(m_raytracingNormalsBuffer->GetBufferSize(), 0));

        if (positions == nullptr || normals == nullptr)
        {
            AZ_Error(TerrainMeshManagerName, false, "Enable to map buffers for ray tracing mesh.");
            return;
        }

        uint32_t xMin = aznumeric_cast<uint32_t>((bounds.GetMin().GetX() - m_worldBounds.GetMin().GetX()) / request.m_vertexSpacing);
        uint32_t xMax = xMin + request.m_samplesX;
        uint32_t yMin = aznumeric_cast<uint32_t>((bounds.GetMin().GetY() - m_worldBounds.GetMin().GetY()) / request.m_vertexSpacing);
        uint32_t yMax = yMin + request.m_samplesY;

        constexpr uint32_t RayTracingVertices1D = RayTracingQuads1D + 1;

        for (uint32_t y = yMin; y < yMax; ++y)
        {
            for (uint32_t x = xMin; x < xMax; ++x)
            {
                uint32_t index = y * RayTracingVertices1D + x;
                uint32_t localIndex = (y - yMin) * request.m_samplesX + (x - xMin);
                AZ::Vector2 xyPosition = AZ::Vector2(m_worldBounds.GetMin()) + AZ::Vector2(float(x), float(y)) * request.m_vertexSpacing;

                float floatHeight = 0.0f;
                if (meshHeightsNormals.at(localIndex).m_height != NoTerrainVertexHeight)
                {
                    floatHeight = meshHeightsNormals.at(localIndex).m_height / float(AZStd::numeric_limits<uint16_t>::max()) * m_worldBounds.GetZExtent();
                }

                positions[index] = { xyPosition.GetX(), xyPosition.GetY(), floatHeight };

                float normalX = aznumeric_cast<float>(meshHeightsNormals.at(localIndex).m_normal.first) / AZStd::numeric_limits<int16_t>::max();
                float normalY = aznumeric_cast<float>(meshHeightsNormals.at(localIndex).m_normal.second) / AZStd::numeric_limits<int16_t>::max();
                float normalZ = sqrtf(1.0f - (normalX * normalX) - (normalY * normalY));
                normals[index] = { normalX, normalY, normalZ };
            }
        }

        m_raytracingPositionsBuffer->Unmap();
        m_raytracingNormalsBuffer->Unmap();

        // setup the stream and shader buffer views
        AZ::RHI::Buffer& rhiPositionsBuffer = *m_raytracingPositionsBuffer->GetRHIBuffer();
        uint32_t positionsBufferByteCount = aznumeric_cast<uint32_t>(rhiPositionsBuffer.GetDescriptor().m_byteCount);
        AZ::RHI::Format positionsBufferFormat = m_raytracingPositionsBuffer->GetBufferViewDescriptor().m_elementFormat;
        uint32_t positionsBufferElementSize = AZ::RHI::GetFormatSize(positionsBufferFormat);
        AZ::RHI::StreamBufferView positionsVertexBufferView(rhiPositionsBuffer, 0, positionsBufferByteCount, positionsBufferElementSize);
        AZ::RHI::BufferViewDescriptor positionsBufferDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, positionsBufferByteCount);

        AZ::RHI::Buffer& rhiNormalsBuffer = *m_raytracingNormalsBuffer->GetRHIBuffer();
        uint32_t normalsBufferByteCount = aznumeric_cast<uint32_t>(rhiNormalsBuffer.GetDescriptor().m_byteCount);
        AZ::RHI::Format normalsBufferFormat = m_raytracingNormalsBuffer->GetBufferViewDescriptor().m_elementFormat;
        uint32_t normalsBufferElementSize = AZ::RHI::GetFormatSize(normalsBufferFormat);
        AZ::RHI::StreamBufferView normalsVertexBufferView(rhiNormalsBuffer, 0, normalsBufferByteCount, normalsBufferElementSize);
        AZ::RHI::BufferViewDescriptor normalsBufferDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, normalsBufferByteCount);

        AZ::RHI::Buffer& rhiIndexBuffer = *m_raytracingIndexBuffer->GetRHIBuffer();
        uint32_t indexBufferByteCount = aznumeric_cast<uint32_t>(rhiIndexBuffer.GetDescriptor().m_byteCount);
        AZ::RHI::IndexFormat indexBufferFormat = AZ::RHI::IndexFormat::Uint32;
        AZ::RHI::IndexBufferView indexBufferView(rhiIndexBuffer, 0, indexBufferByteCount, indexBufferFormat);

        uint32_t indexElementSize = AZ::RHI::GetIndexFormatSize(indexBufferFormat);
        uint32_t indexElementCount = indexBufferByteCount / indexElementSize;
        AZ::RHI::BufferViewDescriptor indexBufferDescriptor;
        indexBufferDescriptor.m_elementOffset = 0;
        indexBufferDescriptor.m_elementCount = indexElementCount;
        indexBufferDescriptor.m_elementSize = indexElementSize;
        indexBufferDescriptor.m_elementFormat = AZ::RHI::Format::R32_UINT;

        // build the terrain raytracing submesh
        AZ::Render::RayTracingFeatureProcessor::SubMeshVector subMeshVector;
        AZ::Render::RayTracingFeatureProcessor::SubMesh& subMesh = subMeshVector.emplace_back();
        subMesh.m_positionFormat = positionsBufferFormat;
        subMesh.m_positionVertexBufferView = positionsVertexBufferView;
        subMesh.m_positionShaderBufferView = rhiPositionsBuffer.GetBufferView(positionsBufferDescriptor);
        subMesh.m_normalFormat = normalsBufferFormat;
        subMesh.m_normalVertexBufferView = normalsVertexBufferView;
        subMesh.m_normalShaderBufferView = rhiNormalsBuffer.GetBufferView(normalsBufferDescriptor);
        subMesh.m_indexBufferView = indexBufferView;
        subMesh.m_indexShaderBufferView = rhiIndexBuffer.GetBufferView(indexBufferDescriptor);

        // add the submesh to the raytracing scene
        // Note: we use the terrain mesh UUID as the AssetId since it is dynamically created and will not have multiple instances
        rayTracingFeatureProcessor->AddMesh(m_rayTracingMeshUuid, AZ::Data::AssetId(m_rayTracingMeshUuid), subMeshVector, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne());
    }

    template<typename Callback>
    void TerrainMeshManager::ForOverlappingSectors(const AZ::Aabb& bounds, Callback callback)
    {
        const AZ::Vector2 boundsMin2d = AZ::Vector2(bounds.GetMin());
        const AZ::Vector2 boundsMax2d = AZ::Vector2(bounds.GetMax());

        for (uint32_t lodLevel = 0; lodLevel < m_sectorStack.size(); ++lodLevel)
        {
            // Expand the bounds by the spacing of the lod since vertex normals are affected by neighbors.
            // The bounds needs to be 2x what's expected because clod also encodes information about the normals
            // for the next lod level in the current lod level (which has vertices spaced 2x as far apart)
            const AZ::Vector2 lodSpacing = AZ::Vector2(m_sampleSpacing * (1 << lodLevel) * 2.0f);
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
