/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_traits.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/iterator/const_iterator.h>
#include <AzCore/std/ranges/common_view.h>
#include <AzCore/std/ranges/as_rvalue_view.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_integral.h>

namespace AZStd
{
    namespace Internal
    {
        struct list_node_base
        {
            list_node_base*     m_next;
            list_node_base*     m_prev;
        };

        template<typename ValueType>
        struct list_node
            : public list_node_base
        {
            ValueType                       m_value;
        };
    }

    /**
     * List constant iterator implementation. SCARY iterator implementation.
     */
    template< class T >
    class list_iterator
    {
        enum
        {
            ITERATOR_VERSION = 1
        };

        template<class E, class A>
        friend class list;
        typedef list_iterator                       this_type;
    public:
        typedef T                                   value_type;
        typedef AZStd::ptrdiff_t                    difference_type;
        typedef T*                                  pointer;
        typedef T&                                  reference;
        typedef AZStd::bidirectional_iterator_tag   iterator_category;

        typedef Internal::list_node<T>              node_type;
        typedef node_type*                          node_ptr_type;
        typedef Internal::list_node_base            base_node_type;
        typedef base_node_type*                     base_node_ptr_type;

        AZ_FORCE_INLINE list_iterator() = default;

        AZ_FORCE_INLINE explicit list_iterator(base_node_ptr_type node)
            : m_node(node) {}
        AZ_FORCE_INLINE reference operator*() const { return static_cast<node_ptr_type>(m_node)->m_value; }
        AZ_FORCE_INLINE pointer operator->() const { return &static_cast<node_ptr_type>(m_node)->m_value; }
        AZ_FORCE_INLINE this_type& operator++()
        {
            AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::list::const_iterator_impl invalid node!");
            m_node = m_node->m_next;
            return *this;
        }

        AZ_FORCE_INLINE this_type operator++(int)
        {
            AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::list::const_iterator_impl invalid node!");
            this_type temp = *this;
            m_node = m_node->m_next;
            return temp;
        }

        AZ_FORCE_INLINE this_type& operator--()
        {
            AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::list::const_iterator_impl invalid node!");
            m_node = m_node->m_prev;
            return *this;
        }

        AZ_FORCE_INLINE this_type operator--(int)
        {
            AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::list::const_iterator_impl invalid node!");
            this_type temp = *this;
            m_node = m_node->m_prev;
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
        base_node_ptr_type m_node{};
    };


    // To allow incomplete types to be used with the const_iterator
    // the list_iterator has specializations added for the basic_const_iterator
    // constraints
    template<class T>
    inline constexpr bool input_or_output_iterator<list_iterator<T>> = true;

    /**
    * The list container (double linked list) is complaint with \ref CStd (23.2.2). In addition we introduce the following \ref ListExtensions "extensions".
    *
    * \par
    * The list keep the values in nodes (with prev and next element pointers).
    * Each node is allocated separately, so adding 100 elements will cause 100 allocations,
    * that's why it's generally good idea (when speed is important), to either use \ref fixed_list or pool allocator like \ref static_pool_allocator.
    * To find the node type use AZStd::list::node_type.
    * You can see examples of all that \ref AZStdExamples.
    * \todo We should change the list default allocator to be pool allocator based on the default allocator. This will reduce the number of allocations
    * in the general case.
    * \note For single linked list use \ref forward_list.
    */
    template< class T, class Allocator = AZStd::allocator >
    class list
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef list<T, Allocator>                       this_type;
    public:
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;

        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef Allocator                               allocator_type;

        // AZSTD extension.
        typedef T                                       value_type;
        typedef Internal::list_node<T>                  node_type;
        typedef node_type*                              node_ptr_type;
        typedef Internal::list_node_base                base_node_type;
        typedef base_node_type*                         base_node_ptr_type;

        using iterator_impl = list_iterator<T>;
        using const_iterator_impl = basic_const_iterator<iterator_impl>;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        using iterator = Debug::checked_bidirectional_iterator<iterator_impl, this_type>;
        using const_iterator = Debug::checked_bidirectional_iterator<const_iterator_impl, this_type>;
#else
        using iterator = iterator_impl;
        using const_iterator = const_iterator_impl;
#endif
        using difference_type = typename iterator::difference_type;
        using size_type = make_unsigned_t<difference_type>;

        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;

