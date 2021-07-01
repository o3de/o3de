/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/PlatformMemoryInstrumentation.h>

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
        AllocatorBase(IAllocatorAllocate* allocationSource, const char* name, const char* desc);
        ~AllocatorBase();

    public:

        //---------------------------------------------------------------------
        // IAllocator implementation
        //---------------------------------------------------------------------
        const char* GetName() const override;
        const char* GetDescription() const override;
        IAllocatorAllocate* GetSchema() override;
        Debug::AllocationRecords* GetRecords() final;
        void SetRecords(Debug::AllocationRecords* records) final;
        bool IsReady() const final;
        bool CanBeOverridden() const final;
        void PostCreate() override;
        void PreDestroy() final;
        void SetLazilyCreated(bool lazy) final;
        bool IsLazilyCreated() const final;
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

        /// Call to disallow this allocator from being overridden.
        /// Only kernel-level allocators where it would be especially problematic for them to be overridden should do this.
        void DisableOverriding();

        /// Call to disallow this allocator from being registered with the AllocatorManager.
        /// Only kernel-level allocators where it would be especially problematic for them to be registered with the AllocatorManager should do this.
        void DisableRegistration();

        /// Records an allocation for profiling.
        void ProfileAllocation(void* ptr, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, int suppressStackRecord);

        /// Records a deallocation for profiling.
        void ProfileDeallocation(void* ptr, size_t byteSize, size_t alignment, Debug::AllocationInfo* info);

        /// Records a reallocation for profiling.
        void ProfileReallocationBegin(void* ptr, size_t newSize);

        /// Records the beginning of a reallocation for profiling.
        void ProfileReallocationEnd(void* ptr, void* newPtr, size_t newSize, size_t newAlignment);

        /// Deprecated.
        /// @deprecated Please use ProfileReallocationBegin/ProfileReallocationEnd instead.
        void ProfileReallocation(void* ptr, void* newPtr, size_t newSize, size_t newAlignment);

        /// Records a resize for profiling.
        void ProfileResize(void* ptr, size_t newSize);

        /// User allocator should call this function when they run out of memory!
        bool OnOutOfMemory(size_t byteSize, size_t alignment, int flags, const char* name, const char* fileName, int lineNum);

    private:

        const char* m_name = nullptr;
        const char* m_desc = nullptr;
        Debug::AllocationRecords* m_records = nullptr;  // Cached pointer to allocation records. Works together with the MemoryDriller.
        size_t m_memoryGuardSize = 0;
        bool m_isLazilyCreated = false;
        bool m_isProfilingActive = false;
        bool m_isReady = false;
        bool m_canBeOverridden = true;
        bool m_registrationEnabled = true;
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
        uint16_t m_platformMemoryInstrumentationGroupId = 0;
#endif
    };

    namespace Internal  {
        struct AllocatorDummy{};
    }
}
