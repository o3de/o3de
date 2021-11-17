/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/allocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/allocator_traits.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/typetraits/alignment_of.h>
#include <AzCore/std/typetraits/is_integral.h>

namespace AZStd
{
    /**
    * The vector container (AKA dynamic array) is complaint with \ref CStd (23.2.4). In addition we introduce the following \ref VectorExtensions "extensions".
    * \par
    * The vector keeps all elements in a single memory block. When it
    * grows it reallocated all elements. The rule for growing is that we allocate
    * every time the 50% more memory then then current number of elements. This way to insert ~1000 elements
    * 1 by 1 it will take 17 allocations. Of course if you know that in advance it will be smart to set the
    * capacity in advance, this way you will have only 1 allocation \ref AZStdExamples.
    * When you need a vector with preset capacity use \ref fixed_vector.
    */
    template< class T, class Allocator = AZStd::allocator >
    class vector
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef vector<T, Allocator>                     this_type;
    public:
        //#pragma region Type definitions
        typedef T*                                      pointer;
        typedef const T*                                const_pointer;

        typedef T&                                      reference;
        typedef const T&                                const_reference;
        typedef typename Allocator::difference_type     difference_type;
        typedef typename Allocator::size_type           size_type;

        typedef pointer                                 iterator_impl;
        typedef const_pointer                           const_iterator_impl;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_randomaccess_iterator<iterator_impl, this_type>       iterator;
        typedef Debug::checked_randomaccess_iterator<const_iterator_impl, this_type> const_iterator;
#else
        typedef iterator_impl                           iterator;
        typedef const_iterator_impl                     const_iterator;
#endif
        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T                                       value_type;
        typedef Allocator                               allocator_type;

        // AZSTD extension.
        /**
         * \brief Allocation node type. Common for all AZStd containers.
         * In vectors case we allocate always "sizeof(node_type)*capacity" block.
         */
        typedef value_type                              node_type;
        //#pragma endregion

        //////////////////////////////////////////////////////////////////////////
        // 23.2.4.1 construct/copy/destroy
        /// Construct an empty vector.
        AZ_FORCE_INLINE vector()
            : m_start(0)
            , m_last(0)
            , m_end(0)
        {}

        AZ_FORCE_INLINE explicit vector(const allocator_type& allocator)
            : m_start(0)
            , m_last(0)
            , m_end(0)
            , m_allocator(allocator)
        {}

        explicit vector(size_type numElements)
            : m_start(0)
            , m_last(0)
            , m_end(0)
        {
            if (numElements > 0)
            {
                size_type byteSize = sizeof(node_type) * numElements;
                m_start = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                m_end = m_start + numElements;
                Internal::construct<pointer, value_type, false>::range(m_start, m_end);
                m_last = m_end;
            }
        }

        vector(size_type numElements, const_reference value)
            : m_start(0)
            , m_last(0)
            , m_end(0)
        {
            if (numElements > 0)
            {
                size_type byteSize = sizeof(node_type) * numElements;
                m_start = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                m_end   = m_start + numElements;
                AZStd::uninitialized_fill_n(m_start, numElements, value);
                m_last  = m_end;
            }
        }
        vector(size_type numElements, const_reference value, const allocator_type& allocator)
            : m_start(0)
            , m_last(0)
            , m_end(0)
            , m_allocator(allocator)
        {
            if (numElements > 0)
            {
                size_type byteSize = sizeof(node_type) * numElements;
                m_start = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                m_end   = m_start + numElements;
                AZStd::uninitialized_fill_n(m_start, numElements, value);
                m_last  = m_end;
            }
        }

        template <class InputIterator>
        vector(const InputIterator& first, const InputIterator& last)
            : m_start(0)
            , m_last(0)
            , m_end(0)
        {
            // We need some help here, since this function can be mistaken with the vector(size_type, const_reference value, const allocator_type&) one...
            // so we need to handle this case.
            construct_iter(first, last, is_integral<InputIterator>());
        }
        template <class InputIterator>
        vector(const InputIterator& first, const InputIterator& last, const allocator_type& allocator)
            : m_start(0)
            , m_last(0)
            , m_end(0)
            , m_allocator(allocator)
        {
            // We need some help here, since this function can be mistaken with the vector(size_type, const_reference value, const allocator_type&) one...
            // so we need to handle this case.
            construct_iter(first, last, is_integral<InputIterator>());
        }
        vector(std::initializer_list<T> list, const allocator_type& allocator = allocator_type())
            : m_start(0)
            , m_last(0)
            , m_end(0)
            , m_allocator(allocator)
        {
            construct_iter(list.begin(), list.end(), is_integral<std::initializer_list<T> >());
        }

        vector(const this_type& rhs)
            : m_allocator(rhs.m_allocator)
        {
            size_type byteSize = sizeof(node_type) * (rhs.m_last - rhs.m_start);
            if (byteSize)
            {
                m_start = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                m_last  = AZStd::uninitialized_copy(rhs.m_start, rhs.m_last, m_start, is_trivially_copy_constructible<value_type>());
            }
            else
            {
                m_start = 0;
                m_last = 0;
            }
            m_end   = m_last;
        }

