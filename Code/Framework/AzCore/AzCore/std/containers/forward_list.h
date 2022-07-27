/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/algorithm.h>
#include <AzCore/std/allocator_traits.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace AZStd
{
    namespace Internal
    {
        struct forward_list_node_base
        {
            forward_list_node_base* m_next;
        };

        // Node type
        template<typename ValueType>
        struct forward_list_node
            : public forward_list_node_base
        {
            ValueType           m_value;
        };
    }

    /**
     * Constant forward list iterator implementation. SCARY iterator implementation.
     */
    template<class T>
    class forward_list_const_iterator
    {
        enum
        {
            ITERATOR_VERSION = 1
        };

        template<class E, class A>
        friend class forward_list;
        typedef forward_list_const_iterator         this_type;
    public:
        typedef T                                   value_type;
        typedef AZStd::ptrdiff_t                    difference_type;
        typedef const T*                            pointer;
        typedef const T&                            reference;
        typedef AZStd::forward_iterator_tag         iterator_category;

        typedef Internal::forward_list_node<T>      node_type;
        typedef node_type*                          node_ptr_type;
        typedef Internal::forward_list_node_base    base_node_type;
        typedef base_node_type*                     base_node_ptr_type;

        AZ_FORCE_INLINE forward_list_const_iterator()
            : m_node(0) {}
        AZ_FORCE_INLINE forward_list_const_iterator(base_node_ptr_type baseNode)
            : m_node(baseNode) {}
        AZ_FORCE_INLINE reference operator*() const { return static_cast<node_ptr_type>(m_node)->m_value; }
        AZ_FORCE_INLINE pointer operator->() const { return &static_cast<node_ptr_type>(m_node)->m_value; }
        AZ_FORCE_INLINE this_type& operator++()
        {
            AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::forward_list::const_iterator_impl invalid node!");
            m_node = m_node->m_next;
            return *this;
        }

        AZ_FORCE_INLINE this_type operator++(int)
        {
            AZSTD_CONTAINER_ASSERT(m_node != 0, "AZSTD::forward_list::const_iterator_impl invalid node!");
            this_type temp = *this;
            m_node = m_node->m_next;
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
        base_node_ptr_type  m_node;
    };

    /**
     * Forward list iterator implementation. SCARY iterator implementation.
     */
    template<class T>
    class forward_list_iterator
        : public forward_list_const_iterator<T>
    {
        typedef forward_list_iterator           this_type;
        typedef forward_list_const_iterator<T>  base_type;
    public:
        typedef T*                              pointer;
        typedef T&                              reference;

        typedef typename base_type::node_type           node_type;
        typedef typename base_type::node_ptr_type       node_ptr_type;
        typedef typename base_type::base_node_type      base_node_type;
        typedef typename base_type::base_node_ptr_type  base_node_ptr_type;

        AZ_FORCE_INLINE forward_list_iterator() {}
        AZ_FORCE_INLINE forward_list_iterator(base_node_ptr_type baseNode)
            : forward_list_const_iterator<T>(baseNode) {}
        AZ_FORCE_INLINE reference operator*() const { return static_cast<node_ptr_type>(base_type::m_node)->m_value; }
        AZ_FORCE_INLINE pointer operator->() const { return &(static_cast<node_ptr_type>(base_type::m_node)->m_value); }
        AZ_FORCE_INLINE this_type& operator++()
        {
            AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::forward_list::iterator_impl invalid node!");
            base_type::m_node = base_type::m_node->m_next;
            return *this;
        }

        AZ_FORCE_INLINE this_type operator++(int)
        {
            AZSTD_CONTAINER_ASSERT(base_type::m_node != 0, "AZSTD::forward_list::iterator_impl invalid node!");
            this_type temp = *this;
            base_type::m_node = base_type::m_node->m_next;
            return temp;
        }
    };


    /**
    * The list container (single linked list) is complaint with \ref CStd (23.2.2). In addition we introduce the following \ref ListExtensions "extensions".
    *
    * \par
    * The forward_list keep the values in nodes (next element pointers).
    * Each node is allocated separately, so adding 100 elements will cause 100 allocations,
    * that's why it's generally good idea (when speed is important), to either use \ref fixed_list or pool allocator like \ref static_pool_allocator.
    * You can see examples of all that \ref AZStdExamples.
    * \todo We should change the forward_list default allocator to be pool allocator based on the default allocator. This will reduce the number of allocations
    * in the general case.
    */
    template< class T, class Allocator = AZStd::allocator >
    class forward_list
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        using this_type = forward_list<T, Allocator>;
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;

        using reference = T&;
        using const_reference = const T&;
    private:
        using node_type = Internal::forward_list_node<T>;

    public:
        using allocator_type = Allocator;
        using difference_type = typename AZStd::allocator_traits<allocator_type>::difference_type;
        using size_type = typename AZStd::allocator_traits<allocator_type>::size_type;

    private:
        // AZSTD extension.
        using node_ptr_type = node_type*;
        using base_node_type = Internal::forward_list_node_base;
        using base_node_ptr_type = base_node_type*;

        using const_iterator_impl = forward_list_const_iterator<T>;
        using iterator_impl = forward_list_iterator<T>;

    public:
        // Non-extension type aliases
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        using iterator = Debug::checked_forward_iterator<iterator_impl, this_type>;
        using const_iterator = Debug::checked_forward_iterator<const_iterator_impl, this_type>;
#else
        using iterator = iterator_impl;
        using const_iterator = const_iterator_impl;
#endif

        AZ_FORCE_INLINE forward_list()
            : m_numElements(0)
        {
            m_head.m_next = m_lastNode = &m_head;
        }
        AZ_FORCE_INLINE explicit forward_list(const allocator_type& allocator)
            : m_numElements(0)
            , m_allocator(allocator)
        {
            m_head.m_next = m_lastNode = &m_head;
        }

        AZ_FORCE_INLINE explicit forward_list(size_type numElements, const_reference value = value_type())
            : m_numElements(0)
        {
            m_head.m_next = m_lastNode = &m_head;
            insert_after(before_begin(), numElements, value);
        }
        AZ_FORCE_INLINE forward_list(size_type numElements, const_reference value, const allocator_type& allocator)
            : m_numElements(0)
            , m_allocator(allocator)
        {
            m_head.m_next = m_lastNode = &m_head;
            insert_after(before_begin(), numElements, value);
        }

        forward_list(const forward_list& rhs)
            : m_allocator(rhs.m_allocator)
        {
            m_head.m_next = m_lastNode = &m_head;
            insert_after(before_begin(), rhs.begin(), rhs.end());
        }

        forward_list(forward_list&& rhs)
            : m_allocator(rhs.m_allocator)
        {
            m_head.m_next = m_lastNode = &m_head;
            swap(rhs);
        }

        template<class InputIterator>
        AZ_FORCE_INLINE forward_list(InputIterator first, InputIterator last, const allocator_type& allocator = allocator_type())
            : m_allocator(allocator)
        {
            m_head.m_next = m_lastNode = &m_head;
            insert_after_iter(before_begin(), first, last, is_integral<InputIterator>());
        }

        forward_list(initializer_list<T> list, const allocator_type& allocator = allocator_type())
            : forward_list(list.begin(), list.end(), allocator)
        {
        }

        AZ_FORCE_INLINE ~forward_list()
        {
            clear();
        }

        AZ_FORCE_INLINE forward_list& operator=(const forward_list& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }
            assign(rhs.begin(), rhs.end());
            return (*this);
        }

        forward_list& operator=(forward_list&& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }
            swap(rhs);
            return *this;
        }
        inline void assign(size_type numElements, const_reference value)
        {
            iterator i = begin();
            size_type numToAssign = (numElements < m_numElements) ? numElements : m_numElements;
            size_type numToInsert = numElements - numToAssign;
            iterator prevI = i;
            for (; numToAssign > 0; ++i, --numToAssign)
            {
                *i = value;
                prevI = i;
            }

            if (numToInsert > 0)
            {
                insert_after(before_end(), numToInsert, value);
            }
            else
            {
                erase_after(prevI, end());
            }
        }
        template <class InputIterator>
        AZ_FORCE_INLINE void assign(InputIterator first, InputIterator last)
        {
            assign_iter(first, last, is_integral<InputIterator>());
        }

        void assign(initializer_list<T> ilist)
        {
            assign(ilist.begin(), ilist.end());
        }

        // AZStd Extensions - std::forward_list minimizes size as much as possible
        // so it doesn't store the size of the container.
        // The extension here is to store off size in a member variable
        AZ_FORCE_INLINE size_type   size() const { return m_numElements; }

        // Non-extension members
        AZ_FORCE_INLINE size_type   max_size() const { return AZStd::allocator_traits<allocator_type>::max_size(m_allocator) / sizeof(node_type); }
        [[nodiscard]] bool    empty() const { return (m_numElements == 0); }

        AZ_FORCE_INLINE iterator begin() { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, m_head.m_next)); }
        AZ_FORCE_INLINE const_iterator begin() const { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, m_head.m_next)); }
        AZ_FORCE_INLINE iterator end() { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, &m_head)); }
        AZ_FORCE_INLINE const_iterator end() const { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<base_node_ptr_type>(&m_head))); }
        AZ_FORCE_INLINE iterator before_begin() { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, &m_head)); }
        AZ_FORCE_INLINE const_iterator before_begin() const { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<base_node_ptr_type>(&m_head))); }

        // AZStd Extensions for tracking the last node within the forward_list
        // This isn't a fast function, as it involves traversing the list from the beginning
        // until until the spuplied iter in order to find the previous entry
        // This gives it a big-O run time of O(n)(as opposed to a double linked list where it is O(1))
        AZ_FORCE_INLINE iterator previous(iterator iter)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type iNode = iter.get_iterator().m_node;
