/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/base.h>

#include <AzCore/Memory/IAllocator.h>
#include <AzCore/RTTI/RTTI.h>

#if defined(AZ_ENABLE_TRACING)
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Memory/AllocatorDebug.h>
#include <AzCore/std/containers/vector.h>
#endif

namespace AZ
{
#if defined(AZ_ENABLE_TRACING)
    struct IAllocatorTrackingRecorderData;
#endif

    class IAllocatorTrackingRecorder
    {
    public:
        AZ_RTTI(IAllocatorTrackingRecorder, "{10468A58-A4E3-4FD0-8121-60F6DD13981C}")

        virtual ~IAllocatorTrackingRecorder() = default;

        // query APIs

        // Total amount of requested bytes to the allocator
        virtual AZStd::size_t GetRequested() const = 0;

        // Total amount of bytes allocated (i.e. requested to the OS)
        virtual AZStd::size_t GetAllocated() const = 0;

        // Amount of memory not being used by the allocator (allocated but not assigned to any request)
        // Note: the value returned by this method may not be exact, there is the potential for a race condition
        //       between the substraction and assignment of other methods.
        virtual AZStd::size_t GetFragmented() const = 0;

        // Prints all allocations that have been recorded
        virtual void PrintAllocations() const = 0;

        // Gets the amount of allocations (used in some tests)
        virtual AZStd::size_t GetAllocationCount() const = 0;
    };

#if defined(AZ_ENABLE_TRACING)
    struct AllocationRecord
    {
        AllocationRecord(void* address, AZStd::size_t requested, AZStd::size_t allocated, AZStd::size_t alignment)
            : m_address(address)
            , m_requested(requested)
            , m_allocated(allocated)
            , m_alignment(alignment)
        {
        }

        static AllocationRecord Create(
            void* address,
            AZStd::size_t requested,
            AZStd::size_t allocated,
            AZStd::size_t alignment,
            const AZStd::size_t stackFramesToSkip);

        void Print() const;

        bool operator<(const AllocationRecord& other) const
        {
            return this->m_address < other.m_address;
        }

        bool operator==(const AllocationRecord& other) const
        {
            return this->m_address == other.m_address;
        }

        void* m_address;
        AZStd::size_t m_requested;
        AZStd::size_t m_allocated;
        AZStd::size_t m_alignment;
        using StackTrace = AZStd::vector<AZ::Debug::StackFrame, AZ::Debug::DebugAllocator>;
        StackTrace m_allocationStackTrace;
    };
    using AllocationRecordVector = AZStd::vector<AllocationRecord, AZ::Debug::DebugAllocator>;
#endif

    /// Default implementation of IAllocatorTrackingRecorder.
    /// We also inherit from IAllocator to prevent derived classes from having to inherit a dreaded diamond
    class IAllocatorWithTracking
        : public IAllocator
        , public IAllocatorTrackingRecorder
    {
    public:
        AZ_RTTI(IAllocatorWithTracking, "{FACD0B30-2983-46CB-8D48-EFB0E0637510}", IAllocator, IAllocatorTrackingRecorder)

        IAllocatorWithTracking();
        ~IAllocatorWithTracking() override;

        // query APIs

        // Total amount of requested bytes to the allocator
        AZStd::size_t GetRequested() const override;

        // Total amount of bytes allocated (i.e. requested to the OS)
        AZStd::size_t GetAllocated() const override;

        // Amount of memory not being used by the allocator (allocated but not assigned to any request)
        // Note: the value returned by this method may not be exact, there is the potential for a race condition
        //       between the substraction and assignment of other methods.
        AZStd::size_t GetFragmented() const override;

        void PrintAllocations() const override;

        AZStd::size_t GetAllocationCount() const override;

#if defined(AZ_ENABLE_TRACING)
        AllocationRecordVector GetAllocationRecords() const;
#endif

        // Kept for backwards-compatibility reasons
        /////////////////////////////////////////////
        AZ_DEPRECATED_MESSAGE("Use GetAllocated instead")
        size_type NumAllocatedBytes() const override
        {
            return GetAllocated();
        }
        /////////////////////////////////////////////

    protected:
#if defined(AZ_ENABLE_TRACING)
        void AddAllocated(AZStd::size_t allocated);
        void RemoveAllocated(AZStd::size_t allocated);

        void AddAllocationRecord(void* address, AZStd::size_t requested, AZStd::size_t allocated, AZStd::size_t alignment, AZStd::size_t stackFramesToSkip = 0);
        void RemoveAllocationRecord(void* address, AZStd::size_t requested, AZStd::size_t allocated);

        // we need to keep this header clean of includes since this will be included by allocators
        // we can end up with include loops where we cannot find definitions
        IAllocatorTrackingRecorderData* m_data;
#endif
    };
}