        vector(this_type&& rhs)
            : m_start(0)
            , m_last(0)
            , m_end(0)
            , m_allocator(rhs.m_allocator)
        {
            assign_rv(AZStd::forward<this_type>(rhs));
        }
        vector(this_type&& rhs, const allocator_type& allocator)
            :  m_start(0)
            , m_last(0)
            , m_end(0)
            , m_allocator(allocator)
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
            if (m_allocator != rhs.m_allocator)
            {
                clear();
                for (iterator iter = rhs.begin(); iter != rhs.end(); ++iter)
                {
                    push_back(AZStd::forward<T>(*iter));
                }
            }
            else
            {
                if (m_start)
                {
                    // Call destructor if we need to.
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory if we need to.
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), 0);
                }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                swap_all((this_type&)rhs);
#endif
                m_start = rhs.m_start;
                m_last = rhs.m_last;
                m_end = rhs.m_end;

                rhs.m_start = 0;
                rhs.m_last = 0;
                rhs.m_end = 0;
            }
        }

        void push_back(value_type&& value)
        {
            // insert element at end
            pointer element = AZStd::addressof(value);
            if (element >= m_start && element < m_last)
            {
                size_t index = element - m_start;
                if (m_last == m_end)
                {
                    size_t newSize = (m_last - m_start) + 1;
                    size_t capacity = m_end - m_start;
                    // grow by 50%, if we can.
                    capacity += capacity / 2;
                    if (capacity < newSize)
                    {
                        capacity = newSize;
                    }
                    reserve(capacity);
                }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(m_last, m_last);
#endif
                Internal::construct<pointer>::single(m_last, AZStd::forward<value_type>(m_start[index]));
                ++m_last;
            }
            else
            {
                if (m_last == m_end)
                {
                    size_t newSize = (m_last - m_start) + 1;
                    size_t capacity = m_end - m_start;
                    // grow by 50%, if we can.
                    capacity += capacity / 2;
                    if (capacity < newSize)
                    {
                        capacity = newSize;
                    }
                    reserve(capacity);
                }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(m_last, m_last);
#endif
                Internal::construct<pointer>::single(m_last, AZStd::forward<value_type>(value));
                ++m_last;
            }
        }

        template<class ... Args>
        // must be variadic for now treat it as convertable
        inline reference emplace_back(Args&& ... args)
        {
            if (m_last == m_end)
            {
                size_t newSize = (m_last - m_start) + 1;
                size_t capacity = m_end - m_start;
                // grow by 50%, if we can.
                capacity += capacity / 2;
                if (capacity < newSize)
                {
                    capacity = newSize;
                }
                reserve(capacity);
            }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(m_last, m_last);
#endif //
            Internal::construct<pointer>::single(m_last, AZStd::forward<Args>(args) ...);
            reference emplacedElement = *m_last;
            ++m_last;
            return emplacedElement;
        }

        inline iterator insert(const_iterator pos, value_type&& value)
        {
            return emplace(pos, AZStd::forward<value_type>(value));
        }

        template<class ... Args>
        inline iterator emplace(const_iterator pos, Args&& ... args)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(pos.m_container == this, "Iterator doesn't belong to this container");
            pointer element = const_cast<pointer>(pos.m_iter);
#else
            pointer element = const_cast<pointer>(pos);
