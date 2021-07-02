/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/exponential_backoff.h>

namespace AZStd
{
    /**
     * This is the node you need to include in your objects, if you want to use
     * it in lock_free_intrusive_stack. You can do that either by inheriting it
     * or add it as a \b public member. They way you include the node should be in
     * using the appropriate hooks.
     * Check the lock_free_intrusive_stack \ref AZStdExamples.
     */
    template< class T, int Unused = 0 /*This if left here because of a bug with autoexp.dat parsing. This is VS debugger data visualizer.*/>
    struct lock_free_intrusive_stack_node
    {
#ifdef AZ_DEBUG_BUILD
        lock_free_intrusive_stack_node()
            : m_next(0) {}
        ~lock_free_intrusive_stack_node()
        {
            AZSTD_CONTAINER_ASSERT(m_next == 0, "AZStd::lock_free_intrusive_stack_node - intrusive node is being destroyed while still a member of a list!");
        }
#endif
        T* m_next;
    };

    /**
     * Hook when our object inherits \ref lock_free_intrusive_stack_node. So the list
     * will use the base class.
     * Check the lock_free_intrusive_stack \ref AZStdExamples.
     */
    template< class T >
    struct lock_free_intrusive_stack_base_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef lock_free_intrusive_stack_node<T>       node_type;
        typedef node_type*                              node_ptr_type;
        typedef const node_type*                        const_node_ptr_type;

        static node_ptr_type        to_node_ptr(pointer ptr)        { return ptr; }
        static const_node_ptr_type  to_node_ptr(const_pointer ptr)  { return ptr; }
    };

    /**
     * Hook for member nodes. The node should be declared public so we can access it
     * directly.
     * Check the lock_free_intrusive_stack \ref AZStdExamples.
     */
    template< class T, lock_free_intrusive_stack_node<T> T::* PtrToMember >
    struct lock_free_intrusive_stack_member_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef lock_free_intrusive_stack_node<T>       node_type;
        typedef node_type*                              node_ptr_type;
        typedef const node_type*                        const_node_ptr_type;

        static node_ptr_type    to_node_ptr(pointer ptr)
        {
            return static_cast<node_ptr_type>(&(ptr->*PtrToMember));
        }

        static const_node_ptr_type to_node_ptr(const_pointer ptr)
        {
            return static_cast<const node_ptr_type>(&(ptr->*PtrToMember));
        }
    };

    /**
     * A lock-free intrusive stack implementation. The stored type must either inherit from
     * lock_free_intrusive_stack_node or include it as a member, and the appropriate hook type must be specified.
     *
     * IMPORTANT NOTE: This stack implementation is vulnerable to the ABA problem. Since the nodes are managed by the
     * user, it is up to the user to ensure that the ABA problem does not occur. DO NOT USE THIS STACK UNLESS YOU
     * THOROUGHLY UNDERSTAND THE ABA PROBLEM, YOU WILL RANDOMLY CORRUPT THE STACK AND CAUSE CRASHES. Use the
     * lock_free_intrusive_stamped_stack instead as it does not suffer from the ABA problem.
     */
    template<typename T, typename Hook>
    class lock_free_intrusive_stack
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

        lock_free_intrusive_stack();

        ~lock_free_intrusive_stack();

        ///Pushes a value onto the top of the stack
        void push(const_reference value);

        ///Attempts to pop a value from the top of the stack. Returns NULL if the stack was empty, otherwise returns
        ///a pointer to the popped value
        pointer pop();

        ///Tests if the stack is empty, limited utility for a concurrent container.
        bool empty() const;

    private:
        //non-copyable
        lock_free_intrusive_stack(const lock_free_intrusive_stack&);
        lock_free_intrusive_stack& operator=(const lock_free_intrusive_stack&);

        atomic<node_type*> m_top;
    };

    //============================================================================================================
    //============================================================================================================
    //============================================================================================================

    template<typename T, typename Hook>
    inline lock_free_intrusive_stack<T, Hook>::lock_free_intrusive_stack()
    {
        m_top.store(NULL, memory_order_release);
    }

    template<typename T, typename Hook>
    inline lock_free_intrusive_stack<T, Hook>::~lock_free_intrusive_stack()
    {
#ifdef AZ_DEBUG_BUILD
        node_type* node = m_top.load(memory_order_acquire);
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
    inline void lock_free_intrusive_stack<T, Hook>::push(const T& value)
    {
        pointer newNode = const_cast<pointer>(&value);
        hook_node_type* newHookNode = Hook::to_node_ptr(newNode);
#ifdef AZ_DEBUG_BUILD
        AZSTD_CONTAINER_ASSERT(!newHookNode->m_next, "Node is already in an intrusive list");
#endif
        exponential_backoff backoff;
        while (true)
        {
            node_type* oldTop = m_top.load(memory_order_acquire);
            newHookNode->m_next = oldTop;
            if (m_top.compare_exchange_weak(oldTop, newNode, memory_order_acq_rel, memory_order_acquire))
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
    inline typename lock_free_intrusive_stack<T, Hook>::pointer lock_free_intrusive_stack<T, Hook>::pop()
    {
        exponential_backoff backoff;
        while (true)
        {
            node_type* oldTop = m_top.load(memory_order_acquire);
            if (!oldTop)
            {
                return NULL;
            }
            hook_node_type* oldHookTop = Hook::to_node_ptr(oldTop);
            node_type* newTop = oldHookTop->m_next;
            if (m_top.compare_exchange_weak(oldTop, newTop, memory_order_acq_rel, memory_order_acquire))
            {
#ifdef AZ_DEBUG_BUILD
                oldHookTop->m_next = NULL;
#endif
                return oldTop;
            }
            backoff.wait();
        }
    }

    template<typename T, typename Hook>
    inline bool lock_free_intrusive_stack<T, Hook>::empty() const
    {
        return (m_top.load(memory_order_acquire) == NULL);
    }
}

