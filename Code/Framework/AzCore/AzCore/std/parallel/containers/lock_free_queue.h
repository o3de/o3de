/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/exponential_backoff.h>

namespace AZStd
{
    namespace Internal
    {
        template<typename T>
        struct lock_free_queue_node
        {
            T m_value;
            atomic<lock_free_queue_node*> m_next;
        };
    }

    /**
     * A lock-free queue implementation. Requires an allocator, which must also be lock-free, allow stale reads, and
     * not recycle allocations concurrently (to avoid the ABA problem).
     */
    template<typename T, typename Allocator>
    class lock_free_queue
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
        typedef Internal::lock_free_queue_node<T>   node_type;
        typedef node_type*                          node_ptr_type;

        lock_free_queue();

        ~lock_free_queue();

        ///Pushes a value onto the back of the queue
        void push(const_reference value);

        ///Attempts to pop a value from the front of the queue. Returns false if the queue was empty, otherwise popped
        ///value is stored in value_out and returns true.
        bool pop(pointer value_out);

        ///Tests if the queue is empty, limited utility for a concurrent container.
        bool empty() const;

    private:
        //non-copyable
        lock_free_queue(const lock_free_queue&);
        lock_free_queue& operator=(const lock_free_queue&);

        node_ptr_type create_node();
        void destroy_node(node_ptr_type node);

        atomic<node_type*> m_head;
        atomic<node_type*> m_tail;
        allocator_type m_allocator;
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    template<typename T, typename Allocator>
    inline lock_free_queue<T, Allocator>::lock_free_queue()
    {
        AZ_Assert(m_allocator.is_lock_free(), "Lock-free container requires a lock-free allocator");
        //allocator must allow stale read access, as queue can access a node which has been deallocated
        AZ_Assert(m_allocator.is_stale_read_allowed(), "Allocator for lock_free_queue must allow stale reads");
        //allocator must not recycle while lock_free_queue operations are active, as queue is vulnerable to ABA problem
        AZ_Assert(m_allocator.is_delayed_recycling(),   "Allocator for lock_free_queue must not recycle"
            "allocations, use lock_free_stamped_queue to relax this restriction");

        node_type* sentinel = create_node();
        sentinel->m_next.store(NULL, memory_order_release);
        m_head.store(sentinel, memory_order_release);
        m_tail.store(sentinel, memory_order_release);
    }

    template<typename T, typename Allocator>
    inline lock_free_queue<T, Allocator>::~lock_free_queue()
    {
        node_type* node = m_head.load(memory_order_acquire);
        while (node)
        {
            node_type* next = node->m_next.load(memory_order_acquire);
            destroy_node(node);
            node = next;
        }
    }

    template<typename T, typename Allocator>
    inline void lock_free_queue<T, Allocator>::push(const T& value)
    {
        node_type* node = create_node();
        node->m_value = value;
        node->m_next.store(NULL, memory_order_release);
        exponential_backoff backoff;
        while (true)
        {
            node_type* last = m_tail.load(memory_order_acquire);
            node_type* next = last->m_next.load(memory_order_acquire);
            if (last == m_tail.load(memory_order_acquire))
            {
                if (next == NULL)
                {
                    if (last->m_next.compare_exchange_weak(next, node, memory_order_acq_rel, memory_order_acquire))
                    {
                        m_tail.compare_exchange_weak(last, node, memory_order_acq_rel, memory_order_acquire);
                        return;
                    }
                }
                else
                {
                    m_tail.compare_exchange_weak(last, next, memory_order_acq_rel, memory_order_acquire);
                }
            }
            backoff.wait();
        }
    }

    template<typename T, typename Allocator>
    inline bool lock_free_queue<T, Allocator>::pop(T* value_out)
    {
        exponential_backoff backoff;
        while (true)
        {
            node_type* first = m_head.load(memory_order_acquire);
            node_type* last = m_tail.load(memory_order_acquire);
            node_type* next = first->m_next.load(memory_order_acquire);
            if (first == m_head.load(memory_order_acquire))
            {
                if (first == last)
                {
                    if (next == NULL)
                    {
                        return false;
                    }
                    m_tail.compare_exchange_weak(last, next, memory_order_acq_rel, memory_order_acquire);
                }
                else
                {
                    *value_out = next->m_value;
                    if (m_head.compare_exchange_weak(first, next, memory_order_acq_rel, memory_order_acquire))
                    {
                        destroy_node(first);
                        return true;
                    }
                }
            }
            backoff.wait();
        }
    }

    template<typename T, typename Allocator>
    inline bool lock_free_queue<T, Allocator>::empty() const
    {
        return (m_head.load(memory_order_acquire)->m_next.load(memory_order_acquire) == NULL);
    }

    template<typename T, typename Allocator>
    inline typename lock_free_queue<T, Allocator>::node_ptr_type lock_free_queue<T, Allocator>::create_node()
    {
        node_type* node = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));
        new(node) node_type;
        return node;
    }

    template<typename T, typename Allocator>
    inline void lock_free_queue<T, Allocator>::destroy_node(node_ptr_type node)
    {
        node->~node_type();
        m_allocator.deallocate(node, sizeof(node_type), alignment_of<node_type>::value);
    }
}