#endif
            AZ_Assert(element >= m_start && element <= m_last, "where iterator outsize vector range [0,%d]", size());
            size_t offset = element - m_start;
            emplace_back(AZStd::forward<Args>(args) ...);
            AZStd::rotate(m_start + offset, m_last - 1, m_last);
            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start + offset));
        }
        void swap(this_type&& rhs)
        {
            assign_rv(AZStd::forward<this_type>(rhs));
        }

        ~vector()
        {
            if (m_start)
            {
                // Call destructor if we need to.
                Internal::destroy<pointer>::range(m_start, m_last);
                // Free memory if we need to.
                deallocate_memory(typename allocator_type::allow_memory_leaks(), 0);
            }
        }

        this_type& operator=(const this_type& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            size_type newSize = rhs.m_last - rhs.m_start;
            size_type capacity = m_end - m_start;
            size_type expandedSize = 0;

            // if we have an allocated block and we need more memory, try to expand first!
            if (m_start && capacity < newSize)
            {
                expandedSize = m_allocator.resize(m_start, newSize * sizeof(node_type));
                if (expandedSize % sizeof(node_type) == 0)
                {
                    size_type expandedCapacity = expandedSize / sizeof(node_type);
                    if (capacity < expandedCapacity)
                    {
                        capacity = expandedCapacity;
                        m_end = m_start + expandedCapacity;
                    }
                }
            }

            if (newSize > capacity)
            {
                // Not enough capacity, need to allocate more.
                if (m_start)
                {
                    // Destroy current vector.
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory if we need to.
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), expandedSize);
                }

                // allocate and copy new
                size_type byteSize = sizeof(node_type) * newSize;
                m_start = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                m_last  = AZStd::uninitialized_copy(rhs.m_start, rhs.m_last, m_start, is_trivially_copy_constructible<value_type>());
                m_end   = m_last;
            }
            else
            {
                size_type size = m_last - m_start;
                if (size >= newSize)
                {
                    if (newSize > 0)
                    {
                        // We have capacity to hold the new data.
                        pointer newLast = Internal::copy(rhs.m_start, rhs.m_last, m_start, is_trivially_copy_assignable<value_type>());

                        // Destroy the rest.
                        Internal::destroy<pointer>::range(newLast, m_last);

                        m_last = newLast;
                    }
                    else
                    {
                        if (m_start)
                        {
                            // Destroy the rest.
                            Internal::destroy<pointer>::range(m_start, m_last);
                            // Free memory if we need to.
                            deallocate_memory(typename allocator_type::allow_memory_leaks(), expandedSize);

                            m_start = 0;
                            m_end = 0;
                            m_last = 0;
                        }
                    }
                }
                else
                {
                    pointer newLast = Internal::copy(rhs.m_start, rhs.m_start + size, m_start, is_trivially_copy_assignable<value_type>());

                    m_last = AZStd::uninitialized_copy(rhs.m_start + size, rhs.m_last, newLast, is_trivially_copy_constructible<value_type>());
                }
            }

            return *this;
        }

        AZ_FORCE_INLINE size_type   size() const        { return m_last - m_start; }
        AZ_FORCE_INLINE size_type   max_size() const    { return AZStd::allocator_traits<allocator_type>::max_size(m_allocator) / sizeof(node_type); }
        AZ_FORCE_INLINE bool        empty() const       { return m_start == m_last; }

        void reserve(size_type numElements)
        {
            size_type capacity = m_end - m_start;
            size_type expandedSize = 0;
            // if we have an allocated block and we need more memory, try to expand first!
            if (m_start && capacity < numElements)
            {
                expandedSize = m_allocator.resize(m_start, numElements * sizeof(node_type));
                if (expandedSize % sizeof(node_type) == 0) // we need exact size to be able to compute the size on free
                {
                    size_type expandedCapacity = expandedSize / sizeof(node_type);
                    if (expandedCapacity >= numElements)
                    {
                        m_end = m_start + expandedCapacity;
                        return;
                    }
                }
            }
            if (numElements > capacity)
            {
                // need more capacity - reallocate
                size_type byteSize = sizeof(node_type) * numElements;
                pointer newStart = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                pointer newLast = AZStd::uninitialized_move(m_start, m_last, newStart);

                // Destroy old array
                if (m_start)
                {
                    // Call destructor if we need to.
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory (if needed).
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), expandedSize);
                }

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_all();
#endif
                m_start = newStart;
                m_last  = newLast;
                m_end   = m_start + numElements;
            }
        }
        void shrink_to_fit()
        {
            set_capacity(m_last - m_start);
        }
        AZ_FORCE_INLINE size_type               capacity() const    { return m_end - m_start; }

        AZ_FORCE_INLINE iterator                begin()             { return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)); }
        AZ_FORCE_INLINE const_iterator          begin() const       { return const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)); }
        AZ_FORCE_INLINE iterator                end()               { return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)); }
        AZ_FORCE_INLINE const_iterator          end() const         { return const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)); }

        AZ_FORCE_INLINE reverse_iterator        rbegin()            { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  rbegin() const      { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator        rend()              { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator  rend() const        { return const_reverse_iterator(begin()); }

        AZ_FORCE_INLINE const_iterator          cbegin() const      { return const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)); }
        AZ_FORCE_INLINE const_iterator          cend() const        { return const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)); }
        AZ_FORCE_INLINE const_reverse_iterator  crbegin() const     { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  crend() const       { return const_reverse_iterator(begin()); }

        AZ_FORCE_INLINE void resize(size_type newSize)
        {
            size_type size = m_last - m_start;
            if (size < newSize)
            {
                size_t capacity = m_end - m_start;
                if (capacity < newSize)
                {
                    reserve(newSize);
                }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(m_last, m_last);
#endif
                pointer newLast = m_start + newSize;
                // We always want to construct default value even for integral types, according to the standard
                Internal::construct<pointer, value_type, false>::range(m_last, newLast);
                m_last = newLast;
            }
            else if (newSize < size)
            {
                erase(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)) + newSize, const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)));
            }
        }

        /*!
        * Extension it will set the increase size without calling default constructors.
        * This is provided so we can use the vector as a generic buffer where we don't have to pay the cost of elements that we will construct over.
        */
        void resize_no_construct(size_type newSize)
        {
            size_type size = m_last - m_start;
            if (size < newSize)
            {
                size_t capacity = m_end - m_start;
                if (capacity < newSize)
                {
                    reserve(newSize);
                }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(m_last, m_last);
#endif
                m_last = m_start + newSize;
            }
            else if (newSize < size)
            {
                erase(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)) + newSize, const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)));
            }
        }

        AZ_FORCE_INLINE void resize(size_type newSize, const_reference value)
        {
            size_type size = m_last - m_start;
            if (size < newSize)
            {
                insert(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)), newSize - size, value);
            }
            else if (newSize < size)
            {
                erase(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)) + newSize, const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_last)));
            }
        }

        AZ_FORCE_INLINE reference at(size_type position)
        {
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - m_start), "AZStd::vector<>::at - position is out of range");
            return *(m_start + position);
        }
        AZ_FORCE_INLINE const_reference at(size_type position) const
        {
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - m_start), "AZStd::vector<>::at - position is out of range");
            return *(m_start + position);
        }


        AZ_FORCE_INLINE reference operator[](size_type position)
        {
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - m_start), "AZStd::vector<>::at - position is out of range");
            return *(m_start + position);
        }
        AZ_FORCE_INLINE const_reference operator[](size_type position) const
        {
            AZSTD_CONTAINER_ASSERT(position < size_type(m_last - m_start), "AZStd::vector<>::at - position is out of range");
            return *(m_start + position);
        }

        AZ_FORCE_INLINE reference       front()         { AZSTD_CONTAINER_ASSERT(m_last != m_start, "AZStd::vector<>::front - container is empty!"); return *m_start; }
        AZ_FORCE_INLINE const_reference front() const   { AZSTD_CONTAINER_ASSERT(m_last != m_start, "AZStd::vector<>::front - container is empty!"); return *m_start; }
        AZ_FORCE_INLINE reference       back()          { AZSTD_CONTAINER_ASSERT(m_last != m_start, "AZStd::vector<>::back - container is empty!"); return *(m_last - 1); }
        AZ_FORCE_INLINE const_reference back() const    { AZSTD_CONTAINER_ASSERT(m_last != m_start, "AZStd::vector<>::back - container is empty!"); return *(m_last - 1); }
        inline void     push_back(const_reference value)
        {
            size_type size = m_last - m_start;
            size_type capacity = m_end - m_start;
            if (size < capacity)
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(m_last, m_last);
#endif
                AZStd::uninitialized_fill_n(m_last, 1, value);
                ++m_last;
            }
            else
            {
                insert(end(), value);
            }
        }

        inline void     pop_back()
        {
            AZSTD_CONTAINER_ASSERT(m_start != m_last, "AZStd::vector<>::pop_back - no element to pop!");
            pointer toDestroy = m_last - 1;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(toDestroy, m_last);
#endif
            Internal::destroy<pointer>::single(toDestroy);
            m_last = toDestroy;
        }

        inline void     assign(size_type numElements, const_reference value)
        {
            // operator = is optimized we can optimize this too
            value_type valueCopy = value;   // in case value is in sequence
            clear();
            insert(begin(), numElements, valueCopy);
        }

        template<class InputIterator>
        AZ_FORCE_INLINE void        assign(const InputIterator& first, const InputIterator& last)
        {
            assign_iter(first, last, is_integral<InputIterator>());
        }

        inline iterator insert(const_iterator insertPos, const_reference value)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            const_pointer insertPosPtr;
            if (m_start != 0)
            {
                insertPosPtr = insertPos.get_iterator();    // we validate iterators only if this container has elements. It is possible all to be 0.
            }
            else
            {
                AZ_Assert(insertPos.m_container == this, "Iterator doesn't belong to this container");
                insertPosPtr = insertPos.m_iter;
                AZ_Assert(insertPosPtr == 0 && m_start == m_end && m_start == m_last && m_start == insertPosPtr, "vector::insert_iter - This is allowed only if the container has no elements");
            }
