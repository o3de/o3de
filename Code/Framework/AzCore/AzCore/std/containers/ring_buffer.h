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
#include <AzCore/std/utils.h>

namespace AZStd
{
    /**
     * Ring buffer container, based on the boost <a href="http://www.boost.org/doc/libs/1_39_0/libs/circular_buffer/doc/circular_buffer.html">circular_buffer</a>.
     */
    template <class T, class Allocator = AZStd::allocator >
    class ring_buffer
#ifdef AZSTD_HAS_CHECKED_ITERATORS
        : public Debug::checked_container_base
#endif
    {
        enum
        {
            CONTAINER_VERSION = 1
        };

        typedef ring_buffer<T, Allocator>            this_type;
    public:
        typedef T                                   value_type;
        typedef T*                                  pointer;
        typedef const T*                            const_pointer;
        typedef T&                                  reference;
        typedef const T&                            const_reference;
        typedef typename Allocator::difference_type difference_type;
        typedef typename Allocator::size_type       size_type;
        typedef Allocator                           allocator_type;

        class const_iterator_impl
        {
            enum
            {
                ITERATOR_VERSION = 1
            };

            friend class ring_buffer;
            typedef const_iterator_impl                 this_type;
            typedef ring_buffer<T, Allocator>            container_type;
        public:
            typedef T                                   value_type;
            typedef AZStd::ptrdiff_t                    difference_type;
            typedef const T*                            pointer;
            typedef const T&                            reference;
            typedef AZStd::random_access_iterator_tag   iterator_category;

            AZ_FORCE_INLINE const_iterator_impl()
                : m_container(0)
                , m_element(0) {}
            //AZ_FORCE_INLINE const_iterator_impl(const this_type& rhs) : m_container(rhs.m_container), m_element(rhs.m_element) {}
            AZ_FORCE_INLINE const_iterator_impl(const container_type* container, pointer element)
                : m_container(container)
                , m_element(const_cast<typename container_type::pointer>(element)) {}
            AZ_FORCE_INLINE this_type& operator = (const this_type& rhs)
            {
                if (this == &rhs)
                {
                    return *this;
                }

                m_container = rhs.m_container;
                m_element = rhs.m_element;
                return *this;
            }
            AZ_FORCE_INLINE reference operator*() const { return *m_element; }
            AZ_FORCE_INLINE pointer operator->() const  { return m_element; }
            AZ_FORCE_INLINE this_type& operator++()
            {
                m_container->increment(m_element);
                if (m_element == m_container->m_last)
                {
                    m_element = 0;
                }
                return *this;
            }
            AZ_FORCE_INLINE this_type operator++(int)   { this_type tmp = *this; ++*this; return tmp; }
            AZ_FORCE_INLINE this_type& operator--()
            {
                if (m_element == 0)
                {
                    m_element = m_container->m_last;
                }
                m_container->decrement(m_element);
                return *this;
            }
            AZ_FORCE_INLINE this_type operator-- (int)  { this_type tmp = *this; --*this; return tmp; }

            AZ_FORCE_INLINE this_type& operator+=(difference_type n)
            {
                if (n > 0)
                {
                    m_element = m_container->add(m_element, n);
                    if (m_element == m_container->m_last)
                    {
                        m_element = 0;
                    }
                }
                else if (n < 0)
                {
                    m_element = m_container->sub(m_element == 0 ? m_container->m_last : m_element, -n);
                }
                return *this;
            }
            AZ_FORCE_INLINE this_type operator+(difference_type n) const { return this_type(*this) += n;  }
            AZ_FORCE_INLINE this_type& operator-=(difference_type n)
            {
                if (n > 0)
                {
                    m_element = m_container->sub(m_element == 0 ? m_container->m_last : m_element, n);
                }
                else if (n < 0)
                {
                    m_element = m_container->add(m_element, -n);
                    if (m_element == m_container->m_last)
                    {
                        m_element = 0;
                    }
                }
                return *this;
            }
            AZ_FORCE_INLINE this_type operator- (difference_type n) const { return this_type(*this) -= n; }
            AZ_FORCE_INLINE difference_type operator-(const this_type& it) const    { return linearize_pointer() - it.linearize_pointer(); }
            AZ_FORCE_INLINE reference operator[] (difference_type n) const { return *(*this + n); }
            AZ_FORCE_INLINE bool operator== (const this_type& rhs) const { return m_element == rhs.m_element; }
            AZ_FORCE_INLINE bool operator!= (const this_type& rhs) const { return m_element != rhs.m_element; }
            AZ_FORCE_INLINE bool operator< (const this_type& rhs) const { return linearize_pointer() < rhs.linearize_pointer(); }
            AZ_FORCE_INLINE bool operator> (const this_type& rhs) const { return linearize_pointer() > rhs.linearize_pointer(); }
            AZ_FORCE_INLINE bool operator<= (const this_type& rhs) const { return !(rhs < *this); }
            AZ_FORCE_INLINE bool operator>= (const this_type& rhs) const { return !(*this < rhs); }

        protected:
            /// Get a pointer which would point to the same element as the iterator in case the circular buffer is linearized.
            AZ_FORCE_INLINE pointer linearize_pointer() const
            {
                return m_element == 0 ? m_container->m_buff + m_container->size() :
                       (m_element < m_container->m_first ? m_element + (m_container->m_end - m_container->m_first)
                        : m_container->m_buff + (m_element - m_container->m_first));
            }
            const container_type*               m_container;
            typename container_type::pointer    m_element;
        };

        class iterator_impl
            : public const_iterator_impl
        {
            typedef iterator_impl               this_type;
            typedef const_iterator_impl         base_type;
            typedef ring_buffer<T, Allocator>    container_type;
        public:
            typedef T*                          pointer;
            typedef T&                          reference;
            typedef AZStd::ptrdiff_t            difference_type;

            AZ_FORCE_INLINE iterator_impl() {}
            //AZ_FORCE_INLINE iterator_impl(const this_type& rhs) : base_type(rhs) {}
            AZ_FORCE_INLINE iterator_impl(const container_type* container, const pointer element)
                : base_type(container, element) {}
            AZ_FORCE_INLINE reference operator*() const { return *base_type::m_element; }
            AZ_FORCE_INLINE pointer operator->() const  { return base_type::m_element; }
            AZ_FORCE_INLINE this_type& operator++ ()        { base_type::operator++(); return *this; }
            AZ_FORCE_INLINE this_type operator ++ (int) { this_type tmp = *this; ++*this; return tmp; }
            AZ_FORCE_INLINE this_type& operator-- ()        { base_type::operator--(); return *this; }
            AZ_FORCE_INLINE this_type operator-- (int)  { this_type tmp = *this; --*this; return tmp; }
            AZ_FORCE_INLINE this_type& operator+= (difference_type n) { base_type::operator+=(n); return *this; }

            AZ_FORCE_INLINE this_type operator+ (difference_type n) const   { return this_type(*this) += n; }
            AZ_FORCE_INLINE this_type& operator-= (difference_type n)       { base_type::operator-=(n); return *this; }
            AZ_FORCE_INLINE this_type operator- (difference_type n) const   { return this_type(*this) -= n; }
            AZ_FORCE_INLINE difference_type operator-(const this_type& rhs) const   { return base_type::linearize_pointer() - rhs.linearize_pointer(); }
            AZ_FORCE_INLINE reference operator[] (difference_type n) const  { return *(*this + n); }
        };

#ifdef AZSTD_HAS_CHECKED_ITERATORS
        typedef Debug::checked_randomaccess_iterator<iterator_impl, this_type>       iterator;
        typedef Debug::checked_randomaccess_iterator<const_iterator_impl, this_type> const_iterator;
#else
        typedef iterator_impl                           iterator;
        typedef const_iterator_impl                     const_iterator;
#endif
        typedef AZStd::reverse_iterator<iterator>       reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;

        typedef AZStd::pair<pointer, size_type>         array_range;
        typedef AZStd::pair<const_pointer, size_type>   const_array_range;

        /**
        * \brief Allocation node type. Common for all AZStd containers.
        * For ring_buffer's case we allocate always "sizeof(node_type)*capacity" block.
        */
        typedef value_type                              node_type;

    public:
        AZ_FORCE_INLINE explicit ring_buffer(const allocator_type& alloc = allocator_type())
            : m_buff(0)
            , m_end(0)
            , m_first(0)
            , m_last(0)
            , m_size(0)
            , m_allocator(alloc) {}
        AZ_FORCE_INLINE explicit ring_buffer(size_type capacity, const allocator_type& alloc = allocator_type())
            : m_size(0)
            , m_allocator(alloc)
        {
            initialize_buffer(capacity);
            m_first = m_last = m_buff;
        }
        AZ_FORCE_INLINE ring_buffer(size_type size, const_reference value, const allocator_type& alloc = allocator_type())
            : m_size(size)
            , m_allocator(alloc)
        {
            initialize_buffer(size, value);
            m_first = m_last = m_buff;
        }
        AZ_FORCE_INLINE ring_buffer(size_type capacity, size_type size, const_reference value, const allocator_type& alloc = allocator_type())
            : m_size(size)
            , m_allocator(alloc)
        {
            AZSTD_CONTAINER_ASSERT(capacity >= size, "AZStd::ring_buffer(capacity,size,value) - capacity must be >= size!");
            initialize_buffer(capacity, value);
            m_first = m_buff;
            m_last = capacity == size ? m_buff : m_buff + size;
        }

        AZ_FORCE_INLINE ring_buffer(const this_type& rhs)
            : m_size(rhs.m_size)
            , m_allocator(rhs.m_allocator)
        {
            if (rhs.capacity() > 0)
            {
                initialize_buffer(rhs.capacity());
                m_first = m_buff;
                m_last = AZStd::uninitialized_copy(rhs.begin(), rhs.end(), m_buff, Internal::is_fast_copy<iterator, pointer>());
                if (m_last == m_end)
                {
                    m_last = m_buff;
                }
            }
            else
            {
                m_buff = m_end = m_first = m_last = 0;
            }
        }

        template <class InputIterator>
        AZ_FORCE_INLINE ring_buffer(InputIterator first, InputIterator last, const allocator_type& alloc = allocator_type())
            : m_allocator(alloc)
        {
            initialize(first, last, is_integral<InputIterator>());
        }

        template <class InputIterator>
        AZ_FORCE_INLINE ring_buffer(size_type capacity, InputIterator first, InputIterator last,    const allocator_type& alloc = allocator_type())
            : m_allocator(alloc)
        {
            initialize(capacity, first, last, is_integral<InputIterator>());
        }

        AZ_FORCE_INLINE ~ring_buffer()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            for (size_type i = 0; i < m_size; ++i, increment(m_first))
            {
                Internal::destroy<pointer>::single(m_first);
            }
            deallocate_memory();
        }

