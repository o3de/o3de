/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_INTRUSIVE_LIST_H
#define AZSTD_INTRUSIVE_LIST_H 1

#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    /**
     * This is the node you need to include in you objects, if you want to use
     * it in intrusive list. You can do that either by inheriting it
     * or add it as a \b public member. They way you include the node should be in
     * using the appropriate hooks.
     * Check the intrusive_list \ref AZStdExamples.
     */
    template <class T>
    struct intrusive_list_node
    {
#ifdef AZ_DEBUG_BUILD
        ~intrusive_list_node()
        {
            AZSTD_CONTAINER_ASSERT(m_prev == nullptr && m_next == nullptr, "AZStd::intrusive_list_node - intrusive list node is being destroyed while still a member of a list!");
        }
#endif
        T* m_next = nullptr;
        T* m_prev = nullptr;
    } AZ_MAY_ALIAS; // we access the object in such way that we potentially can break strict aliasing rules

    /**
     * Hook when our object inherits \ref intrusive_list_node. So the list
     * will use the base class.
     * Check the intrusive_list \ref AZStdExamples.
     */
    template< class T >
    struct list_base_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef intrusive_list_node<T>                  node_type;
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
    template< class T, intrusive_list_node<T> T::* PtrToMember >
    struct list_member_hook
    {
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;
        typedef intrusive_list_node<T>                  node_type;
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
     * Intrusive list is a AZStd extension container.
     * \par
     * It uses the \ref intrusive_list_node contained in the provided user type T.
     * You have 2 choices you can make your T type to either inherit from \ref intrusive_list_node or just have intrusive_list_node
     * as a \b public member. Based on which option you prefer you should use the appropriate hook type.
     * Hook parameter should be list_base_hook (if you inherit the \ref intrusive_list_node) or list_member_hook if you have it as a public member.
     * Intrusive list never allocate any memory, destroy or create any objects. Just used the provided nodes to store it's information.
     * Generally speaking intrusive containers are better for the cache.
     * You can see examples of all that \ref AZStdExamples.
     */
    template< class T, class Hook >
    class intrusive_list
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef intrusive_list<T, Hook>          this_type;
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
        typedef node_ptr_type                   const_node_ptr_type;
        typedef intrusive_list_node<T>          hook_node_type;
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

            friend class intrusive_list;
            typedef const_iterator_impl         this_type;
        public:
            typedef T                           value_type;
            typedef AZStd::ptrdiff_t            difference_type;
            typedef const T*                    pointer;
            typedef const T&                    reference;
            typedef bidirectional_iterator_tag  iterator_category;

            AZ_FORCE_INLINE const_iterator_impl()
                : m_node(0) {}
            AZ_FORCE_INLINE const_iterator_impl(node_ptr_type node)
                : m_node(node) {}
            AZ_FORCE_INLINE const_reference operator*() const   { return *m_node; }
            AZ_FORCE_INLINE pointer operator->() const          { return m_node; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                m_node = Hook::to_node_ptr(m_node)->m_next;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = Hook::to_node_ptr(m_node)->m_next;
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                m_node = Hook::to_node_ptr(m_node)->m_prev;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::intrusive_list::const_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = Hook::to_node_ptr(m_node)->m_prev;
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
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                base_type::m_node = Hook::to_node_ptr(base_type::m_node)->m_next;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                this_type temp = *this;
                base_type::m_node = Hook::to_node_ptr(base_type::m_node)->m_next;
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                base_type::m_node = Hook::to_node_ptr(base_type::m_node)->m_prev;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::intrusive_list::iterator_impl invalid node!");
                this_type temp = *this;
                base_type::m_node = Hook::to_node_ptr(base_type::m_node)->m_prev;
                return temp;
            }
        };

        /**
         * Constant reverse iterator implementation. Intrusive list
         * uses bidirectional iterators as any double linked list.
         */
        template<typename Iter>
        class reverse_iterator_impl
        {
            friend class intrusive_list;
            typedef reverse_iterator_impl this_type;
        public:
            using value_type = typename AZStd::iterator_traits<Iter>::value_type;
            using difference_type = typename AZStd::iterator_traits<Iter>::difference_type;
            using pointer = typename AZStd::iterator_traits<Iter>::pointer;
            using reference = typename AZStd::iterator_traits<Iter>::reference;
            using iterator_category = typename AZStd::iterator_traits<Iter>::iterator_category;
            using iterator_type = Iter;

            AZ_FORCE_INLINE reverse_iterator_impl()
                : m_node(nullptr) {}
            AZ_FORCE_INLINE reverse_iterator_impl(node_ptr_type node)
                : m_node(node) {}

            iterator_type base() const
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_list::reverse_iterator_impl base invoked on invalid node!");
                return iterator_type(Hook::to_node_ptr(m_node)->m_next);
            }
            AZ_FORCE_INLINE reference operator*() const { return *m_node; }
            AZ_FORCE_INLINE pointer operator->() const { return m_node; }

            AZ_FORCE_INLINE this_type& operator++()
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_list::reverse_iterator_impl invalid node!");
                m_node = Hook::to_node_ptr(m_node)->m_prev;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator++(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_list::reverse_iterator_impl invalid node!");
                this_type temp = *this;
                m_node = Hook::to_node_ptr(m_node)->m_prev;
                return temp;
            }

            AZ_FORCE_INLINE this_type& operator--()
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_list::reverse_iterator_impl invalid node!");
                m_node = Hook::to_node_ptr(m_node)->m_next;
                return *this;
            }

            AZ_FORCE_INLINE this_type operator--(int)
            {
                AZSTD_CONTAINER_ASSERT(m_node != nullptr, "AZStd::intrusive_list::reverse_iterator_impl invalid node!");
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

#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_bidirectional_iterator<iterator_impl, this_type>                 iterator;
        typedef Debug::checked_bidirectional_iterator<const_iterator_impl, this_type>           const_iterator;
        typedef Debug::checked_bidirectional_iterator<reverse_iterator_impl<iterator>, this_type>         reverse_iterator;
        typedef Debug::checked_bidirectional_iterator<reverse_iterator_impl<const_iterator>, this_type>   const_reverse_iterator;
#else
        typedef iterator_impl                           iterator;
        typedef const_iterator_impl                     const_iterator;
        typedef reverse_iterator_impl<iterator>         reverse_iterator;
        typedef reverse_iterator_impl<const_iterator>   const_reverse_iterator;
#endif

        AZ_FORCE_INLINE explicit intrusive_list()
            : m_numElements(0)
        {
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            headHook->m_next = headHook->m_prev = head;
        }
        template<class InputIterator>
        AZ_FORCE_INLINE intrusive_list(const InputIterator& first, const InputIterator& last)
            : m_numElements(0)
        {
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            headHook->m_next = headHook->m_prev = head;
            insert(begin(), first, last);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // We can completely forbit copy/assign operators once we fully switch for containers that requre movable only.
        AZ_FORCE_INLINE intrusive_list(const this_type& rhs)
            : m_numElements(0)
        {
            (void)rhs;
            AZ_Assert(rhs.m_numElements == 0, "You can't copy an intrusive linked list, use move semantics!");
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            headHook->m_next = headHook->m_prev = head;
        }
        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            (void)rhs;
            AZ_Assert(rhs.m_numElements == 0, "You can't copy an intrusive linked list, use move semantics!");
            return *this;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        AZ_FORCE_INLINE intrusive_list(this_type&& rhs)
        {
            assign_rv(AZStd::forward<this_type>(rhs));
        }

        this_type& operator=(this_type&& rhs)
        {
            assign_rv(AZStd::forward<this_type>(rhs));
            return *this;
        }

        void assign_rv(this_type&& rhs)
        {
            if (this == &rhs)
            {
                return;
            }

            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            if (rhs.m_numElements == 0)
            {
                headHook->m_next = headHook->m_prev = head;
                m_numElements = 0;
            }
            else
            {
                node_ptr_type rhsHead = rhs.get_head();
                hook_node_ptr_type rhsHeadHook = Hook::to_node_ptr(rhsHead);

                Hook::to_node_ptr(rhsHeadHook->m_next)->m_prev = head;
                Hook::to_node_ptr(rhsHeadHook->m_prev)->m_next = head;
                headHook->m_next = rhsHeadHook->m_next;
                headHook->m_prev = rhsHeadHook->m_prev;
                m_numElements = rhs.m_numElements;

                rhsHeadHook->m_next = rhsHeadHook->m_prev = rhsHead;
                rhs.m_numElements = 0;
            }
        }

        void swap(this_type&& rhs)
        {
            assign_rv(AZStd::forward(rhs));
        }

        AZ_FORCE_INLINE ~intrusive_list()
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
            const hook_node_type* headHook = Hook::to_node_ptr(const_cast<this_type&>(*this).get_head());
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, headHook->m_next));
        }
        AZ_FORCE_INLINE iterator end()
        {
            node_ptr_type head = get_head();
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, head));
        }
        AZ_FORCE_INLINE const_iterator end() const
        {
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<this_type&>(*this).get_head()));
        }
        AZ_FORCE_INLINE reverse_iterator rbegin()
        { 
            hook_node_ptr_type headHook = Hook::to_node_ptr(get_head());
            return reverse_iterator(AZSTD_CHECKED_ITERATOR(reverse_iterator<iterator>, headHook->m_prev));
        }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const
        { 
            const hook_node_type* headHook = Hook::to_node_ptr(const_cast<this_type&>(*this).get_head());
            return const_reverse_iterator(AZSTD_CHECKED_ITERATOR(reverse_iterator<const_iterator>, headHook->m_prev));
        }
        const_reverse_iterator crbegin() const
        {
            const hook_node_type* headHook = Hook::to_node_ptr(const_cast<this_type&>(*this).get_head());
            return const_reverse_iterator(AZSTD_CHECKED_ITERATOR(reverse_iterator<const_iterator>, headHook->m_prev));
        }
        AZ_FORCE_INLINE reverse_iterator rend()
        { 
            node_ptr_type head = get_head();
            return reverse_iterator(AZSTD_CHECKED_ITERATOR(reverse_iterator<iterator>, head));
        }
        AZ_FORCE_INLINE const_reverse_iterator rend() const
        { 
            return const_reverse_iterator(AZSTD_CHECKED_ITERATOR(reverse_iterator<const_iterator>, const_cast<this_type&>(*this).get_head()));
        }
        const_reverse_iterator crend() const
        {
            return const_reverse_iterator(AZSTD_CHECKED_ITERATOR(reverse_iterator<const_iterator>, const_cast<this_type&>(*this).get_head()));
        }

        AZ_FORCE_INLINE reference front()                       { return (*begin()); }
        AZ_FORCE_INLINE const_reference front() const           { return (*begin()); }
        AZ_FORCE_INLINE reference back()                        { return (*(--end())); }
        AZ_FORCE_INLINE const_reference back() const            { return (*(--end())); }

        AZ_FORCE_INLINE void push_front(const_reference value)  { insert(begin(), value); }
        AZ_FORCE_INLINE void pop_front()                        { erase(begin()); }
        AZ_FORCE_INLINE void push_back(const_reference value)   { insert(end(), value); }
        AZ_FORCE_INLINE void pop_back()                         { erase(--end()); }

        AZ_INLINE iterator insert(iterator insertPos, const_reference value)
        {
            pointer node = const_cast<pointer>(&value);
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
#ifdef AZ_DEBUG_BUILD
            AZSTD_CONTAINER_ASSERT(nodeHook->m_prev == 0 && nodeHook->m_next == 0, "AZStd::intrusive_list::insert - this node is already in a list, erase it first.");
#endif
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type insNode = insertPos.get_iterator().m_node;
#else
            node_ptr_type insNode = insertPos.m_node;
#endif
            hook_node_ptr_type insertPosHook = Hook::to_node_ptr(insNode);

            nodeHook->m_next = insNode;
            nodeHook->m_prev = insertPosHook->m_prev;
            insertPosHook->m_prev = node;
            Hook::to_node_ptr(nodeHook->m_prev)->m_next = node;

            ++m_numElements;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, node));
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void insert(iterator insertPos, InputIterator first, InputIterator last)
        {
            for (; first != last; ++first)
            {
                insert(insertPos, *first);
            }
        }

        inline iterator erase(const_reference value)
        {
            pointer node = const_cast<pointer>(&value);
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
#ifdef AZ_DEBUG_BUILD
            AZSTD_CONTAINER_ASSERT(nodeHook->m_prev != 0 && nodeHook->m_next != 0, "AZStd::intrusive_list::erase - this node is not in a list.");
#endif
            node_ptr_type prevNode = nodeHook->m_prev;
            node_ptr_type nextNode = nodeHook->m_next;
            Hook::to_node_ptr(prevNode)->m_next = nextNode;
            Hook::to_node_ptr(nextNode)->m_prev = prevNode;
            nodeHook->m_next = nullptr;
            nodeHook->m_prev = nullptr;

            --m_numElements;
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, nextNode));
        }
        inline iterator erase(const_iterator toErase)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type node = toErase.get_iterator().m_node;
            orphan_node(node);