#else
            const_pointer insertPosPtr = const_cast<pointer>(insertPos);
#endif

            size_type offset = insertPosPtr - m_start;
            insert(insertPos, (size_type)1, value);
            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)) + offset;
        }

        void    insert(const_iterator insertPos, size_type numElements, const_reference value)
        {
            if (numElements == 0)
            {
                return;
            }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer insertPosPtr;
            if (m_start != 0)
            {
                insertPosPtr = const_cast<pointer>(insertPos.get_iterator());   // we validate iterators only if this container has elements. It is possible all to be 0.
            }
            else
            {
                AZ_Assert(insertPos.m_container == this, "Iterator doesn't belong to this container");
                insertPosPtr = const_cast<pointer>(insertPos.m_iter);
                AZ_Assert(insertPosPtr == 0 && m_start == m_end && m_start == m_last && m_start == insertPosPtr, "vector::insert_iter - This is allowed only if the container has no elements");
            }
#else
            pointer insertPosPtr = const_cast<pointer>(insertPos);
#endif
            size_type capacity   = m_end - m_start;
            size_type size       = m_last - m_start;
            size_type newSize    = size + numElements;
            size_type expandedSize = 0;

            // if we have an allocated block and we need more memory, try to expand first!
            if (m_start && capacity < newSize)
            {
                expandedSize = m_allocator.resize(m_start, newSize * sizeof(node_type));
                if (expandedSize % sizeof(node_type) == 0)
                {
                    size_type expandedCapacity = expandedSize / sizeof(node_type);
                    if (capacity < expandedCapacity)
                    {
                        capacity = expandedCapacity;
                        m_end = m_start + expandedCapacity;
                    }
                }
            }

            if (capacity < newSize)
            {
                // Not enough room so reallocate and copy.
                // grow by 50%, if we can.
                capacity += capacity / 2;
                if (capacity < newSize)
                {
                    capacity = newSize;
                }

                size_type byteSize = capacity * sizeof(node_type);

                pointer newStart = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                // Copy the elements before insert position.
                pointer newLast = AZStd::uninitialized_move(m_start, insertPosPtr, newStart);
                // add new data
                AZStd::uninitialized_fill_n(newLast, numElements, value);

                newLast += numElements;
                // Copy the elements after the insert position.
                newLast = AZStd::uninitialized_move(insertPosPtr, m_last, newLast);

                // Destroy old array
                if (m_start)
                {
                    // Call destructor if we need to.
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory (if needed).
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), expandedSize);
                }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_all();
