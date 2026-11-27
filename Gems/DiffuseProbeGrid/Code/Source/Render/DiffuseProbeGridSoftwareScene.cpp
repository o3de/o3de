/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Render/DiffuseProbeGridSoftwareScene.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/std/algorithm.h>

namespace AZ::Render
{
    namespace
    {
        // BufferAssetView retains an asset id after CPU data is released. Request a private reference asynchronously
        // instead of blocking rendering or retaining every model's buffer assets globally.
        Data::Asset<RPI::BufferAsset> RequestBuffer(const RPI::BufferAssetView& view)
        {
            return Data::AssetManager::Instance().GetAsset<RPI::BufferAsset>(
                view.GetBufferAsset().GetId(), Data::AssetLoadBehavior::QueueLoad);
        }

        AZStd::span<const uint8_t> GetBytes(const Data::Asset<RPI::BufferAsset>& asset, const RPI::BufferAssetView& view)
        {
            if (!asset.IsReady())
            {
                return {};
            }
            const auto bytes = asset->GetBuffer();
            const auto& descriptor = view.GetBufferViewDescriptor();
            const uint64_t offset = uint64_t(descriptor.m_elementOffset) * descriptor.m_elementSize;
            const uint64_t size = uint64_t(descriptor.m_elementCount) * descriptor.m_elementSize;
            if (offset > bytes.size() || size > bytes.size() - offset)
            {
                return {};
            }
            return bytes.subspan(aznumeric_cast<size_t>(offset), aznumeric_cast<size_t>(size));
        }
    }

    void DiffuseProbeGridSoftwareScene::Clear()
    {
        m_instances.clear();
        m_geometries.clear();
    }

    bool DiffuseProbeGridSoftwareScene::BuildGeometry(Geometry& geometry)
    {
        const auto& mesh = geometry.m_modelAsset->GetLodAssets()[geometry.m_lod]->GetMeshes()[geometry.m_mesh];
        const auto* positionView = mesh.GetSemanticBufferAssetView(Name("POSITION"));
        if (!positionView || mesh.GetIndexCount() < 3)
        {
            return false;
        }
        const auto& indexView = mesh.GetIndexBufferAssetView();
        if (!geometry.m_positions.GetId().IsValid())
        {
            geometry.m_positions = RequestBuffer(*positionView);
            geometry.m_indices = RequestBuffer(indexView);
        }
        if (!geometry.m_positions.IsReady() || !geometry.m_indices.IsReady())
        {
            return false;
        }
        const auto positions = GetBytes(geometry.m_positions, *positionView);
        const auto indices = GetBytes(geometry.m_indices, indexView);
        const uint32_t positionStride = positionView->GetBufferViewDescriptor().m_elementSize;
        const uint32_t indexStride = indexView.GetBufferViewDescriptor().m_elementSize;
        if ((indexStride != 2 && indexStride != 4) || indices.size() / indexStride < mesh.GetIndexCount())
        {
            AZ_WarningOnce("DiffuseProbeGrid", false, "Software GI cannot read a mesh's triangle index buffer");
            return false;
        }
        const uint32_t triangleCount = mesh.GetIndexCount() / 3;
        AZStd::vector<Aabb> bounds(triangleCount, Aabb::CreateNull());
        // Atom's standard POSITION stream is float3. Other formats retain a valid balanced topology and are bounded
        // using the actual vertex format on the GPU before tracing; no quantized data is reinterpreted as float3.
        const bool floatPositions = positionStride == sizeof(float) * 3 && positions.size() / positionStride >= mesh.GetVertexCount();
        for (uint32_t triangle = 0; triangle < triangleCount; ++triangle)
        {
            if (!floatPositions)
            {
                bounds[triangle] = Aabb::CreateFromPoint(Vector3(static_cast<float>(triangle), 0.0f, 0.0f));
                continue;
            }
            for (uint32_t corner = 0; corner < 3; ++corner)
            {
                const uint32_t offset = (triangle * 3 + corner) * indexStride;
                uint32_t vertex = 0;
                if (indexStride == 2)
                {
                    uint16_t index;
                    memcpy(&index, indices.data() + offset, sizeof(index));
                    vertex = index;
                }
                else
                {
                    memcpy(&vertex, indices.data() + offset, sizeof(vertex));
                }
                if (vertex >= mesh.GetVertexCount())
                {
                    AZ_WarningOnce("DiffuseProbeGrid", false, "Software GI found an out-of-range mesh index");
                    return false;
                }
                float position[3];
                memcpy(position, positions.data() + size_t(vertex) * positionStride, sizeof(position));
                const Vector3 point = Vector3::CreateFromFloat3(position);
                if (!point.IsFinite())
                {
                    return false;
                }
                bounds[triangle].AddPoint(point);
            }
        }
        geometry.m_bvh.Build(bounds);
        geometry.m_ready = !geometry.m_bvh.m_nodes.empty();
        // Spatial partitioning needs these bytes only once. Current vertices are subsequently read from MeshInfo.
        geometry.m_positions.Reset();
        geometry.m_indices.Reset();
        return geometry.m_ready;
    }

