/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    /**
     * This allocator uses aligned_storage to declare a static buffer, which is used for all allocations.
     * This allocator is designed to be used as a temporary allocator on the stack.
     * It just allocates from the buffer never frees anything, until you call reset.
     * It does allow memory leaks, thus allowing containers never to call deallocate, which makes them faster.
     */
    template<size_t Size, size_t Alignment>
    struct static_buffer_allocator
    {
        static_assert(Alignment > 0, "Alignment cannot be 0, the buffer must be aligned on some power of 2 greater than 0");
        static_assert(Size > 0, "Size of static buffer must be not be 0");

        using pointer = void*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        AZ_FORCE_INLINE static_buffer_allocator()
            : m_lastAllocation(nullptr)
        {
            m_freeData = reinterpret_cast<char*>(&m_data);
        }

        // When we copy the allocator we don't copy the allocated memory since it's the user responsibility.
        AZ_FORCE_INLINE static_buffer_allocator(const static_buffer_allocator&)
            : m_freeData(reinterpret_cast<char*>(&m_data))
            , m_lastAllocation(nullptr)
        {
        }

        AZ_FORCE_INLINE static_buffer_allocator& operator=(const static_buffer_allocator&)
        {
            return *this;
        }

        constexpr size_type max_size() const
        {
            return Size;
        }

        AZ_FORCE_INLINE size_type get_allocated_size() const
        {
            return m_freeData - reinterpret_cast<const char*>(&m_data);
        }

        [[nodiscard]] pointer allocate(
            size_type byteSize,
            size_type alignment,
            [[maybe_unused]] int flags = 0)
        {
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_buffer_allocator::allocate - alignment must be > 0 and power of 2");
            char* address = AZ::PointerAlignUp(m_freeData, alignment);
            m_freeData = address + byteSize;
            if (static_cast<size_t>(m_freeData - reinterpret_cast<char*>(&m_data)) > Size)
            {
                AZ_Assert(false, "AZStd::static_buffer_allocator - run out of memory!");
                return nullptr;
            }

            m_lastAllocation = address;
            return address;
        }

        AZ_FORCE_INLINE void deallocate(
            pointer ptr,
            [[maybe_unused]] size_type byteSize = 0,
            [[maybe_unused]] size_type alignment = 0)
        {
            // we can free only the last allocation
            if (ptr == m_lastAllocation)
            {
                m_lastAllocation = nullptr;
                m_freeData = static_cast<char*>(ptr);
            }
        }

        AZ_FORCE_INLINE size_type resize(pointer ptr, size_type newSize)
        {
            // We allow resize on last allocations only
            if (ptr == m_lastAllocation && newSize <= (Size - static_cast<size_t>(static_cast<char*>(ptr) - reinterpret_cast<char*>(&m_data))))
            {
                m_freeData = static_cast<char*>(m_lastAllocation) + newSize;
                return newSize;
            }
            return 0;
        }

        AZ_FORCE_INLINE void reset()
        {
            m_freeData = reinterpret_cast<char*>(&m_data);
        }

        AZ_FORCE_INLINE bool inrange(void* address) const
        {
            return address >= &m_data && address < reinterpret_cast<const char*>(&m_data) + Size;
        }

        friend AZ_FORCE_INLINE bool operator==(const static_buffer_allocator& a, const static_buffer_allocator& b)
        {
            return &a == &b;
        }

    private:
        aligned_storage_t<Size, Alignment> m_data;
        char* m_freeData;
        void* m_lastAllocation;
    };

    /**
     * Declares a static buffer of Node[NumNodes], and them pools them.
     * This is perfect allocator for pooling list or hash table nodes.
     * Internally the buffer is allocated using aligned_storage.
     *
     * \note This is not thread safe allocator.
     * \note be careful if you use this on the stack, since many platforms do NOT support alignment more than 16 bytes.
     * In such cases you will need to do it manually.
     */
    template<class Node, size_t NumNodes>
    struct static_pool_allocator
    {
        using pointer = void*;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        AZ_FORCE_INLINE static_pool_allocator()
        {
            reset();
        }

        AZ_FORCE_INLINE ~static_pool_allocator()
        {
            AZ_Assert(m_numOfAllocatedNodes == 0, "We still have allocated nodes. Call leak_before_destroy() before you destroy the container, to indicate this is ok.");
        }

        // When we copy the allocator we don't copy the allocated memory since it's the user responsibility.
        AZ_FORCE_INLINE static_pool_allocator(const static_pool_allocator&)
        {
            reset();
        }

        AZ_FORCE_INLINE static_pool_allocator& operator=(const static_pool_allocator&)
        {
            return *this;
        }

        constexpr size_type max_size() const
        {
            return NumNodes * sizeof(Node);
        }

        AZ_FORCE_INLINE size_type get_allocated_size() const
        {
            return m_numOfAllocatedNodes * sizeof(Node);
        }

    private:
        using index_type = int32_t;
        union pool_node
        {
            aligned_storage_t<sizeof(Node), alignment_of_v<Node>> m_node;
            index_type m_index;
        };

    public:
        [[nodiscard]] AZ_FORCE_INLINE Node* allocate()
        {
            AZ_Assert(m_firstFreeNode != index_type(-1), "AZStd::static_pool_allocator - No more free nodes!");
            pool_node* poolNode = reinterpret_cast<pool_node*>(&m_data) + m_firstFreeNode;
            m_firstFreeNode = poolNode->m_index;
            ++m_numOfAllocatedNodes;
            return reinterpret_cast<Node*>(poolNode);
        }

        [[nodiscard]] AZ_FORCE_INLINE pointer allocate(
            [[maybe_unused]] size_type byteSize,
            [[maybe_unused]] size_type alignment,
            [[maybe_unused]] int flags = 0)
        {
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_pool_allocator::allocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize == sizeof(Node), "AZStd::static_pool_allocator - We can allocate only node sizes from the pool!");
            AZ_Assert(alignment <= AZStd::alignment_of<Node>::value, "AZStd::static_pool_allocator - Invalid data alignment!");

            return allocate();
        }

        void deallocate(Node* ptr)
        {
            AZ_Assert(((char*)ptr >= (char*)&m_data) && ((char*)ptr < ((char*)&m_data + sizeof(Node) * NumNodes)), "AZStd::static_pool_allocator - Pointer is out of range!");

            pool_node* firstPoolNode = reinterpret_cast<pool_node*>(&m_data);
            pool_node* curPoolNode = reinterpret_cast<pool_node*>(ptr);
            curPoolNode->m_index = m_firstFreeNode;
            m_firstFreeNode = static_cast<index_type>(curPoolNode - firstPoolNode);
            --m_numOfAllocatedNodes;
        }

        AZ_FORCE_INLINE void deallocate(
            pointer ptr,
            [[maybe_unused]] size_type byteSize,
            [[maybe_unused]] size_type alignment)
        {
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_pool_allocator::deallocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize <= sizeof(Node), "AZStd::static_pool_allocator - We can allocate only node sizes from the pool!");
            deallocate(static_cast<Node*>(ptr));
        }

        AZ_FORCE_INLINE size_type resize(
            [[maybe_unused]] pointer ptr,
            [[maybe_unused]] size_type newSize)
        {
            return sizeof(pool_node); // this is the max size we can have.
        }

        void reset()
        {
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = 0;
            pool_node* poolNode = reinterpret_cast<pool_node*>(&m_data);
            for (index_type iNode = 1; iNode < static_cast<index_type>(NumNodes); ++iNode, ++poolNode)
            {
                poolNode->m_index = iNode;
            }

            poolNode->m_index = -1;
        }

        void leak_before_destroy()
        {
#ifdef AZ_Assert // used only to confirm that it is ok for use to have allocated nodes
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = -1; // We are not allowed to allocate after we call leak_before_destroy.
#endif
        }

        AZ_FORCE_INLINE void* data() const
        {
            return &m_data;
        }

        AZ_FORCE_INLINE constexpr size_type data_size() const
        {
            return sizeof(Node) * NumNodes;
        }

        friend AZ_FORCE_INLINE bool operator==(const static_pool_allocator& a, const static_pool_allocator& b)
        {
            return &a == &b;
        }

    private:
        aligned_storage_t<sizeof(Node) * NumNodes, alignment_of_v<Node>> m_data;
        index_type m_firstFreeNode = 0;
        size_type m_numOfAllocatedNodes = 0;
    };
}