#endif
                m_start = newStart;
                m_last  = newLast;
                m_end   = m_start + capacity;
            }
            else if (size_type(m_last - insertPosPtr) < numElements)
            {
                pointer newLast;
                // Number of elements we can just set not init needed.
                size_type numInitializedToFill = size_type(m_last - insertPosPtr);

                const_pointer valuePtr = AZStd::addressof(value);
                if (valuePtr >= insertPosPtr && valuePtr < m_last)
                {
                    // value is in the vector make a copy first
                    value_type valueCopy = value;

                    // Move the elements after insert position.
                    newLast = AZStd::uninitialized_move(insertPosPtr, m_last, insertPosPtr + numElements);

                    // Add new elements to uninitialized elements.
                    AZStd::uninitialized_fill_n(m_last, numElements - numInitializedToFill, valueCopy);

                    // Add new data
                    AZStd::fill_n(insertPosPtr, numInitializedToFill, valueCopy);
                }
                else
                {
                    // Move the elements after insert position.
                    newLast = AZStd::uninitialized_move(insertPosPtr, m_last, insertPosPtr + numElements);

                    // Add new elements to uninitialized elements.
                    AZStd::uninitialized_fill_n(m_last, numElements - numInitializedToFill, value);

                    // Add new data
                    AZStd::fill_n(insertPosPtr, numInitializedToFill, value);
                }

                m_last = newLast;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(insertPosPtr, m_last);
#endif
            }
            else
            {
                pointer newLast;
                pointer nonOverlap = m_last - numElements;
                const_pointer valuePtr = AZStd::addressof(value);
                if (valuePtr >= insertPosPtr && valuePtr < m_last)
                {
                    // value is in the vector make a copy first
                    value_type valueCopy = value;

                    // first copy the data that will not overlap.
                    newLast = AZStd::uninitialized_move(nonOverlap, m_last, m_last);

                    // move the area with overlapping
                    AZStd::move_backward(insertPosPtr, nonOverlap, m_last);

                    // add new elements
                    AZStd::fill_n(insertPosPtr, numElements, valueCopy);
                }
                else
                {
                    // first copy the data that will not overlap.
                    newLast = AZStd::uninitialized_move(nonOverlap, m_last, m_last);

                    // move the area with overlapping
                    AZStd::move_backward(insertPosPtr, nonOverlap, m_last);

                    // add new elements
                    AZStd::fill_n(insertPosPtr, numElements, value);
                }

                m_last = newLast;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(insertPosPtr, m_last);
#endif
            }
        }

        template<class InputIterator>
        AZ_FORCE_INLINE void        insert(const_iterator insertPos, const InputIterator& first, const InputIterator& last)
        {
            insert_impl(insertPos, first, last, is_integral<InputIterator>());
        }

        AZ_FORCE_INLINE void insert(const_iterator insertPos, std::initializer_list<T> ilist)
        {
            insert_impl(insertPos, ilist.begin(), ilist.end(), is_integral<std::initializer_list<T> >());
        }

        inline iterator erase(const_iterator elementIter)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer element = const_cast<pointer>(elementIter.get_iterator());
            orphan_range(element, m_last);
#else
            pointer element = const_cast<pointer>(elementIter);
#endif
            size_type offset = element - m_start;
            // unless we have 1 elements we have memory overlapping, so we need to use move.
            AZStd::move(element + 1, m_last, element);
            --m_last;
            Internal::destroy<pointer>::single(m_last);

            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)) + offset;
        }

        inline iterator erase(const_iterator first, const_iterator last)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            pointer firstPtr = const_cast<pointer>(first.get_iterator());
            pointer lastPtr = const_cast<pointer>(last.get_iterator());
            orphan_range(firstPtr, m_last);
#else
            pointer firstPtr = const_cast<pointer>(first);
            pointer lastPtr = const_cast<pointer>(last);
