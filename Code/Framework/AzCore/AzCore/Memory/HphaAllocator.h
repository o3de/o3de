/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/typetraits/aligned_storage.h>

namespace AZ
{
    /**
    * Heap allocator schema, based on Dimitar Lazarov "High Performance Heap Allocator".
    */
    template<bool DebugAllocator = false>
    class HphaSchemaBase
        : public IAllocator
    {
    public:
        /**
        * Description - configure the heap allocator. By default
        * we will allocate system memory using system calls. You can
        * provide arena (memory block) with pre-allocated memory.
        */
        AZ_TYPE_INFO(HphaSchema, "{2C91A6EC-41E5-4711-9A4E-7B93A3A1EAA2}")

        HphaSchemaBase();
        virtual ~HphaSchemaBase();

        pointer         allocate(size_type byteSize, size_type alignment) override;
        void            deallocate(pointer ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer         reallocate(pointer ptr, size_type newSize, size_type newAlignment) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        size_type       NumAllocatedBytes() const override;

        /// Return unused memory to the OS. Don't call this unless you really need free memory, it is slow.
        void            GarbageCollect() override;

        static size_t GetMemoryGuardSize();
        static size_t GetFreeLinkSize();

    private:
        // Forward declare HpAllocator class
        // It is a private class implemented in the cpp
        class HpAllocator;

        // this must be at least the max size of HpAllocator (defined in the cpp) + any platform compiler padding
        // A static assert inside of HphaSchema.cpp validates that this is the case
        // as of commit https://github.com/o3de/o3de/commit/92cd457c256e1ec91eeabe04b56d1d4c61f8b1af
        // When MULTITHREADED and USE_MUTEX_PER_BUCKET is defined
        // the largest sizeof for HpAllocator is 16640 on MacOS
        // On Windows the sizeof HpAllocator is 8384
        // Up this value to 18 KiB to be safe
        static constexpr size_t hpAllocatorStructureSize = 18 * 1024;

        HpAllocator*        m_allocator;
        AZStd::aligned_storage_t<hpAllocatorStructureSize, 16> m_hpAllocatorBuffer;    ///< Memory buffer for HpAllocator
    };

    // Template is externed here and explicitly instantiated in the cpp file
    extern template class HphaSchemaBase<false>;
    extern template class HphaSchemaBase<true>;

    namespace Internal
    {
        // HphaSchema class defaults to disabling the allocator debug functionality
        constexpr bool HphaDebugAllocator = false;
    }

    class HphaSchema
        : public HphaSchemaBase<Internal::HphaDebugAllocator>
    {
    public:
        using HphaSchemaBase<Internal::HphaDebugAllocator>::HphaSchemaBase;
    };
} // namespace AZ