    DiffuseProbeGridSoftwareScene::GpuData DiffuseProbeGridSoftwareScene::BuildGpuData() const
    {
        GpuData data;
        struct SharedGeometry
        {
            const Geometry* m_geometry;
            uint32_t m_topologyBase;
            uint32_t m_boundsBase;
            uint32_t m_primitiveBase;
            bool m_hasStaticRefit = false;
        };
        AZStd::vector<SharedGeometry> shared;
        auto append = [&](const DiffuseProbeGridSoftwareBvh& bvh)
        {
            const uint32_t topologyBase = aznumeric_cast<uint32_t>(data.m_nodes.size());
            const uint32_t boundsBase = aznumeric_cast<uint32_t>(data.m_bounds.size());
            const uint32_t primitiveBase = aznumeric_cast<uint32_t>(data.m_primitiveIndices.size());
            data.m_nodes.insert(data.m_nodes.end(), bvh.m_nodes.begin(), bvh.m_nodes.end());
            data.m_bounds.insert(data.m_bounds.end(), bvh.m_bounds.begin(), bvh.m_bounds.end());
            data.m_primitiveIndices.insert(data.m_primitiveIndices.end(), bvh.m_primitiveIndices.begin(), bvh.m_primitiveIndices.end());
            return GpuInstance{topologyBase, boundsBase, primitiveBase, 0};
        };
        auto addRefit = [&](const DiffuseProbeGridSoftwareBvh& bvh, const GpuInstance& instance,
            uint32_t firstLevel, uint32_t leafKind, bool dynamic)
        {
            const size_t levelCount = firstLevel + bvh.m_refitLevels.size() + 1;
            data.m_initialRefit.resize(AZStd::max(data.m_initialRefit.size(), levelCount));
            data.m_dynamicRefit.resize(AZStd::max(data.m_dynamicRefit.size(), levelCount));
            auto add = [&](uint32_t level, uint32_t node, uint32_t kind)
            {
                RefitWork work{node, instance.m_topologyBase, instance.m_boundsBase, instance.m_primitiveBase,
                    instance.m_meshInfoIndex, kind};
                data.m_initialRefit[level].push_back(work);
                if (dynamic)
                {
                    data.m_dynamicRefit[level].push_back(work);
                }
            };
            for (uint32_t node = 0; node < bvh.m_nodes.size(); ++node)
            {
                if (bvh.m_nodes[node].m_count)
                {
                    add(firstLevel, node, leafKind);
                }
            }
            for (uint32_t level = 0; level < bvh.m_refitLevels.size(); ++level)
            {
                for (const uint32_t node : bvh.m_refitLevels[level])
                {
                    add(firstLevel + level + 1, node, 1);
                }
            }
        };
        AZStd::vector<Aabb> instanceBounds;
        for (const auto& instance : m_instances)
        {
            auto found = AZStd::find_if(shared.begin(), shared.end(), [&](const auto& entry)
            {
                return entry.m_geometry == instance.m_geometry.get();
            });
            if (found == shared.end())
            {
                const auto packed = append(instance.m_geometry->m_bvh);
                shared.push_back({instance.m_geometry.get(), packed.m_topologyBase, packed.m_boundsBase, packed.m_primitiveBase});
                found = shared.end() - 1;
            }
            GpuInstance packed{found->m_topologyBase, found->m_boundsBase, found->m_primitiveBase,
                aznumeric_cast<uint32_t>(instance.m_meshInfoHandle.GetIndex())};
            if (instance.m_skinned)
            {
                // Static instances keep the shared bounds; skinned instances share topology, not deformed bounds.
                packed.m_boundsBase = aznumeric_cast<uint32_t>(data.m_bounds.size());
                const auto& bounds = instance.m_geometry->m_bvh.m_bounds;
                data.m_bounds.insert(data.m_bounds.end(), bounds.begin(), bounds.end());
                addRefit(instance.m_geometry->m_bvh, packed, 0, 0, true);
            }
            else if (!found->m_hasStaticRefit)
            {
                addRefit(instance.m_geometry->m_bvh, packed, 0, 0, false);
                found->m_hasStaticRefit = true;
            }
            data.m_instances.push_back(packed);
            instanceBounds.push_back(instance.m_worldBounds);
        }
        if (!instanceBounds.empty())
        {
            DiffuseProbeGridSoftwareBvh tlas;
            tlas.Build(instanceBounds);
            const auto packed = append(tlas);
            data.m_tlasTopologyBase = packed.m_topologyBase;
            data.m_tlasBoundsBase = packed.m_boundsBase;
            data.m_tlasPrimitiveBase = packed.m_primitiveBase;
            // TLAS leaves must run after the last animated BLAS refit, then TLAS parents run bottom-up.
            addRefit(tlas, packed, aznumeric_cast<uint32_t>(data.m_initialRefit.size()), 2, true);
        }
        return data;
    }

