/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Material/PersistentIndexAllocator.h>
#include <Atom/RPI.Public/Pass/GpuDriven/IndirectRasterPass.h>
#include <Atom/RPI.Public/Scene.h>
#include <Mesh/GpuDriven/GpuBatchTable.h>
#include <Mesh/GpuDriven/GpuMaterialTypeBucketing.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>

namespace AZ::Render
{
    using GpuInstanceHandle = RHI::Handle<int32_t>;

    // CPU-side mirror of the GpuInstanceData AZSL struct.
    // Must match layout and size of the GPU struct exactly.
    struct GpuInstanceDataCpu
    {
        uint32_t m_objectId = 0;
        uint32_t m_meshInfoIndex = 0;
        uint32_t m_drawListMask = 0;
        uint32_t m_flags = 0;

        float m_boundingSphereCenter[3] = { 0.f, 0.f, 0.f };
        float m_boundingSphereRadius = 0.f;

        uint32_t m_indexCount = 0;
        uint32_t m_indexOffset = 0;
        int32_t  m_vertexOffset = 0;
        uint32_t m_instanceCount = 1;

        float    m_lodScreenCoverageMin = 0.f;
        float    m_lodScreenCoverageMax = 1.f;
        uint32_t m_lodIndex = 0;
        uint32_t m_lodCount = 1;

        uint32_t m_sortKeyLow = 0;
        uint32_t m_sortKeyHigh = 0;
        float    m_lodCrossFadeAlpha = 0.f;  // 0=old LOD, 1=new LOD, used during LOD transitions
        uint32_t m_batchId = 0;              // dense batch index assigned by GpuBatchTable
    };

    static_assert(sizeof(GpuInstanceDataCpu) % 16 == 0, "GpuInstanceDataCpu must be 16-byte aligned");
    static_assert(sizeof(GpuInstanceDataCpu) == 80, "GpuInstanceDataCpu must be 80 bytes (5 x float4)");

    enum GpuInstanceFlags : uint32_t
    {
        GpuInstanceFlag_None        = 0,
        GpuInstanceFlag_Dynamic     = 1,
        GpuInstanceFlag_Skinned     = 2,
        GpuInstanceFlag_CastsShadow = 4,
        GpuInstanceFlag_Released    = 8,
        GpuInstanceFlag_PrevVisible = 16,  // Was visible in the previous frame (temporal occlusion)
    };

    // Mirrors GpuDrivenConstants.azsli -- batchId is now a per-draw root constant (no vertex cap).
    static constexpr uint32_t GpuDrivenMaxBatches = 65536;

    class GpuInstanceBufferManager
    {
    public:
        GpuInstanceBufferManager();

        void Activate(RPI::Scene* scene);
        void Deactivate();

        GpuInstanceHandle AcquireSlot();
        void ReleaseSlot(GpuInstanceHandle handle);

        void UpdateSlot(GpuInstanceHandle handle, const GpuInstanceDataCpu& data);

        void UploadBuffer();

        uint32_t GetInstanceCount() const;
        const Data::Instance<RPI::Buffer>& GetBuffer() const;

        // Batch table: assign/free a dense batchId for an instance's MeshInstanceGroupKey.
        uint32_t AcquireBatch(const MeshInstanceGroupKey& key, uint32_t indexCount) { return m_batchTable.Acquire(key, indexCount); }
        void ReleaseBatch(const MeshInstanceGroupKey& key) { m_batchTable.Release(key); }
        bool TryGetBatchId(const MeshInstanceGroupKey& key, uint32_t& outId) const { return m_batchTable.TryGetBatchId(key, outId); }
        uint32_t GetBatchCount() const { return m_batchTable.GetBatchCount(); }
        const Data::Instance<RPI::Buffer>& GetBatchInfoBuffer() const { return m_batchInfoBuffer.GetCurrentBuffer(); }

        // Live batches grouped by material type (see GpuMaterialTypeBucketing.h). Recomputed
        // whenever the batch table changes.
        const GpuMaterialTypeBuckets& GetMaterialTypeBuckets() const { return m_materialTypeBuckets; }

        // GetMaterialTypeBuckets() converted into the contiguous element ranges
        // IndirectRasterPass::SetIndirectBuckets expects. A type whose batchIds aren't
        // contiguous is skipped (with a warning) rather than expressed incorrectly -- see the
        // ponytail note on IndirectRasterPass::IndirectDrawBucket for why and the upgrade path.
        AZStd::vector<RPI::IndirectRasterPass::IndirectDrawBucket> GetIndirectDrawBuckets() const;

        // Oversized-submesh tracking (submeshes exceeding GpuDrivenMaxVerticesPerDraw).
        struct OversizedEntry
        {
            AZStd::string meshName;
            uint32_t      indexCount = 0;
        };

        void     RecordOversizedSubmesh(const char* meshName, uint32_t indexCount);
        void     ResetFrameOversizedCount();
        // These accessors are safe to call from the main/render thread only.
        // They do NOT acquire m_mutex -- call from the same thread as UploadBuffer.
        uint32_t GetOversizedFrameCount() const { return m_oversizedFrameCount; }
        uint32_t GetOversizedTotalCount() const { return m_oversizedTotalCount; }
        const AZStd::fixed_vector<OversizedEntry, 16>& GetOversizedLog() const { return m_oversizedLog; }

        bool IsEnabled() const { return m_isEnabled; }
        void SetEnabled(bool enabled) { m_isEnabled = enabled; }

        // Read-only accessor for the CPU-side instance data.
        // Safe to call from the same thread as UploadBuffer (main render thread).
        const AZStd::vector<GpuInstanceDataCpu>& GetCpuData() const { return m_cpuData; }

    private:
        void EnsureBufferExists();

        uint32_t m_oversizedFrameCount = 0;
        uint32_t m_oversizedTotalCount = 0;
        AZStd::fixed_vector<OversizedEntry, 16> m_oversizedLog;
        uint32_t m_oversizedLogHead = 0;

        bool m_isEnabled = false;

        RPI::SceneId m_sceneId;

        RPI::PersistentIndexAllocator<int32_t> m_indices;
        AZStd::mutex m_mutex;

        AZStd::vector<GpuInstanceDataCpu> m_cpuData;
        RPI::RingBuffer m_gpuBuffer;

        GpuBatchTable m_batchTable;
        RPI::RingBuffer m_batchInfoBuffer;
        GpuMaterialTypeBuckets m_materialTypeBuckets;

        bool m_needsUpload = false;

        RHI::ShaderInputNameIndex m_gpuInstanceDataIndex = "m_gpuInstanceData";
        RHI::ShaderInputNameIndex m_gpuInstanceCountIndex = "m_gpuInstanceCount";
        RHI::ShaderInputNameIndex m_batchInfoIndex = "m_batchInfo";
        RHI::ShaderInputNameIndex m_batchCountIndex = "m_gpuBatchCount";
        RPI::Scene::PrepareSceneSrgEvent::Handler m_updateSceneSrgHandler;
    };

} // namespace AZ::Render
