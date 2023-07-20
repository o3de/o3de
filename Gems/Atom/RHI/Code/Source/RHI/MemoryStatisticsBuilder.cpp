/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ::RHI
{
    MemoryStatisticsReportFlags MemoryStatisticsBuilder::GetReportFlags() const
    {
        return m_reportFlags;
    }

    void MemoryStatisticsBuilder::Begin(MemoryStatistics& memoryStatistics, MemoryStatisticsReportFlags reportFlags)
    {
        AZ_Assert(m_statistics == nullptr, "Memory statistics is not null. Did you forget to call End?");

        m_reportFlags = reportFlags;
        m_statistics = &memoryStatistics;
        m_statistics->m_pools.clear();
        m_statistics->m_heaps.clear();
    }

    MemoryStatistics::Heap* MemoryStatisticsBuilder::AddHeap()
    {
        m_statistics->m_heaps.emplace_back();
        return &m_statistics->m_heaps.back();
    }

    MemoryStatistics::Pool* MemoryStatisticsBuilder::BeginPool()
    {
        AZ_Assert(m_currentPool == nullptr, "Currently processing a pool! Did you forget to call EndPool?");

        m_statistics->m_pools.emplace_back();
        m_currentPool = &m_statistics->m_pools.back();
        return m_currentPool;
    }

    MemoryStatistics::Buffer* MemoryStatisticsBuilder::AddBuffer()
    {
        AZ_Assert(m_currentPool, "Pool is null. Make sure to call BeginPool before using this method.");
        m_currentPool->m_buffers.emplace_back();
        return &m_currentPool->m_buffers.back();
    }

    MemoryStatistics::Image* MemoryStatisticsBuilder::AddImage()
    {
        AZ_Assert(m_currentPool, "Pool is null. Make sure to call BeginPool before using this method.");
        m_currentPool->m_images.emplace_back();
        return &m_currentPool->m_images.back();
    }

    void MemoryStatisticsBuilder::EndPool()
    {
        AZ_Assert(m_currentPool, "Pool is null. Did you forget to call BeginPool?");
        m_currentPool = nullptr;
    }

    void MemoryStatisticsBuilder::End()
    {
        AZ_Assert(m_currentPool == nullptr, "Currently building pool: %s", m_currentPool->m_name.GetCStr());
        AZ_Assert(m_statistics, "MemoryStatistics is null. Did you forget to call Begin?");
        m_statistics = nullptr;
    }
}