#else
            base_node_ptr_type iNode = iter.m_node;
#endif
            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, previous(iNode)));
        }
        AZ_FORCE_INLINE const_iterator previous(const_iterator iter)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type iNode = iter.get_iterator().m_node;
#else
            base_node_ptr_type iNode = iter.m_node;
#endif
            return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, previous(iNode)));
        }
        AZ_FORCE_INLINE iterator before_end() { return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, m_lastNode)); }
        AZ_FORCE_INLINE const_iterator before_end() const { return const_iterator(AZSTD_CHECKED_ITERATOR(const_iterator_impl, const_cast<base_node_ptr_type>(m_lastNode))); }

        inline void resize(size_type newNumElements, const_reference value = value_type())
        {
            // determine new length, padding with value elements as needed
            if (m_numElements < newNumElements)
            {
                insert_after(iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, m_lastNode)), newNumElements - m_numElements, value);
            }
            else
            {
                if (newNumElements != m_numElements)
                {
                    iterator prevFrom = begin();
                    AZStd::advance(prevFrom, newNumElements - 1);
                    erase_after(prevFrom, end());
                }
            }
        }

        AZ_FORCE_INLINE reference front() { return *begin(); }
        AZ_FORCE_INLINE const_reference front() const { return *begin(); }

        void push_front(const_reference value) { emplace_front(value); }
        void push_front(value_type&& value) { emplace_front(AZStd::move(value)); }
        template<class... Args>
        reference emplace_front(Args&&... args)
        {
            return *emplace_after(before_begin(), AZStd::forward<Args>(args)...);
        }
        void pop_front() { erase_after(before_begin()); }

        // AZStd extension - back operations
        // std::forward_list doesn't keep track of the last dereference node within the list
        // so operations such as back/push_back/emplace_back/append_range aren't implemented in the standard
        // since it would involve traversing the entire list to find the end.
        reference back() { return *before_end(); }
        const_reference back() const { return *before_end(); }
        void push_back(const_reference value) { emplace_back(value); }
        void push_back(value_type&& value) { emplace_back(AZStd::move(value)); }
        template<class... Args>
        reference emplace_back(Args&&... args)
        {
            return *emplace_after(before_end(), AZStd::forward<Args>(args)...);
        }

        // AZStd extension - insert operations
        // It has performance characteristics of O(n)
        // If this function is needed, then it is preferred to use a doubly linked list(AZStd::list)
        // 
        // In order for insert to work with a forward_list the list has to be traversed
        // from the beginning to the specified iterator position in order to find the previous node
        // since it each node only has pointers to the next node(reverse iteration can't be done)
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, const_reference value)
        {
            return insert_after(previous(insertPos), value);
        }

        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, size_type numElements, const_reference value)
        {
            return insert_after(previous(insertPos), numElements, value);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE iterator insert(const_iterator insertPos, InputIterator first, InputIterator last)
        {
            return insert_after_iter(previous(insertPos), first, last, is_integral<InputIterator>());
        }

        template <class InputIterator>
        iterator insert_after(const_iterator insertPos, InputIterator first, InputIterator last)
        {
            return insert_after_iter(insertPos, first, last, is_integral<InputIterator>());
        }

        iterator insert_after(const_iterator insertPos, initializer_list<T> ilist)
        {
            return insert_after(insertPos, ilist.begin(), ilist.end());
        }

        iterator insert_after(const_iterator insertPos, const_reference value)
        {
            return emplace_after(insertPos, value);
        }
        iterator insert_after(const_iterator insertPos, value_type&& value)
        {
            return emplace_after(insertPos, AZStd::move(value));
        }
        template<class... Args>
        iterator emplace_after(const_iterator insertPos, Args&&... args)
        {
            node_ptr_type newNode = reinterpret_cast<node_ptr_type>(m_allocator.allocate(sizeof(node_type), alignof(node_type)));

            // forward to the constructor 
            pointer ptr = &newNode->m_value;
            construct_at(ptr, AZStd::forward<Args>(args)...);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type prevNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type prevNode = insertPos.m_node;
#endif

            ++m_numElements;

            if (m_lastNode == prevNode)
            {
                m_lastNode = newNode;
            }

            newNode->m_next = prevNode->m_next;
            prevNode->m_next = newNode;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, newNode));
        }

        iterator insert_after(const_iterator insertPos, size_type numElements, const_reference value)
        {
            m_numElements += numElements;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type prevNode = insertPos.get_iterator().m_node;
#else
            base_node_ptr_type prevNode = insertPos.m_node;
#endif
            // Tracks the first insert node to return as the iterator value
            iterator returnIter(insertPos.m_node);
            base_node_ptr_type insNode = prevNode->m_next;

            for (bool firstIteration = true; 0 < numElements; --numElements, firstIteration = false)
            {
                node_ptr_type newNode = reinterpret_cast<node_ptr_type>(m_allocator.allocate(sizeof(node_type), alignment_of<node_type>::value));
                AZSTD_CONTAINER_ASSERT(newNode != nullptr, "AZSTD::forward_list::insert - failed to allocate node!");

                if (firstIteration)
                {
                    returnIter = iterator(newNode);
                }

                if (m_lastNode == prevNode)
                {
                    m_lastNode = newNode;
                }

                // copy construct
                pointer ptr = &newNode->m_value;
                Internal::construct<pointer>::single(ptr, value);

                newNode->m_next = insNode;
                insNode = newNode;
            }

            prevNode->m_next = insNode;

            return returnIter;
        }

        inline iterator erase_after(const_iterator toEraseNext)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            base_node_ptr_type prevNode = toEraseNext.get_iterator().m_node;
            ++toEraseNext; // This will validate the operation.
            base_node_ptr_type node = toEraseNext.get_iterator().m_node;
            orphan_node(node);