        //////////////////////////////////////////////////////////////////////////
        // 23.2.2 construct/copy/destroy
        list()
        {
            m_head.m_next = m_head.m_prev = &m_head;
        }
        AZ_FORCE_INLINE explicit list(const allocator_type& allocator)
            : m_numElements(0)
            , m_allocator(allocator)
        {
            m_head.m_next = m_head.m_prev = &m_head;
        }

        AZ_FORCE_INLINE explicit list(size_type numElements, const_reference value = value_type())
            : m_numElements(0)
        {
            m_head.m_next = m_head.m_prev = &m_head;
            insert(begin(), numElements, value);
        }
        AZ_FORCE_INLINE list(size_type numElements, const_reference value, const allocator_type& allocator)
            : m_numElements(0)
            , m_allocator(allocator)
        {
            m_head.m_next = m_head.m_prev = static_cast<node_ptr_type>(&m_head);
            insert(begin(), numElements, value);
        }

        AZ_FORCE_INLINE list(const this_type& rhs)
            : m_numElements(0)
            , m_allocator(rhs.m_allocator)
        {
            m_head.m_next = m_head.m_prev = &m_head;

            insert(begin(), rhs.begin(), rhs.end());
        }

        template<class InputIterator>
        AZ_FORCE_INLINE list(InputIterator first, InputIterator last, const allocator_type& allocator = allocator_type())
            : m_allocator(allocator)
        {
            m_head.m_next = m_head.m_prev = &m_head;
            insert_iter(begin(), first, last, is_integral<InputIterator>());
        }

        template<class R, class = enable_if_t<Internal::container_compatible_range<R, value_type>>>
        list(from_range_t, R&& rg, const allocator_type& alloc = Allocator())
            : m_allocator(alloc)
        {
            m_head.m_next = m_head.m_prev = &m_head;
            assign_range(AZStd::forward<R>(rg));
        }

        AZ_FORCE_INLINE list(initializer_list<T> ilist, const allocator_type& alloc = allocator_type())
            : list(ilist.begin(), ilist.end(), alloc)
        {
        }

        AZ_FORCE_INLINE ~list()
        {
            clear();
        }

        AZ_FORCE_INLINE this_type& operator=(const this_type& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }
            assign(rhs.begin(), rhs.end());
            return *this;
        }
        AZ_INLINE void assign(size_type numElements, const_reference value)
        {
            iterator i = begin();
            iterator last = end();
            size_type numToAssign = (numElements < m_numElements) ? numElements : m_numElements;
            size_type numToInsert = numElements - numToAssign;
            for (; numToAssign > 0; ++i, --numToAssign)
            {
                *i = value;
            }
            if (numToInsert > 0)
            {
                insert(last, numToInsert, value);
            }
            else
            {
                erase(i, last);
            }
        }
        template <class InputIterator>
        void assign(InputIterator first, InputIterator last)
        {
            assign_iter(first, last, is_integral<InputIterator>());
        }


