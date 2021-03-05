/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZSTD_INTRUSIVE_SLIST_H
#define AZSTD_INTRUSIVE_SLIST_H 1

#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    /**
     * This is the node you need to include in you objects, if you want to use
     * it in intrusive forward_list. You can do that either by inheriting it
     * or add it as a \b public member. They way you include the node should be in
     * using the appropriate hooks.
     * Check the intrusive_slist \ref AZStdExamples.
     */
    template< class T, int Unused = 0 /*This if left here because of a bug with autoexp.dat parsing. This is VS debugger data visualizer.*/>
    struct intrusive_slist_node
    {
#ifdef AZ_DEBUG_BUILD
        intrusive_slist_node()
            : m_next(0) {}
        ~intrusive_slist_node()
        {
            AZSTD_CONTAINER_ASSERT(m_next == 0, "AZStd::intrusive_slist_node - intrusive forward_list node is being destroyed while still a member of a list!");
        }
#endif
        T* m_next;
    } AZ_MAY_ALIAS; // we access the object in such way that we potentially can break strict aliasing rules

    /**
     * Hook when our object inherits \ref intrusive_slist_node. So the list
     * will use the base class.
     * Check the intrusive_slist \ref AZStdExamples.
     */
    template< class T >
    struct slist_base_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef intrusive_slist_node<T>                 node_type;
        typedef node_type*                              node_ptr_type;
        typedef const node_type*                        const_node_ptr_type;

        static node_ptr_type        to_node_ptr(pointer ptr)        { return ptr; }
        static const_node_ptr_type  to_node_ptr(const_pointer ptr)  { return ptr; }
    };

    /**
     * Hook for member nodes. The node should be declared public so we can access it
     * directly.
     * Check the intrusive_list \ref AZStdExamples.
     */
    template< class T, intrusive_slist_node<T> T::* PtrToMember >
    struct slist_member_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef intrusive_slist_node<T>                 node_type;
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
     * Intrusive forward_list is a AZStd extension container.
     * \par
     * It uses the \ref intrusive_slist_node contained in the provided user type T.
     * You have 2 choices you can make your T type to either inherit from \ref intrusive_slist_node or just have intrusive_slist_node
     * as a \b public member. Based on which option you prefer you should use the appropriate hook type.
     * Hook parameter should be slist_base_hook (if you inherit the \ref intrusive_slist_node) or slist_member_hook if you have it as a public member.
     * Intrusive list never allocate any memory, destroy or create any objects. Just used the provided nodes to store it's information.
     * Generally speaking intrusive containers are better for the cache.
     * You can see examples of all that \ref AZStdExamples.
     */
    template< class T, class Hook >
    class intrusive_slist
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef intrusive_slist<T, Hook>         this_type;
    public:
        typedef T*                              pointer;
        typedef const T*                        const_pointer;

        typedef T&                              reference;
        typedef const T&                        const_reference;
        typedef typename AZStd::size_t          difference_type;
        typedef typename AZStd::size_t          size_type;

        // AZSTD extension.
        typedef T                               value_type;
        typedef T                               node_type;
        typedef node_type*                      node_ptr_type;
        typedef intrusive_slist_node<T>         hook_node_type;
        typedef hook_node_type*                 hook_node_ptr_type;

        /**
         * Constant iterator implementation. Intrusive list
         * uses bidirectional iterators as any double linked list.
         */
        class const_iterator_impl
        {
            enum
            {
                ITERATOR_VERSION = 1
            };

            friend class intrusive_slist;
            typedef const_iterator_impl         this_type;
        public:
            typedef T                           value_type;
            typedef AZStd::ptrdiff_t            difference_type;
            typedef const T*                    pointer;
            typedef const T&                    reference;
            typedef forward_iterator_tag        iterator_category;

            AZ_FORCE_INLINE const_iterator_impl()
                : m_node(0) {}
            AZ_FORCE_INLINE const_iterator_impl(node_ptr_type node)
                : m_node(node) {}
            AZ_FORCE_INLINE const_reference operator*() const   { return *m_node; }
            AZ_FORCE_INLINE pointer operator->() const          { return m_node; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_slist::const_iterator_impl invalid node!");
                m_node = Hook::to_node_ptr(m_node)->m_next;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_slist::const_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = Hook::to_node_ptr(m_node)->m_next;
                return temp;
            }

            AZ_FORCE_INLINE bool operator==(const this_type& rhs) const
            {
                return (m_node == rhs.m_node);
            }

            AZ_FORCE_INLINE bool operator!=(const this_type& rhs) const
            {
                return (m_node != rhs.m_node);
            }
        protected:
            node_ptr_type   m_node;
        };

        /**
         * Iterator implementation.
         */
        class iterator_impl
            : public const_iterator_impl
        {
            typedef iterator_impl       this_type;
            typedef const_iterator_impl base_type;
        public:
            typedef T*                              pointer;
            typedef T&                              reference;

            AZ_FORCE_INLINE iterator_impl() {}
            AZ_FORCE_INLINE iterator_impl(node_ptr_type node)
                : const_iterator_impl(node) {}
            AZ_FORCE_INLINE reference operator*() const { return *base_type::m_node; }
            AZ_FORCE_INLINE pointer operator->() const { return base_type::m_node; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_slist::iterator_impl invalid node!");
                base_type::m_node = Hook::to_node_ptr(base_type::m_node)->m_next;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_slist::iterator_impl invalid node!");
                this_type temp = *this;
                base_type::m_node = Hook::to_node_ptr(base_type::m_node)->m_next;
                return temp;
            }
        };