#else
            node_ptr_type node = toErase.m_node;
#endif
            hook_node_ptr_type nodeHook = Hook::to_node_ptr(node);
#ifdef AZ_DEBUG_BUILD
            AZSTD_CONTAINER_ASSERT(nodeHook->m_prev != 0 && nodeHook->m_next != 0, "AZStd::intrusive_list::erase - this node is not in a list.");
#endif
            node_ptr_type prevNode = nodeHook->m_prev;
            node_ptr_type nextNode = nodeHook->m_next;
            Hook::to_node_ptr(prevNode)->m_next = nextNode;
            Hook::to_node_ptr(nextNode)->m_prev = prevNode;
            nodeHook->m_next = nullptr;
            nodeHook->m_prev = nullptr;

            --m_numElements;
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, nextNode));
        }

        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)
        {
            while (first != last)
            {
                erase(first++);
            }
            return AZStd::Internal::ConstIteratorCast<iterator>(last);
        }

        inline void clear()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            node_ptr_type head = get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);

            node_ptr_type cur = headHook->m_next;
            while (cur != head)
            {
                hook_node_ptr_type curHook = Hook::to_node_ptr(cur);
                cur = curHook->m_next;
                curHook->m_next = curHook->m_prev = 0;
            }

            headHook->m_next = headHook->m_prev = head;
            m_numElements = 0;
        }

        void swap(this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZStd::intrusive_list::swap - it's pointless to swap the list itself!");
            node_ptr_type head = get_head();
            node_ptr_type rhsHead = rhs.get_head();
            hook_node_ptr_type headHook = Hook::to_node_ptr(head);
            hook_node_ptr_type rhsHeadHook = Hook::to_node_ptr(rhsHead);

            if (m_numElements == 0)
            {
                if (rhs.m_numElements == 0)
                {
                    return;
                }
                Hook::to_node_ptr(rhsHeadHook->m_next)->m_prev = head;
                Hook::to_node_ptr(rhsHeadHook->m_prev)->m_next = head;
                headHook->m_next = rhsHeadHook->m_next;
                headHook->m_prev = rhsHeadHook->m_prev;
                rhsHeadHook->m_next = rhsHeadHook->m_prev = rhsHead;
            }
            else
            {
                if (rhs.m_numElements == 0)
                {
                    Hook::to_node_ptr(headHook->m_next)->m_prev = rhsHead;
                    Hook::to_node_ptr(headHook->m_prev)->m_next = rhsHead;
                    rhsHeadHook->m_next = headHook->m_next;
                    rhsHeadHook->m_prev = headHook->m_prev;
                    headHook->m_next = headHook->m_prev = head;
                }
                else
                {
                    // 2 lists with elements
                    AZStd::swap(Hook::to_node_ptr(headHook->m_next)->m_prev, Hook::to_node_ptr(rhsHeadHook->m_next)->m_prev);
                    AZStd::swap(Hook::to_node_ptr(headHook->m_prev)->m_next, Hook::to_node_ptr(rhsHeadHook->m_prev)->m_next);
                    AZStd::swap(headHook->m_next, rhsHeadHook->m_next);
                    AZStd::swap(headHook->m_prev, rhsHeadHook->m_prev);
                }
            }

            AZStd::swap(m_numElements, rhs.m_numElements);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            swap_all(rhs);
#endif
        }

        inline void splice(iterator splicePos, this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::intrusive_list::splice - splicing it self!");
            AZSTD_CONTAINER_ASSERT(rhs.m_numElements > 0, "AZSTD::intrusive_list::splice - No elements to splice!");

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type firstNode = rhs.begin().get_iterator().m_node;
            node_ptr_type lastNode = rhs.end().get_iterator().m_node;
            node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
            rhs.orphan_all();
#else
            node_ptr_type firstNode = rhs.begin().m_node;
            node_ptr_type lastNode = rhs.end().m_node;
            node_ptr_type splicePosNode = splicePos.m_node;
#endif

            m_numElements += rhs.m_numElements;
            rhs.m_numElements = 0;

            //////////////////////////////////////////////////////////////////////////
            // Common code for splices
            node_ptr_type firstPrevNode = Hook::to_node_ptr(firstNode)->m_prev;
            Hook::to_node_ptr(firstPrevNode)->m_next = lastNode;
            node_ptr_type lastPrevNode = Hook::to_node_ptr(lastNode)->m_prev;
            Hook::to_node_ptr(lastPrevNode)->m_next = splicePosNode;
            node_ptr_type splicePrevNode = Hook::to_node_ptr(splicePosNode)->m_prev;
            Hook::to_node_ptr(splicePrevNode)->m_next = firstNode;

            Hook::to_node_ptr(splicePosNode)->m_prev = lastPrevNode;
            Hook::to_node_ptr(lastNode)->m_prev = firstPrevNode;
            Hook::to_node_ptr(firstNode)->m_prev = splicePrevNode;
            //////////////////////////////////////////////////////////////////////////
        }

        inline void splice(iterator splicePos, this_type& rhs, iterator first)
        {
            AZSTD_CONTAINER_ASSERT(first != rhs.end(), "AZSTD::intrusive_list::splice invalid iterator!");

            iterator last = first;
            ++last;
            if (splicePos == first || splicePos == last)
            {
                return;
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

            m_numElements += 1;
            rhs.m_numElements -= 1;

            //////////////////////////////////////////////////////////////////////////
            // Common code for splices
            node_ptr_type firstPrevNode = Hook::to_node_ptr(firstNode)->m_prev;
            Hook::to_node_ptr(firstPrevNode)->m_next = lastNode;
            node_ptr_type lastPrevNode = Hook::to_node_ptr(lastNode)->m_prev;
            Hook::to_node_ptr(lastPrevNode)->m_next = splicePosNode;
            node_ptr_type splicePrevNode = Hook::to_node_ptr(splicePosNode)->m_prev;
            Hook::to_node_ptr(splicePrevNode)->m_next = firstNode;

            Hook::to_node_ptr(splicePosNode)->m_prev = lastPrevNode;
            Hook::to_node_ptr(lastNode)->m_prev = firstPrevNode;
            Hook::to_node_ptr(firstNode)->m_prev = splicePrevNode;
            //////////////////////////////////////////////////////////////////////////
        }
        void splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        {
            AZSTD_CONTAINER_ASSERT(first != last, "AZSTD::instrusive_list::splice - no elements for splice");
            AZSTD_CONTAINER_ASSERT(this != &rhs || splicePos != last, "AZSTD::instrusive_list::splice invalid iterators");

            if (this != &rhs) // otherwise we are just rearranging the list
            {
                if (first == rhs.begin() && last == rhs.end())  // optimization for the whole list (may be we should issue a warning and require the user to call the entire list splice)
                {
                    m_numElements += rhs.m_numElements;
                    rhs.m_numElements = 0;
                }
                else
                {
                    size_type numElements = distance(first, last);
                    m_numElements += numElements;
                    rhs.m_numElements -= numElements;
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

            //////////////////////////////////////////////////////////////////////////
            // Common code for splices
            node_ptr_type firstPrevNode = Hook::to_node_ptr(firstNode)->m_prev;
            Hook::to_node_ptr(firstPrevNode)->m_next = lastNode;
            node_ptr_type lastPrevNode = Hook::to_node_ptr(lastNode)->m_prev;
            Hook::to_node_ptr(lastPrevNode)->m_next = splicePosNode;
            node_ptr_type splicePrevNode = Hook::to_node_ptr(splicePosNode)->m_prev;
            Hook::to_node_ptr(splicePrevNode)->m_next = firstNode;

            Hook::to_node_ptr(splicePosNode)->m_prev = lastPrevNode;
            Hook::to_node_ptr(lastNode)->m_prev = firstPrevNode;
            Hook::to_node_ptr(firstNode)->m_prev = splicePrevNode;
            //////////////////////////////////////////////////////////////////////////
        }

        inline void remove(const_reference value)
        {
            // Different STL implementations handle this in a different way.
            // The question is do we need the copy of the value ? If the value passed
            // is in the list and get destroyed in the middle of the remove this will cause a crash.
            // AZSTD doesn't handle this since we prefer speed, it's used responsibility to make sure
            // this is not the case. We can add assert in some FULL STL DEBUG MODE.
            iterator first = begin();
            iterator last = end();
            while (first != last)
            {
                if (value == *first)
                {
                    first = erase(first);
                }
                else
                {
                    ++first;
                }
            }
        }
        template <class Predicate>
        inline void remove_if(Predicate pred)
        {
            iterator first = begin();
            iterator last = end();
            while (first != last)
            {
                if (pred(*first))
                {
                    first = erase(first);
                }
                else
                {
                    ++first;
                }
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
                        next = erase(next);
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
                        next = erase(next);
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
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::intrusive_list::merge - can not merge itself!");

            iterator first1 = begin();
            iterator last1 = end();
            iterator first2 = rhs.begin();
            iterator last2 = rhs.end();

            // \todo check that both lists are ordered

            while (first1 != last1 && first2 != last2)
            {
                if (compare(*first2, *first1))
                {
                    iterator mid2 = first2;
                    splice(first1, rhs, first2, ++mid2);
                    first2 = mid2;
                }
                else
                {
                    ++first1;
                }
            }
            if (first2 != last2)
            {
                splice(last1, rhs, first2, last2);
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
                const size_t MAXBINS = 25;
                this_type Templist, Binlist[MAXBINS + 1];
                size_t Maxbin = 0;

                while (!empty())
                {   // sort another element, using bins
                    Templist.splice(Templist.begin(), *this, begin(), ++begin());

                    size_t Bin;
                    for (Bin = 0; Bin < Maxbin && !Binlist[Bin].empty(); ++Bin)
                    {   // merge into ever larger bins
                        Binlist[Bin].merge(Templist, comp);
                        Binlist[Bin].swap(Templist);
                    }

                    if (Bin == MAXBINS)
                    {
                        Binlist[Bin - 1].merge(Templist, comp);
                    }
                    else
                    {   // spill to new bin, while they last
                        Binlist[Bin].swap(Templist);
                        if (Bin == Maxbin)
                        {
                            ++Maxbin;
                        }
                    }
                }

                for (size_t Bin = 1; Bin < Maxbin; ++Bin)
                {
                    Binlist[Bin].merge(Binlist[Bin - 1], comp);  // merge up
                }
                splice(begin(), Binlist[Maxbin - 1]);   // result in last bin
                //////////////////////////////////////////////////////////////////////////
            }
        }
        void reverse()
        {
            if (m_numElements > 1)
            {
                iterator last = end();
                for (iterator next = ++begin(); next != last; )
                {
                    iterator prev = next;
                    splice(begin(), *this, prev, ++next);
                }
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
            m_numElements = 0;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

    protected:
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
        typename aligned_storage<sizeof(node_type), alignment_of<node_type>::value>::type m_root;

        inline node_ptr_type get_head() { return reinterpret_cast<node_ptr_type>(&m_root); }

        // Debug
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        inline void orphan_node(node_ptr_type node) const
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
    AZ_FORCE_INLINE bool operator==(const intrusive_list<T, Hook>& left, const intrusive_list<T, Hook>& right)
    {
        return &left == &right;
    }

    template< class T, class Hook >
    AZ_FORCE_INLINE bool operator!=(const intrusive_list<T, Hook>& left, const intrusive_list<T, Hook>& right)
    {
        return !(&left == &right);
    }
}

#endif // AZSTD_INTRUSIVE_LIST_H
#pragma once
