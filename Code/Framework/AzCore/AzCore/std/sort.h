/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_SORT_H
#define AZSTD_SORT_H 1

#include <AzCore/std/createdestroy.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/alignment_of.h>

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Sort algorithms
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    /**
    * \page SortAlgorithms
    *
    * \li partial_sort
    * \li insertion_sort
    * \li sort (quick sort with insertion sort when elements are less than a threshold)
    * \li stable_sort
    * \li radix_sort
    * \li bucket_sort
    * \li shell_sort
    * \li merge_sort
    * \li comb_sort
    */

    namespace Internal
    {
        template<class T, class Allocator>
        class TemporaryBuffer
        {
        public:
            typedef T*                  pointer_type;
            typedef AZStd::size_t       size_type;
            typedef pointer_type        iterator;
            typedef const pointer_type  const_iterator;

            AZ_FORCE_INLINE TemporaryBuffer(size_type capacity)
                : m_data(0)
                , m_capacity(capacity)
                , m_numConstructed(0)
                , m_size(0)
            {
                m_data = reinterpret_cast<pointer_type>(static_cast<void*>(m_allocator.allocate(sizeof(T) * m_capacity, alignof(T))));
            }

            AZ_FORCE_INLINE TemporaryBuffer(size_type capacity, const Allocator& allocator)
                : m_allocator(allocator)
                , m_capacity(capacity)
                , m_numConstructed(0)
                , m_size(0)
            {
                m_data = reinterpret_cast<pointer_type>(static_cast<void*>(m_allocator.allocate(sizeof(T) * m_capacity, alignof(T))));
            }

            AZ_FORCE_INLINE ~TemporaryBuffer()
            {
                if constexpr (!AZStd::is_trivially_destructible_v<T>)
                {
                    if (m_numConstructed > 0)
                    {
                        iterator first = begin();
                        iterator last = first + m_numConstructed;
                        Internal::destroy<iterator>::range(first, last);
                    }
                }

                m_allocator.deallocate(m_data, sizeof(T) * m_capacity, alignment_of<T>::value);
            }

            AZ_FORCE_INLINE size_type   capacity() const    { return m_capacity; }

            template<class InputIterator>
            AZ_FORCE_INLINE void    copy(InputIterator first, InputIterator last)
            {
                size_type newSize = AZStd::distance(first, last);
                AZ_Assert(newSize <= m_capacity, "TemporaryBuffer::copy - new size is bigger than the buffer capacity!");
                if (newSize > m_size)
                {
                    if constexpr (!AZStd::is_trivially_constructible_v<T>)
                    {
                        iterator bufferFirst = begin() + m_numConstructed;
                        iterator bufferLast = begin() + newSize;
                        Internal::construct<iterator>::range(bufferFirst, bufferLast);
                    }
                    m_numConstructed = newSize;
                }

                m_size = newSize;
                Internal::copy(first, last, begin(), Internal::is_fast_copy<InputIterator, iterator>());
            }

            AZ_FORCE_INLINE void set_size(size_type newSize)
            {
                AZ_Assert(newSize <= m_capacity, "TemporaryBuffer::set_size - new size is bigger than the buffer capacity!");
                if (newSize > m_size)
                {
                    if constexpr (!AZStd::is_trivially_constructible_v<T>)
                    {
                        iterator bufferFirst = begin() + m_numConstructed;
                        iterator bufferLast = begin() + newSize;
                        Internal::construct<iterator>::range(bufferFirst, bufferLast);
                    }
                    m_numConstructed = newSize;
                }

                m_size = newSize;
            }

            AZ_FORCE_INLINE iterator begin()        { return m_data; }
            AZ_FORCE_INLINE iterator end()      { return m_data + m_size; }

        private:
            Allocator       m_allocator;
            pointer_type    m_data;
            size_type       m_capacity;
            size_type       m_numConstructed;
            size_type       m_size;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // Merge
    template <class InputIterator1, class InputIterator2, class OutputIterator>
    OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
    {
        // copy merging ranges, both using operator<
        for (; first1 != last1 && first2 != last2; ++result)
        {
            if (*first2 < *first1)
            {
                *result = *first2;
                ++first2;
            }
            else
            {
                *result = *first1;
                ++first1;
            }
        }

        result = Internal::copy(first1, last1, result, Internal::is_fast_copy<InputIterator1, OutputIterator>());   // copy any tail
        return Internal::copy(first2, last2, result, Internal::is_fast_copy<InputIterator2, OutputIterator>());
    }

    template <class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
    OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
    {
        // copy merging ranges, both using operator<
        for (; first1 != last1 && first2 != last2; ++result)
        {
            if (comp(*first2, *first1))
            {
                *result = *first2;
                ++first2;
            }
            else
            {
                *result = *first1;
                ++first1;
            }
        }

        result = Internal::copy(first1, last1, result, Internal::is_fast_copy<InputIterator1, OutputIterator>());   // copy any tail
        return Internal::copy(first2, last2, result, Internal::is_fast_copy<InputIterator1, OutputIterator>());
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2, class BidirectionalIterator3, class Compare>
    BidirectionalIterator3 merge_backward(BidirectionalIterator1 first1, BidirectionalIterator1 last1, BidirectionalIterator2 first2, BidirectionalIterator2 last2, BidirectionalIterator3 result, Compare comp)
    {
        // merge backwards to result, using compare predicate
        for (;; )
        {
            if (first1 == last1)
            {
                return Internal::copy_backward(first2, last2, result, Internal::is_fast_copy<BidirectionalIterator2, BidirectionalIterator3>());
            }
            else if (first2 == last2)
            {
                return Internal::copy_backward(first1, last1, result, Internal::is_fast_copy<BidirectionalIterator1, BidirectionalIterator3>());
            }
            else if (comp(*--last2, *--last1))
            {
                *--result = *last1;
                ++last2;
            }
            else
            {
                *--result = *last2;
                ++last1;
            }
        }
    }
    template<class BidirectionalIterator, class Difference, class Buffer, class Compare>
    void merge_buffered(BidirectionalIterator first, BidirectionalIterator mid, BidirectionalIterator last, Difference count1, Difference count2, Buffer& buffer, Compare comp)
    {
        // merge [first, mid) with [mid, last), using predicate
        if (count1 + count2 == 2)
        {   // order two one-element partitions
            if (*mid < *first)
            {
                AZStd::iter_swap(first, mid);
            }
        }
        else if (count1 <= count2 && count1 <= Difference(buffer.capacity()))
        {   // buffer left partition, then merge
            buffer.copy(first, mid);
            merge(buffer.begin(), buffer.end(), mid, last, first, comp);
        }
        else if (count2 <= Difference(buffer.capacity()))
        {   // buffer right partition, then merge
            buffer.copy(mid, last);
            merge_backward(first, mid, buffer.begin(), buffer.end(), last, comp);
        }
        else
        {   // buffer too small, divide and conquer
            BidirectionalIterator firstn, lastn;
            Difference count1n, count2n;

            if (count2 < count1)
            {   // left larger, cut it in half and partition right to match
                count1n = count1 / 2, count2n = 0;
                firstn = first;
                AZStd::advance(firstn, count1n);
                lastn = AZStd::lower_bound(mid, last, *firstn, comp);
                count2n = AZStd::distance(mid, lastn);
            }
            else
            {   // right larger, cut it in half and partition left to match
                count1n = 0, count2n = count2 / 2;
                lastn = mid;
                AZStd::advance(lastn, count2n);
                firstn = AZStd::upper_bound(first, mid, *lastn, comp);
                count1n = AZStd::distance(first, firstn);
            }

            BidirectionalIterator midn = buffered_rotate(firstn, mid, lastn, count1 - count1n, count2n, buffer);    // rearrange middle
            merge_buffered(first, firstn, midn, count1n, count2n, buffer, comp); // merge each new part
            merge_buffered(midn, lastn, last, count1 - count1n, count2 - count2n, buffer, comp);
        }
    }
    // TEMPLATE FUNCTION inplace_merge
    template<class BidirectionalIterator, class Difference, class Buffer>
    BidirectionalIterator buffered_rotate(BidirectionalIterator first, BidirectionalIterator mid, BidirectionalIterator last, Difference count1, Difference count2, Buffer& buffer)
    {
        // rotate [first, last) using temp buffer
        if (count1 <= count2 && count1 <= Difference(buffer.capacity()))
        {   // buffer left partition, then copy parts
            buffer.copy(first, mid);
            Internal::copy(mid, last, first, Internal::is_fast_copy<BidirectionalIterator, BidirectionalIterator>());
            return Internal::copy_backward(buffer.begin(), buffer.end(), last, Internal::is_fast_copy<typename Buffer::iterator, BidirectionalIterator>());
        }
        else if (count2 <= Difference(buffer.capacity()))
        {   // buffer right partition, then copy parts
            buffer.copy(mid, last);
            Internal::copy_backward(first, mid, last, Internal::is_fast_copy<BidirectionalIterator, BidirectionalIterator>());
            return Internal::copy(buffer.begin(), buffer.end(), first, Internal::is_fast_copy<typename Buffer::iterator, BidirectionalIterator>());
        }
        else
        {   // buffer too small, rotate in place
            AZStd::rotate(first, mid, last);
            AZStd::advance(first, count2);
            return first;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Partial sort
    template <class RandomAccessIterator, class Compare>
    void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last, Compare comp)
    {
        AZStd::make_heap(first, middle, comp);
        for (RandomAccessIterator next = middle; next < last; ++next)
        {
            if (comp(*next, *first))
            {
                AZStd::pop_heap(first, middle, next, comp);
            }
        }
        AZStd::sort_heap(first, middle, comp);
    }

    template <class RandomAccessIterator>
    void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last)
    {
        AZStd::make_heap(first, middle);
        for (RandomAccessIterator next = middle; next < last; ++next)
        {
            if (*next < *first)
            {
                AZStd::pop_heap(first, middle, next);
            }
        }
        AZStd::sort_heap(first, middle);
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Insertion sort
    const int AZSTD_MAX_INSERTION_SORT_SIZE = 32;

    /**
     * insertion_sort using default less predicate.
     * [first, last), using operator<
     */
    template<class RandomAccessIterator>
    void insertion_sort(RandomAccessIterator first, RandomAccessIterator last)
    {
        if (first != last)
        {
            for (RandomAccessIterator next = first; ++next != last; )
            {
                // order next element
                RandomAccessIterator next1 = next;
                typename iterator_traits<RandomAccessIterator>::value_type value = *next;

                if (value < *first)
                {
                    // found new earliest element, move to front
                    AZStd::Internal::move_backward(first, next, ++next1, Internal::is_fast_copy<RandomAccessIterator, RandomAccessIterator>());

                    *first = value;
                }
                else
                {   // look for insertion point after first
                    for (RandomAccessIterator first1 = next1; value < *--first1; next1 = first1)
                    {
                        *next1 = *first1;   // move hole down
                    }
                    *next1 = value; // insert element in hole
                }
            }
        }
    }

    /**
    * insertion_sort using user compare.
    * [first, last)
    */
    template<class RandomAccessIterator, class Compare>
    void insertion_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        if (first != last)
        {
            for (RandomAccessIterator next = first; ++next != last; )
            {
                // order next element
                RandomAccessIterator next1 = next;
                typename iterator_traits<RandomAccessIterator>::value_type value = *next;

                if (comp(value, *first))
                {
                    // found new earliest element, move to front
                    AZStd::Internal::move_backward(first, next, ++next1, Internal::is_fast_copy<RandomAccessIterator, RandomAccessIterator>());

                    *first = value;
                }
                else
                {   // look for insertion point after first
                    for (RandomAccessIterator first1 = next1; comp(value, *--first1); next1 = first1)
                    {
                        *next1 = *first1;   // move hole down
                    }
                    *next1 = value; // insert element in hole
                }
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////

    namespace Internal
    {
        template<class RandomAccessIterator>
        AZ_INLINE void med3(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
        {   // sort median of three elements to middle
            if (*mid < *first)
            {
                AZStd::iter_swap(mid, first);
            }
            if (*last < *mid)
            {
                AZStd::iter_swap(last, mid);
            }
            if (*mid < *first)
            {
                AZStd::iter_swap(mid, first);
            }
        }

        template<class RandomAccessIterator, class Compare>
        AZ_INLINE void med3(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last, Compare comp)
        {   // sort median of three elements to middle
            if (comp(*mid, *first))
            {
                AZStd::iter_swap(mid, first);
            }
            if (comp(*last, *mid))
            {
                AZStd::iter_swap(last, mid);
            }
            if (comp(*mid, *first))
            {
                AZStd::iter_swap(mid, first);
            }
        }

        template<class RandomAccessIterator>
        AZ_INLINE void median(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last)
        {   // sort median element to middle
            if (40 < last - first)
            {   // median of nine
                AZStd::size_t step = (last - first + 1) / 8;
                med3(first, first + step, first + 2 * step);
                med3(mid - step, mid, mid + step);
                med3(last - 2 * step, last - step, last);
                med3(first + step, mid, last - step);
            }
            else
            {
                med3(first, mid, last);
            }
        }

        template<class RandomAccessIterator, class Compare>
        AZ_INLINE void median(RandomAccessIterator first, RandomAccessIterator mid, RandomAccessIterator last, Compare comp)
        {   // sort median element to middle
            if (40 < last - first)
            {   // median of nine
                AZStd::size_t step = (last - first + 1) / 8;
                med3(first, first + step, first + 2 * step, comp);
                med3(mid - step, mid, mid + step, comp);
                med3(last - 2 * step, last - step, last, comp);
                med3(first + step, mid, last - step, comp);
            }
            else
            {
                med3(first, mid, last, comp);
            }
        }

        template<class RandomAccessIterator>
        AZStd::pair<RandomAccessIterator, RandomAccessIterator> unguarded_partition(RandomAccessIterator first, RandomAccessIterator last)
        {   // partition [_First, _Last), using operator<
            RandomAccessIterator mid = first + (last - first) / 2;  // sort median to mid
            median(first, mid, last - 1);
            RandomAccessIterator pfirst = mid;
            RandomAccessIterator plast = pfirst + 1;

            while (first < pfirst
                   && !(*(pfirst - 1) < *pfirst)
                   && !(*pfirst < *(pfirst - 1)))
            {
                --pfirst;
            }
            while (plast < last
                   && !(*plast < *pfirst)
                   && !(*pfirst < *plast))
            {
                ++plast;
            }

            RandomAccessIterator gfirst = plast;
            RandomAccessIterator glast = pfirst;

            for (;; )
            {   // partition
                for (; gfirst < last; ++gfirst)
                {
                    if (*pfirst < *gfirst)
                    {
                        ;
                    }
                    else if (*gfirst < *pfirst)
                    {
                        break;
                    }
                    else
                    {
                        AZStd::iter_swap(plast++, gfirst);
                    }
                }
                for (; first < glast; --glast)
                {
                    if ((*(glast - 1) < *pfirst))
                    {
                        ;
                    }
                    else if (*pfirst < *(glast - 1))
                    {
                        break;
                    }
                    else
                    {
                        AZStd::iter_swap(--pfirst, glast - 1);
                    }
                }
                if (glast == first && gfirst == last)
                {
                    return AZStd::pair<RandomAccessIterator, RandomAccessIterator>(pfirst, plast);
                }

                if (glast == first)
                {           // no room at bottom, rotate pivot upward
                    if (plast != gfirst)
                    {
                        AZStd::iter_swap(pfirst, plast);
                    }
                    ++plast;
                    AZStd::iter_swap(pfirst++, gfirst++);
                }
                else if (gfirst == last)
                {           // no room at top, rotate pivot downward
                    if (--glast != --pfirst)
                    {
                        AZStd::iter_swap(glast, pfirst);
                    }
                    AZStd::iter_swap(pfirst, --plast);
                }
                else
                {
                    AZStd::iter_swap(gfirst++, --glast);
                }
            }
        }

        template<class RandomAccessIterator, class Compare>
        AZStd::pair<RandomAccessIterator, RandomAccessIterator> unguarded_partition(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
        {   // partition [_First, _Last), using operator<
            RandomAccessIterator mid = first + (last - first) / 2;  // sort median to mid
            median(first, mid, last - 1, comp);
            RandomAccessIterator pfirst = mid;
            RandomAccessIterator plast = pfirst + 1;

            while (first < pfirst
                   && !comp(*(pfirst - 1), *pfirst)
                   && !comp(*pfirst, *(pfirst - 1)))
            {
                --pfirst;
            }
            while (plast < last
                   && !comp(*plast, *pfirst)
                   && !comp(*pfirst, *plast))
            {
                ++plast;
            }

            RandomAccessIterator gfirst = plast;
            RandomAccessIterator glast = pfirst;

            for (;; )
            {   // partition
                for (; gfirst < last; ++gfirst)
                {
                    if (comp(*pfirst, *gfirst))
                    {
                        ;
                    }
                    else if (comp(*gfirst, *pfirst))
                    {
                        break;
                    }
                    else
                    {
                        AZStd::iter_swap(plast++, gfirst);
                    }
                }
                for (; first < glast; --glast)
                {
                    if (comp(*(glast - 1), *pfirst))
                    {
                        ;
                    }
                    else if (comp(*pfirst, *(glast - 1)))
                    {
                        break;
                    }
                    else
                    {
                        AZStd::iter_swap(--pfirst, glast - 1);
                    }
                }
                if (glast == first && gfirst == last)
                {
                    return AZStd::pair<RandomAccessIterator, RandomAccessIterator>(pfirst, plast);
                }

                if (glast == first)
                {           // no room at bottom, rotate pivot upward
                    if (plast != gfirst)
                    {
                        AZStd::iter_swap(pfirst, plast);
                    }
                    ++plast;
                    AZStd::iter_swap(pfirst++, gfirst++);
                }
                else if (gfirst == last)
                {           // no room at top, rotate pivot downward
                    if (--glast != --pfirst)
                    {
                        AZStd::iter_swap(glast, pfirst);
                    }
                    AZStd::iter_swap(pfirst, --plast);
                }
                else
                {
                    AZStd::iter_swap(gfirst++, --glast);
                }
            }
        }

        template<class RandomAccessIterator, class Difference>
        void sort_recursive(RandomAccessIterator first, RandomAccessIterator last, Difference ideal)
        {
            Difference count;

            for (; AZSTD_MAX_INSERTION_SORT_SIZE < (count = last - first) && 0 < ideal; )
            {   // divide and conquer by quicksort
                AZStd::pair<RandomAccessIterator, RandomAccessIterator> mid = AZStd::Internal::unguarded_partition(first, last);
                ideal /= 2, ideal += ideal / 2; // allow 1.5 log2(N) divisions

                if ((mid.first - first) < (last - mid.second))
                {   // loop on second half
                    sort_recursive(first, mid.first, ideal);
                    first = mid.second;
                }
                else
                {   // loop on first half
                    sort_recursive(mid.second, last, ideal);
                    last = mid.first;
                }
            }

            if (count > AZSTD_MAX_INSERTION_SORT_SIZE)
            {   // heap sort if too many divisions
                AZStd::make_heap(first, last);
                AZStd::sort_heap(first, last);
            }
            else if (count > 1)
            {
                AZStd::insertion_sort(first, last); // small
            }
        }

        template<class RandomAccessIterator, class Difference, class Compare>
        void sort_recursive(RandomAccessIterator first, RandomAccessIterator last, Difference ideal, Compare comp)
        {
            Difference count;

            for (; AZSTD_MAX_INSERTION_SORT_SIZE < (count = last - first) && 0 < ideal; )
            {   // divide and conquer by quicksort
                AZStd::pair<RandomAccessIterator, RandomAccessIterator> mid = AZStd::Internal::unguarded_partition(first, last, comp);
                ideal /= 2, ideal += ideal / 2; // allow 1.5 log2(N) divisions

                if ((mid.first - first) < (last - mid.second))
                {   // loop on second half
                    sort_recursive(first, mid.first, ideal, comp);
                    first = mid.second;
                }
                else
                {   // loop on first half
                    sort_recursive(mid.second, last, ideal, comp);
                    last = mid.first;
                }
            }

            if (count > AZSTD_MAX_INSERTION_SORT_SIZE)
            {   // heap sort if too many divisions
                AZStd::make_heap(first, last, comp);
                AZStd::sort_heap(first, last, comp);
            }
            else if (count > 1)
            {
                AZStd::insertion_sort(first, last, comp);   // small
            }
        }

        template<class BidirectionalIterator, class OutputIterator, class DifferenceType, class Compare>
        void chunked_merge(BidirectionalIterator first, BidirectionalIterator last, OutputIterator result, DifferenceType chunk, DifferenceType count, Compare comp)
        {
            // copy merging chunks, using operator<
            for (DifferenceType chunk2 = chunk * 2; chunk2 <= count; count -= chunk2)
            {   // copy merging pairs of adjacent chunks
                BidirectionalIterator mid1 = first;
                AZStd::advance(mid1, chunk);
                BidirectionalIterator mid2 = mid1;
                AZStd::advance(mid2, chunk);

                result = AZStd::merge(first, mid1, mid1, mid2, result, comp);
                first = mid2;
            }

            if (count <= chunk)
            {
                Internal::copy(first, last, result, Internal::is_fast_copy<BidirectionalIterator, OutputIterator>());   // copy partial last chunk
            }
            else
            {
                // copy merging whole and partial last chunk
                BidirectionalIterator mid = first;
                AZStd::advance(mid, chunk);

                AZStd::merge(first, mid, mid, last, result, comp);
            }
        }
        template<class BidirectionalIterator, class DifferenceType, class Buffer, class Compare>
        void merge_sort_buffered(BidirectionalIterator first, BidirectionalIterator last, DifferenceType count, Buffer& buffer, Compare comp)
        {
            // sort using temp buffer for merges, using predicate
            BidirectionalIterator mid = first;
            for (DifferenceType nleft = count; AZSTD_MAX_INSERTION_SORT_SIZE <= nleft; nleft -= AZSTD_MAX_INSERTION_SORT_SIZE)
            {
                // sort chunks
                BidirectionalIterator midend = mid;
                AZStd::advance(midend, (int)AZSTD_MAX_INSERTION_SORT_SIZE);

                AZStd::insertion_sort(mid, midend, comp);
                mid = midend;
            }
            AZStd::insertion_sort(mid, last, comp); // sort partial last chunk

            for (DifferenceType chunk = AZSTD_MAX_INSERTION_SORT_SIZE; chunk < count; chunk *= 2)
            {
                // merge adjacent pairs of chunks to and from temp buffer
                buffer.set_size(count);
                chunked_merge(first, last, buffer.begin(), chunk, count, comp);
                chunked_merge(buffer.begin(), buffer.end(), first, chunk *= 2, count, comp);
            }
        }
        template< class BidirectionalIterator, class DifferenceType, class Buffer, class Compare >
        void stable_sort_recursive(BidirectionalIterator first, BidirectionalIterator last, DifferenceType count, Buffer& buffer, Compare comp)
        {
            //  sort preserving order of equivalents, using predicate
            if (count <= AZSTD_MAX_INSERTION_SORT_SIZE)
            {
                AZStd::insertion_sort(first, last, comp);   // small
            }
            else
            {   // sort halves and merge
                DifferenceType count2 = (count + 1) / 2;
                BidirectionalIterator mid = first;
                AZStd::advance(mid, count2);
                DifferenceType count3 = count - count2;

                if (count2 <= DifferenceType(buffer.capacity()))
                {   // temp buffer big enough, sort each half using buffer
                    Internal::merge_sort_buffered(first, mid, count2, buffer, comp);
                    Internal::merge_sort_buffered(mid, last, count3, buffer, comp);
                }
                else
                {   // temp buffer not big enough, divide and conquer
                    stable_sort_recursive(first, mid, count2, buffer, comp);
                    stable_sort_recursive(mid, last, count3, buffer, comp);
                }

                merge_buffered(first, mid, last, count2, count3, buffer, comp); // merge sorted halves
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // Sort
    using std::sort;

    //////////////////////////////////////////////////////////////////////////
    // Stable sort
    using std::stable_sort;

    //////////////////////////////////////////////////////////////////////////
    // Bucket sort
    //list sort( s, min, max )
    //  list s;
    //typekey min, max;

    //{
    //  int i;
    //  typekey div, maxb[M], minb[M];
    //  list head[M], t;
    //  struct rec aux;
    //  extern list Last;
    //  if (s==NULL)    return(s);
    //  if (max==min)   {
    //      for (Last=s; Last->next!=NULL; Last = Last->next);
    //      return( s );
    //  }
    //  div = (max-min) / M;    /* Find dividing factor */
    //  if (div==0) div = 1;
    //  for (i=0; i<M; i++) head[i] = NULL;
    //  /* Place records in buckets */
    //  while (s != NULL)   {
    //      i = (s->k-min) / div;
    //      if (i<0) i = 0; else    if (i>=M) i = M-1;
    //      t = s;
    //      s = s->next;
    //      t->next = head[i];
    //      if (head[i]==NULL)  minb[i] = maxb[i] = t->k;
    //      head[i] = t;
    //      if ( t->k > maxb[i] )   maxb[i] = t->k;
    //      if ( t->k < minb[i] )   minb[i] = t->k;
    //  }
    //  /* sort recursively */
    //  t = &aux;
    //  for (i=0; i<M; i++) if (head[i]!=NULL)  {
    //      t->next = sort( head[i], minb[i], maxb[i] );
    //      t = Last;
    //  }
    //  return(aux.next);
    //}

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Shell sort

    //  // TYPE is just a template parameter
    //  // comp defaults to a simple less-than comparison
    //  // the inout is probably unnecessary, I wouldn't have known that back then
    //  void shellSort(inout TYPE[] a, size_t beg = 0, size_t end = 0, cmpType cmp =
    //      null) {
    //          if (cmp is null) cmp = this.comp;
    //          if (end == 0) end = a.length;
    //
    //          // can't figure out how to get it working for arbitrary beg and end, hence kept
    //          separate here
    //              void actualShellSort(inout TYPE[] ra) {
    //                  size_t h = 1, hh = 1;
    //                  while (h < ra.length) {
    //                      // magic numbers from formula:
    //                      //
    //http://www.research.att.com/cgi-bin/access.cgi/as/njas/sequences/eisA.cgi?Anum=A033622
    //                      if (hh % 2)
    //                          h = 8 << hh - 6 << ((hh + 1) / 2) + 1;
    //                      else
    //                          h = 9 << hh - 9 << ( hh      / 2) + 1;
    //                      ++hh;
    //                  }
    //
    //                  while (h > 0) {
    //                      // for each set, of which there are h
    //                      for (size_t i = h - 1; i < ra.length; ++i) {
    //                          // pick last element in set
    //                          TYPE tmp = ra[i];
    //                          size_t j = i;
    //
    //                          // compare tmp to the one before it in the set
    //                          // if they are not in order, continue this loop, moving
    //                          // elements back to make room for tmp
    //                          for (; j >= h && !cmp(tmp, ra[j-h]); j -= h)
    //                              ra[j] = ra[j - h];
    //
    //                          ra[j] = tmp;
    //                      }
    //
    //                      // all h-sets sorted
    //                      h /= 3;
    //                  }
    //          }
    //
    //          if (!smartToSort(a, beg, end, cmp)) return;
    //
    //          TYPE[] foo = a[beg..end];
    //          foo.actualShellSort();
    //  }
    //
    //  // other relevant stuff to get the above to compile (not tested, though):
    //
    //  bit smartToSort(inout TYPE[] a, size_t beg, size_t end, cmpType cmp) {
    //      if (end <= beg)
    //          return false;
    //      if (end - beg > a.length)
    //          return false;
    //
    //      if (end - beg < 2)
    //          return false;
    //      else if (end - beg == 2) {
    //          if (!cmp(a[end - 1], a[beg]))
    //              a.swap(beg, end - 1);
    //          return false;
    //      }
    //
    //      return true;
    //  }
    //
    //  alias int function(in TYPE left, in TYPE right) cmpType;

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Comb sort

    // Algoritm 1
    ///*    Another bubble sort variation, this one makes passes in which the
    //* compared elements are spaced apart, by a smaller distance with each
    //* successive pass.  This is a surprisingly fast algorithm.
    //* Working memory:
    //*     O(1) all cases
    //*/
    //void combSort(uint[] data)
    //  out {
    //      debug (100) printList(data);
    //      checkSorted(data);
    //}
    //body {
    //  const float FACTOR = 0.7;

    //  for (int space = cast(int) (data.length * FACTOR);
    //      space > 1; space *= FACTOR) {
    //          for (int index = 0; index < data.length - space; index++) {
    //              if (data[index] > data[index + space]) {
    //                  swap(data[index], data[index + space]);
    //              }
    //          }
    //  }

    //  lastSwapBubbleSort(data);
    //}

    /// Algorithm 2
    /*static int newGap(int gap) {
        gap = gap * 10 / 13;
        if (gap < 1)
            gap = 1;
        if (gap == 9 || gap == 10)
            gap = 11;
        return gap;
    }

    void combsort(int a[], int length) {
        int gap = length;
        for (;;) {
            int swapped = 0;
            int i;
            gap = newGap(gap);
            for (i = 0; i < length - gap; i++) {
                if (a [i] > a [i + gap]) {
                    swap(a + i, a + i + gap);
                    swapped = 1;
                }
            }
            if (gap == 1 && !swapped)
                break;
        }
    }*/

    // Algoritm 3
    //void combSort11(uint[] a)
    //out {
    //  debug (100) printList(a);
    //  checkSorted(a);
    //} body {
    //  // empirically found to be good at:
    //  // http://world.std.com/~jdveale/combsort.htm
    //  // 1 / (1 - 1 / (e ^ phi)), phi being golden ratio = (sqrt(5) + 1)/2
    //  const real SHRINK_FACTOR =
    //      1.2473309501039786540366528676643474234433714124826L;
    //
    //  bit swapped = false;
    //  size_t gap = a.length;
    //
    //  do {
    //      if (gap > 1) {
    //          gap = cast(size_t)(cast(real) gap / SHRINK_FACTOR);
    //
    //          // hence, comb sort 11
    //          if (gap == 9 || gap == 10)
    //              gap = 11;
    //      }
    //
    //      swapped = false;
    //
    //      for (size_t i = 0; i < a.length - gap; ++i) {
    //          size_t j = i + gap;
    //
    //          if (a[i] > a[j]) {
    //              swap(a[i], a[j]);
    //              swapped = true;
    //          }
    //      }
    //
    //  } while (swapped || gap > 1);
    //}


    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Merge sort
    /*  Buffered merge sort.  There's also an in-place merge sort, but that's
    *   O(n^2) at least in the form I've seen it.  This implementation assumes,
    *   as is true in the benchmarks, that data.length is a power of 2.
    *   Time complexity:
    *       O(n log n) all cases
    *   Working memory:
    *       O(n) all cases
    */
    //void mergeSort(uint[] data)
    //  out {
    //      debug (100) printList(data);
    //      checkSorted(data);
    //}
    //body {
    //  int blockSize = 2;
    //  uint[] aux = new uint[data.length];

    //  void doMerge(uint[] fromFirst, uint[] fromSecond, uint[] to) {
    //      uint* inFirst = fromFirst.ptr, inSecond = fromSecond.ptr;
    //      uint* endFirst = inFirst + fromFirst.length;
    //      uint* endSecond = inSecond + fromSecond.length;

    //      foreach (int index, inout uint dest; to) {
    //          if (inFirst == endFirst) {
    //              to[index..to.length] = inSecond[0..to.length-index];
    //              break;
    //          } else if (inSecond == endSecond) {
    //              to[index..to.length] = inFirst[0..to.length-index];
    //              break;
    //          } else if (*inFirst <= *inSecond) {
    //              dest = *(inFirst++);
    //          } else {
    //              dest = *(inSecond++);
    //          }
    //      }
    //  }

    //  void doPass(uint[] from, uint[] to) {
    //      for (int index = 0; index < from.length; index += 2 * blockSize) {
    //          doMerge(from[index..index + blockSize],
    //              from[index + blockSize..index + 2 * blockSize],
    //              to[index..index + 2 * blockSize]);
    //      }
    //      blockSize *= 2;
    //  }

    //  // order pairs first
    //  for (int index = 0; index < data.length; index += 2) {
    //      if (data[index] <= data[index + 1]) {
    //          aux[index..index+2] = data[index..index+2];
    //      } else {
    //          aux[index] = data[index + 1];
    //          aux[index + 1] = data[index];
    //      }
    //  }

    //  assert (data.length >= 4);
    //  doPass(aux, data);

    //  while (blockSize < data.length) {
    //      doPass(data, aux);
    //      if (blockSize < data.length) {
    //          doPass(aux, data);
    //      } else {
    //          data[] = aux;
    //      }
    //  }
    //}

    //////////////////////////////////////////////////////////////////////////
}

#endif // AZSTD_SORT_H
#pragma once