#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_forward_iterator<iterator_impl, this_type>            iterator;
        typedef Debug::checked_forward_iterator<const_iterator_impl, this_type>      const_iterator;
#else
        typedef iterator_impl                           iterator;
        typedef const_iterator_impl                     const_iterator;
#endif
        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;

        AZ_FORCE_INLINE explicit intrusive_slist()
            : m_numElements(0)
        {
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            headHook->m_next = m_lastNode = head;
        }
        template<class InputIterator>
        AZ_FORCE_INLINE intrusive_slist(const InputIterator& first, const InputIterator& last)
            : m_numElements(0)
        {
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            headHook->m_next = m_lastNode = head;
            insert_after_iter(before_begin(), first, last, is_integral<InputIterator>());
        }

        AZ_FORCE_INLINE ~intrusive_slist()
        {
#ifdef AZ_DEBUG_BUILD
            clear();
#endif
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void assign(const InputIterator& first, const InputIterator& last)
        {
            clear();
            insert(end(), first, last);
        }

        AZ_FORCE_INLINE size_type   size() const        { return m_numElements; }
        AZ_FORCE_INLINE size_type   max_size() const    { return size_type(-1); }
        AZ_FORCE_INLINE bool    empty() const           { return (m_numElements == 0); }

        AZ_FORCE_INLINE iterator begin()
        {
            hook_node_ptr_type headHook = Hook::to_node_ptr(get_head());
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, headHook->m_next));
        }
        AZ_FORCE_INLINE const_iterator begin() const
        {
            const hook_node_type* headHook = Hook::to_node_ptr(get_head());
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, headHook->m_next));
        }
        AZ_FORCE_INLINE iterator end()
        {
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, const_cast<this_type&>(*this).get_head()));
        }
        AZ_FORCE_INLINE const_iterator end() const
        {
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<this_type&>(*this).get_head()));
        }
        AZ_FORCE_INLINE iterator before_begin()
        {
            node_ptr_type head = get_head();
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, head));
        }
        AZ_FORCE_INLINE const_iterator before_begin() const
        {
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<this_type&>(*this).get_head()));
        }
        AZ_FORCE_INLINE iterator previous(iterator iter)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type iNode = iter.get_iterator().m_node;
#else
            node_ptr_type iNode = iter.m_node;
#endif
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, previous(iNode)));
        }
        AZ_FORCE_INLINE const_iterator previous(const_iterator iter)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type iNode = iter.get_iterator().m_node;
#else
            node_ptr_type iNode = iter.m_node;