        template<class R>
        auto assign_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>>
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                assign_iter(ranges::begin(rangeView), ranges::end(rangeView), false_type{});
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                assign_iter(ranges::begin(rangeView), ranges::end(rangeView), false_type{});
            }
        }

        void assign(initializer_list<T> iList)
        {
            assign(iList.begin(), iList.end());
        }

        AZ_FORCE_INLINE size_type   size() const { return m_numElements; }
        AZ_FORCE_INLINE size_type   max_size() const { return AZStd::allocator_traits<allocator_type>::max_size(m_allocator) / sizeof(node_type); }
        AZ_FORCE_INLINE bool    empty() const { return (m_numElements == 0); }

        AZ_FORCE_INLINE iterator begin() { return iterator(m_head.m_next); }
        AZ_FORCE_INLINE const_iterator begin() const { return const_iterator(iterator(m_head.m_next)); }
        AZ_FORCE_INLINE iterator end() { return iterator(&m_head); }
        AZ_FORCE_INLINE const_iterator end() const { return const_iterator(iterator(const_cast<base_node_ptr_type>(&m_head))); }

        AZ_FORCE_INLINE reverse_iterator rbegin() { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator rend() { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

        AZ_FORCE_INLINE void resize(size_type newNumElements, const_reference value = value_type())
        {   // determine new length, padding with value elements as needed
            if (m_numElements < newNumElements)
            {
                insert(end(), newNumElements - m_numElements, value);
            }
            else
            {
                while (newNumElements < m_numElements)
                {
                    pop_back();
                }
            }
        }

        AZ_FORCE_INLINE reference front() { return (*begin()); }
        AZ_FORCE_INLINE const_reference front() const { return (*begin()); }
        AZ_FORCE_INLINE reference back() { return (*(--end())); }
        AZ_FORCE_INLINE const_reference back() const { return (*(--end())); }

        // 23.2.2.3 modifiers
        AZ_FORCE_INLINE void push_front(const_reference value) { insert(begin(), value); }
        template<class R>
        auto prepend_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, T>>
        {
            insert_range(begin(), AZStd::forward<R>(rg));
        }
        AZ_FORCE_INLINE void pop_front() { erase(begin()); }
        AZ_FORCE_INLINE void push_back(const_reference value) { insert(end(), value); }
        template<class R>
        auto append_range(R&& rg) -> enable_if_t<Internal::container_compatible_range<R, T>>
        {
            insert_range(end(), AZStd::forward<R>(rg));
        }
        AZ_FORCE_INLINE void pop_back() { erase(--end()); }

        template <typename MyAllocator = allocator_type>
        list(this_type&& rhs, typename AZStd::enable_if_t<AZStd::is_default_constructible<MyAllocator>::value>* = nullptr)
            : m_numElements(0)
        {
            m_head.m_next = m_head.m_prev = &m_head;
            swap(rhs);
        }

        list(this_type&& rhs, const allocator_type& allocator)
            : m_numElements(0)
            , m_allocator(allocator)
        {
            m_head.m_next = m_head.m_prev = &m_head;
            swap(rhs);
        }

        this_type& operator=(this_type&& rhs)
        {
            if (this != &rhs)
            {
                clear();
                swap(rhs);
            }
            return (*this);
        }

        void push_front(T&& value)
        {
            insert_element(begin(), AZStd::move(value));
        }

        void push_back(T&& value)
        {
            insert_element(end(), AZStd::move(value));
        }

        iterator insert(const_iterator inserPos, T&& value)
        {
            return emplace(inserPos, AZStd::move(value));
        }

        template<class ... ArgumentsInputs>
        reference emplace_front(ArgumentsInputs&& ... arguments)
        {
            return insert_element(begin(), AZStd::forward<ArgumentsInputs>(arguments) ...);
        }

        template<class ... ArgumentsInputs>
        reference emplace_back(ArgumentsInputs&& ... arguments)
        {
            return insert_element(end(), AZStd::forward<ArgumentsInputs>(arguments) ...);
        }

        template<class ... ArgumentsInputs>
        iterator emplace(const_iterator insertPos, ArgumentsInputs&& ... arguments)
        {
            insert_element(insertPos, AZStd::forward<ArgumentsInputs>(arguments) ...);
            return iterator((--insertPos).base().m_node);
        }

        iterator insert(const_iterator insertPos, initializer_list<T> ilist)
        {
            return insert(insertPos, ilist.begin(), ilist.end());
        }

        inline iterator insert(const_iterator insertPos, const_reference value)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));

            // copy construct
            pointer ptr = &newNode->m_value;
            Internal::construct<pointer>::single(ptr, value);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insNode = insertPos.base().get_iterator().m_node;
#else
            base_node_ptr_type insNode = insertPos.base().m_node;
