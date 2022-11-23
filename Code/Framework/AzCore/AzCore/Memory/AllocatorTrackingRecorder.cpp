/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorTrackingRecorder.h>

#if defined(AZ_ENABLE_TRACING)

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/spin_mutex.h>

// Amount of stack trace entries to record (amount of functions to record)
#define STACK_TRACE_DEPTH_RECORDING 20

#endif

namespace AZ
{
#if defined(AZ_ENABLE_TRACING)
    struct IAllocatorTrackingRecorderData 
    {
        AZStd::atomic_size_t m_allocated; // Total amount of bytes allocated (i.e. requested to the OS, assuming 1-alignment)

        AZStd::spin_mutex m_allocationRecordsMutex;
        using AllocationRecords = AZStd::set<AllocationRecord, AZStd::less<AllocationRecord>, AZ::Debug::DebugAllocator>;
        AllocationRecords m_allocationRecords;
    };

    AllocationRecord AllocationRecord::Create(void* address, AZStd::size_t requested, AZStd::size_t allocated, AZStd::size_t alignment, const AZStd::size_t stackFramesToSkip)
    {
        AllocationRecord record(address, requested, allocated, alignment);
        record.m_allocationStackTrace.resize(STACK_TRACE_DEPTH_RECORDING); // make enough room for the recording
        unsigned int numOfFrames = Debug::StackRecorder::Record(record.m_allocationStackTrace.begin(), STACK_TRACE_DEPTH_RECORDING, static_cast<unsigned int>(stackFramesToSkip + 1));
        if (numOfFrames < STACK_TRACE_DEPTH_RECORDING)
        {
            record.m_allocationStackTrace.resize(numOfFrames);
            record.m_allocationStackTrace.shrink_to_fit();
        }
        return record;
    }

    void AllocationRecord::Print() const
    {
        AZ_Printf("Memory", "  Allocation Addr: 0%p, Requested: %zu, Allocated: %zu, Alignment: %zu\n", m_address, m_requested, m_allocated, m_alignment);

        // We wont have more entries than STACK_TRACE_DEPTH_RECORDING
        Debug::SymbolStorage::StackLine lines[STACK_TRACE_DEPTH_RECORDING];
        const size_t stackSize = m_allocationStackTrace.size();
        Debug::SymbolStorage::DecodeFrames(m_allocationStackTrace.begin(), static_cast<unsigned int>(stackSize), lines);
        
        for (AZStd::size_t i = 0; i < stackSize; ++i)
        {
            AZ_Printf("Memory", "    %s\n", lines[i]);
        }
    }

    void IAllocatorWithTracking::AddAllocated(AZStd::size_t allocated)
    {
        m_data->m_allocated += allocated;
    }

    void IAllocatorWithTracking::RemoveAllocated(AZStd::size_t allocated)
    {
        m_data->m_allocated -= allocated;
    }

    void IAllocatorWithTracking::AddAllocationRecord(void* address, AZStd::size_t requested, AZStd::size_t allocated, AZStd::size_t alignment, AZStd::size_t stackFramesToSkip)
    {
        AllocationRecord record = AllocationRecord::Create(address, requested, allocated, alignment, stackFramesToSkip + 1);
        AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
        auto it = m_data->m_allocationRecords.emplace(AZStd::move(record));
        if (!it.second)
        {
            AZ_Assert(false, "Allocation with address 0%p already exists");
            AZ_Printf("Memory", "Trying to replace this record:");
            it.first->Print();
            AZ_Printf("Memory", "with this one:");
            record.Print();
        }
    }

    void IAllocatorWithTracking::RemoveAllocationRecord(void* address, [[maybe_unused]] AZStd::size_t requested, [[maybe_unused]] AZStd::size_t allocated)
    {
        AllocationRecord record(address, 0, 0, 0); // those other values do not matter because the set just compares the address
        AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
        auto it = m_data->m_allocationRecords.find(record);
        AZ_Assert(it != m_data->m_allocationRecords.end(), "Allocation with address 0%p was not found");
        AZ_Assert(requested ? it->m_requested == requested : true, "Mismatch on requested size");
        AZ_Assert(allocated ? it->m_allocated == allocated : true, "Mismatch on allocated size");
        m_data->m_allocationRecords.erase(it);
    }

#endif // defined(AZ_ENABLE_TRACING)

    IAllocatorWithTracking::IAllocatorWithTracking()
    {
#if defined(AZ_ENABLE_TRACING)
        m_data = new (AZ::Debug::DebugAllocator().allocate(sizeof(IAllocatorTrackingRecorderData), alignof(IAllocatorTrackingRecorderData))) IAllocatorTrackingRecorderData();
#endif
    }

    IAllocatorWithTracking::~IAllocatorWithTracking()
    {
#if defined(AZ_ENABLE_TRACING)
        m_data->~IAllocatorTrackingRecorderData();
        AZ::Debug::DebugAllocator().deallocate(m_data, sizeof(IAllocatorTrackingRecorderData), alignof(IAllocatorTrackingRecorderData));
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetRequested() const
    {
#if defined(AZ_ENABLE_TRACING)
        // Because the requested size is mostly lost on deallocations (byteSize is usually not passed to the deallocate function),
        // we rely on the allocation records to get that information
        AZStd::size_t requested = 0;
        {
            AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
            for (const AllocationRecord& record : m_data->m_allocationRecords)
            {
                requested += record.m_requested;
            }
        }
        return requested;
#else
        return 0;
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetAllocated() const
    {
#if defined(AZ_ENABLE_TRACING)
        return m_data->m_allocated;
#else
        return 0;
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetFragmented() const
    {
#if defined(AZ_ENABLE_TRACING)
        return m_data->m_allocated - GetRequested();
#else
        return 0;
#endif
    }

    void IAllocatorWithTracking::PrintAllocations() const
    {
#if defined(AZ_ENABLE_TRACING)
        // Create a copy of allocation records so other allocations can happen while we print these
        IAllocatorTrackingRecorderData::AllocationRecords allocations;
        {
            AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
            allocations = m_data->m_allocationRecords;
        }

        if (allocations.empty())
        {
            AZ_Printf("Memory", "There are no allocations in allocator 0%p (%s)\n", this, GetName());
        }
        else
        {
            AZ_Error("Memory", false, "There are %d allocations in allocator 0%p (%s):\n", allocations.size(), this, GetName());
            for (const AllocationRecord& record : allocations)
            {
                record.Print();
            }
        }
#else
        AZ_Printf("Memory", "Allocation recording is disabled, AZ_ENABLE_TRACING needs to be enabled.");
#endif
    }

    AZStd::size_t IAllocatorWithTracking::GetAllocationCount() const
    {
#if defined(AZ_ENABLE_TRACING)
        return m_data->m_allocationRecords.size();
#else
        return 0;
#endif
    }

#if defined(AZ_ENABLE_TRACING)
    AllocationRecordVector IAllocatorWithTracking::GetAllocationRecords() const
    {
        AZStd::vector<AllocationRecord, AZ::Debug::DebugAllocator> allocationRecords;
        allocationRecords.reserve(m_data->m_allocationRecords.size());
        {
            AZStd::scoped_lock lock(m_data->m_allocationRecordsMutex);
            AZStd::copy(m_data->m_allocationRecords.begin(), m_data->m_allocationRecords.end(), AZStd::back_inserter(allocationRecords));
        }
        return allocationRecords;
    }
#endif

} // namespace AZ

#if defined(AZ_ENABLE_TRACING)
#undef STACK_TRACE_DEPTH_RECORDING
#endif