        AZ_FORCE_INLINE this_type& operator = (const this_type& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }
            clear();
            deallocate_memory();
            m_buff = reinterpret_cast<pointer>(static_cast<void*>(m_allocator.allocate(rhs.capacity() * sizeof(node_type), alignof(node_type))));
            m_end = m_buff + rhs.capacity();
            m_first = m_buff;
            m_size = rhs.m_size;
            m_last = AZStd::uninitialized_copy(rhs.begin(), rhs.end(), m_buff, Internal::is_fast_copy<iterator, pointer>());
            if (m_last == m_end)
            {
                m_last = m_buff;
            }
            return *this;
        }

        AZ_FORCE_INLINE iterator            begin() { return iterator(AZSTD_CHECKED_ITERATOR_2(iterator, this, (m_size == 0) ? nullptr : m_first)); }
        AZ_FORCE_INLINE const_iterator  begin() const { return const_iterator(AZSTD_CHECKED_ITERATOR_2(const_iterator, this, (m_size == 0) ? nullptr : m_first)); }
        AZ_FORCE_INLINE iterator            end() { return iterator(AZSTD_CHECKED_ITERATOR_2(iterator, this, nullptr)); }
        AZ_FORCE_INLINE const_iterator  end() const { return const_iterator(AZSTD_CHECKED_ITERATOR_2(const_iterator, this, nullptr)); }

        AZ_FORCE_INLINE reverse_iterator            rbegin() { return reverse_iterator(end()); }
        AZ_FORCE_INLINE const_reverse_iterator  rbegin() const { return const_reverse_iterator(end()); }
        AZ_FORCE_INLINE reverse_iterator            rend() { return reverse_iterator(begin()); }
        AZ_FORCE_INLINE const_reverse_iterator  rend() const { return const_reverse_iterator(begin()); }

        AZ_FORCE_INLINE reference operator[] (size_type index) { return *add(m_first, index); }
        AZ_FORCE_INLINE const_reference operator[] (size_type index) const { return *add(m_first, index); }
        AZ_FORCE_INLINE reference at(size_type index) { return *add(m_first, index); }
        AZ_FORCE_INLINE const_reference at(size_type index) const { return *add(m_first, index); }

        AZ_FORCE_INLINE reference front() { AZSTD_CONTAINER_ASSERT(m_size > 0, "AZStd::ring_buffer::front - container is empty!"); return *m_first; }
        AZ_FORCE_INLINE reference back() { AZSTD_CONTAINER_ASSERT(m_size > 0, "AZStd::ring_buffer::back - container is empty!"); return *((m_last == m_buff ? m_end : m_last) - 1); }
        AZ_FORCE_INLINE const_reference front() const { AZSTD_CONTAINER_ASSERT(m_size > 0, "AZStd::ring_buffer::front - container is empty!"); return *m_first; }
        AZ_FORCE_INLINE const_reference back() const { AZSTD_CONTAINER_ASSERT(m_size > 0, "AZStd::ring_buffer::back - container is empty!"); return *((m_last == m_buff ? m_end : m_last) - 1); }

        /// Get the first continuous array of the internal buffer.
        AZ_FORCE_INLINE array_range array_one() { return array_range(m_first, (m_last <= m_first && (m_size > 0) ? m_end : m_last) - m_first); }
        AZ_FORCE_INLINE array_range array_two() { return array_range(m_buff, m_last <= m_first && (m_size > 0) ? m_last - m_buff : 0); }
        AZ_FORCE_INLINE const_array_range array_one() const { return const_array_range(m_first, (m_last <= m_first && (m_size > 0) ? m_end : m_last) - m_first); }
        AZ_FORCE_INLINE const_array_range array_two() const { return const_array_range(m_buff, m_last <= m_first && (m_size > 0) ? m_last - m_buff : 0); }

        /// Linearize the internal buffer into a continuous array.
        inline pointer linearize()
        {
            if (m_size == 0)
            {
                return 0;
            }
            if (m_first < m_last || m_last == m_buff)
            {
                return m_first;                                        // already linear
            }
            pointer src = m_first;
            pointer dest = m_buff;
            size_type moved = 0;
            size_type constructed = 0;
            for (pointer first = m_first; dest < src; src = first)
            {
                for (size_type ii = 0; src < m_end; ++src, ++dest, ++moved, ++ii)
                {
                    if (moved == size())
                    {
                        first = dest;
                        break;
                    }
                    if (dest == first)
                    {
                        first += ii;
                        break;
                    }
                    if (is_uninitialized(dest))
                    {
                        Internal::construct<pointer>::single(m_last, *src);
                        ++constructed;
                    }
                    else
                    {
                        value_type tmp = *src;
                        replace(src, *dest);
                        replace(dest, tmp);
                    }
                }
            }

            for (src = m_end - constructed; src < m_end; ++src)
            {
                Internal::destroy<pointer>::single(src);
            }
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(m_end - constructed, m_end);
#endif
            m_first = m_buff;
            m_last = add(m_buff, m_size);

            return m_buff;
        }

        AZ_FORCE_INLINE bool is_linearized() const { return m_first < m_last || m_last == m_buff; }

        /// Rotate elements in the <code>circular_buffer</code>. A more effective implementation of <code><a href="http://www.sgi.com/tech/stl/rotate.html">AZStd::rotate</a></code>.
        inline void rotate(iterator new_begin)
        {
            AZSTD_CONTAINER_ASSERT(AZSTD_GET_ITER(new_begin).m_element != 0, "AZStd::ring_buffer::rotate - invalid iterator!");      // check for iterator pointing to end()
            if (full())
            {
                m_first = m_last = const_cast<pointer>(AZSTD_GET_ITER(new_begin).m_element);
            }
            else
            {
                difference_type m = end() - new_begin;
                difference_type n = new_begin - begin();
                if (m < n)
                {
                    for (; m > 0; --m)
                    {
                        push_front(back());
                        pop_back();
                    }
                }
                else
                {
                    for (; n > 0; --n)
                    {
                        push_back(front());
                        pop_front();
                    }
                }
            }
        }

        AZ_FORCE_INLINE size_type size() const { return m_size; }
        AZ_FORCE_INLINE size_type max_size() const { return AZStd::allocator_traits<allocator_type>::max_size(m_allocator) / sizeof(node_type); }
        AZ_FORCE_INLINE bool empty() const { return m_size == 0; }
        AZ_FORCE_INLINE bool full() const { return size_type(m_end - m_buff) == m_size; }
        AZ_FORCE_INLINE size_type free() const { return size_type(m_end - m_buff) - m_size; }
        AZ_FORCE_INLINE size_type capacity() const { return m_end - m_buff; }

        inline void resize(size_type new_size, const_reference value = value_type())
        {
            if (new_size > size())
            {
                if (new_size > capacity())
                {
                    set_capacity(new_size);
                }
                insert(end(), new_size - size(), value);
            }
            else
            {
                iterator e = end();
                erase(e - (size() - new_size), e);
            }
        }

        //AZ_INLINE void rresize(size_type new_size, const_reference value = value_type()) {
        //  if (new_size > size()) {
        //      if (new_size > capacity())
        //          set_capacity(new_size);
        //      rinsert(begin(), new_size - size(), value);
        //  } else {
        //      rerase(begin(), end() - new_size);
        //  }
        //}


        //AZ_FORCE_INLINE void assign(size_type size, const_reference value) { assign_n(n, n, cb_details::assign_n<const_reference, allocator_type>(n, item, m_alloc)); }
        //AZ_FORCE_INLINE void assign(size_type capacity, size_type size, const_reference item) {
        //  AZSTD_CONTAINER_ASSERT(capacity >= size);
        //  assign_n(capacity, n, cb_details::assign_n<const_reference, allocator_type>(n, item, m_alloc));
        //}
        //template <class InputIterator>
        //AZ_FORCE_INLINE void assign(InputIterator first, InputIterator last) { assign(first, last, is_integral<InputIterator>()); }
        //template <class InputIterator>
        //AZ_FORCE_INLINE void assign(size_type capacity, InputIterator first, InputIterator last) { assign(capacity, first, last, is_integral<InputIterator>()); }
        AZ_FORCE_INLINE void swap(this_type& rhs)
        {
            if (m_allocator == rhs.m_allocator)
            {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
                swap_all(rhs);
#endif
                AZStd::swap(m_buff, rhs.m_buff);
                AZStd::swap(m_end, rhs.m_end);
                AZStd::swap(m_first, rhs.m_first);
                AZStd::swap(m_last, rhs.m_last);
                AZStd::swap(m_size, rhs.m_size);
            }
            else
            {
                this_type temp = *this;
                *this = rhs;
                rhs = temp;
            }
        }

        inline void push_back(const_reference value)
        {
            emplace_back(value);
        }

        inline void push_back(value_type&& value)
        {
            emplace_back(AZStd::move(value));
        }

        template<class... Args>
        inline reference emplace_back(Args&&... args)
        {
            if (full())
            {
                AZSTD_CONTAINER_ASSERT(m_size > 0, "Cannot replaces last element in a ring buffer of zero capacity");
                Internal::destroy<pointer>::single(m_last);
                Internal::construct<pointer>::single(m_last, AZStd::forward<Args>(args)...);
                reference emplaceRef = *m_last;
                increment(m_last);
                m_first = m_last;
                return emplaceRef;
            }
            else
            {
                Internal::construct<pointer>::single(m_last, AZStd::forward<Args>(args)...);
                reference emplaceRef = *m_last;
                increment(m_last);
                ++m_size;
                return emplaceRef;
            }
        }

        inline void push_front(const_reference value)
        {
            emplace_front(value);
        }

        inline void push_front(value_type&& value)
        {
            emplace_front(AZStd::move(value));
        }

        template<class... Args>
        inline reference emplace_front(Args&&... args)
        {
            if (full())
            {
                AZSTD_CONTAINER_ASSERT(m_size > 0, "Cannot replaces first element in a ring buffer of zero capacity");
                decrement(m_first);
                Internal::destroy<pointer>::single(m_first);
                Internal::construct<pointer>::single(m_first, AZStd::forward<Args>(args)...);
                reference emplaceRef = *m_first;
                m_last = m_first;
                return emplaceRef;
            }
            else
            {
                decrement(m_first);
                Internal::construct<pointer>::single(m_first, AZStd::forward<Args>(args)...);
                reference emplaceRef = *m_first;
                ++m_size;
                return emplaceRef;
            }
        }
        AZ_FORCE_INLINE void pop_back()
        {
            AZSTD_CONTAINER_ASSERT(m_size > 0, "AZStd::ring_buffer::pop_back - container is empty!");
            decrement(m_last);
            destroy_item(m_last);
            --m_size;
        }
        AZ_FORCE_INLINE void pop_front()
        {
            AZSTD_CONTAINER_ASSERT(m_size > 0, "AZStd::ring_buffer::pop_front - container is empty!");
            destroy_item(m_first);
            increment(m_first);
            --m_size;
        }

        AZ_FORCE_INLINE iterator insert(iterator pos, const_reference value = value_type())
        {
            iterator b = begin();
            if (full() && pos == b)
            {
                return b;
            }
            return insert_item(pos, value);
        }
        inline void insert(iterator pos, size_type size, const_reference value)
        {
            if (size == 0)
            {
                return;
            }
            size_type copy = capacity() - (end() - pos);
            if (copy == 0)
            {
                return;
            }
            if (size > copy)
            {
                size = copy;
            }
            insert_n(pos, size, value);
        }

        template <class InputIterator>
        AZ_FORCE_INLINE void insert(iterator pos, InputIterator first, InputIterator last) { insert(pos, first, last, AZStd::is_integral<InputIterator>()); }

        inline iterator erase(const_iterator constPos)
        {
            iterator pos = AZStd::Internal::ConstIteratorCast<iterator>(constPos);
            pointer next = AZSTD_GET_ITER(pos).m_element;
            AZSTD_CONTAINER_ASSERT(next != 0, "AZStd::ring_buffer::erase - invalid position!");
            increment(next);
            for (pointer p = AZSTD_GET_ITER(pos).m_element; next != m_last; p = next, increment(next))
            {
                replace(p, *next);
            }
            decrement(m_last);
            destroy_item(m_last);
            --m_size;
            return m_last == AZSTD_GET_ITER(pos).m_element ? end() : iterator(AZSTD_CHECKED_ITERATOR_2(iterator_impl, this, AZSTD_GET_ITER(pos).m_element));
        }

        inline iterator erase(const_iterator constFirst, const_iterator constLast)
        {
            iterator first = AZStd::Internal::ConstIteratorCast<iterator>(constFirst);
            iterator last = AZStd::Internal::ConstIteratorCast<iterator>(constLast);
            AZSTD_CONTAINER_ASSERT(first <= last, "AZStd::ring_buffer::front - invalid iterator range!");        // check for wrong range
            if (first == last)
            {
                return first;
            }
            pointer p = AZSTD_GET_ITER(first).m_element;
            while (AZSTD_GET_ITER(last).m_element != 0)
            {
                replace(AZSTD_GET_ITER((first++)).m_element, *last++);
            }
            do
            {
                decrement(m_last);
                destroy_item(m_last);
                --m_size;
            } while (m_last != AZSTD_GET_ITER(first).m_element);
            return m_last == p ? end() : iterator(AZSTD_CHECKED_ITERATOR_2(iterator_impl, this, p));
        }

        inline iterator rerase(iterator pos)
        {
            pointer prev = AZSTD_GET_ITER(pos).m_element;
            AZSTD_CONTAINER_ASSERT(prev != 0, "AZStd::ring_buffer::rerase - invalid position!");
            pointer p = prev;
            for (decrement(prev); p != m_first; p = prev, decrement(prev))
            {
                replace(p, *prev);
            }
            destroy_item(m_first);
            increment(m_first);
            --m_size;
            return p == AZSTD_GET_ITER(pos).m_element ? begin() : iterator(AZSTD_CHECKED_ITERATOR_2(iterator_impl, this, AZSTD_GET_ITER(pos).m_element));
        }

        /// Erase the range <code>[first, last)</code>.
        inline iterator rerase(iterator first, iterator last)
        {
            AZSTD_CONTAINER_ASSERT(first <= last, "AZStd::ring_buffer::rerase - invalid erase range!");        // check for wrong range
            if (first == last)
            {
                return first;
            }
            pointer p = map_pointer(AZSTD_GET_ITER(last).m_element);
            AZSTD_GET_ITER(last).m_element = p;
            while (AZSTD_GET_ITER(first).m_element != m_first)
            {
                decrement(AZSTD_GET_ITER(first).m_element);
                decrement(p);
                replace(p, *AZSTD_GET_ITER(first).m_element);
            }
            do
            {
                destroy_item(m_first);
                increment(m_first);
                --m_size;
            } while (m_first != p);
            if (m_first == AZSTD_GET_ITER(last).m_element)
            {
                return begin();
            }
            decrement(AZSTD_GET_ITER(last).m_element);
            return iterator(AZSTD_CHECKED_ITERATOR_2(iterator_impl, this, AZSTD_GET_ITER(last).m_element));
        }

        AZ_FORCE_INLINE void clear()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            for (size_type i = 0; i < size(); ++i, increment(m_first))
            {
                Internal::destroy<pointer>::single(m_first);
            }
            m_size = 0;
        }

        void set_capacity(size_type new_capacity)
        {
            if (new_capacity == capacity())
            {
                return;
            }
            pointer buff = reinterpret_cast<pointer>(static_cast<void*>(m_allocator.allocate(new_capacity * sizeof(node_type), alignof(node_type))));
            iterator b = begin();
            pointer last = AZStd::uninitialized_copy(b, b + AZStd::GetMin(new_capacity, m_size), buff, Internal::is_fast_copy<iterator, pointer>());

            clear();
            deallocate_memory();

            m_size = last - buff;
            m_buff = m_first = buff;
            m_end = m_buff + new_capacity;
            m_last = (last == m_end) ? m_buff : last;
        }

        void rset_capacity(size_type new_capacity)
        {
            if (new_capacity == capacity())
            {
                return;
            }
            pointer buff = reinterpret_cast<pointer>(static_cast<void*>(m_allocator.allocate(new_capacity * sizeof(node_type), alignof(node_type))));
            iterator e = end();
            pointer last = AZStd::uninitialized_copy(e - AZStd::GetMin(new_capacity, m_size), e, buff, Internal::is_fast_copy<iterator, pointer>());

            clear();
            deallocate_memory();

            m_size = last - buff;
            m_buff = m_first = buff;
            m_end = m_buff + new_capacity;
            m_last = (last == m_end) ? m_buff : last;
        }

        /// Validate container status.
        inline bool     validate() const
        {
            if (m_first < m_buff || m_last < m_buff)
            {
                return false;
            }
            if (m_first > m_end || m_last > m_end)
            {
                return false;
            }
            return true;
        }
        /// Validates an iter iterator. Returns a combination of \ref iterator_status_flag.
        inline int      validate_iterator(const iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
#endif
            pointer iterPtr = AZSTD_GET_ITER(iter).m_element;
            if (m_first < m_last)
            {
                if (iterPtr == 0 /*end*/)
                {
                    return isf_valid;
                }
                else if (iterPtr < m_first || iterPtr > m_last)
                {
                    return isf_none;
                }
            }
            else
            {
                if (iterPtr == 0 /*end*/)
                {
                    return isf_valid;
                }
                else if (iterPtr < m_buff || iterPtr > m_end)
                {
                    return isf_none;
                }
                else if (iterPtr > m_last && iterPtr < m_first)
                {
                    return isf_none;
                }
            }

            return isf_valid | isf_can_dereference;
        }
        inline int      validate_iterator(const const_iterator& iter) const
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            AZ_Assert(iter.m_container == this, "Iterator doesn't belong to this container");
#endif
            pointer iterPtr = AZSTD_GET_ITER(iter).m_element;
            if (m_first < m_last)
            {
                if (iterPtr == 0 /*end*/)
                {
                    return isf_valid;
                }
                else if (iterPtr < m_first || iterPtr > m_last)
                {
                    return isf_none;
                }
            }
            else
            {
                if (iterPtr == 0 /*end*/)
                {
                    return isf_valid;
                }
                else if (iterPtr < m_buff || iterPtr > m_end)
                {
                    return isf_none;
                }
                else if (iterPtr > m_last && iterPtr < m_first)
                {
                    return isf_none;
                }
            }
            return isf_valid | isf_can_dereference;
        }

    private:
        AZ_FORCE_INLINE void    deallocate_memory()
        {
            if (m_buff)
            {
                size_type byteSize = sizeof(node_type) * capacity();
                m_allocator.deallocate(m_buff, byteSize, alignof(node_type));
            }
        }
        AZ_FORCE_INLINE void increment(pointer& p) const
        {
            if (++p == m_end)
            {
                p = m_buff;
            }
        }
        AZ_FORCE_INLINE void decrement(pointer& p) const
        {
            if (p == m_buff)
            {
                p = m_end;
            }
            --p;
        }
        AZ_FORCE_INLINE pointer add(pointer p, difference_type n) const { return p + (n < (m_end - p) ? n : n - capacity()); }
        AZ_FORCE_INLINE pointer sub(pointer p, difference_type n) const { return p - (n > (p - m_buff) ? n - capacity() : n); }
        /// Map the null pointer to virtual end of circular buffer.
        AZ_FORCE_INLINE pointer map_pointer(pointer p) const { return p == 0 ? m_last : p; }
        /// Does the pointer point to the uninitialized memory?
        AZ_FORCE_INLINE bool is_uninitialized(const_pointer p) const { return p >= m_last && (m_first < m_last || p < m_first); }
        AZ_FORCE_INLINE void replace(pointer pos, const_reference value)
        {
            *pos = value;
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(pos, pos);
#endif
        }

        /// Construct or replace an element.
        AZ_FORCE_INLINE void construct_or_replace(bool construct, pointer pos, const_reference value)
        {
            if (construct)
            {
                Internal::construct<pointer>::single(pos, value);
            }
            else
            {
                replace(pos, value);
            }
        }

        //! Destroy an item.
        AZ_FORCE_INLINE void destroy_item(pointer p)
        {
            Internal::destroy<pointer>::single(p);
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_range(p, p);
#endif
        }
        AZ_FORCE_INLINE void initialize_buffer(size_type capacity)
        {
            m_buff = reinterpret_cast<pointer>(static_cast<void*>(m_allocator.allocate(capacity * sizeof(node_type), alignof(node_type))));
            m_end = m_buff + capacity;
        }
        AZ_FORCE_INLINE void initialize_buffer(size_type capacity, const_reference value)
        {
            initialize_buffer(capacity);
            AZStd::uninitialized_fill_n(m_buff, m_size, value, Internal::is_fast_fill<pointer>());
        }
        template <class IntegralType>
        AZ_FORCE_INLINE void initialize(IntegralType n, IntegralType item, const true_type&)
        {
            m_size = static_cast<size_type>(n);
            initialize_buffer(m_size, item);
            m_first = m_last = m_buff;
        }
        template <class Iterator>
        AZ_FORCE_INLINE void initialize(Iterator first, Iterator last, const false_type&)
        {
            initialize(first, last, typename AZStd::iterator_traits<Iterator>::iterator_category());
        }
        //template <class InputIterator>
        //void initialize(InputIterator first, InputIterator last, const AZStd::input_iterator_tag&) {
        //  // for containers
        //  AZStd::deque<value_type, allocator_type> tmp(first, last, m_allocator);
        //  size_type distance = tmp.size();
        //  initialize(distance, tmp.begin(), tmp.end(), distance);
        //}
        template <class ForwardIterator>
        void initialize(ForwardIterator first, ForwardIterator last, const AZStd::forward_iterator_tag&)
        {
            size_type distance = AZStd::distance(first, last);
            initialize(distance, first, last, distance);
        }
        template <class IntegralType>
        void initialize(size_type capacity, IntegralType size, IntegralType value, const true_type&)
        {
            m_size = static_cast<size_type>(size);
            AZSTD_CONTAINER_ASSERT(capacity >= m_size, "AZStd::ring_buffer::initialize - capacity must be >= size!");
            initialize_buffer(capacity, value);
            m_first = m_buff;
            m_last = (capacity == m_size) ? m_buff : m_buff + m_size;
        }
        template <class Iterator>
        void initialize(size_type capacity, Iterator first, Iterator last, const false_type&)
        {
            initialize(capacity, first, last, typename AZStd::iterator_traits<Iterator>::iterator_category());
        }

        //template <class InputIterator>
        //AZ_INLINE void initialize(size_type capacity,InputIterator first,InputIterator last,const AZStd::input_iterator_tag&) {
        //      initialize_buffer(capacity);
        //      m_first = m_last = m_buff;
        //      m_size = 0;
        //      if (capacity == 0)
        //          return;
        //      while (first != last && !full()) {
        //          Internal::construct<pointer>::single(m_last,*first++);
        //          increment(m_last);
        //          ++m_size;
        //      }
        //      while (first != last) {
        //          replace(m_last, *first++);
        //          increment(m_last);
        //          m_first = m_last;
        //      }
        //}

        template <class ForwardIterator>
        AZ_FORCE_INLINE void initialize(size_type capacity, ForwardIterator first, ForwardIterator last, const AZStd::forward_iterator_tag&)
        {
            initialize(capacity, first, last, AZStd::distance(first, last));
        }

        //! Initialize the circular buffer.
        template <class ForwardIterator>
        inline void initialize(size_type capacity, ForwardIterator first, ForwardIterator last, size_type distance)
        {
            initialize_buffer(capacity);
            m_first = m_buff;
            if (distance > capacity)
            {
                AZStd::advance(first, distance - capacity);
                m_size = capacity;
            }
            else
            {
                m_size = distance;
            }

            m_last = AZStd::uninitialized_copy(first, last, m_buff, Internal::is_fast_copy<ForwardIterator, pointer>());
            if (m_last == m_end)
            {
                m_last = m_buff;
            }
        }

        inline void leak_and_reset()
        {
#ifdef AZSTD_HAS_CHECKED_ITERATORS
            orphan_all();
#endif
            m_first = m_last = m_buff = m_end = 0;
            m_size = 0;
        }

        //! Specialized assign method.
        /*template <class IntegralType>
        inline void assign(IntegralType n, IntegralType item, const true_type&) { assign(static_cast<size_type>(n), static_cast<value_type>(item)); }*/

        inline iterator insert_item(const iterator& pos, const_reference value)
        {
            pointer p = AZSTD_GET_ITER(pos).m_element;
            if (p == 0)
            {
                construct_or_replace(!full(), m_last, value);
                p = m_last;
            }
            else
            {
                pointer src = m_last;
                pointer dest = m_last;
                bool construct = !full();
                while (src != p)
                {
                    decrement(src);
                    construct_or_replace(construct, dest, *src);
                    decrement(dest);
                    construct = false;
                }
                replace(p, value);
            }
            increment(m_last);
            if (full())
            {
                m_first = m_last;
            }
            else
            {
                ++m_size;
            }
            return iterator(AZSTD_CHECKED_ITERATOR_2(iterator_impl, this, p));
        }

        template <class IntegralType>
        AZ_FORCE_INLINE void insert(const iterator& pos, IntegralType n, IntegralType item, const true_type&)
        {
            insert(pos, static_cast<size_type>(n), static_cast<value_type>(item));
        }

        template <class Iterator>
        AZ_FORCE_INLINE void insert(const iterator& pos, Iterator first, Iterator last, const false_type&)
        {
            insert(pos, first, last, AZStd::iterator_traits<Iterator>::iterator_category());
        }

        ////! Specialized insert method.
        //template <class InputIterator>
        //AZ_INLINE void insert(iterator pos, InputIterator first, InputIterator last, const AZStd::input_iterator_tag&) {
        //  if (!full() || pos != begin()) {
        //      for (;first != last; ++pos)
        //          pos = insert_item(pos, *first++);
        //  }
        //}

        template <class ForwardIterator>
        inline void insert(const iterator& pos, ForwardIterator first, ForwardIterator last, const AZStd::forward_iterator_tag&)
        {
            size_type size = AZStd::distance(first, last);
            AZSTD_CONTAINER_ASSERT(first > last, "AZStd::ring_buffer::insert - there are no elements to insert!");
            if (size == 0)
            {
                return;
            }
            size_type copy = capacity() - (end() - pos);
            if (copy == 0)
            {
                return;
            }
            if (size > copy)
            {
                AZStd::advance(first, size - copy);
                size = copy;
            }
            insert_n(pos, size, first);
        }

        inline void insert_n(const iterator& pos, size_type size, iterator first)
        {
            size_type construct = free();
            if (construct > size)
            {
                construct = size;
            }
            if (pos.m_element == 0)
            {
                size_type ii = 0;
                pointer p = m_last;
                for (; ii < construct; ++ii, increment(p))
                {
                    Internal::construct<pointer>::single(p, *first++);
                }
                ;
                for (; ii < size; ++ii, increment(p))
                {
                    replace(p, *first++);
                }
            }
            else
            {
                pointer src = m_last;
                pointer dest = add(m_last, size - 1);
                pointer p = pos.m_element;
                size_type ii = 0;
                while (src != pos.m_element)
                {
                    decrement(src);
                    construct_or_replace(is_uninitialized(dest), dest, *src);
                    decrement(dest);
                }
                for (; ii < size; ++ii, increment(p))
                {
                    construct_or_replace(is_uninitialized(p), p, *first++);
                }
            }
            m_last = add(m_last, size);
            m_first = add(m_first, size - construct);
            m_size += construct;
        }

        inline void insert_n(const iterator& pos, size_type size, const_reference value)
        {
            size_type construct = free();
            if (construct > size)
            {
                construct = size;
            }
            if (AZSTD_GET_ITER(pos).m_element == 0)
            {
                size_type ii = 0;
                pointer p = m_last;
                for (; ii < construct; ++ii, increment(p))
                {
                    Internal::construct<pointer>::single(p, value);
                }
                ;
                for (; ii < size; ++ii, increment(p))
                {
                    replace(p, value);
                }
            }
            else
            {
                pointer src = m_last;
                pointer dest = add(m_last, size - 1);
                pointer p = AZSTD_GET_ITER(pos).m_element;
                size_type ii = 0;
                while (src != AZSTD_GET_ITER(pos).m_element)
                {
                    decrement(src);
                    construct_or_replace(is_uninitialized(dest), dest, *src);
                    decrement(dest);
                }
                for (; ii < size; ++ii, increment(p))
                {
                    construct_or_replace(is_uninitialized(p), p, value);
                }
            }
            m_last = add(m_last, size);
            m_first = add(m_first, size - construct);
            m_size += construct;
        }

        pointer     m_buff;     ///< Internal buffer used for storing elements in the circular buffer.
        pointer     m_end;      ///< Internal buffer's end (end of the storage space).
        pointer     m_first;    ///< Virtual beginning of the circular buffer.
        pointer     m_last;     ///< Virtual end of the circular buffer (one behind the last element).
        size_type   m_size; ///< Number of items currently stored in the circular buffer.
        allocator_type  m_allocator;

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
                AZ_Assert(iter->m_container == static_cast<const checked_container_base*>(this), "ring_buffer::orphan_range - iterator was corrupted!");
                pointer iterPtr = static_cast<iterator*>(iter)->m_iter.m_element;
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

    // Non-member functions
    template <class T, class Allocator>
    inline bool operator == (const ring_buffer<T, Allocator>& lhs, const ring_buffer<T, Allocator>& rhs)
    {
        return lhs.size() == rhs.size() && AZStd::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <class T, class Allocator>
    inline bool operator != (const ring_buffer<T, Allocator>& lhs, const ring_buffer<T, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    //template <class T, class Allocator>
    //inline bool operator < (const ring_buffer<T, Allocator>& lhs, const ring_buffer<T, Allocator>& rhs) {
    //  return AZStd::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    //}
    //template <class T, class Allocator>
    //inline bool operator > (const ring_buffer<T, Allocator>& lhs, const ring_buffer<T, Allocator>& rhs) {
    //  return rhs < lhs;
    //}
    //template <class T, class Allocator>
    //inline bool operator <= (const ring_buffer<T, Allocator>& lhs, const ring_buffer<T, Allocator>& rhs) {
    //  return !(rhs < lhs);
    //}
    //template <class T, class Allocator>
    //inline bool operator >= (const ring_buffer<T, Allocator>& lhs, const ring_buffer<T, Allocator>& rhs) {
    //  return !(lhs < rhs);
    //}

    template <class T, class Allocator>
    AZ_FORCE_INLINE void swap(ring_buffer<T, Allocator>& lhs, ring_buffer<T, Allocator>& rhs)
    {
        lhs.swap(rhs);
    }
}
