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

    namespace ShaderInputs
    {
        static const char* const PatchData("m_patchData");
    }

    void TerrainMeshManager::Initialize()
    {
        if (!InitializePatchModel())
        {
            AZ_Error(TerrainMeshManagerName, false, "Failed to create Terrain render buffers!");
            return;
        }

        OnTerrainDataChanged(AZ::Aabb::CreateNull(), TerrainDataChangedMask::HeightData);
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        m_isInitialized = true;
    }

    bool TerrainMeshManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainMeshManager::Reset()
    {
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
        m_patchModel = {};
        m_sectorData.clear();
        m_rebuildSectors = true;
        m_isInitialized = false;
    }

    bool TerrainMeshManager::CheckRebuildSurfaces(MaterialInstance materialInstance, AZ::RPI::Scene& parentScene)
    {
        if (!m_rebuildSectors)
        {
            return false;
        }

        m_rebuildSectors = false;
        m_sectorData.clear();

        const auto layout = materialInstance->GetAsset()->GetObjectSrgLayout();

        AZ::RHI::ShaderInputConstantIndex patchDataIndex = layout->FindShaderInputConstantIndex(AZ::Name(ShaderInputs::PatchData));
        AZ_Error(TerrainMeshManagerName, patchDataIndex.IsValid(), "Failed to find shader input constant %s.", ShaderInputs::PatchData);

        const float xFirstPatchStart = AZStd::floorf(m_worldBounds.GetMin().GetX() / GridMeters) * GridMeters;
        const float xLastPatchStart = AZStd::floorf(m_worldBounds.GetMax().GetX() / GridMeters) * GridMeters;
        const float yFirstPatchStart = AZStd::floorf(m_worldBounds.GetMin().GetY() / GridMeters) * GridMeters;
        const float yLastPatchStart = AZStd::floorf(m_worldBounds.GetMax().GetY() / GridMeters) * GridMeters;

        const auto& materialAsset = materialInstance->GetAsset();
        const auto& shaderAsset = materialAsset->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();

        for (float yPatch = yFirstPatchStart; yPatch <= yLastPatchStart; yPatch += GridMeters)
        {
            for (float xPatch = xFirstPatchStart; xPatch <= xLastPatchStart; xPatch += GridMeters)
            {
                ShaderTerrainData objectSrgData;
                objectSrgData.m_xyTranslation = { xPatch, yPatch };

                m_sectorData.push_back();
                SectorData& sectorData = m_sectorData.back();

                for (auto& lod : m_patchModel->GetLods())
                {
                    objectSrgData.m_xyScale = m_sampleSpacing * GridSize;

                    auto objectSrg = AZ::RPI::ShaderResourceGroup::Create(shaderAsset, materialAsset->GetObjectSrgLayout()->GetName());
                    if (!objectSrg)
                    {
                        AZ_WarningOnce(TerrainMeshManagerName, false, "Failed to create a new shader resource group, skipping.");
                        continue;
                    }
                    objectSrg->SetConstant(patchDataIndex, objectSrgData);
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
                        AZ::Vector3(xPatch + GridMeters, yPatch + GridMeters, m_worldBounds.GetMax().GetZ())
                    );
            }
        }
        return true;
    }

    void TerrainMeshManager::DrawMeshes(const AZ::RPI::FeatureProcessor::RenderPacket& process)
    {
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

            AZ::Vector2 queryResolution2D = AZ::Vector2(1.0f);
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                queryResolution2D, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);
            // Currently query resolution is multidimensional but the rendering system only supports this changing in one dimension.
            float queryResolution = queryResolution2D.GetX();

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

    bool TerrainMeshManager::InitializePatchModel()
    {
        AZ::RPI::ModelAssetCreator modelAssetCreator;
        modelAssetCreator.Begin(AZ::Uuid::CreateRandom());

        uint16_t gridSize = GridSize;

        for (uint32_t i = 0; i < AZ::RPI::ModelLodAsset::LodCountMax && gridSize > 0; ++i)
        {
            PatchData patchData;
            InitializeTerrainPatch(gridSize, patchData);

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
            modelLodAssetCreator.AddMeshStreamBuffer(AZ::RHI::ShaderSemantic{ "POSITION" }, AZ::Name(), {positionsOutcome.GetValue(), positionBufferViewDesc});
            modelLodAssetCreator.SetMeshIndexBuffer({indicesOutcome.GetValue(), indexBufferViewDesc});

            AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0, 0.0, 0.0), AZ::Vector3(GridMeters, GridMeters, 0.0));
            modelLodAssetCreator.SetMeshAabb(AZStd::move(aabb));
            modelLodAssetCreator.SetMeshName(AZ::Name("Terrain Patch"));
            modelLodAssetCreator.EndMesh();

            AZ::Data::Asset<AZ::RPI::ModelLodAsset> modelLodAsset;
            modelLodAssetCreator.End(modelLodAsset);

            modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));

            gridSize = gridSize / 2;
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset;
        bool success = modelAssetCreator.End(modelAsset);

        m_patchModel = AZ::RPI::Model::FindOrCreate(modelAsset);

        return success;
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