#endif

            ++m_numElements;
            newNode->m_next = insNode;
            newNode->m_prev = insNode->m_prev;
            insNode->m_prev = newNode;
            newNode->m_prev->m_next = newNode;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, newNode));
        }

        inline iterator insert(const_iterator insertPos, size_type numElements, const_reference value)
        {
            m_numElements += numElements;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insNode = insertPos.base().get_iterator().m_node;
#else
            base_node_ptr_type insNode = insertPos.base().m_node;
#endif

            for (; 0 < numElements; --numElements)
            {
                node_ptr_type newNode = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));
                // copy construct
                pointer ptr = &newNode->m_value;
                Internal::construct<pointer>::single(ptr, value);

                // we can speed this up...
                newNode->m_next = insNode;
                newNode->m_prev = insNode->m_prev;
                insNode->m_prev = newNode;
                newNode->m_prev->m_next = newNode;

                // Insert before newNode instead of insNode
                insNode = newNode;
            }

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, insNode));
        }

        template <class InputIterator>
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, InputIterator first, InputIterator last)
        {
            return insert_iter(insertPos, first, last, is_integral<InputIterator>());
        }

        template<class R>
        auto insert_range(const_iterator insertPos, R&& rg) -> enable_if_t<Internal::container_compatible_range<R, value_type>, iterator>
        {
            if constexpr (is_lvalue_reference_v<R>)
            {
                auto rangeView = AZStd::forward<R>(rg) | views::common;
                return insert_iter(insertPos, ranges::begin(rangeView), ranges::end(rangeView), false_type{});
            }
            else
            {
                auto rangeView = AZStd::forward<R>(rg) | views::as_rvalue | views::common;
                return insert_iter(insertPos, ranges::begin(rangeView), ranges::end(rangeView), false_type{});
            }
        }

        inline iterator erase(const_iterator toErase)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            node_ptr_type node = static_cast<node_ptr_type>(toErase.base().get_iterator().m_node);
            orphan_node(node);
#else
            node_ptr_type node = static_cast<node_ptr_type>(toErase.base().m_node);
