/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>

namespace AZ::RHI
{
    //! Global allocation manager for Atom. It has access to all
    //! registered Atom allocators. Controls profiling of those allocators and
    //! access to allocation data.
    class AllocatorManager
    {
    public:

        AllocatorManager();
        ~AllocatorManager();

        static AllocatorManager& Instance();
        //! Returns true if the global instance is constructed
        static bool IsReady();

        AZ_FORCE_INLINE  int            GetNumAllocators() const    { return m_numAllocators; }
        AZ_FORCE_INLINE  IAllocator*    GetAllocator(int index)
        {
            AZ_Assert(index < m_numAllocators, "Invalid allocator index %d [0,%d]!", index, m_numAllocators - 1);
            return m_allocators[index];
        }
        //! Set memory track mode for all allocators already created.
        void SetTrackingMode(AZ::Debug::AllocationRecords::Mode mode);

        //! Reset the peak bytes for all allocators
        void ResetPeakBytes();

        //! Enter or exit profiling mode
        void SetProfilingMode(bool value);
        bool GetProfilingMode() const;

        //! Allocation information about the allocators
        struct AllocatorStats
        {
            AllocatorStats() = default;
            AllocatorStats(const char* name, size_t requestedBytes, size_t requestedAllocs, size_t requestedBytesPeak)
                : m_name(name)
                , m_requestedBytes(requestedBytes)
                , m_requestedAllocs(requestedAllocs)
                , m_requestedBytesPeak(requestedBytesPeak)
            {}

            AZStd::string m_name;
            //! Requested user bytes. IMPORTANT: This is user requested memory! Any allocator overhead is NOT included.
            size_t m_requestedBytes = 0;
            //! Total number of requested allocations
            size_t m_requestedAllocs = 0;
            //! Peak of requested memory. IMPORTANT: This is user requested memory! Any allocator overhead is NOT included.
            size_t m_requestedBytesPeak = 0;
        };

        //! Returns the AllocatorStats for each registered allocator
        //! @param requestedBytes Stores the total number of request bytes for all allocators
        //! @param requestedAllocs Stores the total number of allocations for all allocators
        //! @param requestedBytesPeak Stores the peak of request of requested bytes for all allocators
        void GetAllocatorStats(
            size_t& requestedBytes, size_t& requestedAllocs, size_t& requestedBytesPeak, AZStd::vector<AllocatorStats>* outStats = nullptr);

        // Called from IAllocator
        void RegisterAllocator(IAllocator* alloc);
        void UnRegisterAllocator(IAllocator* alloc);

    private:
        void InternalDestroy();

        AllocatorManager(const AllocatorManager&);

        static const int m_maxNumAllocators = 100;
        IAllocator*         m_allocators[m_maxNumAllocators];
        volatile int        m_numAllocators;
        AZStd::mutex        m_allocatorListMutex;
        bool                m_defaultProfileMode = false;
        Debug::AllocationRecords::Mode m_defaultTrackingMode = Debug::AllocationRecords::Mode::RECORD_STACK_NEVER;
    };
}