#endif

            size_type offset = firstPtr - m_start;
            // unless we have 1 elements we have memory overlapping, so we need to use move.
            pointer newLast = AZStd::move(lastPtr, m_last, firstPtr);
            Internal::destroy<pointer>::range(newLast, m_last);
            m_last = newLast;
            return iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)) + offset;
        }

        AZ_FORCE_INLINE void        clear()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            Internal::destroy<pointer>::range(m_start, m_last);
            m_last = m_start;
        }
        AZ_INLINE void      swap(this_type& rhs)
        {
            if (m_allocator == rhs.m_allocator)
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
                // Same allocator, swap the vectors.
                AZStd::swap(m_start, rhs.m_start);
                AZStd::swap(m_last, rhs.m_last);
                AZStd::swap(m_end, rhs.m_end);
            }
            else
            {
                this_type temp(m_allocator);
                // Different allocators, move elements
                for (auto& element : * this)
                {
                    temp.emplace_back(AZStd::move(element));
                }
                clear();
                for (auto& element : rhs)
                {
                    emplace_back(AZStd::move(element));
                }
                rhs.clear();
                for (auto& element : temp)
                {
                    rhs.emplace_back(AZStd::move(element));
                }
            }
        }

        /**
         * \anchor VectorExtensions
         * \name Extensions
         * @{
         */

        /// TR1 Extension. Return pointer to the vector data. The vector data is guaranteed to be stored as an array.
        AZ_FORCE_INLINE pointer         data()          { return m_start; }
        AZ_FORCE_INLINE const_pointer   data() const    { return m_start; }

        /// The only difference from the standard is that we return the allocator instance, not a copy.
        AZ_FORCE_INLINE allocator_type&         get_allocator()         { return m_allocator; }
        AZ_FORCE_INLINE const allocator_type&   get_allocator() const   { return m_allocator; }
        /// Set the vector allocator. If different than then current all elements will be reallocated.
        void                                    set_allocator(const allocator_type& allocator)
        {
            if (m_allocator != allocator)
            {
                allocator_type newAllocator = allocator;
                if (m_start != 0)
                {
                    // if we have something allocated make sure we re-alloc the vector to the new allocator.
                    size_type size = m_last - m_start;

                    size_type byteSize = sizeof(node_type) * size;

                    pointer newStart = reinterpret_cast<pointer>(newAllocator.allocate(byteSize, alignment_of<node_type>::value));
                    pointer newLast = AZStd::uninitialized_move(m_start, m_last, newStart);

                    // destroy objects
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory (if needed).
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), 0);

                    m_start = newStart;
                    m_last  = newLast;
                    m_end   = newLast;

    #ifdef AZSTD_HAS_CHECKED_ITERATORS
                    orphan_all();
    #endif
                }

                m_allocator = newAllocator;
            }
        }

        // Validate container status.
        AZ_FORCE_INLINE bool    validate() const
        {
            if (m_last > m_end || m_start > m_last)
            {
                return false;
            }
            return true;
        }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        AZ_FORCE_INLINE int     validate_iterator(const iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            pointer iterPtr = iter.m_iter;
#else
            pointer iterPtr = iter;
#endif
            if (iterPtr < m_start || iterPtr > m_last)
            {
                return isf_none;
            }
            else if (iterPtr == m_last)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        AZ_FORCE_INLINE int     validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
            const_pointer iterPtr = iter.m_iter;
#else
            const_pointer iterPtr = iter;
#endif
            if (iterPtr < m_start || iterPtr > m_last)
            {
                return isf_none;
            }
            else if (iterPtr == m_last)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }
        AZ_FORCE_INLINE int     validate_iterator(const reverse_iterator& iter) const       { return validate_iterator(iter.base()); }
        AZ_FORCE_INLINE int     validate_iterator(const const_reverse_iterator& iter) const { return validate_iterator(iter.base()); }

        /**
         *  Pushes an element at the end of the vector without a provided instance. This can be used for value types
         *  with expensive constructors so we don't want to create temporary one.
         */
        inline void push_back()
        {
            size_type size      = m_last - m_start;
            size_type capacity  = m_end - m_start;
            if (size >= capacity)
            {
                // Reallocate - this is so expensive that I rather just call regular push, with a copy of a default object;
                push_back(value_type());
            }
            else
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(m_last, m_last);
#endif
                Internal::construct<pointer>::single(m_last);
                ++m_last;
            }
        }


        /**
        * Resets the container without deallocating any memory or calling any destructor.
        * This function should be used when we need very quick tear down. Generally it's used for temporary vectors and we can just nuke them that way.
        * In addition the provided \ref Allocators, has leak and reset flag which will enable automatically this behavior. So this function should be used in
        * special cases \ref AZStdExamples.
        * \note This function is added to the vector for consistency. In the vector case we have only one allocation, and if the allocator allows memory leaks
        * it can just leave deallocate function empty, which performance wise will be the same. For more complex containers this will make big difference.
        */
        AZ_FORCE_INLINE void leak_and_reset()
        {
            m_start = 0;
            m_end = 0;
            m_last = 0;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

        /**
         * Set the capacity of the vector, if necessary it will destroy elements at the end of the container to match the new capacity.
         */
        void                set_capacity(size_type numElements)
        {
            // sets the new capacity of the vector, can be smaller than size()
            size_type capacity = m_end - m_start;
            size_type expandedSize = 0;

            // if we have an allocated block and we need more memory, try to expand first!
            if (m_start && capacity < numElements)
            {
                expandedSize = m_allocator.resize(m_start, numElements * sizeof(node_type));
                if (expandedSize % sizeof(node_type) == 0)
                {
                    size_type expandedCapacity = expandedSize / sizeof(node_type);
                    if (expandedCapacity >= numElements)
                    {
                        m_end = m_start + expandedCapacity;
                        return;
                    }
                }
            }
            if (capacity != numElements)
            {
                pointer newStart;
                pointer newLast;

                if (numElements > 0)
                {
                    size_type size = m_last - m_start;
                    size_type numMoved = (size > numElements) ? numElements : size;

                    // need more capacity - reallocate
                    size_type byteSize = sizeof(node_type) * numElements;

                    newStart = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));

                    if (numMoved > 0)
                    {
                        newLast  = AZStd::uninitialized_move(m_start, m_start + numMoved, newStart);
                    }
                    else
                    {
                        newLast = newStart;
                    }
                }
                else
                {
                    newStart = 0;
                    newLast = 0;
                }

                // Destroy old array
                if (m_start)
                {
                    // Call destructor if we need to.
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory (if needed).
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), expandedSize);
                }

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_all();
#endif
                m_start = newStart;
                m_last  = newLast;
                m_end   = newStart + numElements;
            }
        }

        /// @}
    private:

        //#pragma region Deallocate memory specializations
        AZ_FORCE_INLINE void    deallocate_memory(const true_type& /* allocator::allow_memory_leaks */, size_type /*expandedSize*/)
        {
        }

        AZ_FORCE_INLINE void    deallocate_memory(const false_type& /* !allocator::allow_memory_leaks */, size_type expandedSize)
        {
            size_type byteSize = (expandedSize == 0) ? (sizeof(node_type) * (m_end - m_start)) : expandedSize;
            m_allocator.deallocate(m_start, byteSize, alignment_of<node_type>::value);
        }
        //#pragma endregion

        //#pragma region Insert iterator specializations
        template<class Iterator>
        AZ_FORCE_INLINE void insert_impl(const_iterator insertPos, const Iterator& first, const Iterator& last, const true_type& /* is_integral<Iterator> */)
        {
            // we actually are calling this with integral types.
            insert(insertPos, (size_type)first, (const_reference)last);
        }

        template<class Iterator>
        AZ_FORCE_INLINE void insert_impl(const_iterator insertPos, const Iterator& first, const Iterator& last, const false_type& /* is_integral<Iterator> */)
        {
            // specialize for specific interators.
            insert_iter(insertPos, first, last, typename iterator_traits<Iterator>::iterator_category());
        }

        template<class Iterator>
        void insert_iter(const_iterator insertPos, const Iterator& first, const Iterator& last, const forward_iterator_tag&)
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            // We can template this func if we want to check if first and last belong to the same container. It's possible
            // for them to be pointers or unchecked iterators.
            pointer insertPosPtr;
            if (m_start != 0)
            {
                insertPosPtr = const_cast<pointer>(insertPos.get_iterator());   // we validate iterators only if this container has elements. It is possible all to be 0.
            }
            else
            {
                AZ_Assert(insertPos.m_container == this, "Iterator doesn't belong to this container");
                insertPosPtr = const_cast<pointer>(insertPos.m_iter);
                AZ_Assert(insertPosPtr == 0 && m_start == m_end && m_start == m_last && m_start == insertPosPtr, "vector::insert_iter - This is allowed only if the container has no elements");
            }
