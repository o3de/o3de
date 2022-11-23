/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_ALLOCATOR_CONCURRENT_STATIC_H
#define AZSTD_ALLOCATOR_CONCURRENT_STATIC_H

#include <AzCore/std/base.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    /**
     *  Declares a static buffer of Node[NumNodes], and them pools them. It provides concurrent
     *  safe access. This is a perfect allocator for pooling lists or hash table nodes.
     *  Internally the buffer is allocated using aligned_storage.
     *  \note only allocate/deallocate are thread safe. 
     *  reset, leak_before_destroy and comparison operators are not thread safe.
     *  get_allocated_size is thread safe but the returned value is not perfectly in 
     *  sync on the actual number of allocations (the number of allocations is incremented before the
     *  allocation happens and decremented after the allocation happens, trying to give a conservative
     *  number)
     *  \note be careful if you use this on the stack, since many platforms
     *  do NOT support alignment more than 16 bytes. In such cases you will need to do it manually.
     */
    template< class Node, AZStd::size_t NumNodes >
    class static_pool_concurrent_allocator
    {
        typedef static_pool_concurrent_allocator<Node, NumNodes> this_type;

        typedef int32_t            index_type;

        union  pool_node
        {
            typename aligned_storage<sizeof(Node), AZStd::alignment_of<Node>::value >::type m_node;
            index_type  m_index;
        };

        static constexpr index_type invalid_index = index_type(-1);

    public:
        using value_type = Node;
        using pointer = Node*;
        using size_type = AZStd::size_t;
        using difference_type = AZStd::ptrdiff_t;

        AZ_FORCE_INLINE static_pool_concurrent_allocator(const char* name = "AZStd::static_pool_concurrent_allocator")
            : m_name(name)
        {
            reset();
        }

        AZ_FORCE_INLINE ~static_pool_concurrent_allocator()
        {
            AZ_Assert(m_numOfAllocatedNodes == 0, "We still have allocated nodes. Call leak_before_destroy() before you destroy the container, to indicate this is ok.");
        }

        // When we copy the allocator we don't copy the allocated memory since it's the user responsibility.
        AZ_FORCE_INLINE static_pool_concurrent_allocator(const this_type& rhs)
            : m_name(rhs.m_name)    {  reset(); }
        AZ_FORCE_INLINE static_pool_concurrent_allocator(const this_type& rhs, const char* name)
            : m_name(name) { (void)rhs; reset(); }
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)      { m_name = rhs.m_name; return *this; }

        AZ_FORCE_INLINE const char*  get_name() const           { return m_name; }
        AZ_FORCE_INLINE void         set_name(const char* name) { m_name = name; }
        constexpr size_type          max_size() const           { return NumNodes * sizeof(Node); }
        AZ_FORCE_INLINE size_type   get_allocated_size() const  { return m_numOfAllocatedNodes.load(AZStd::memory_order_relaxed) * sizeof(Node); }

        inline Node* allocate()
        {
            index_type firstFreeNode = m_firstFreeNode.load(AZStd::memory_order_relaxed);
            if (firstFreeNode != invalid_index)
            {
                ++m_numOfAllocatedNodes;

                // weak compare_exchange is used here since firstFreeNode could change invalidating the value being passed to compare_exchange (2nd parameter)
                // which requires the loop to execute. Therefore, since we need to execute the loop anyway, a spurious failure will yield to better results.
                while (!m_firstFreeNode.compare_exchange_weak(
                    firstFreeNode,
                    (reinterpret_cast<pool_node*>(&m_data) + firstFreeNode)->m_index,
                    AZStd::memory_order_release,
                    AZStd::memory_order_relaxed))
                {
                    if (firstFreeNode == invalid_index)
                    {
                        AZ_Assert(false, "AZStd::static_pool_concurrent_allocator - No more free nodes!");
                        return nullptr;
                    }
                }
                return reinterpret_cast<Node*>(reinterpret_cast<pool_node*>(&m_data) + firstFreeNode);
            }
            else
            {
                AZ_Assert(false, "AZStd::static_pool_concurrent_allocator - No more free nodes!");
                return nullptr;
            }
        }

        inline pointer allocate(size_type byteSize, size_type alignment, int flags = 0)
        {
            (void)alignment;
            (void)byteSize;
            (void)flags;

            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_pool_concurrent_allocator::allocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize == sizeof(Node), "AZStd::static_pool_concurrent_allocator - We can allocate only node sizes from the pool!");
            AZ_Assert(alignment <= AZStd::alignment_of<Node>::value, "AZStd::static_pool_concurrent_allocator - Invalid data alignment!");

            return allocate();
        }

        void  deallocate(Node* ptr)
        {
            AZ_Assert(((char*)ptr >= (char*)&m_data) && ((char*)ptr < ((char*)&m_data + sizeof(Node) * NumNodes)), "AZStd::static_pool_concurrent_allocator - Pointer is out of range!");

            pool_node* firstPoolNode = reinterpret_cast<pool_node*>(&m_data);
            pool_node* curPoolNode = reinterpret_cast<pool_node*>(ptr);
            do 
            {
                curPoolNode->m_index = m_firstFreeNode.load(AZStd::memory_order_relaxed);
            } while (!m_firstFreeNode.compare_exchange_weak(
                curPoolNode->m_index,
                (index_type)(curPoolNode - firstPoolNode),
                AZStd::memory_order_release,
                AZStd::memory_order_relaxed));
            --m_numOfAllocatedNodes;
        }

        inline void  deallocate(pointer ptr, size_type byteSize, size_type alignment)
        { 
            (void)byteSize;
            (void)alignment;
            AZ_Assert(alignment > 0 && (alignment & (alignment - 1)) == 0, "AZStd::static_pool_concurrent_allocator::deallocate - alignment must be > 0 and power of 2");
            AZ_Assert(byteSize <= sizeof(Node), "AZStd::static_pool_concurrent_allocator - We can allocate only node sizes from the pool!");
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

            poolNode->m_index = invalid_index;
        }

        void leak_before_destroy()
        {
#ifdef AZ_Assert  // used only to confirm that it is ok for use to have allocated nodes
            m_numOfAllocatedNodes = 0;
            m_firstFreeNode = invalid_index; // We are not allowed to allocate after we call leak_before_destroy.
#endif
        }

        AZ_FORCE_INLINE void*               data()      const { return &m_data; }
        AZ_FORCE_INLINE constexpr size_type data_size() const { return sizeof(Node) * NumNodes; }

    private:
        typename aligned_storage<sizeof(Node)* NumNodes, AZStd::alignment_of<Node>::value>::type m_data;
        const char*    m_name;
        atomic<index_type>      m_firstFreeNode;
        atomic<size_type>       m_numOfAllocatedNodes;
    };

    template< class Node, AZStd::size_t NumNodes >
    AZ_FORCE_INLINE bool operator==(const static_pool_concurrent_allocator<Node, NumNodes>& a, const static_pool_concurrent_allocator<Node, NumNodes>& b)
    {
        return &a == &b;
    }

    template< class Node, AZStd::size_t NumNodes >
    AZ_FORCE_INLINE bool operator!=(const static_pool_concurrent_allocator<Node, NumNodes>& a, const static_pool_concurrent_allocator<Node, NumNodes>& b)
    {
        return &a != &b;
    }
}

#endif // AZSTD_ALLOCATOR_CONCURRENT_STATIC_H
#pragma once