#endif
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, previous(iNode)));
        }
        AZ_FORCE_INLINE iterator last()                     { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, m_lastNode)); }
        AZ_FORCE_INLINE const_iterator last() const         { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<node_ptr_type>(m_lastNode))); }

        AZ_FORCE_INLINE reference front()                   { return *begin(); }
        AZ_FORCE_INLINE const_reference front() const       { return *begin(); }
        AZ_FORCE_INLINE reference back()                        { return *last(); }
        AZ_FORCE_INLINE const_reference back() const            { return *last(); }

        AZ_FORCE_INLINE void push_front(const_reference value)  { insert_after(before_begin(), value); }
        AZ_FORCE_INLINE void pop_front()                        { erase_after(before_begin()); }
        AZ_FORCE_INLINE void push_back(const_reference value)   { insert_after(last(), value);  }

        AZ_INLINE iterator insert(iterator insertPos, const_reference value)
        {
            return insert_after(previous(insertPos), value);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void insert(iterator insertPos, InputIterator first, InputIterator last)
        {
            insert_after_iter(previous(insertPos), first, last, is_integral<InputIterator>());
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void insert_after(iterator insertPos, InputIterator first, InputIterator last)
        {
            insert_after_iter(insertPos, first, last, is_integral<InputIterator>());
        }

        AZ_INLINE iterator insert_after(iterator insertPos, const_reference value)
        {
            pointer node = const_cast<pointer>(&value);
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
#ifdef AZ_DEBUG_BUILD
            AZSTD_CONTAINER_ASSERT(nodeHook->m_next == 0, "AZStd::intrusive_slist::insert_after - this node is already in a list, erase it first.");
#endif
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type insNode = insertPos.get_iterator().m_node;
#else
            node_ptr_type insNode = insertPos.m_node;
#endif
            hook_node_ptr_type insertPosHook = Hook::to_node_ptr(insNode);

            if (m_lastNode == insNode)
            {
                m_lastNode = node;
            }

            nodeHook->m_next        = insertPosHook->m_next;
            insertPosHook->m_next   = node;

            ++m_numElements;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, node));
        }

        inline iterator erase_after(const_reference value)
        {
            node_ptr_type prevNode = const_cast<node_ptr_type>(&value);
            node_ptr_type node = Hook::to_node_ptr(prevNode)->m_next;

            hook_node_ptr_type prevNodeHook = Hook::to_node_ptr(prevNode);
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
#ifdef AZ_DEBUG_BUILD
            AZSTD_CONTAINER_ASSERT(nodeHook->m_next != 0, "AZStd::intrusive_slist::erase_after - this node is not in a list.");
#endif
            prevNodeHook->m_next = nodeHook->m_next;
#ifdef AZ_DEBUG_BUILD
            nodeHook->m_next = 0;
#endif
            if (m_lastNode == node)
            {
                m_lastNode = prevNode;
            }

            --m_numElements;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, prevNodeHook->m_next));
        }
        inline iterator erase_after(iterator toEraseNext)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type prevNode = toEraseNext.get_iterator().m_node;
            ++toEraseNext; // This will validate the operation.
            node_ptr_type node = toEraseNext.get_iterator().m_node;
            orphan_node(node);
#else
            AZSTD_CONTAINER_ASSERT(toEraseNext.m_node != 0, "AZStd::intrusive_slist::erase_after - invalid node!");
            node_ptr_type prevNode = toEraseNext.m_node;
            node_ptr_type node = Hook::to_node_ptr(prevNode)->m_next;
#endif
            hook_node_ptr_type prevNodeHook = Hook::to_node_ptr(prevNode);
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
#ifdef AZ_DEBUG_BUILD
            AZSTD_CONTAINER_ASSERT(nodeHook->m_next != 0, "AZStd::intrusive_slist::erase_after - this node is not in a list.");
#endif
            prevNodeHook->m_next = nodeHook->m_next;
#ifdef AZ_DEBUG_BUILD
            nodeHook->m_next = 0;
#endif
            if (m_lastNode == node)
            {
                m_lastNode = prevNode;
            }

            --m_numElements;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, prevNodeHook->m_next));
        }

        inline iterator erase_after(iterator prevFirst, iterator last)
        {
            iterator first = prevFirst;
            ++first; // to the first element to delete
            while (first != last)
            {
                first = erase_after(prevFirst);
            }

            return last;
        }

        AZ_FORCE_INLINE iterator erase(const_reference value)
        {
            return erase_after(previous(value));
        }
        AZ_FORCE_INLINE iterator erase(const_iterator toErase)
        {
            return erase_after(previous(iterator(const_cast<pointer>(&*toErase))));
        }

        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)
        {
            iterator prevFirst = previous(AZStd::Internal::ConstIteratorCast<iterator>(first));
            return erase_after(prevFirst, AZStd::Internal::ConstIteratorCast<iterator>(last));
        }

        AZ_FORCE_INLINE void clear()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