    void DiffuseProbeGridSoftwareScene::Update(
        MeshFeatureProcessorInterface& meshes, TransformServiceFeatureProcessorInterface& transforms)
    {
        m_instances.clear();
        for (auto& geometry : m_geometries)
        {
            geometry->m_used = false;
        }
        meshes.EnumerateRayTracingMeshes([&](ModelDataInstanceInterface& model)
        {
            const auto& renderedModel = model.GetModel();
            if (!renderedModel || renderedModel->GetLodCount() == 0)
            {
                return;
            }
            // Use the product's immutable topology even when the rendered model owns cloned skinned vertex buffers.
            auto asset = model.GetProductModelAsset();
            if (!asset.IsReady())
            {
                asset = renderedModel->GetModelAsset();
            }
            if (!asset.IsReady() || asset->GetLodCount() == 0)
            {
                return;
            }
            const uint32_t lod = aznumeric_cast<uint32_t>(AZStd::min(asset->GetLodCount(), renderedModel->GetLodCount()) - 1);
            const auto meshAssets = asset->GetLodAssets()[lod]->GetMeshes();
            for (uint32_t meshIndex = 0; meshIndex < meshAssets.size(); ++meshIndex)
            {
                const auto handle = model.GetMeshInfoHandle(lod, meshIndex);
                if (!handle.IsValid())
                {
                    continue;
                }
                auto info = meshes.GetMeshInfoEntry(handle);
                if (!info)
                {
                    continue;
                }
                auto found = AZStd::find_if(m_geometries.begin(), m_geometries.end(), [&](const auto& geometry)
                {
                    return geometry->m_modelAsset.Get() == asset.Get() && geometry->m_lod == lod && geometry->m_mesh == meshIndex;
                });
                AZStd::shared_ptr<Geometry> geometry;
                if (found == m_geometries.end())
                {
                    geometry = AZStd::make_shared<Geometry>();
                    geometry->m_modelAsset = asset;
                    geometry->m_lod = lod;
                    geometry->m_mesh = meshIndex;
                    m_geometries.push_back(geometry);
                }
                else
                {
                    geometry = *found;
                }
                geometry->m_used = true;
                if (!geometry->m_ready && !BuildGeometry(*geometry))
                {
                    continue;
                }
                const auto objectId = model.GetObjectId();
                Matrix3x4 transform = Matrix3x4::CreateFromTransform(transforms.GetTransformForId(objectId));
                transform.MultiplyByScale(transforms.GetNonUniformScaleForId(objectId));
                const Aabb worldBounds = meshAssets[meshIndex].GetAabb().GetTransformedAabb(transform);
                if (worldBounds.IsValid() && worldBounds.GetMin().IsFinite() && worldBounds.GetMax().IsFinite())
                {
                    m_instances.push_back({geometry, info, handle, worldBounds, model.IsSkinnedMesh(), transform});
                }
            }
        });
        m_geometries.erase(AZStd::remove_if(m_geometries.begin(), m_geometries.end(), [](const auto& geometry)
        {
            return !geometry->m_used;
        }), m_geometries.end());
    }
}