#endif
            base_node_ptr_type prevNode = node->m_prev;
            base_node_ptr_type nextNode = node->m_next;
            prevNode->m_next = nextNode;
            nextNode->m_prev = prevNode;
            pointer toDestroy = &node->m_value;
            Internal::destroy<pointer>::single(toDestroy);
            deallocate_node(node);
            --m_numElements;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, nextNode));
        }

        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)
        {
            while (first != last)
            {
                erase(first++);
            }

            return last.base();
        }

        AZ_FORCE_INLINE void clear()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            base_node_ptr_type cur = m_head.m_next;
            while (cur != &m_head)
            {
                node_ptr_type toDelete = static_cast<node_ptr_type>(cur);
                cur = cur->m_next;
                pointer toDestroy = &toDelete->m_value;
                Internal::destroy<pointer>::single(toDestroy);
                deallocate_node(toDelete);
            }

            m_head.m_next = m_head.m_prev = &m_head;
            m_numElements = 0;
        }

        void swap(this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZStd::list::swap - it's pointless to swap the vector itself!");
            if (m_allocator == rhs.m_allocator)
            {
                // Same allocator, swap pointers.
                if (m_numElements == 0)
                {
                    if (rhs.m_numElements == 0)
                    {
                        return;
                    }

                    base_node_ptr_type rhsHead = rhs.m_head.m_next->m_prev;
                    base_node_ptr_type head = m_head.m_next; // pointer to itself (&m_head)
                    rhs.m_head.m_next->m_prev = head;
                    rhs.m_head.m_prev->m_next = head;
                    m_head.m_next = rhs.m_head.m_next;
                    m_head.m_prev = rhs.m_head.m_prev;
                    rhs.m_head.m_next = rhs.m_head.m_prev = rhsHead;
                }
                else
                {
                    if (rhs.m_numElements == 0)
                    {
                        base_node_ptr_type head = m_head.m_next->m_prev;
                        base_node_ptr_type rhsHead = rhs.m_head.m_next; // pointer to itself (&rhs.m_head)
                        m_head.m_next->m_prev = rhsHead;
                        m_head.m_prev->m_next = rhsHead;
                        rhs.m_head.m_next = m_head.m_next;
                        rhs.m_head.m_prev = m_head.m_prev;
                        m_head.m_next = m_head.m_prev = head;
                    }
                    else
                    {
                        // 2 lists with elements
                        AZStd::swap(m_head.m_next->m_prev, rhs.m_head.m_next->m_prev);
                        AZStd::swap(m_head.m_prev->m_next, rhs.m_head.m_prev->m_next);
                        AZStd::swap(m_head, rhs.m_head);
                    }
                }

                AZStd::swap(m_numElements, rhs.m_numElements);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
            }
            else
            {
                this_type temp(m_allocator);

                // Different allocators, move elements
                for (auto& element : *this)
                {
                    temp.push_back(AZStd::move(element));
                }
                clear();
                for (auto& element : rhs)
                {
                    push_back(AZStd::move(element));
                }
                rhs.clear();
                for (auto& element : temp)
                {
                    rhs.push_back(AZStd::move(element));
                }
            }
        }


        // 23.2.2.4 list operations:
        void splice(iterator splicePos, this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::list::splice - splicing it self!");
            AZSTD_CONTAINER_ASSERT(rhs.m_numElements > 0, "AZSTD::list::splice - No elements to splice!");

            if (m_allocator == rhs.m_allocator)
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                base_node_ptr_type firstNode = rhs.begin().get_iterator().m_node;
                base_node_ptr_type lastNode = rhs.end().get_iterator().m_node;
                base_node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
                rhs.orphan_all();
#else
                base_node_ptr_type firstNode = rhs.begin().m_node;
                base_node_ptr_type lastNode = rhs.end().m_node;
                base_node_ptr_type splicePosNode = splicePos.m_node;
#endif

                m_numElements += rhs.m_numElements;
                rhs.m_numElements = 0;

                //////////////////////////////////////////////////////////////////////////
                // Common code for splices
                firstNode->m_prev->m_next = lastNode;
                lastNode->m_prev->m_next = splicePosNode;
                splicePosNode->m_prev->m_next = firstNode;

                base_node_ptr_type prevSpliceNode = splicePosNode->m_prev;
                splicePosNode->m_prev = lastNode->m_prev;
                lastNode->m_prev = firstNode->m_prev;
                firstNode->m_prev = prevSpliceNode;
                //////////////////////////////////////////////////////////////////////////
            }
            else
            {
                for (iterator cur = rhs.begin(), end = rhs.end(); cur != end; ++cur)
                {
                    insert(splicePos, AZStd::move(*cur));
                }
                rhs.clear();
            }
        }

        void splice(iterator splicePos, this_type& rhs, iterator first)
        {
            AZSTD_CONTAINER_ASSERT(first != rhs.end(), "AZSTD::list::splice invalid iterator!");
            iterator last = first;
            ++last;
            if (splicePos == first || splicePos == last)
            {
                return;
            }
            if (m_allocator == rhs.m_allocator)
            {
                m_numElements += 1;
                rhs.m_numElements -= 1;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                base_node_ptr_type firstNode = first.get_iterator().m_node;
                base_node_ptr_type lastNode = last.get_iterator().m_node;
                base_node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
                if (this != &rhs)  // only if we actually moving nodes, from different lists.
                {
                    for (iterator next = first; next != last; )
                    {
                        base_node_ptr_type node = next.get_iterator().m_node;
                        ++next;
                        rhs.orphan_node(node);
                    }
                }
#else
                base_node_ptr_type firstNode = first.m_node;
                base_node_ptr_type lastNode = last.m_node;
                base_node_ptr_type splicePosNode = splicePos.m_node;
#endif

                //////////////////////////////////////////////////////////////////////////
                // Common code for splices
                firstNode->m_prev->m_next = lastNode;
                lastNode->m_prev->m_next = splicePosNode;
                splicePosNode->m_prev->m_next = firstNode;

                base_node_ptr_type prevSpliceNode = splicePosNode->m_prev;
                splicePosNode->m_prev = lastNode->m_prev;
                lastNode->m_prev = firstNode->m_prev;
                firstNode->m_prev = prevSpliceNode;
                //////////////////////////////////////////////////////////////////////////
            }
            else
            {
                insert(splicePos, AZStd::move(*first));
                rhs.erase(first);
            }
        }
        void splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        {
            AZSTD_CONTAINER_ASSERT(first != last, "AZSTD::list::splice - no elements for splice");
            AZSTD_CONTAINER_ASSERT(this != &rhs || splicePos != last, "AZSTD::list::splice invalid iterators");

            if (m_allocator == rhs.m_allocator)
            {
                if (this != &rhs) // otherwise we are just rearranging the list
                {
                    if (first == rhs.begin() && last == rhs.end())  // optimization for the whole list
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
                base_node_ptr_type firstNode = first.get_iterator().m_node;
                base_node_ptr_type lastNode = last.get_iterator().m_node;
                base_node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
                if (this != &rhs)  // only if we actually moving nodes, from different lists.
                {
                    for (iterator next = first; next != last; )
                    {
                        base_node_ptr_type node = next.get_iterator().m_node;
                        ++next;
                        rhs.orphan_node(node);
                    }
                }
#else
                base_node_ptr_type firstNode = first.m_node;
                base_node_ptr_type lastNode = last.m_node;
                base_node_ptr_type splicePosNode = splicePos.m_node;
#endif

                //////////////////////////////////////////////////////////////////////////
                // Common code for splices
                firstNode->m_prev->m_next = lastNode;
                lastNode->m_prev->m_next = splicePosNode;
                splicePosNode->m_prev->m_next = firstNode;

                base_node_ptr_type prevSpliceNode = splicePosNode->m_prev;
                splicePosNode->m_prev = lastNode->m_prev;
                lastNode->m_prev = firstNode->m_prev;
                firstNode->m_prev = prevSpliceNode;
                //////////////////////////////////////////////////////////////////////////
            }
            else
            {
                for (iterator cur = first; cur != last; ++cur)
                {
                    splicePos = insert(splicePos, AZStd::move(*cur));
                    ++splicePos;
                }
                rhs.erase(first, last);
            }
        }

        auto remove(const_reference value) -> size_type
        {
            // Different STL implementations handle this in a different way.
            // The question is do we need the copy of the value ? If the value passed
            // is in the list and get destroyed in the middle of the remove this will cause a crash.
            // AZSTD doesn't handle this since we prefer speed, it's used responsibility to make sure
            // this is not the case. We can add assert in some FULL STL DEBUG MODE.
            iterator first = begin();
            iterator last = end();
            size_type removeCount = 0;
            while (first != last)
            {
                if (value == *first)
                {
                    ++removeCount;
                    first = erase(first);
                }
                else
                {
                    ++first;
                }
            }

            return removeCount;
        }
        template <class Predicate>
        auto remove_if(Predicate pred) -> size_type
        {
            iterator first = begin();
            iterator last = end();
            size_type removeCount = 0;
            while (first != last)
            {
                if (pred(*first))
                {
                    ++removeCount;
                    first = erase(first);
                }
                else
                {
                    ++first;
                }
            }

            return removeCount;
        }
        void unique()
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
        void unique(BinaryPredicate compare)
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
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::list::merge - can not merge itself!");

            iterator first1 = begin();
            iterator last1 = end();
            iterator first2 = rhs.begin();
            iterator last2 = rhs.end();

            // \todo check that both lists are ordered

            if (m_allocator == rhs.m_allocator)
            {
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
            else
            {
                while (first1 != last1 && first2 != last2)
                {
                    if (compare(*first2, *first1))
                    {
                        first1 = insert(first1, *first2);
                        ++first2;
                    }
                    else
                    {
                        ++first1;
                    }
                }

                if (first2 != last2)
                {
                    insert(first1, first2, last2);
                }

                rhs.clear();
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
                this_type _Templist(m_allocator), _Binlist[_MAXBINS + 1];
                for (size_t i = 0; i < AZ_ARRAY_SIZE(_Binlist); ++i)
                {
                    _Binlist[i].set_allocator(m_allocator);
                }

                size_t _Maxbin = 0;

                while (!empty())
                {   // sort another element, using bins
                    _Templist.splice(_Templist.begin(), *this, begin(), ++begin() /*, 1, true*/); // don't invalidate iterators

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
                splice(begin(), _Binlist[_Maxbin - 1]); // result in last bin
                //////////////////////////////////////////////////////////////////////////
            }
        }
        inline void reverse()
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


        /**
        * \anchor ListExtensions
        * \name Extensions
        * @{
        */
        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type& get_allocator() { return m_allocator; }
        AZ_FORCE_INLINE const allocator_type& get_allocator() const { return m_allocator; }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        inline void                     set_allocator(const allocator_type& allocator)
        {
            if (m_allocator != allocator)
            {
                if (m_numElements > 0)
                {
                    // create temp list with all elements relocated.
                    this_type templist(begin(), end(), allocator);
                    clear();
                    m_allocator = allocator;
                    swap(templist);
                }
                else
                {
                    m_allocator = allocator;
                }
            }
        }

        // Validate container status.
        inline bool     validate() const
        {
            // \noto \todo traverse the list and make sure it's properly linked.
            return true;
        }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        inline int      validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "This iterator doesn't belong to this container");
            base_node_ptr_type iterNode = iter.base().m_iter.m_node;
#else
            base_node_ptr_type iterNode = iter.base().m_node;
#endif

            if (iterNode == &m_head)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        inline int      validate_iterator(const iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "This iterator doesn't belong to this container");
            base_node_ptr_type iterNode = iter.base().m_iter.m_node;
#else
            base_node_ptr_type iterNode = iter.base().m_node;
#endif
            if (iterNode == &m_head)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        //////////////////////////////////////////////////////////////////////////
        // Insert methods without provided src value.

        /**
        *  Inserts an element at the user position in the list without a provided instance. This can be used for value types
        *  with expensive constructors so we don't want to create temporary one.
        */
        inline iterator insert(iterator insertPos)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));

            // construct
            pointer ptr = &newNode->m_value;
            Internal::construct<pointer>::single(ptr);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type insNode = insertPos.m_node;