#ifdef AZ_DEBUG_BUILD
            node_ptr_type cur = headHook->m_next;
            while (cur != head)
            {
                hook_node_ptr_type curHook = Hook::to_node_ptr(cur);
                cur = curHook->m_next;
                curHook->m_next = 0;
            }
#endif
            headHook->m_next = m_lastNode = head;
            m_numElements = 0;
        }

        void swap(this_type& rhs)
        {
            if (this == &rhs)
            {
                return;               // already done
            }
            node_ptr_type headNode = get_head();
            node_ptr_type rhsHeadNode = rhs.get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(headNode);
            hook_node_ptr_type rhsHeadHook = Hook::to_node_ptr(rhsHeadNode);

            if (m_numElements == 0)
            {
                if (rhs.m_numElements == 0)
                {
                    return;
                }

                node_ptr_type rhsFirst = rhsHeadHook->m_next;
                node_ptr_type rhsLast = rhs.m_lastNode;

                Hook::to_node_ptr(rhsLast)->m_next = headNode;
                headHook->m_next = rhsFirst;

                m_lastNode = rhs.m_lastNode;
                rhsHeadHook->m_next = rhs.m_lastNode = rhsHeadNode;
            }
            else
            {
                if (rhs.m_numElements == 0)
                {
                    node_ptr_type first = headHook->m_next;
                    node_ptr_type last = m_lastNode;

                    Hook::to_node_ptr(last)->m_next = rhsHeadNode;
                    rhsHeadHook->m_next = first;

                    rhs.m_lastNode = m_lastNode;
                    headHook->m_next = m_lastNode = headNode;
                }
                else
                {
                    node_ptr_type rhsFirst = rhsHeadHook->m_next;
                    node_ptr_type rhsLast = rhs.m_lastNode;

                    node_ptr_type first = headHook->m_next;
                    node_ptr_type last = m_lastNode;

                    Hook::to_node_ptr(rhsLast)->m_next = headNode;
                    headHook->m_next = rhsFirst;

                    Hook::to_node_ptr(last)->m_next = rhsHeadNode;
                    rhsHeadHook->m_next = first;

                    AZStd::swap(m_lastNode, rhs.m_lastNode);
                }
            }

            AZStd::swap(m_numElements, rhs.m_numElements);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            swap_all(rhs);
#endif
        }

        inline void splice(iterator splicePos, this_type& rhs)
        {
            splice_after(previous(splicePos), rhs);
        }

        inline void splice(iterator splicePos, this_type& rhs, iterator first)
        {
            splice_after(previous(splicePos), rhs, first);
        }
        inline void splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        {
            splice_after(previous(splicePos), rhs, first, last);
        }

        void splice_after(iterator splicePos, this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::intrusive_slist::splice_after - splicing it self!");
            AZSTD_CONTAINER_ASSERT(rhs.m_numElements > 0, "AZSTD::intrusive_slist::splice_after - No elements to splice!");

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type firstNode = rhs.begin().get_iterator().m_node;
            node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
            rhs.orphan_all();
#else
            node_ptr_type firstNode = rhs.begin().m_node;
            node_ptr_type splicePosNode = splicePos.m_node;
#endif

            m_numElements += rhs.m_numElements;
            rhs.m_numElements = 0;

            // get the last valid node
            node_ptr_type lastNode = rhs.m_lastNode;

            Hook::to_node_ptr(lastNode)->m_next = Hook::to_node_ptr(splicePosNode)->m_next;
            Hook::to_node_ptr(splicePosNode)->m_next = firstNode;

            if (m_lastNode == splicePosNode)
            {
                m_lastNode = rhs.m_lastNode;
            }

            Hook::to_node_ptr(reinterpret_cast<node_ptr_type>(&rhs.m_root))->m_next = rhs.m_lastNode = reinterpret_cast<node_ptr_type>(&rhs.m_root);
        }

        void splice_after(iterator splicePos, this_type& rhs, iterator first)
        {
            AZSTD_CONTAINER_ASSERT(first != rhs.end(), "AZSTD::intrusive_slist::splice_after invalid iterator!");
            iterator last = first;
            ++last;
            if (splicePos == first || splicePos == last)
            {
                return;
            }

            m_numElements += 1;
            rhs.m_numElements -= 1;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type firstNode = first.get_iterator().m_node;
            node_ptr_type lastNode = last.get_iterator().m_node;
            node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
            if (this != &rhs)  // only if we actually moving nodes, from different lists.
            {
                for (iterator next = first; next != last; )
                {
                    node_ptr_type node = next.get_iterator().m_node;
                    ++next;
                    rhs.orphan_node(node);
                }
            }
#else
            node_ptr_type firstNode = first.m_node;
            node_ptr_type lastNode = last.m_node;
            node_ptr_type splicePosNode = splicePos.m_node;
#endif
            node_ptr_type prevFirst = rhs.previous(firstNode);

            if (rhs.m_lastNode == firstNode)
            {
                rhs.m_lastNode = prevFirst;
            }

            Hook::to_node_ptr(prevFirst)->m_next = lastNode;
            Hook::to_node_ptr(firstNode)->m_next = Hook::to_node_ptr(splicePosNode)->m_next;
            Hook::to_node_ptr(splicePosNode)->m_next = firstNode;

            if (m_lastNode == splicePosNode)
            {
                m_lastNode = firstNode;
            }
        }

        void splice_after(iterator splicePos, this_type& rhs, iterator first, iterator last)
        {
            AZSTD_CONTAINER_ASSERT(first != last, "AZSTD::intrusive_slist::splice_after - no elements for splice");
            AZSTD_CONTAINER_ASSERT(this != &rhs || splicePos != last, "AZSTD::intrusive_slist::splice_after invalid iterators");

            if (this != &rhs) // otherwise we are just rearranging the list
            {
                if (first == rhs.begin() && last == rhs.end())  // optimization for the whole list
                {
                    splice_after(splicePos, rhs);
                    return;
                }
            }

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type firstNode = first.get_iterator().m_node;
            node_ptr_type lastNode = last.get_iterator().m_node;
            node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
            if (this != &rhs)  // only if we actually moving nodes, from different lists.
            {
                for (iterator next = first; next != last; )
                {
                    node_ptr_type node = next.get_iterator().m_node;
                    ++next;
                    rhs.orphan_node(node);
                }
            }
#else
            node_ptr_type firstNode = first.m_node;
            node_ptr_type lastNode = last.m_node;
            node_ptr_type splicePosNode = splicePos.m_node;
#endif

            node_ptr_type preLastNode = firstNode;
            size_type numElements  = 1;  // 1 for the first element
            while (Hook::to_node_ptr(preLastNode)->m_next != lastNode)
            {
                preLastNode = Hook::to_node_ptr(preLastNode)->m_next;
                ++numElements;
                AZSTD_CONTAINER_ASSERT(numElements <= rhs.m_numElements, "AZSTD::intrusive_slist::splice_after rhs container is corrupted!");
            }

            node_ptr_type prevFirstNode = rhs.previous(firstNode);

            if (rhs.m_lastNode == preLastNode)
            {
                rhs.m_lastNode = prevFirstNode;
            }

            // if the container is the same this is pointless... but is very fast.
            m_numElements += numElements;
            rhs.m_numElements -= numElements;

            Hook::to_node_ptr(prevFirstNode)->m_next = lastNode;

            Hook::to_node_ptr(preLastNode)->m_next = Hook::to_node_ptr(splicePosNode)->m_next;
            Hook::to_node_ptr(splicePosNode)->m_next = firstNode;

            if (m_lastNode == splicePosNode)
            {
                m_lastNode = preLastNode;
            }
        }

        inline void remove(const_reference value)
        {
            // Different STL implementations handle this in a different way.
            // The question is do we need the copy of the value ? If the value passed
            // is in the list and get destroyed in the middle of the remove this will cause a crash.
            // AZSTD doesn't handle this since we prefer speed, it's used responsibility to make sure
            // this is not the case. We can add assert in some FULL STL DEBUG MODE.
            if (!empty())
            {
                iterator prevFirst = before_begin();
                iterator first = begin();
                iterator last = end();
                do
                {
                    if (value == *first)
                    {
                        first = erase_after(prevFirst);
                    }
                    else
                    {
                        prevFirst = first;
                        ++first;
                    }
                }
                while (first != last);
            }
        }
        template <class Predicate>
        inline void remove_if(Predicate pred)
        {
            if (!empty())
            {
                iterator prevFirst = before_begin();
                iterator first = begin();
                iterator last = end();

                do
                {
                    if (pred(*first))
                    {
                        first = erase_after(prevFirst);
                    }
                    else
                    {
                        prevFirst = first;
                        ++first;
                    }
                }
                while (first != last);
            }
        }
        inline void unique()
        {
            if (m_numElements >= 2)
            {
                iterator first = begin();
                iterator last = end();
                iterator next = first;
                ++next;
                while (next != last)
                {
                    if (*first == *next)
                    {
                        next = erase_after(first);
                    }
                    else
                    {
                        first = next++;
                    }
                }
            }
        }
        template <class BinaryPredicate>
        inline void unique(BinaryPredicate compare)
        {
            if (m_numElements >= 2)
            {
                iterator first = begin();
                iterator last = end();
                iterator next = first;
                ++next;
                while (next != last)
                {
                    if (compare(*first, *next))
                    {
                        next = erase_after(first);
                    }
                    else
                    {
                        first = next++;
                    }
                }
            }
        }
        AZ_FORCE_INLINE void merge(this_type& rhs)
        {
            merge(rhs, AZStd::less<value_type>());
        }
        template <class Compare>
        void merge(this_type& rhs, Compare compare)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::forward_list::merge - can not merge itself!");

            iterator prevIter1 = before_begin();
            iterator iter1 = begin();
            iterator end1 = end();

            // \todo check that both lists are ordered
            while (iter1 != end1 && !rhs.empty())
            {
                if (compare(rhs.front(), *iter1))
                {
                    //AZSTD_CONTAINER_ASSERT(!compare(*first1, rhs.front()),("AZStd::forward_list::merge invalid predicate!"));
                    splice_after(prevIter1, rhs, /*rhs.before_begin()*/ rhs.begin());
                }
                else
                {
                    ++iter1;
                }

                ++prevIter1;
            }
            if (!rhs.empty())
            {
                splice_after(prevIter1, rhs);
            }
        }
        AZ_FORCE_INLINE void sort()
        {
            sort(AZStd::less<value_type>());
        }
        template <class Compare>
        void sort(Compare comp)
        {
            if (m_numElements >= 2)
            {
                //////////////////////////////////////////////////////////////////////////
                // note for now we will use the Dinkumware version till we have all of our algorithms are up and running.
                // worth sorting, do it
                const size_t _MAXBINS = 25;
                this_type _Templist, _Binlist[_MAXBINS + 1];
                size_t _Maxbin = 0;

                while (!empty())
                {
                    // sort another element, using bins
                    _Templist.splice_after(_Templist.before_begin(), *this, begin(), ++begin() /*, 1, true*/);    // don't invalidate iterators

                    size_t _Bin;
                    for (_Bin = 0; _Bin < _Maxbin && !_Binlist[_Bin].empty(); ++_Bin)
                    {   // merge into ever larger bins
                        _Binlist[_Bin].merge(_Templist, comp);
                        _Binlist[_Bin].swap(_Templist);
                    }

                    if (_Bin == _MAXBINS)
                    {
                        _Binlist[_Bin - 1].merge(_Templist, comp);
                    }
                    else
                    {   // spill to new bin, while they last
                        _Binlist[_Bin].swap(_Templist);
                        if (_Bin == _Maxbin)
                        {
                            ++_Maxbin;
                        }
                    }
                }

                for (size_t _Bin = 1; _Bin < _Maxbin; ++_Bin)
                {
                    _Binlist[_Bin].merge(_Binlist[_Bin - 1], comp);  // merge up
                }
                splice_after(before_begin(), _Binlist[_Maxbin - 1]);    // result in last bin
                //////////////////////////////////////////////////////////////////////////
            }
        }
        inline void reverse()
        {
            if (m_numElements > 1)
            {
                node_ptr_type headNode = get_head();
                node_ptr_type next = Hook::to_node_ptr(headNode)->m_next;
                node_ptr_type end = headNode;
                m_lastNode = next;
                node_ptr_type node = Hook::to_node_ptr(next)->m_next;
                Hook::to_node_ptr(next)->m_next = end;
                while (node != end)
                {
                    node_ptr_type temp = Hook::to_node_ptr(node)->m_next;
                    Hook::to_node_ptr(node)->m_next = next;
                    next = node;
                    node = temp;
                }
                Hook::to_node_ptr(headNode)->m_next = next;
            }
        }


        // Validate container status.
        inline bool     validate() const
        {
            // \noto \todo traverse the list and make sure it's properly linked.
            return true;
        }
        // Validate iterator.
        inline int      validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            node_ptr_type iterNode = iter.m_iter.m_node;
