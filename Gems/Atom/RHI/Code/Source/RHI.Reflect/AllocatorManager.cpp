/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Allocators.h>
#include <Atom/RHI.Reflect/AllocatorManager.h>
#include <AzCore/Math/Crc.h>

namespace AZ::RHI
{
static EnvironmentVariable<AllocatorManager>& GetAllocatorManagerEnvVar()
{
    static EnvironmentVariable<AllocatorManager> s_allocManager;
    return s_allocManager;
}

static AllocatorManager* s_allocManagerDebug;  // For easier viewing in crash dumps

//////////////////////////////////////////////////////////////////////////
// The only allocator manager instance.
AllocatorManager& AllocatorManager::Instance()
{
    auto& allocatorManager = GetAllocatorManagerEnvVar();
    if (!allocatorManager)
    {
        allocatorManager = Environment::CreateVariable<AllocatorManager>(AZ_CRC_CE("AZ::RHI::AllocatorManager::s_allocManager"));

        s_allocManagerDebug = &(*allocatorManager);
    }

    return *allocatorManager;
}
//////////////////////////////////////////////////////////////////////////
bool AllocatorManager::IsReady()
{
    return GetAllocatorManagerEnvVar().IsConstructed();
}

AllocatorManager::AllocatorManager()
{
    m_numAllocators = 0;
}

AllocatorManager::~AllocatorManager()
{
    InternalDestroy();
}

void AllocatorManager::RegisterAllocator(class IAllocator* alloc)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    AZ_Assert(m_numAllocators < m_maxNumAllocators, "Too many allocators %d! Max is %d", m_numAllocators, m_maxNumAllocators);

    for (size_t i = 0; i < m_numAllocators; i++)
    {
        AZ_Assert(m_allocators[i] != alloc, "Allocator %s (%s) registered twice!", alloc->GetName());
    }

    m_allocators[m_numAllocators++] = alloc;
    alloc->SetProfilingActive(m_defaultProfileMode);
    if (auto* records = alloc->GetRecords())
    {
        records->SetMode(m_defaultTrackingMode);
    }
}

void AllocatorManager::InternalDestroy()
{
    while (m_numAllocators > 0)
    {
        IAllocator* allocator = m_allocators[m_numAllocators - 1];
        (void)allocator;
        m_allocators[--m_numAllocators] = nullptr;
        // Do not actually destroy the lazy allocator as it may have work to do during non-deterministic shutdown
    }
}

void AllocatorManager::UnRegisterAllocator(class IAllocator* alloc)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

    for (int i = 0; i < m_numAllocators; ++i)
    {
        if (m_allocators[i] == alloc)
        {
            --m_numAllocators;
            m_allocators[i] = m_allocators[m_numAllocators];
        }
    }
}

void AllocatorManager::SetTrackingMode(Debug::AllocationRecords::Mode mode)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        Debug::AllocationRecords* records = m_allocators[i]->GetRecords();
        if (records)
        {
            records->SetMode(mode);
        }
    }
    m_defaultTrackingMode = mode;
}

void AllocatorManager::ResetPeakBytes()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        Debug::AllocationRecords* records = m_allocators[i]->GetRecords();
        if (records)
        {
            records->ResetPeakBytes();
        }
    }
}

void AllocatorManager::SetProfilingMode(bool value)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        m_allocators[i]->SetProfilingActive(value);
    }
    m_defaultProfileMode = value;
}

bool AllocatorManager::GetProfilingMode() const
{
    return m_defaultProfileMode;
}

void AllocatorManager::GetAllocatorStats(
    size_t& requestedBytes, size_t& requestedAllocs, size_t& requestedBytesPeak, AZStd::vector<AllocatorStats>* outStats)
{
    requestedBytes = 0;
    requestedAllocs = 0;
    requestedBytesPeak = 0;

    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    const int allocatorCount = GetNumAllocators();

    for (int i = 0; i < allocatorCount; ++i)
    {
        IAllocator* allocator = GetAllocator(i);
        if (outStats)
        {
            AllocatorStats stats;
            stats.m_name = allocator->GetName();
            auto* records = allocator->GetRecords();
            if (records)
            {
                stats.m_requestedBytes = records->RequestedBytes();
                stats.m_requestedAllocs = records->RequestedAllocs();
                stats.m_requestedBytesPeak = records->RequestedBytesPeak();
            }
            requestedBytes += stats.m_requestedBytes;
            requestedAllocs += stats.m_requestedAllocs;
            requestedBytesPeak += stats.m_requestedBytesPeak;
            outStats->emplace_back(AZStd::move(stats));
        }
    }
}
} // namespace AZ::RHI
