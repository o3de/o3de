/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class OverrunDetectionSchemaImpl;

    /**
     * Overrun detection allocator scheme
     * Use if you suspect a memory overrun. Application will crash at the point of memory access outside the allocated range.
     * This allocator is intended for short-term debugging purposes only. It consumes a lot of extra memory and leaks it at 
     * shutdown.
     *
     * In broad strokes, this technique works by reserving but not allocating an extra "trap" page after the requested space,
     * and realigning the requested allocation to end right before the trap begins. Attempts to write to memory beyond the 
     * requested allocation will write into the trap page, triggering an exception.
     *
     * This greatly bloats memory allocations, as the minimum allocation size becomes 2x the page size of the OS (a page for 
     * the requested memory, plus the trap page). On most platforms this is 8kb (4kb * 2 pages).
     */
    class OverrunDetectionSchema
        : public IAllocatorAllocate
    {
    public:
        AZ_TYPE_INFO("OverrunDetectionSchema", "{0DF781AC-1615-40AE-81F7-6CA5841E2914}");

        typedef void*       pointer_type;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        /// An abstraction for low-level memory mapping functions provided by the OS.
        class PlatformAllocator
        {
        public:
            struct SystemInformation
            {
                size_t m_pageSize;
                size_t m_minimumAllocationSize;
            };

            virtual ~PlatformAllocator()
            {
            }

            virtual SystemInformation GetSystemInformation() = 0;
            virtual void* ReserveBytes(size_t amount) = 0;
            virtual void ReleaseReservedBytes(void* base) = 0;
            virtual void* CommitBytes(void* base, size_t amount) = 0;
            virtual void DecommitBytes(void* base, size_t amount) = 0;
        };

        /**
         * Description - configure the overrun detection allocator. By default
         * we detect overruns (writes after the end of the allocation) instead of 
         * underruns (writes before the start of the allocation).
         */
        struct Descriptor
        {
            Descriptor(bool underrunDetection = false)
                : m_underrunDetection(underrunDetection)
            {}

            bool    m_underrunDetection;    ///< Set to true to detect underruns instead of overruns.
        };

        OverrunDetectionSchema(const Descriptor& desc = Descriptor());
        virtual ~OverrunDetectionSchema();

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type Resize(pointer_type ptr, size_type newSize) override;
        size_type AllocationSize(pointer_type ptr) override;

        size_type NumAllocatedBytes() const override;
        size_type Capacity() const override;
        size_type GetMaxAllocationSize() const override;
        size_type GetMaxContiguousAllocationSize() const override;
        IAllocatorAllocate* GetSubAllocator() override;
        void GarbageCollect() override;

    private:
        OverrunDetectionSchemaImpl* m_impl;
    };

    /**
    * Overrun detection allocator
    * In most cases, you will just use the OverrunDetectionSchema and override the behavior of an existing allocator.
    * This exists should you wish to use the schema directly with its own allocator, though.
    */

    class OverrunDetectionAllocator
        : public AZ::SimpleSchemaAllocator<AZ::OverrunDetectionSchema>
    {
    public:
        AZ_TYPE_INFO(OverrunDetectionAllocator, "{D06FB33E-7DFC-4311-9839-1E807806DC56}");

        using Base = AZ::SimpleSchemaAllocator<AZ::OverrunDetectionSchema>;
        using Descriptor = Base::Descriptor;

        OverrunDetectionAllocator()
            : Base("OverrunDetectionAllocator", "Debug allocator for detecting memory overruns")
        {
        }
    };
}