#else
            pointer insertPosPtr = const_cast<pointer>(insertPos);
#endif

            size_type numElements = distance(first, last);
            if (numElements == 0)
            {
                return;
            }
            //AZSTD_CONTAINER_ASSERT(numElements>0,("AZStd::vector<T>::insert<Iterator> - No point to insert 0 elements!"));

            size_type capacity = m_end - m_start;
            size_type size = m_last - m_start;
            size_type newSize = size + numElements;
            size_type expandedSize = 0;

            // if we have an allocated block and we need more memory, try to expand first!
            if (m_start && capacity < newSize)
            {
                expandedSize = m_allocator.resize(m_start, newSize * sizeof(node_type));
                if (expandedSize % sizeof(node_type) == 0) // make sure it's the exact number of nodes otherwise we can't compute the size on free
                {
                    size_type expandedCapacity = expandedSize / sizeof(node_type);
                    if (capacity < expandedCapacity)
                    {
                        capacity = expandedCapacity;
                        m_end = m_start + expandedCapacity;
                    }
                }
            }

            if (capacity <  newSize)
            {
                // No enough room, reallocate

                // grow by 50%, if we can.
                capacity += capacity / 2;
                if (capacity < newSize)
                {
                    capacity = newSize;
                }

                size_type byteSize = capacity * sizeof(node_type);

                pointer newStart = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
                // Copy the elements before insert position.
                pointer newLast = AZStd::uninitialized_move(m_start, insertPosPtr, newStart);
                // add new data (just copy no move)
                newLast = AZStd::uninitialized_copy(first, last, newLast);
                // Copy the elements after the insert position.
                newLast = AZStd::uninitialized_move(insertPosPtr, m_last, newLast);

                // Destroy old array
                if (m_start)
                {
                    // Call destructor if we need to.
                    Internal::destroy<pointer>::range(m_start, m_last);
                    // Free memory (if needed).
                    deallocate_memory(typename allocator_type::allow_memory_leaks(), expandedSize);
                }

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_all();
#endif

                m_start = newStart;
                m_last  = newLast;
                m_end   = m_start + capacity;
            }
            else if (size_type(m_last - insertPosPtr) < numElements)
            {
                // Copy the elements after insert position.
                pointer newLast = AZStd::uninitialized_move(insertPosPtr, m_last, insertPosPtr + numElements);

                // Number of elements we can assign.
                size_type numInitializedToFill = size_type(m_last - insertPosPtr);

                // get last iterator to fill
                Iterator lastToAssign = first;
                AZStd::advance(lastToAssign, numInitializedToFill);

                // Add new elements to uninitialized elements.
                AZStd::uninitialized_copy(lastToAssign, last, m_last);

                m_last = newLast;

                // Add assign new data
                AZStd::copy(first, lastToAssign, insertPosPtr);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(insertPosPtr, m_last);
#endif
            }
            else
            {
                // We need to copy data in a careful way.

                // first copy the data that will not overlap.
                pointer nonOverlap = m_last - numElements;
                pointer newLast = AZStd::uninitialized_move(nonOverlap, m_last, m_last);

                // move the area with overlapping
                AZStd::move_backward(insertPosPtr, nonOverlap, m_last);

                // add new elements
                AZStd::copy(first, last, insertPosPtr);

                m_last = newLast;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
                orphan_range(insertPosPtr, m_last);
#endif
            }
        }


        template<class Iterator>
        inline void insert_iter(const_iterator insertPos, const Iterator& first, const Iterator& last, const input_iterator_tag&)
        {
            const_iterator start(AZSTD_POINTER_ITERATOR_PARAMS(m_start));
            size_type offset = AZStd::distance(start, insertPos);

            Iterator iter(first);
            for (; iter != last; ++iter, ++offset)
            {
                insert(start + offset, *iter);
            }
        }
        //#pragma endregion

        //#pragma region Construct interator specializations (construct_iter)
        template <class InputIterator>
        inline void construct_iter(const InputIterator& first, const InputIterator& last, const true_type& /* is_integral<InputIterator> */)
        {
            size_type numElements = first;
            value_type value = last;

            // ok so we did not really mean iterators when the called this function.
            size_type byteSize = sizeof(node_type) * numElements;

            m_start = reinterpret_cast<pointer>(m_allocator.allocate(byteSize, alignment_of<node_type>::value));
            m_end   = m_start + numElements;
            AZStd::uninitialized_fill_n(m_start, numElements, value);
            m_last  = m_end;

#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void    construct_iter(const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            insert(const_iterator(AZSTD_POINTER_ITERATOR_PARAMS(m_start)), first, last);
        }
        //#pragma endregion

        //#pragma region Assign iterator specializations (assign_iter)
        template <class InputIterator>
        AZ_FORCE_INLINE void    assign_iter(const InputIterator& numElements, const InputIterator& value, const true_type& /* is_integral<InputIterator> */)
        {
            assign((size_type)numElements, value);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void    assign_iter(const InputIterator& first, const InputIterator& last, const false_type& /* !is_integral<InputIterator> */)
        {
            clear();
            insert(begin(), first, last);
        }
        //#pragma endregion

    protected:
        pointer         m_start;            ///< Pointer to the first allocated element of the array.
        pointer         m_last;             ///< Pointer after the last used element.
        pointer         m_end;              ///< Pointer after the last allocated element.
        allocator_type  m_allocator;        ///< Instance of the allocator.

        // Debug
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        inline void orphan_range(pointer first, pointer last) const
        {
#ifdef AZSTD_CHECKED_ITERATORS_IN_MULTI_THREADS
            AZ_GLOBAL_SCOPED_LOCK(get_global_section());
#endif
            Debug::checked_iterator_base* iter = m_iteratorList;
            while (iter != 0)
            {
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "vector::orphan_range - iterator was corrupted!");
                pointer iterPtr = static_cast<iterator*>(iter)->m_iter;

                if (iterPtr >= first && iterPtr <= last)
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

    //#pragma region Vector equality/inequality
    template <class T, class Allocator>
    AZ_FORCE_INLINE bool operator==(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return (a.size() == b.size() && equal(a.begin(), a.end(), b.begin()));
    }

    template <class T, class Allocator>
    AZ_FORCE_INLINE bool operator!=(const vector<T, Allocator>& a, const vector<T, Allocator>& b)
    {
        return !(a == b);
    }
    //#pragma endregion

    template<class T, class Allocator, class U>
    decltype(auto) erase(vector<T, Allocator>& container, const U& value)
    {
        auto iter = AZStd::remove(container.begin(), container.end(), value);
        auto removedCount = AZStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }
    template<class T, class Allocator, class Predicate>
    decltype(auto) erase_if(vector<T, Allocator>& container, Predicate predicate)
    {
        auto iter = AZStd::remove_if(container.begin(), container.end(), predicate);
        auto removedCount = AZStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }
}