#else
            AZSTD_CONTAINER_ASSERT(toEraseNext.m_node != 0, "AZStd::forward_list::erase_after - invalid node!");
            base_node_ptr_type prevNode = toEraseNext.m_node;
            base_node_ptr_type node = prevNode->m_next;
#endif
            pointer toDestroy = &static_cast<node_ptr_type>(node)->m_value;
            prevNode->m_next = node->m_next;

            if (m_lastNode == node)
            {
                m_lastNode = prevNode;
            }

            Internal::destroy<pointer>::single(toDestroy);
            deallocate_node(static_cast<node_ptr_type>(node), typename allocator_type::allow_memory_leaks());
            --m_numElements;

            return iterator(AZSTD_CHECKED_ITERATOR(iterator_impl, prevNode->m_next));
        }

        inline iterator erase_after(const_iterator constPrevFirst, const_iterator constLast)
        {
            iterator prevFirst = AZStd::Internal::ConstIteratorCast<iterator>(constPrevFirst);
            iterator last = AZStd::Internal::ConstIteratorCast<iterator>(constLast);
            iterator first = prevFirst;
            ++first; // to the first element to delete
            while (first != last)
            {
                first = erase_after(prevFirst);
            }

            return last;
        }

        // AZStd extension - erase operations
        // It has performance characteristics of O(n)
        // If this function is needed, then it is preferred to use a doubly linked list(AZStd::list)
        // 
        // In order for eraseto work with a forward_list the list has to be traversed
        // from the beginning to the specified iterator position in order to find the previous node
        // since it each node only has pointers to the next node(reverse iteration can't be done)
        AZ_FORCE_INLINE iterator erase(const_iterator toErase)
        {
            return erase_after(previous(toErase));
        }

        AZ_FORCE_INLINE iterator erase(const_iterator first, const_iterator last)
        {
            const_iterator prevFirst = previous(first);
            return erase_after(prevFirst, last);
        }

        inline void clear()
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
                deallocate_node(toDelete, typename allocator_type::allow_memory_leaks());
            }

            m_head.m_next = m_lastNode = &m_head;
            m_numElements = 0;
        }

        friend bool operator==(const forward_list& left, const forward_list& right)
        {
            // test for list equality
            return ranges::equal(left, right);
        }

        friend bool operator!=(const forward_list& left, const forward_list& right)
        {
            return !(left == right);
        }

        void swap(this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZStd::forward_list::swap - it's pointless to swap the vector itself!");
            if (m_allocator == rhs.m_allocator)
            {
                // Same allocator.
                if (m_numElements == 0)
                {
                    if (rhs.m_numElements == 0)
                    {
                        return;
                    }

                    // if( IsLinear )
                    // {
                    //   m_head->m_next = rhs.m_head->m_next;
                    //   rhs.m_head->m_next = 0;
                    // }
                    // else
                    {
                        base_node_ptr_type rhsFirst = rhs.m_head.m_next;
                        base_node_ptr_type rhsLast = rhs.m_lastNode;

                        rhsLast->m_next = &m_head;
                        m_head.m_next = rhsFirst;

                        m_lastNode = rhs.m_lastNode;
                        rhs.m_head.m_next = rhs.m_lastNode = &rhs.m_head;
                    }
                }
                else
                {
                    if (rhs.m_numElements == 0)
                    {
                        // if( IsLinear )
                        // {
                        //   rhs.m_head->m_next = m_head->m_next;
                        //   m_head->m_next = 0;
                        // }
                        // else
                        {
                            base_node_ptr_type first = m_head.m_next;
                            base_node_ptr_type last = m_lastNode;

                            last->m_next = &rhs.m_head;
                            rhs.m_head.m_next = first;

                            rhs.m_lastNode = m_lastNode;
                            m_head.m_next = m_lastNode = &m_head;
                        }
                    }
                    else
                    {
                        // if( IsLinear )
                        // {
                        //      // for linear the end iterator is 0.
                        //      AZStd::swap(m_head.m_next,rhs.m_head.m_next);
                        // }
                        // else
                        {
                            base_node_ptr_type rhsFirst = rhs.m_head.m_next;
                            base_node_ptr_type rhsLast = rhs.m_lastNode;

                            base_node_ptr_type first = m_head.m_next;
                            base_node_ptr_type last = m_lastNode;

                            rhsLast->m_next = &m_head;
                            m_head.m_next = rhsFirst;

                            last->m_next = &rhs.m_head;
                            rhs.m_head.m_next = first;

                            AZStd::swap(m_lastNode, rhs.m_lastNode);
                        }
                    }
                }

                AZStd::swap(m_numElements, rhs.m_numElements);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
            }
            else
            {
                // Different allocators, use assign.
                this_type temp = *this;
                *this = rhs;
                rhs = temp;
            }
        }

        // AZStd extension - splice operations
        // It has performance characteristics of O(n)
        // If this function is needed, then it is preferred to use a doubly linked list(AZStd::list)
        // 
        // In order for splice to work with a forward_list the list has to be traversed
        // from the beginning to the specified iterator position in order to find the previous node
        // to splice onto
        AZ_FORCE_INLINE void splice(iterator splicePos, this_type& rhs)
        {
            splice_after(previous(splicePos), rhs);
        }
        AZ_FORCE_INLINE void splice(iterator splicePos, this_type& rhs, iterator first)
        {
            splice_after(previous(splicePos), rhs, first);
        }

        AZ_FORCE_INLINE void splice(iterator splicePos, this_type& rhs, iterator first, iterator last)
        {
            splice_after(previous(splicePos), rhs, first, last);
        }

        void splice_after(iterator splicePos, this_type& rhs)
        {
            AZSTD_CONTAINER_ASSERT(this != &rhs, "AZSTD::forward_list::splice_after - splicing it self!");
            AZSTD_CONTAINER_ASSERT(rhs.m_numElements > 0, "AZSTD::forward_list::splice_after - No elements to splice!");

            if (m_allocator == rhs.m_allocator)
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                base_node_ptr_type firstNode = rhs.begin().get_iterator().m_node;
                base_node_ptr_type splicePosNode = splicePos.get_iterator().m_node;
                rhs.orphan_all();
#else
                base_node_ptr_type firstNode = rhs.begin().m_node;
                base_node_ptr_type splicePosNode = splicePos.m_node;
#endif

                m_numElements += rhs.m_numElements;
                rhs.m_numElements = 0;

                // get the last valid node
                base_node_ptr_type lastNode = rhs.m_lastNode;

                lastNode->m_next = splicePosNode->m_next;
                splicePosNode->m_next = firstNode;

                if (m_lastNode == splicePosNode)
                {
                    m_lastNode = rhs.m_lastNode;
                }

                rhs.m_head.m_next = rhs.m_lastNode = &rhs.m_head;
            }
            else
            {
                insert_after(splicePos, rhs.begin(), rhs.end());
                rhs.clear();
            }
        }
        void splice_after(iterator splicePos, this_type& rhs, iterator first)
        {
            AZSTD_CONTAINER_ASSERT(first != rhs.end(), "AZSTD::forward_list::splice_after invalid iterator!");
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
                base_node_ptr_type prevFirst = rhs.previous(firstNode);

                if (rhs.m_lastNode == firstNode)
                {
                    rhs.m_lastNode = prevFirst;
                }

                prevFirst->m_next = lastNode;
                firstNode->m_next = splicePosNode->m_next;
                splicePosNode->m_next = firstNode;

                if (m_lastNode == splicePosNode)
                {
                    m_lastNode = firstNode;
                }
            }
            else
            {
                insert_after(splicePos, *first);
                rhs.erase(first);
            }
        }

        void splice_after(iterator splicePos, this_type& rhs, iterator first, iterator last)
        {
            AZSTD_CONTAINER_ASSERT(first != last, "AZSTD::forward_list::splice_after - no elements for splice");
            AZSTD_CONTAINER_ASSERT(this != &rhs || splicePos != last, "AZSTD::forward_list::splice_after invalid iterators");

            if (m_allocator == rhs.m_allocator)
            {
                if (this != &rhs) // otherwise we are just rearranging the list
                {
                    if (first == rhs.begin() && last == rhs.end())  // optimization for the whole list
                    {
                        splice_after(splicePos, rhs);
                        return;
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

                base_node_ptr_type preLastNode = firstNode;
                size_type numElements  = 1;  // 1 for the first element
                while (preLastNode->m_next != lastNode)
                {
                    preLastNode = preLastNode->m_next;
                    ++numElements;
                    //if( IsLinear)
                    // AZSTD_CONTAINER_ASSERT(preLastNode!=0,("AZSTD::forward_list::splice_after you passed the end of the list");
                    //else
                    AZSTD_CONTAINER_ASSERT(numElements <= rhs.m_numElements, "AZSTD::forward_list::splice_after rhs container is corrupted!");
                }

                base_node_ptr_type prevFirstNode = rhs.previous(firstNode);

                if (rhs.m_lastNode == preLastNode)
                {
                    rhs.m_lastNode = prevFirstNode;
                }

                // if the container is the same this is pointless... but is very fast.
                m_numElements += numElements;
                rhs.m_numElements -= numElements;

                prevFirstNode->m_next = lastNode;

                preLastNode->m_next = splicePosNode->m_next;
                splicePosNode->m_next = firstNode;

                if (m_lastNode == splicePosNode)
                {
                    m_lastNode = preLastNode;
                }
            }
            else
            {
                insert_after(splicePos, first, last);
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
            size_type removeCount = 0;
            if (!empty())
            {
                iterator prevFirst = before_begin();
                iterator first = begin();
                iterator last = end();
                do
                {
                    if (value == *first)
                    {
                        ++removeCount;
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

            return removeCount;
        }
        template <class Predicate>
        auto remove_if(Predicate pred) -> size_type
        {
            size_type removeCount = 0;
            if (!empty())
            {
                iterator prevFirst = before_begin();
                iterator first = begin();
                iterator last = end();

                do
                {
                    if (pred(*first))
                    {
                        ++removeCount;
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

            if (m_allocator == rhs.m_allocator)
            {
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
            else
            {
                iterator iter2  = rhs.begin();
                iterator end2   = rhs.end();

                while (iter1 != end1 && iter2 != end2)
                {
                    if (compare(*iter1, *iter2))
                    {
                        //AZSTD_CONTAINER_ASSERT(!compare(*iter1, *iter2),("AZStd::forward_list::merge invalid predicate!"));
                        prevIter1 = iter1;
                        ++iter1;
                    }
                    else
                    {
                        iter1 = insert_after(prevIter1, *(iter2++));
                    }
                }

                insert_after(prevIter1, iter2, rhs.end());

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
        void reverse()
        {
            if (m_numElements > 1)
            {
                base_node_ptr_type next = m_head.m_next;
                base_node_ptr_type end = &m_head;
                m_lastNode = next;
                base_node_ptr_type node = next->m_next;
                next->m_next = end;
                while (node != end)
                {
                    base_node_ptr_type temp = node->m_next;
                    node->m_next = next;
                    next = node;
                    node = temp;
                }
                m_head.m_next = next;
            }
        }

        // The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type&         get_allocator()         { return m_allocator; }
        AZ_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_allocator; }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        void                        set_allocator(const allocator_type& allocator)
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
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            base_node_ptr_type iterNode = iter.m_iter.m_node;
#else
            base_node_ptr_type iterNode = iter.m_node;
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
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            base_node_ptr_type iterNode = iter.m_iter.m_node;
#else
            base_node_ptr_type iterNode = iter.m_node;
#endif
            if (iterNode == &m_head)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

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
            m_head.m_next = m_lastNode = &m_head;
            m_numElements = 0;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }
    protected:
        AZ_FORCE_INLINE void    deallocate_node(node_ptr_type node, const true_type& /* allocator::allow_memory_leaks */)
        {
            (void)node;
        }

        AZ_FORCE_INLINE void    deallocate_node(node_ptr_type node, const false_type& /* !allocator::allow_memory_leaks */)
        {
            m_allocator.deallocate(node, sizeof(node_type), alignment_of<node_type>::value);
        }

        /**
         * The iterator to the element before i in the list.
         * Returns the end-iterator, if either i is the begin-iterator or the list empty.
         */
        AZ_FORCE_INLINE base_node_ptr_type previous(base_node_ptr_type iNode)
        {
            base_node_ptr_type node = &m_head;

            if (iNode == node)
            {
                return m_lastNode;
            }

            while (node->m_next != iNode)
            {
                node = node->m_next;
                // This check makes sense for the linear forward_list only.
                AZSTD_CONTAINER_ASSERT(node != 0, "This node it's not in the list or the list is corrupted!");
            }
            return node;
        }

        template <class InputIterator>
        iterator insert_after_iter(const_iterator insertPos, const InputIterator& first, const InputIterator& last, const true_type& /* is_integral<InputIterator> */)
        {
            return insert_after(insertPos, (size_type)first, (value_type)last);
        }

        template <class InputIterator>
        iterator insert_after_iter(const_iterator insertPos, InputIterator first, InputIterator last, const false_type& /* !is_integral<InputIterator> */)
        {
            iterator returnIter(insertPos.m_node);
            for (bool firstIteration = true; first != last; ++first, ++insertPos, firstIteration = false)
            {
                // The first iteration of the loop sets the iterator to the beginning of the inserted elements
                if (iterator insertIter = insert_after(insertPos, *first); firstIteration)
                {
                    returnIter = insertIter;
                }
            }

            return returnIter;
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

        base_node_type      m_head;             ///< Node header.
        base_node_ptr_type  m_lastNode{};         ///< Cached last valid node.
        size_type           m_numElements{};      ///< Number of elements so size() is O(1).
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

    // AZStd::forward_list deduction guides
    template <class InputIt, class Alloc = allocator>
    forward_list(InputIt, InputIt, Alloc = Alloc()) -> forward_list<typename iterator_traits<InputIt>::value_type, Alloc>;

    template<class T, class Allocator, class U>
    decltype(auto) erase(forward_list<T, Allocator>& container, const U& value)
    {
        return container.remove(value);
    }
    template<class T, class Allocator, class Predicate>
    decltype(auto) erase_if(forward_list<T, Allocator>& container, Predicate predicate)
    {
        return container.remove_if(predicate);
    }
}
