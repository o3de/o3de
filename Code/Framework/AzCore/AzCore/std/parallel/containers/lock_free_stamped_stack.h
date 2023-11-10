/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/containers/lock_free_stack.h>

namespace AZStd
{
    /**
    * A lock-free stack implementation, uses stamped references to avoid the ABA problem. Requires an allocator, which
    * must also be lock-free, and allow stale reads. It may recycle allocations concurrently.
    */
    template<typename T, typename Allocator>
    class lock_free_stamped_stack
    {
    public:
        typedef T*                                  pointer;
        typedef const T*                            const_pointer;
        typedef T&                                  reference;
        typedef const T&                            const_reference;
        typedef typename Allocator::difference_type difference_type;
        typedef typename Allocator::size_type       size_type;
        typedef Allocator                           allocator_type;
        typedef T                                   value_type;
        typedef Internal::lock_free_stack_node<T>   node_type;
        typedef node_type*                          node_ptr_type;

        lock_free_stamped_stack();

        ~lock_free_stamped_stack();

        ///Pushes a value onto the top of the stack
        void push(const_reference value);

        ///Attempts to pop a value from the top of the stack. Returns false if the stack was empty, otherwise popped
        ///value is stored in value_out and returns true.
        bool pop(pointer value_out);

        ///Tests if the stack is empty, limited utility for a concurrent container.
        bool empty() const;

    private:
        //non-copyable
        lock_free_stamped_stack(const lock_free_stamped_stack&);
        lock_free_stamped_stack& operator=(const lock_free_stamped_stack&);

        node_ptr_type create_node();
        void destroy_node(node_ptr_type node);

        struct stamped_node_ptr
        {
            node_type* m_node;
            unsigned int m_stamp;
        };

        atomic<stamped_node_ptr> m_top;
        allocator_type m_allocator;
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    template<typename T, typename Allocator>
    inline lock_free_stamped_stack<T, Allocator>::lock_free_stamped_stack()
    {
        AZ_Assert(m_allocator.is_lock_free(), "Lock-free container requires a lock-free allocator");
        //allocator must allow stale read access, as pop can access a node which has been deallocated
        AZ_Assert(m_allocator.is_stale_read_allowed(), "Allocator for lock_free_stamped_stack must allow stale reads");

        stamped_node_ptr nodeStamp;
        nodeStamp.m_node = NULL;
        nodeStamp.m_stamp = 0;
        m_top.store(nodeStamp, memory_order_release);
    }

    template<typename T, typename Allocator>
    inline lock_free_stamped_stack<T, Allocator>::~lock_free_stamped_stack()
    {
        stamped_node_ptr nodeStamp = m_top.load(memory_order_acquire);
        node_type* node = nodeStamp.m_node;
        while (node)
        {
            node_type* next = node->m_next;
            destroy_node(node);
            node = next;
        }
    }

    template<typename T, typename Allocator>
    inline void lock_free_stamped_stack<T, Allocator>::push(const T& value)
    {
        stamped_node_ptr newTop;
        newTop.m_node = create_node();
        newTop.m_node->m_value = value;
        exponential_backoff backoff;
        while (true)
        {
            stamped_node_ptr oldTop = m_top.load(memory_order_acquire);
            newTop.m_node->m_next = oldTop.m_node;
            newTop.m_stamp = oldTop.m_stamp + 1;
            if (m_top.compare_exchange_weak(oldTop, newTop, memory_order_acq_rel, memory_order_acquire))
            {
                break;
            }
            else
            {
                backoff.wait();
            }
        }
    }

    template<typename T, typename Allocator>
    inline bool lock_free_stamped_stack<T, Allocator>::pop(T* value_out)
    {
        exponential_backoff backoff;
        while (true)
        {
            stamped_node_ptr oldTop = m_top.load(memory_order_acquire);
            if (!oldTop.m_node)
            {
                return false;
            }
            stamped_node_ptr newTop;
            newTop.m_node = oldTop.m_node->m_next;
            newTop.m_stamp = oldTop.m_stamp + 1;
            if (m_top.compare_exchange_weak(oldTop, newTop, memory_order_acq_rel, memory_order_acquire))
            {
                *value_out = oldTop.m_node->m_value;
                destroy_node(oldTop.m_node);
                return true;
            }
            backoff.wait();
        }
    }

    template<typename T, typename Allocator>
    inline bool lock_free_stamped_stack<T, Allocator>::empty() const
    {
        return (m_top.load(memory_order_acquire).m_node == NULL);
    }

    template<typename T, typename Allocator>
    inline typename lock_free_stamped_stack<T, Allocator>::node_ptr_type
    lock_free_stamped_stack<T, Allocator>::create_node()
    {
        node_type* node = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));
        new(node) node_type;
        return node;
    }

    template<typename T, typename Allocator>
    inline void lock_free_stamped_stack<T, Allocator>::destroy_node(node_ptr_type node)
    {
        node->~node_type();
        m_allocator.deallocate(node, sizeof(node_type), alignment_of<node_type>::value);
    }
}

