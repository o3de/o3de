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
        struct lock_free_stamped_queue_node;

        template<typename T>
        struct lock_free_stamped_node_ptr
        {
            alignas(8) struct lock_free_stamped_queue_node<T>* m_node;
            unsigned int m_stamp;
        };

        template<typename T>
        struct lock_free_stamped_queue_node
        {
            T m_value;
            atomic<lock_free_stamped_node_ptr<T> > m_next;
        };
    }

    /**
     * A lock-free queue implementation, uses stamped references to avoid the ABA problem. Requires an allocator, which
     * must also be lock-free, and allow stale reads. It may recycle allocations concurrently.
     */
    template<typename T, typename Allocator>
    class lock_free_stamped_queue
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
        typedef Internal::lock_free_stamped_node_ptr<T>     stamped_node_ptr;
        typedef Internal::lock_free_stamped_queue_node<T>   node_type;
        typedef node_type*                          node_ptr_type;

        lock_free_stamped_queue();

        ~lock_free_stamped_queue();

        ///Pushes a value onto the back of the queue
        void push(const_reference value);

        ///Attempts to pop a value from the front of the queue. Returns false if the queue was empty, otherwise popped
        ///value is stored in value_out and returns true.
        bool pop(pointer value_out);

        ///Tests if the queue is empty, limited utility for a concurrent container.
        bool empty() const;

    private:
        //non-copyable
        lock_free_stamped_queue(const lock_free_stamped_queue&);
        lock_free_stamped_queue& operator=(const lock_free_stamped_queue&);

        node_ptr_type create_node();
        void destroy_node(node_ptr_type node);

        atomic<stamped_node_ptr> m_head;
        atomic<stamped_node_ptr> m_tail;
        allocator_type m_allocator;
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    template<typename T, typename Allocator>
    inline lock_free_stamped_queue<T, Allocator>::lock_free_stamped_queue()
    {
        AZ_Assert(m_allocator.is_lock_free(), "Lock-free container requires a lock-free allocator");
        //allocator must allow stale read access, as queue can access a node which has been deallocated
        AZ_Assert(m_allocator.is_stale_read_allowed(), "Allocator for lock_free_queue must allow stale reads");

        node_type* sentinel = create_node();
        stamped_node_ptr nullStamp;
        nullStamp.m_node = NULL;
        nullStamp.m_stamp = 0;
        sentinel->m_next.store(nullStamp, memory_order_release);
        stamped_node_ptr nodeStamp;
        nodeStamp.m_node = sentinel;
        nodeStamp.m_stamp = 0;
        m_head.store(nodeStamp, memory_order_release);
        m_tail.store(nodeStamp, memory_order_release);
    }

    template<typename T, typename Allocator>
    inline lock_free_stamped_queue<T, Allocator>::~lock_free_stamped_queue()
    {
        stamped_node_ptr nodeStamp = m_head.load(memory_order_acquire);
        node_type* node = nodeStamp.m_node;
        while (node)
        {
            nodeStamp = node->m_next.load(memory_order_acquire);
            node_type* next = nodeStamp.m_node;
            destroy_node(node);
            node = next;
        }
    }

    template<typename T, typename Allocator>
    inline void lock_free_stamped_queue<T, Allocator>::push(const T& value)
    {
        stamped_node_ptr nullStamp;
        nullStamp.m_node = NULL;
        nullStamp.m_stamp = 0;
        stamped_node_ptr newNodeStamp;
        newNodeStamp.m_node = create_node();
        newNodeStamp.m_node->m_value = value;
        newNodeStamp.m_node->m_next.store(nullStamp, memory_order_release);
        exponential_backoff backoff;
        while (true)
        {
            stamped_node_ptr lastStamp = m_tail.load(memory_order_acquire);
            node_type* last = lastStamp.m_node;
            stamped_node_ptr nextStamp = last->m_next.load(memory_order_acquire);
            node_type* next = nextStamp.m_node;
            stamped_node_ptr lastStampCheck = m_tail.load(memory_order_acquire);
            if ((lastStamp.m_node == lastStampCheck.m_node) && (lastStamp.m_stamp == lastStampCheck.m_stamp))
            {
                if (next == NULL)
                {
                    newNodeStamp.m_stamp = nextStamp.m_stamp + 1;
                    if (last->m_next.compare_exchange_weak(nextStamp, newNodeStamp, memory_order_acq_rel, memory_order_acquire))
                    {
                        newNodeStamp.m_stamp = lastStamp.m_stamp + 1;
                        m_tail.compare_exchange_weak(lastStamp, newNodeStamp, memory_order_acq_rel, memory_order_acquire);
                        return;
                    }
                }
                else
                {
                    nextStamp.m_stamp = lastStamp.m_stamp + 1;
                    m_tail.compare_exchange_weak(lastStamp, nextStamp, memory_order_acq_rel, memory_order_acquire);
                }
            }
            backoff.wait();
        }
    }

    template<typename T, typename Allocator>
    inline bool lock_free_stamped_queue<T, Allocator>::pop(T* value_out)
    {
        exponential_backoff backoff;
        while (true)
        {
            stamped_node_ptr firstStamp = m_head.load(memory_order_acquire);
            node_type* first = firstStamp.m_node;
            stamped_node_ptr lastStamp = m_tail.load(memory_order_acquire);
            node_type* last = lastStamp.m_node;
            stamped_node_ptr nextStamp = first->m_next.load(memory_order_acquire);
            node_type* next = nextStamp.m_node;
            stamped_node_ptr firstStampCheck = m_head.load(memory_order_acquire);
            if ((firstStamp.m_node == firstStampCheck.m_node) && (firstStamp.m_stamp == firstStampCheck.m_stamp))
            {
                if (first == last)
                {
                    if (next == NULL)
                    {
                        return false;
                    }
                    nextStamp.m_stamp = lastStamp.m_stamp + 1;
                    m_tail.compare_exchange_weak(lastStamp, nextStamp, memory_order_acq_rel, memory_order_acquire);
                }
                else
                {
                    *value_out = next->m_value;
                    nextStamp.m_stamp = firstStamp.m_stamp + 1;
                    if (m_head.compare_exchange_weak(firstStamp, nextStamp, memory_order_acq_rel, memory_order_acquire))
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
    inline bool lock_free_stamped_queue<T, Allocator>::empty() const
    {
        return (m_head.load(memory_order_acquire).m_node->m_next.load(memory_order_acquire).m_node == NULL);
    }

    template<typename T, typename Allocator>
    inline typename lock_free_stamped_queue<T, Allocator>::node_ptr_type
    lock_free_stamped_queue<T, Allocator>::create_node()
    {
        node_type* node = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));
        new(node) node_type;
        return node;
    }

    template<typename T, typename Allocator>
    inline void lock_free_stamped_queue<T, Allocator>::destroy_node(node_ptr_type node)
    {
        node->~node_type();
        m_allocator.deallocate(node, sizeof(node_type), alignment_of<node_type>::value);
    }
}

