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

#include <AzCore/std/parallel/containers/lock_free_intrusive_stack.h>

namespace AZStd
{
    /**
     * A lock-free intrusive stack implementation. The stored type must either inherit from
     * lock_free_intrusive_stack_node or include it as a member, and the appropriate hook type must be specified.
     * Unlike lock_free_intrusive_stack, this uses stamped references to avoid the ABA problem.
     */
    template<typename T, typename Hook>
    class lock_free_intrusive_stamped_stack
    {
    public:
        typedef T*                                  pointer;
        typedef const T*                            const_pointer;
        typedef T&                                  reference;
        typedef const T&                            const_reference;
        typedef typename AZStd::size_t              difference_type;
        typedef typename AZStd::size_t              size_type;
        typedef T                                   value_type;
        typedef T                                   node_type;
        typedef node_type*                          node_ptr_type;
        typedef lock_free_intrusive_stack_node<T>   hook_node_type;
        typedef hook_node_type*                     hook_node_ptr_type;

        lock_free_intrusive_stamped_stack();

        ~lock_free_intrusive_stamped_stack();

        ///Pushes a value onto the top of the stack
        void push(const_reference value);

        ///Attempts to pop a value from the top of the stack. Returns NULL if the stack was empty, otherwise returns
        ///a pointer to the popped value
        pointer pop();

        ///Tests if the stack is empty, limited utility for a concurrent container.
        bool empty() const;

    private:
        //non-copyable
        lock_free_intrusive_stamped_stack(const lock_free_intrusive_stamped_stack&);
        lock_free_intrusive_stamped_stack& operator=(const lock_free_intrusive_stamped_stack&);

        struct stamped_node_ptr
        {
            node_type* m_node;
            unsigned int m_stamp;
        };

        atomic<stamped_node_ptr> m_top;
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    template<typename T, typename Hook>
    inline lock_free_intrusive_stamped_stack<T, Hook>::lock_free_intrusive_stamped_stack()
    {
        stamped_node_ptr nodeStamp;
        nodeStamp.m_node = NULL;
        nodeStamp.m_stamp = 0;
        m_top.store(nodeStamp, memory_order_release);
    }

    template<typename T, typename Hook>
    inline lock_free_intrusive_stamped_stack<T, Hook>::~lock_free_intrusive_stamped_stack()
    {
#ifdef AZ_DEBUG_BUILD
        stamped_node_ptr nodeStamp = m_top.load(memory_order_acquire);
        node_type* node = nodeStamp.m_node;
        while (node)
        {
            hook_node_type* hookNode = Hook::to_node_ptr(node);
            node_type* next = hookNode->m_next;
            hookNode->m_next = NULL;
            node = next;
        }
#endif
    }

    template<typename T, typename Hook>
    inline void lock_free_intrusive_stamped_stack<T, Hook>::push(const T& value)
    {
        stamped_node_ptr newTop;
        newTop.m_node = const_cast<pointer>(&value);
        hook_node_type* newHookNode = Hook::to_node_ptr(newTop.m_node);
#ifdef AZ_DEBUG_BUILD
        AZSTD_CONTAINER_ASSERT(!newHookNode->m_next, "Node is already in an intrusive list");
#endif
        exponential_backoff backoff;
        while (true)
        {
            stamped_node_ptr oldTop = m_top.load(memory_order_acquire);
            newHookNode->m_next = oldTop.m_node;
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

    template<typename T, typename Hook>
    inline typename lock_free_intrusive_stamped_stack<T, Hook>::pointer lock_free_intrusive_stamped_stack<T, Hook>::pop()
    {
        exponential_backoff backoff;
        while (true)
        {
            stamped_node_ptr oldTop = m_top.load(memory_order_acquire);
            if (!oldTop.m_node)
            {
                return NULL;
            }
            hook_node_type* oldHookTop = Hook::to_node_ptr(oldTop.m_node);
            stamped_node_ptr newTop;
            newTop.m_node = oldHookTop->m_next;
            newTop.m_stamp = oldTop.m_stamp + 1;
            if (m_top.compare_exchange_weak(oldTop, newTop, memory_order_acq_rel, memory_order_acquire))
            {
#ifdef AZ_DEBUG_BUILD
                oldHookTop->m_next = NULL;
#endif
                return oldTop.m_node;
            }
            backoff.wait();
        }
    }

    template<typename T, typename Hook>
    inline bool lock_free_intrusive_stamped_stack<T, Hook>::empty() const
    {
        return (m_top.load(memory_order_acquire).m_node == NULL);
    }
}

