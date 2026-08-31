/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Mesh/GpuDriven/GpuBatchTable.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ::Render
{
    uint32_t GpuBatchTable::Acquire(const MeshInstanceGroupKey& key, uint32_t indexCount)
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        Entry& entry = m_map[key];
        if (entry.m_refCount == 0)
        {
            entry.m_batchId = m_indices.Aquire(); // note: O3DE spelling of PersistentIndexAllocator::Aquire
            if (m_indexCount.size() <= static_cast<size_t>(entry.m_batchId))
            {
                m_indexCount.resize(entry.m_batchId + 1, 0u);
            }
            m_needsUpload = true;
        }
        ++entry.m_refCount;
        const uint32_t batchId = static_cast<uint32_t>(entry.m_batchId);
        if (m_indexCount[batchId] != indexCount)
        {
            m_indexCount[batchId] = indexCount;
            m_needsUpload = true;
        }
        return batchId;
    }

    void GpuBatchTable::Release(const MeshInstanceGroupKey& key)
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        auto it = m_map.find(key);
        if (it == m_map.end() || it->second.m_refCount == 0)
        {
            return;
        }
        if (--it->second.m_refCount == 0)
        {
            m_indices.Release(it->second.m_batchId);
            m_map.erase(it);
            m_needsUpload = true;
        }
    }

    uint32_t GpuBatchTable::GetBatchCount() const
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        return static_cast<uint32_t>(m_indices.MaxCount());
    }

    uint32_t GpuBatchTable::GetIndexCount(uint32_t batchId) const
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        return batchId < m_indexCount.size() ? m_indexCount[batchId] : 0u;
    }

    bool GpuBatchTable::TryGetBatchId(const MeshInstanceGroupKey& key, uint32_t& outBatchId) const
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        auto it = m_map.find(key);
        if (it == m_map.end() || it->second.m_refCount == 0)
        {
            return false;
        }
        outBatchId = static_cast<uint32_t>(it->second.m_batchId);
        return true;
    }
} // namespace AZ::Render
