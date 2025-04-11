/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/IAllocator.h>

namespace AZ
{
    namespace Debug
    {
        struct AllocationInfo;
    }

    /**
    * AllocatorBase - all AZ-compatible allocators should inherit from this implementation of IAllocator.
    */
    class AllocatorBase : public IAllocator
    {
    protected:
        AllocatorBase();
        explicit AllocatorBase(bool enableProfiling);
        ~AllocatorBase() override;

    public:
        AZ_RTTI(AllocatorBase, "{E89B953E-FAB2-4BD0-A754-74AD5F8902F5}", IAllocator)

        //---------------------------------------------------------------------
        // IAllocator implementation
        //---------------------------------------------------------------------
        using IAllocator::GetRecords;
        const Debug::AllocationRecords* GetRecords() const override;
        void SetRecords(Debug::AllocationRecords* records) final;
        bool IsReady() const final;
        void PostCreate() override;
        void PreDestroy() final;
        void SetProfilingActive(bool active) final;
        bool IsProfilingActive() const final;
        //---------------------------------------------------------------------

    protected:
        /// Returns the size of a memory allocation after adjusting for tracking.
        AZ_FORCE_INLINE size_t MemorySizeAdjustedUp(size_t byteSize) const
        {
            if (m_records && byteSize > 0)
            {
                byteSize += m_memoryGuardSize;
            }

            return byteSize;
        }

        /// Returns the size of a memory allocation, removing any tracking overhead
        AZ_FORCE_INLINE size_t MemorySizeAdjustedDown(size_t byteSize) const
        {
            if (m_records && byteSize > 0)
            {
                byteSize -= m_memoryGuardSize;
            }

            return byteSize;
        }

        /// Call to disallow this allocator from being registered with the AllocatorManager.
        /// Only kernel-level allocators where it would be especially problematic for them to be registered with the AllocatorManager should do this.
        void DisableRegistration();

        /// Records an allocation for profiling.
        void ProfileAllocation(void* ptr, size_t byteSize, size_t alignment, int suppressStackRecord);

        /// Records a deallocation for profiling.
        void ProfileDeallocation(void* ptr, size_t byteSize, size_t alignment, Debug::AllocationInfo* info);

        /// Records a reallocation for profiling.
        void ProfileReallocation(void* ptr, void* newPtr, size_t newSize, size_t newAlignment);

        /// Records a resize for profiling.
        void ProfileResize(void* ptr, size_t newSize);

        /// User allocator should call this function when they run out of memory!
        bool OnOutOfMemory(size_t byteSize, size_t alignment);

    private:
        Debug::AllocationRecords* m_records = nullptr;  // Cached pointer to allocation records
        size_t m_memoryGuardSize = 0;
        bool m_isProfilingActive = false;
        bool m_isReady = false;
        bool m_registrationEnabled = true;
    };
}