#endif

            ++m_numElements;
            newNode->m_next = insNode;
            newNode->m_prev = insNode->m_prev;
            insNode->m_prev = newNode;
            newNode->m_prev->m_next = newNode;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, newNode));
        }

        inline iterator insert_after(iterator insertPos, const_reference value)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));

            // copy construct
            pointer ptr = &newNode->m_value;
            Internal::construct<pointer>::single(ptr, value);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type insNode = insertPos.m_node;
#endif

            ++m_numElements;
            newNode->m_next = insNode->m_next;
            newNode->m_prev = insNode;
            insNode->m_next = newNode;
            newNode->m_next->m_prev = newNode;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, newNode));
        }

        inline void insert_after(iterator insertPos, size_type numElements, const_reference value)
        {
            m_numElements += numElements;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type insNode = insertPos.m_node;
#endif

            for (; 0 < numElements; --numElements)
            {
                node_ptr_type newNode = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));

                // copy construct
                pointer ptr = &newNode->m_value;
                Internal::construct<pointer>::single(ptr, value);

                newNode->m_next = insNode->m_next;
                newNode->m_prev = insNode;
                insNode->m_next = newNode;
                newNode->m_next->m_prev = newNode;
            }
        }
        //////////////////////////////////////////////////////////////////////////

        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        inline void                     leak_and_reset()
        {
            m_head.m_next = m_head.m_prev = &m_head;
            m_numElements = 0;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

        //! Extension allows for a list node to be unlinked without deletion
        //! This is used to implement a node handle construct in the unordered_map
        node_ptr_type unlink(const_iterator unlinkPos)
        {
            node_ptr_type unlinkNode = static_cast<node_ptr_type>(unlinkPos.base().m_node);

            unlinkNode->m_prev->m_next = unlinkNode->m_next;
            unlinkNode->m_next->m_prev = unlinkNode->m_prev;
            unlinkNode->m_next = nullptr;
            unlinkNode->m_prev = nullptr;

            --m_numElements;
            return unlinkNode;
        }

        //! Extension allows for a list node to be relinked to list
        //! Requirements: The list node must use the same allocator as the list being linked to
        iterator relink(const_iterator insertPos, const base_node_ptr_type nodeToLink)
        {
            base_node_ptr_type insNode = insertPos.base().m_node;

            ++m_numElements;
            nodeToLink->m_next = insNode;
            nodeToLink->m_prev = insNode->m_prev;
            insNode->m_prev = nodeToLink;
            nodeToLink->m_prev->m_next = nodeToLink;
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, nodeToLink));
        }
        /// @}
    protected:
        AZ_FORCE_INLINE void    deallocate_node(node_ptr_type node)
        {
            m_allocator.deallocate(node, sizeof(node_type), alignof(node_type));
        }

        template <class InputIterator>
        AZ_FORCE_INLINE iterator insert_iter(const_iterator insertPos, const InputIterator& first, const InputIterator& last, const true_type& /* is_integral<InputIterator> */)
        {
            return insert(insertPos, (size_type)first, (value_type)last);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE iterator insert_iter(const_iterator insertPos, const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            if (first == last)
            {
                // If the list is empty, return a non-const iterator version of insertPos.
                return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, insertPos.base().m_node));
            }

            InputIterator iter(first);
            // Store the iterator of the element inserted first (to be returned).
            iterator returnValue = insert(insertPos, *iter++);
            for (; iter != last; ++iter)
            {
                insert(insertPos, *iter);
            }
            return returnValue;
        }

        template <class InputIterator>
        inline void assign_iter(const InputIterator& numElements, const InputIterator& value, const true_type& /* is_integral<InputIterator> */)
        {
            assign((size_type)numElements, value);
        }

        template <class InputIterator>
        inline void assign_iter(const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            iterator i = begin();
            iterator curEnd = end();
            InputIterator src(first);
            for (; i != curEnd && src != last; ++i, ++src)
            {
                *i = *src;
            }
            if (src == last)
            {
                erase(i, curEnd);
            }
            else
            {
                insert(curEnd, src, last);
            }
        }

        template<class ... ArgumentsInputs>
        reference insert_element(const_iterator insertPos, ArgumentsInputs&& ... arguments)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(static_cast<void*>(m_allocator.allocate(sizeof(node_type), alignof(node_type))));

            // construct
            pointer ptr = &newNode->m_value;
            Internal::construct<pointer>::single(ptr, AZStd::forward<ArgumentsInputs>(arguments) ...);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type insNode = insertPos.base().get_iterator().m_node;
