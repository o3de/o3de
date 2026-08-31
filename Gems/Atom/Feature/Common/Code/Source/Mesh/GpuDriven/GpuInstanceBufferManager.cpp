/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/GpuDriven/GpuInstanceBufferManager.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ::Render
{
    AZ_CVAR(
        bool,
        r_gpuDrivenRendering,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Enable GPU-driven rendering pipeline (frustum culling and indirect draw submission on GPU).");

    AZ_CVAR(
        bool,
        r_gpuCullingDebugView,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Show GPU culling debug overlay (each visible instance drawn with a unique color).");

    AZ_CVAR(
        bool,
        r_gpuDrivenStats,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Print GPU-driven batching stats (instances, batches, coalescing ratio) to the console ~once per second.");

    // Phase 7: two-pass occlusion culling (pass 1 redraws what was visible last frame from a
    // persistent bitfield, pass 2 occlusion-tests everything else against pass 1's own depth).
    // Read directly by RPI::GpuCullPass (a lower-level module than this one) via IConsole, same as
    // every other GPU-driven pass already reads r_gpuDrivenRendering -- see GpuCullPass::CompileResources.
    // Off by default: the two-pass subtree is wired (GpuTwoPassOcclusionCull.pass) but not yet
    // reachable from MainPipeline, and even if instantiated its cull kernels dispatch zero threads
    // while this is false, so nothing it produces changes a rendered frame until this flips.
    AZ_CVAR(
        bool,
        r_gpuTwoPassOcclusionCulling,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Enable the Phase 7 two-pass GPU occlusion culling subtree (GpuTwoPassOcclusionCullTemplate). "
        "Unverified against a rendered frame -- off by default.");

    GpuInstanceBufferManager::GpuInstanceBufferManager()
        : m_gpuBuffer{ "GpuInstanceData", RPI::CommonBufferPoolType::ReadOnly, static_cast<uint32_t>(sizeof(GpuInstanceDataCpu)) }
        , m_batchInfoBuffer{ "GpuBatchInfo", RPI::CommonBufferPoolType::ReadOnly, static_cast<uint32_t>(sizeof(uint32_t)) }
    {
    }

    void GpuInstanceBufferManager::Activate(RPI::Scene* scene)
    {
        // m_isEnabled deliberately stays false here. MeshFeatureProcessor::CheckForInstancingCVarChange
        // is the single authority: it pushes the r_gpuDrivenRendering value in via SetEnabled every
        // frame-flip, AND'd with the scene actually containing a GpuDrivenForwardPass -- a condition
        // that cannot be evaluated at activation because pipelines are added to the scene later.
        // Reading the CVar here (the old behavior) let scenes with no GPU-driven pass -- material and
        // thumbnail previews -- claim submeshes and suppress their CPU forward draws, losing geometry.

        m_sceneId = scene->GetId();

        // Always create a valid buffer so the SceneSrg StructuredBuffer descriptor is never null.
        // An unbound SRG resource causes device-removed on DX12.
        EnsureBufferExists();

        m_updateSceneSrgHandler = RPI::Scene::PrepareSceneSrgEvent::Handler(
            [this](RPI::ShaderResourceGroup* sceneSrg)
            {
                if (const auto& buffer = GetBuffer(); buffer != nullptr)
                {
                    sceneSrg->SetBufferView(m_gpuInstanceDataIndex, buffer->GetBufferView());
                }
                sceneSrg->SetConstant(m_gpuInstanceCountIndex, GetInstanceCount());

                if (const auto& batchBuffer = GetBatchInfoBuffer(); batchBuffer != nullptr)
                {
                    sceneSrg->SetBufferView(m_batchInfoIndex, batchBuffer->GetBufferView());
                }
                sceneSrg->SetConstant(m_batchCountIndex, m_batchTable.GetBatchCount());
            });
        scene->ConnectEvent(m_updateSceneSrgHandler);
    }

    void GpuInstanceBufferManager::Deactivate()
    {
        m_updateSceneSrgHandler.Disconnect();
        m_cpuData.clear();
        m_isEnabled = false;
    }

    GpuInstanceHandle GpuInstanceBufferManager::AcquireSlot()
    {
        if (!m_isEnabled)
        {
            return GpuInstanceHandle{ GpuInstanceHandle::NullIndex };
        }

        GpuInstanceHandle handle{ GpuInstanceHandle::NullIndex };
        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            constexpr size_t MinEntries = 32;
            auto index = m_indices.Aquire();
            const auto numEntries = AlignUpToPowerOfTwo(
                AZStd::max(MinEntries, static_cast<size_t>(m_indices.MaxCount())));

            if (m_cpuData.size() < m_indices.MaxCount())
            {
                m_cpuData.resize(numEntries, GpuInstanceDataCpu{});
            }
            handle = GpuInstanceHandle{ index };
        }

        m_needsUpload = true;
        return handle;
    }

    void GpuInstanceBufferManager::ReleaseSlot(GpuInstanceHandle handle)
    {
        if (!m_isEnabled)
        {
            return;
        }

        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            m_indices.Release(handle.GetIndex());
            if (m_cpuData.size() > handle.GetIndex())
            {
                m_cpuData[handle.GetIndex()] = GpuInstanceDataCpu{};
                m_cpuData[handle.GetIndex()].m_flags = GpuInstanceFlag_Released;
            }
        }
        m_needsUpload = true;
    }

    void GpuInstanceBufferManager::UpdateSlot(GpuInstanceHandle handle, const GpuInstanceDataCpu& data)
    {
        if (!m_isEnabled)
        {
            return;
        }

        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            AZ_Assert(
                m_cpuData.size() > handle.GetIndex(),
                "GpuInstanceBufferManager::UpdateSlot called with invalid handle %d",
                handle.GetIndex());
            if (m_cpuData.size() > handle.GetIndex())
            {
                m_cpuData[handle.GetIndex()] = data;
            }
        }
        m_needsUpload = true;
    }

    void GpuInstanceBufferManager::EnsureBufferExists()
    {
        if (GetBuffer() != nullptr)
        {
            return;
        }

        GpuInstanceDataCpu dummy{};
        dummy.m_flags = GpuInstanceFlag_Released;
        const auto deviceCount = AZ::RHI::RHISystemInterface::Get()->GetDeviceCount();

        AZStd::unordered_map<int, const void*> updateDataHelper;
        for (auto deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
        {
            updateDataHelper[deviceIndex] = &dummy;
        }
        m_gpuBuffer.AdvanceCurrentBufferAndUpdateData(updateDataHelper, sizeof(GpuInstanceDataCpu));

        // Seed a 1-element batch-info buffer so the SceneSrg StructuredBuffer is never null.
        const uint32_t zero = 0;
        AZStd::unordered_map<int, const void*> batchHelper;
        for (auto deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
        {
            batchHelper[deviceIndex] = &zero;
        }
        m_batchInfoBuffer.AdvanceCurrentBufferAndUpdateData(batchHelper, sizeof(uint32_t));
    }

    void GpuInstanceBufferManager::UploadBuffer()
    {
        ResetFrameOversizedCount();

        if ((!m_needsUpload && !m_batchTable.NeedsUpload()) || !m_isEnabled || m_cpuData.empty())
        {
            m_needsUpload = false;
            return;
        }

        const auto numEntries = m_cpuData.size();
        const auto deviceCount = AZ::RHI::RHISystemInterface::Get()->GetDeviceCount();

        AZStd::unordered_map<int, const void*> updateDataHelper;
        for (auto deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
        {
            updateDataHelper[deviceIndex] = m_cpuData.data();
        }

        m_gpuBuffer.AdvanceCurrentBufferAndUpdateData(updateDataHelper, numEntries * sizeof(GpuInstanceDataCpu));

        if (m_batchTable.NeedsUpload())
        {
            const auto& indexCounts = m_batchTable.GetIndexCounts();
            if (!indexCounts.empty())
            {
                AZStd::unordered_map<int, const void*> batchHelper;
                for (auto deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
                {
                    batchHelper[deviceIndex] = indexCounts.data();
                }
                m_batchInfoBuffer.AdvanceCurrentBufferAndUpdateData(
                    batchHelper, indexCounts.size() * sizeof(uint32_t));
            }
            m_batchTable.MarkUploaded();

            // ponytail: GpuDrivenEligibility (Phase 1) is the only gate feeding this table and it
            // admits exactly one material type (StandardPBR), so a uniform id is accurate today,
            // not a stub -- this yields exactly one bucket spanning the whole table. Upgrade path:
            // once eligibility admits a second material type, thread the real per-batch id through
            // GpuBatchTable::Acquire (add an optional parameter there) instead of this constant.
            const AZStd::vector<uint32_t> batchMaterialType(m_batchTable.GetBatchCount(), 0);
            m_materialTypeBuckets = BucketBatchesByMaterialType(batchMaterialType);
        }

        m_needsUpload = false;
    }

    AZStd::vector<RPI::IndirectRasterPass::IndirectDrawBucket> GpuInstanceBufferManager::GetIndirectDrawBuckets() const
    {
        AZStd::vector<RPI::IndirectRasterPass::IndirectDrawBucket> buckets;
        buckets.reserve(m_materialTypeBuckets.m_typeCount);

        for (uint32_t slot = 0; slot < m_materialTypeBuckets.m_typeCount; ++slot)
        {
            const auto& batchIds = m_materialTypeBuckets.GetBatchIds(slot);
            if (batchIds.empty())
            {
                continue;
            }

            // batchIds are ascending within a slot (BucketBatchesByMaterialType appends in
            // batchId order) but not necessarily contiguous. The GPU-side buffer
            // (GpuBatchFinalize.azsl) is laid out strictly by batchId, not grouped by type, so
            // only a contiguous span can be expressed as a single ExecuteIndirect range.
            const uint32_t first = batchIds.front();
            const uint32_t span = batchIds.back() - first + 1;
            if (span != batchIds.size())
            {
                AZ_Warning("GpuInstanceBufferManager", false,
                    "Material type %u's %zu batches are not contiguous (span %u) -- skipping its "
                    "indirect draw bucket until the GPU buffer is grouped by type.",
                    m_materialTypeBuckets.m_materialTypeIds[slot], batchIds.size(), span);
                continue;
            }

            buckets.push_back({ m_materialTypeBuckets.m_materialTypeIds[slot], first, span });
        }

        return buckets;
    }

    uint32_t GpuInstanceBufferManager::GetInstanceCount() const
    {
        return static_cast<uint32_t>(m_indices.MaxCount());
    }

    const Data::Instance<RPI::Buffer>& GpuInstanceBufferManager::GetBuffer() const
    {
        return m_gpuBuffer.GetCurrentBuffer();
    }

    void GpuInstanceBufferManager::RecordOversizedSubmesh(const char* meshName, uint32_t indexCount)
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        ++m_oversizedFrameCount;
        ++m_oversizedTotalCount;
        OversizedEntry entry{ meshName, indexCount };
        if (m_oversizedLog.size() < m_oversizedLog.capacity())
        {
            m_oversizedLog.push_back(AZStd::move(entry));
        }
        else
        {
            m_oversizedLog[m_oversizedLogHead] = AZStd::move(entry);
            m_oversizedLogHead = (m_oversizedLogHead + 1) % static_cast<uint32_t>(m_oversizedLog.capacity());
        }
    }

    void GpuInstanceBufferManager::ResetFrameOversizedCount()
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        m_oversizedFrameCount = 0;
    }

} // namespace AZ::Render
