/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace SurfaceData
{
    /* mixed_stack_heap_allocator
    
       This allocator is a hybrid between a stack-based allocator and a heap-based allocator.
       It is intended for use with vector<> as a way to get the performance of fixed_vector<> when few nodes are needed, while still
       retaining the flexibility of general-purpose resizing when a large number of nodes are needed.

       In particular, this is useful for APIs that use a temporary vector<> during processing that is sized based on an input to the
       API, and the APIs have different use cases that could significantly vary the size requirements.

       Usage:
       vector<T, mixed_stack_heap_allocator<T, N>> tempData;

       This will create a vector<T> that pre-allocates space for N nodes on the stack. If the vector attempts to allocate
       N entries or less, the allocation will come from the stack space. If more than N entries are allocated, or if the vector attempts
       to grow beyond N entries, the memory will instead come from the heap.

       Limitations:
       - This currently only supports exactly one allocation from the stack, which works well for vector<>, but may not work as well for
         other data types.
       - Once the memory is allocated from the heap, shrinking the allocation won't cause it to use the stack unless the vector<> is fully
         deallocated and reallocated.
    */

    template<class Node, AZStd::size_t NumNodes>
    class mixed_stack_heap_allocator
    {
        static_assert(NumNodes > 0, "Size of static buffer must be more than 0.");
        typedef mixed_stack_heap_allocator<Node, NumNodes> this_type;

    public:
        AZ_TYPE_INFO(mixed_stack_heap_allocator, "{49B6706B-716F-42F2-92CB-7FD1A57BE2F9}");

        AZ_ALLOCATOR_DEFAULT_TRAITS

        mixed_stack_heap_allocator(const char* name = "AZStd::mixed_stack_heap_allocator")
            : m_name(name)
            , m_lastStaticAllocation(nullptr)
        {
        }

        mixed_stack_heap_allocator(const mixed_stack_heap_allocator& rhs)
            : m_name(rhs.m_name)
            , m_lastStaticAllocation(nullptr)
        {
        }

        mixed_stack_heap_allocator([[maybe_unused]] const mixed_stack_heap_allocator& rhs, const char* name)
            : m_name(name)
            , m_lastStaticAllocation(nullptr)
        {
        }

        mixed_stack_heap_allocator& operator=(const mixed_stack_heap_allocator& rhs)
        {
            m_name = rhs.m_name;
            return *this;
        }

        const char* get_name() const
        {
            return m_name;
        }

        void set_name(const char* name)
        {
            m_name = name;
        }

        auto allocate(size_type byteSize, size_type alignment, int flags = 0) -> pointer
        {
            // If the requested allocation will fit in our static buffer, and we aren't already using the static buffer,
            // then mark the static buffer as used and return a pointer to it.
            if ((byteSize <= (sizeof(Node) * NumNodes)) && (alignment <= AZStd::alignment_of<Node>::value) &&
                (m_lastStaticAllocation == nullptr))
            {
                m_lastStaticAllocation = &m_data;
                return m_lastStaticAllocation;
            }

            // Otherwise, allocate from the heap.
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
        }

        void deallocate(pointer ptr, size_type byteSize, size_type alignment)
        {
            // If the pointer is our static buffer, mark the static buffer as unused and return.
            if (ptr == m_lastStaticAllocation)
            {
                m_lastStaticAllocation = nullptr;
                return;
            }

            // Otherwise, deallocate the pointer from the heap.
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(ptr, byteSize, alignment);
        }

        auto reallocate(pointer ptr, size_type newSize, align_type newAlignment = 1) -> pointer
        {
            // If we're trying to reallocate our static buffer, allow it to succeed as long as the new size is within the total size of the
            // static buffer. Otherwise, return 0 to fail the reallocate request.
            if (ptr == m_lastStaticAllocation)
            {
                if (newSize <= (sizeof(Node) * NumNodes))
                {
                    return ptr;
                }
                return nullptr;
            }

            // Resize from the heap.
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().reallocate(ptr, newSize, newAlignment);
        }

        auto max_size() const -> size_type
        {
            // Since we allow both stack and heap allocations, the max allocation size for this container is the heap's maximum.
            return AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetMaxContiguousAllocationSize();
        }

        auto NumAllocatedBytes() const -> size_type
        {
            // Always return the full size of our stack allocation, plus the total amount of heap allocations.
            // While this doesn't seem like an accurate result, it's consistent with how AZStd::allocator behaves.
            // We would need to do a significant amount of extra bookkeeping to provide an accurate number here, and it doesn't
            // appear like anything uses this value, so the extra bookkeeping isn't currently warranted.
            return (sizeof(Node) * NumNodes) + AZ::AllocatorInstance<AZ::SystemAllocator>::Get().NumAllocatedBytes();
        }

        bool is_lock_free()
        {
            return false;
        }

        bool is_stale_read_allowed()
        {
            return false;
        }

        bool is_delayed_recycling()
        {
            return false;
        }

    private:
        const char* m_name;

        // This will point to &m_data if the static allocation is currently in use, and nullptr if it isn't.
        // Eventually, this could also be used to support multiple static allocations from the same buffer.
        void* m_lastStaticAllocation{ nullptr };

        // Stack-based storage that exists for the same lifetime as the data structure using this allocator.
        typename AZStd::aligned_storage<sizeof(Node) * NumNodes, AZStd::alignment_of<Node>::value>::type m_data;
    };

    template<class Node, AZStd::size_t NumNodes>
    bool operator==(
        [[maybe_unused]] const mixed_stack_heap_allocator<Node, NumNodes>& a,
        [[maybe_unused]] const mixed_stack_heap_allocator<Node, NumNodes>& b)
    {
        // Allocators should compare as equal if they can interchangeably handle each other's allocations.
        // Since this allocator can allocate from a private static buffer, it can only process its own allocations.
        return (&a == &b);
    }

    template<class Node, AZStd::size_t NumNodes>
    bool operator!=(
        [[maybe_unused]] const mixed_stack_heap_allocator<Node, NumNodes>& a,
        [[maybe_unused]] const mixed_stack_heap_allocator<Node, NumNodes>& b)
    {
        return (&a != &b);
    }

} // namespace AZStd