#else
            base_node_ptr_type insNode = insertPos.base().m_node;
#endif

            ++m_numElements;
            newNode->m_next = insNode;
            newNode->m_prev = insNode->m_prev;
            insNode->m_prev = newNode;
            newNode->m_prev->m_next = newNode;

            return *ptr;
        }

        base_node_type      m_head;
        size_type           m_numElements{};
        allocator_type      m_allocator;        ///< Instance of the allocator.

        // Debug
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        inline void orphan_node(base_node_ptr_type node) const
        {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "list::orphan_node - iterator was corrupted!");
                base_node_ptr_type nodePtr = static_cast<iterator*>(iter)->m_iter.m_node;

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

    // AZStd::list deduction guides
    template <class InputIt, class Alloc = allocator>
    list(InputIt, InputIt, Alloc = Alloc()) -> list<iter_value_t<InputIt>, Alloc>;

    template<class R, class Alloc = allocator, class = enable_if_t<ranges::input_range<R>>>
    list(from_range_t, R&&, Alloc = Alloc()) -> list<ranges::range_value_t<R>, Alloc>;

    template< class T, class Allocator >
    AZ_FORCE_INLINE bool operator==(const list<T, Allocator>& left, const list<T, Allocator>& right)
    {
        // test for list equality
        return (left.size() == right.size() && equal(left.begin(), left.end(), right.begin()));
    }

    template< class T, class Allocator >
    AZ_FORCE_INLINE bool operator!=(const list<T, Allocator>& left, const list<T, Allocator>& right)
    {
        return !(left == right);
    }

    template<class T, class Allocator, class U>
    decltype(auto) erase(list<T, Allocator>& container, const U& value)
    {
        return container.remove(value);
    }
    template<class T, class Allocator, class Predicate>
    decltype(auto) erase_if(list<T, Allocator>& container, Predicate predicate)
    {
        return container.remove_if(predicate);
    }
}