#else
            node_ptr_type iterNode = iter.m_node;
#endif

            if ((void*)iterNode == (void*)&m_root)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        inline int      validate_iterator(const iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            node_ptr_type iterNode = iter.m_iter.m_node;
#else
            node_ptr_type iterNode = iter.m_node;
#endif
            if ((void*)iterNode == (void*)&m_root)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        /// Reset the container without going to unhook any elements.
        inline void     leak_and_reset()
        {
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            headHook->m_next = headHook->m_prev = head;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

    protected:
        // Non-copyable and non-moveable
        AZ_FORCE_INLINE intrusive_slist(const this_type& rhs);
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs);

        /**
        * The iterator to the element before i in the list.
        * Returns the end-iterator, if either i is the begin-iterator or the list empty.
        */
        inline node_ptr_type previous(node_ptr_type iNode)
        {
            node_ptr_type node = get_head();
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);

            if (iNode == node)
            {
                return m_lastNode;
            }

            while (nodeHook->m_next != iNode)
            {
                node = nodeHook->m_next;
                // This check makes sense for the linear forward_list only.
                AZSTD_CONTAINER_ASSERT(node != 0, "This node it's not in the list or the list is corrupted!");
                nodeHook = Hook::to_node_ptr(node);
            }
            return node;
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void    insert_after_iter(iterator insertPos, const InputIterator& first, const InputIterator& last, const true_type& /* is_integral<InputIterator> */)
        {
            insert_after(insertPos, (size_type)first, (value_type)last);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void    insert_after_iter(iterator insertPos, const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            InputIterator iter(first);
            for (; iter != last; ++iter, ++insertPos)
            {
                insert_after(insertPos, *iter);
            }
        }

        node_ptr_type       m_lastNode;         ///< Cached last valid node.
        size_type           m_numElements;
        /**
         * OK we face different problems when we implement intrusive list. We choose to create fake node_type as a root node!
         * Why ? There are 2 general ways to solve this issue:
         * 1. Store the head and tail pointer here and store pointer to the list in the iterator. This makes iterator 2 size,
         * plus operators -- and ++ include an if. This is the case because we need end iterator (which will be NULL). And we can't move it.
         * 2. We can store the base class in the iterator, and every time we need the value we compute the hook offset and recast the pointer.
         * all of this is fine, but we need to have a "simple" way to debug our containers" in a easy way.
         * So at this stage we consider the wasting a memory for a fake root node as the best solution, while we can debug the container.
         * This can change internally at any moment if needed, no interface change will occur.
         */
        typename aligned_storage<sizeof(node_type), alignment_of<node_type>::value>::type    m_root;

        inline node_ptr_type get_head() { return reinterpret_cast<node_ptr_type>(&m_root); }

        // Debug
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        AZ_INLINE void orphan_node(node_ptr_type node) const
        {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "list::orphan_node - iterator was corrupted!");
                node_ptr_type nodePtr = static_cast<iterator*>(iter)->m_iter.m_node;

                if (nodePtr == node)
                {
                    // orphan the iterator
                    iter->m_container = 0;

                    if (iter->m_prevIterator)
                    {
                        iter->m_prevIterator->m_nextIterator = iter->m_nextIterator;
                    }
                    else
                    {
                        m_iteratorList = iter->m_nextIterator;
                    }

                    if (iter->m_nextIterator)
                    {
                        iter->m_nextIterator->m_prevIterator = iter->m_prevIterator;
                    }
                }

                iter = iter->m_nextIterator;
            }
        }
#endif
    };

    template< class T, class Hook >
    AZ_FORCE_INLINE bool operator==(const intrusive_slist<T, Hook>& left, const intrusive_slist<T, Hook>& right)
    {
        return &left == &right;
    }

    template< class T, class Hook >
    AZ_FORCE_INLINE bool operator!=(const intrusive_slist<T, Hook>& left, const intrusive_slist<T, Hook>& right)
    {
        return !(&left == &right);
    }
}

#endif // AZSTD_INTRUSIVE_SLIST_H
#pragma once
