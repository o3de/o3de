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
     * This allocator uses aligned_storage to declare a static buffer, which
     * is used for all allocations. This allocator is designed to be used as a temporary
     * allocator on the stack. It just allocates from the buffer never frees anything, until
     * you call reset. It does allow memory leaks, thus allowing containers never
     * to call deallocate, which makes them faster.
     */
    template<AZStd::size_t Size, AZStd::size_t Alignment>
    class static_buffer_allocator
    {
        static_assert(Alignment > 0, "Alignment cannot be 0, the buffer must be aligned on some power of 2 greater than 0");
        static_assert(Size > 0, "Size of static buffer must be not be 0");
        typedef static_buffer_allocator<Size, Alignment> this_type;
    public:
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        AZ_FORCE_INLINE static_buffer_allocator()
            : m_lastAllocation(nullptr)
        {
            m_freeData = reinterpret_cast<char*>(&m_data);
        }

        // When we copy the allocator we don't copy the allocated memory since it's the user responsibility.
        AZ_FORCE_INLINE static_buffer_allocator(const this_type&)
            : m_freeData(reinterpret_cast<char*>(&m_data))
            , m_lastAllocation(nullptr)
        {}
        
        AZ_FORCE_INLINE this_type& operator=(const this_type&)
        {
            return *this;
        }
        
        constexpr size_type         max_size() const { return Size; }
        AZ_FORCE_INLINE size_type   get_allocated_size() const { return m_freeData - reinterpret_cast<const char*>(&m_data); }
                
        pointer allocate(size_type byteSize, size_type alignment, [[maybe_unused]] int flags = 0)
        {
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_buffer_allocator::allocate - alignment must be > 0 and power of 2");
            char* address = AZ::PointerAlignUp(m_freeData, alignment);
            m_freeData = address + byteSize;
            if (size_t(m_freeData - reinterpret_cast<char*>(&m_data)) > Size)
            {
                AZ_Assert(false, "AZStd::static_buffer_allocator - run out of memory!");
                return nullptr;
            }

            m_lastAllocation = address;
            return address;
        }

        AZ_FORCE_INLINE void  deallocate(pointer ptr, [[maybe_unused]] size_type byteSize = 0, [[maybe_unused]] size_type alignment = 0)
        {
            // we can free only the last allocation
            if (ptr == m_lastAllocation)
            {
                m_lastAllocation = nullptr;
                m_freeData = reinterpret_cast<char*>(ptr);
            }
        }
        AZ_FORCE_INLINE size_type    resize(pointer ptr, size_type newSize)
        {
            // We allow resize on last allocations only
            if (ptr == m_lastAllocation && newSize <= (Size - size_t(reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(&m_data))))
            {
                m_freeData = reinterpret_cast<char*>(m_lastAllocation) + newSize;
                return newSize;
            }
            return 0;
        }

        AZ_FORCE_INLINE void  reset()
        {
            m_freeData = reinterpret_cast<char*>(&m_data);
        }

        AZ_FORCE_INLINE bool inrange(void* address) const
        {
            return address >= &m_data && address < reinterpret_cast<const char*>(&m_data) + Size;
        }

    private:
        typename aligned_storage<Size, Alignment>::type  m_data;
        char*           m_freeData;
        void*           m_lastAllocation;
    };

    template<AZStd::size_t Size, AZStd::size_t Alignment>
    AZ_FORCE_INLINE bool operator==(const static_buffer_allocator<Size, Alignment>& a, const static_buffer_allocator<Size, Alignment>& b)
    {
        return &a == &b;
    }

    template<AZStd::size_t Size, AZStd::size_t Alignment>
    AZ_FORCE_INLINE bool operator!=(const static_buffer_allocator<Size, Alignment>& a, const static_buffer_allocator<Size, Alignment>& b)
    {
        return &a != &b;
    }

    /**
     *  Declares a static buffer of Node[NumNodes], and them pools them. This
     *  is perfect allocator for pooling list or hash table nodes.
     *  Internally the buffer is allocated using aligned_storage.
     *  \note This is not thread safe allocator.
     *  \note be careful if you use this on the stack, since many platforms
     *  do NOT support alignment more than 16 bytes. In such cases you will need to do it manually.
     */
    template< class Node, AZStd::size_t NumNodes >
    class static_pool_allocator
    {
        typedef static_pool_allocator<Node, NumNodes> this_type;

        typedef int32_t             index_type;

        union  pool_node
        {
            typename aligned_storage<sizeof(Node), AZStd::alignment_of<Node>::value >::type m_node;
            index_type  m_index;
        };

    public:
        using pointer = void*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        AZ_FORCE_INLINE static_pool_allocator()
        {
            reset();
        }

        AZ_FORCE_INLINE ~static_pool_allocator()
        {
            AZ_Assert(m_numOfAllocatedNodes == 0, "We still have allocated nodes. Call leak_before_destroy() before you destroy the container, to indicate this is ok.");
        }

        // When we copy the allocator we don't copy the allocated memory since it's the user responsibility.
        AZ_FORCE_INLINE static_pool_allocator(const this_type&)
            { reset(); }
        AZ_FORCE_INLINE this_type& operator=(const this_type&)      { return *this; }

        constexpr size_type          max_size() const           { return NumNodes * sizeof(Node); }
        AZ_FORCE_INLINE size_type   get_allocated_size() const  { return m_numOfAllocatedNodes * sizeof(Node); }

        inline Node* allocate()
        {
            AZ_Assert(m_firstFreeNode != index_type(-1), "AZStd::static_pool_allocator - No more free nodes!");
            pool_node* poolNode = reinterpret_cast<pool_node*>(&m_data) + m_firstFreeNode;
            m_firstFreeNode = poolNode->m_index;
            ++m_numOfAllocatedNodes;
            return reinterpret_cast<Node*>(poolNode);
        }

        inline pointer allocate(size_type byteSize, size_type alignment, int flags = 0)
        {
            (void)alignment;
            (void)byteSize;
            (void)flags;

            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_pool_allocator::allocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize == sizeof(Node), "AZStd::static_pool_allocator - We can allocate only node sizes from the pool!");
            AZ_Assert(alignment <= AZStd::alignment_of<Node>::value, "AZStd::static_pool_allocator - Invalid data alignment!");

            return allocate();
        }

        void  deallocate(Node* ptr)
        {
            AZ_Assert(((char*)ptr >= (char*)&m_data) && ((char*)ptr < ((char*)&m_data + sizeof(Node) * NumNodes)), "AZStd::static_pool_allocator - Pointer is out of range!");

            pool_node* firstPoolNode = reinterpret_cast<pool_node*>(&m_data);
            pool_node* curPoolNode = reinterpret_cast<pool_node*>(ptr);
            curPoolNode->m_index = m_firstFreeNode;
            m_firstFreeNode = (index_type)(curPoolNode - firstPoolNode);
            --m_numOfAllocatedNodes;
        }

        inline void  deallocate(pointer ptr, size_type byteSize, size_type alignment)
        {
            (void)byteSize;
            (void)alignment;
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_pool_allocator::deallocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize <= sizeof(Node), "AZStd::static_pool_allocator - We can allocate only node sizes from the pool!");
            deallocate(reinterpret_cast<Node*>(ptr));
        }

        AZ_FORCE_INLINE size_type    resize(pointer ptr, size_type newSize)
        {
            (void)ptr;
            (void)newSize;
            return sizeof(pool_node); // this is the max size we can have.
        }

        void  reset()
        {
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = 0;
            pool_node* poolNode = reinterpret_cast<pool_node*>(&m_data);
            for (index_type iNode = 1; iNode < (index_type)NumNodes; ++iNode, ++poolNode)
            {
                poolNode->m_index = iNode;
            }

            poolNode->m_index = index_type(-1);
        }

        void leak_before_destroy()
        {
#ifdef AZ_Assert  // used only to confirm that it is ok for use to have allocated nodes
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = index_type(-1); // We are not allowed to allocate after we call leak_before_destroy.
#endif
        }

        AZ_FORCE_INLINE void*                 data()      const { return &m_data; }
        AZ_FORCE_INLINE constexpr size_type   data_size() const { return sizeof(Node) * NumNodes; }

    private:
        typename aligned_storage<sizeof(Node) * NumNodes, AZStd::alignment_of<Node>::value>::type m_data;
        index_type      m_firstFreeNode;
        size_type       m_numOfAllocatedNodes;
    };

    template< class Node, AZStd::size_t NumNodes >
    AZ_FORCE_INLINE bool operator==(const static_pool_allocator<Node, NumNodes>& a, const static_pool_allocator<Node, NumNodes>& b)
    {
        return &a == &b;
    }

    template< class Node, AZStd::size_t NumNodes >
    AZ_FORCE_INLINE bool operator!=(const static_pool_allocator<Node, NumNodes>& a, const static_pool_allocator<Node, NumNodes>& b)
    {
        return &a != &b;
    }
}
