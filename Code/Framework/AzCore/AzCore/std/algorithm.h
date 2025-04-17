/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/createdestroy.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/functional_basic.h>
#include <AzCore/std/typetraits/common_type.h>
#include <AzCore/std/typetraits/remove_cvref.h>
#include <math.h>
#include <cmath>

#include <algorithm>

namespace AZStd
{
    /**
    * \page Algorithms
    * \subpage SortAlgorithms Sort Algorithms
    *
    * Search algorithms
    */

    //////////////////////////////////////////////////////////////////////////
    // Min, max, clamp
    template<class T>
    constexpr T GetMin(const T& left, const T& right) { return (left < right) ? left : right; }

    template<class T>
    constexpr T min AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right) { return (left < right) ? left : right; }

    template<class T>
    constexpr T GetMax(const T& left, const T& right) { return (left > right) ? left : right; }

    template<class T>
    constexpr T max AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right) { return (left > right) ? left : right; }

    template<class T, class Compare>
    constexpr pair<T, T> minmax AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right, Compare comp) { return comp(right, left) ? AZStd::make_pair(right, left) : AZStd::make_pair(left, right); }

    template<class T>
    constexpr pair<T, T> minmax AZ_PREVENT_MACRO_SUBSTITUTION (const T& left, const T& right) { return AZStd::minmax(left, right, AZStd::less<T>()); }

    using ::floorf;
    using ::ceilf;
    using ::roundf;
    using ::rintf;

    /*
    Finds the smallest and greatest element in the range of [first, last)
    returns a pair consisting of an iterator to the smallest element in .first and an iterator to the largest element in .second.
    If several elements are equivalent to the smallest element it returns the first such element
    If several elements are equivalent to the greatest element it returns the last such element
    */
    template<class ForwardIt, class Compare>
    constexpr pair<ForwardIt, ForwardIt> minmax_element(ForwardIt first, ForwardIt last, Compare comp)
    {
        pair<ForwardIt, ForwardIt> result(first, first);
        // Check for 0 elements in iterator range and return a pair of (first, first)
        if (first == last)
        {
            return result;
        }

        while (++first != last)
        {
            ForwardIt next = first;
            // 1 element left to iterate
            if (++next == last)
            {
                if (comp(*first, *result.first))
                {
                    result.first = first;
                }
                else if (!comp(*first, *result.second))
                {
                    result.second = first;
                }
            }
            // 2+ elements left to iterate
            else
            {
                if (comp(*next, *first))
                {
                    if (comp(*next, *result.first))
                    {
                        result.first = next;
                    }
                    if (!comp(*first, *result.second))
                    {
                        result.second = first;
                    }
                }
                else
                {
                    if (comp(*first, *result.first))
                    {
                        result.first = first;
                    }
                    if (!comp(*next, *result.second))
                    {
                        result.second = next;
                    }
                    
                }
            }
        }

        return result;
    }

    template<class ForwardIt>
    constexpr pair<ForwardIt, ForwardIt> minmax_element(ForwardIt first, ForwardIt last)
    {
        return AZStd::minmax_element(first, last, AZStd::less<typename iterator_traits<ForwardIt>::value_type>());
    }

    template<class T, class Compare>
    constexpr pair<T, T> minmax AZ_PREVENT_MACRO_SUBSTITUTION (std::initializer_list<T> ilist, Compare comp)
    {
        auto minMaxPair = AZStd::minmax_element(ilist.begin(), ilist.end(), comp);
        return AZStd::make_pair(*minMaxPair.first, *minMaxPair.second);
    }

    template<class T>
    constexpr pair<T, T> minmax AZ_PREVENT_MACRO_SUBSTITUTION (std::initializer_list<T> ilist)
    {
        return AZStd::minmax(ilist, AZStd::less<T>());
    }
    
    template<class T>
    constexpr T clamp(const T& val, const T& lower, const T& upper) { return GetMin(upper, GetMax(val, lower)); }

    namespace Internal
    {
        // mismatch helper functions
        template<class InputIterator1, class InputIterator2, class BinaryPredicate>
        constexpr pair<InputIterator1, InputIterator2> mismatch_helper(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate binaryPredicate)
        {
            for (; first1 != last1 && binaryPredicate(*first1, *first2); ++first1, ++first2)
            {
            }

            return { first1, first2 };
        }

        template<class InputIterator1, class InputIterator2, class BinaryPredicate>
        constexpr pair<InputIterator1, InputIterator2> mismatch_helper(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate binaryPredicate)
        {
            for (; first1 != last1 && first2 != last2 && binaryPredicate(*first1, *first2); ++first1, ++first2)
            {
            }

            return { first1, first2 };
        }

        // equal helper functions
        template<class InputIterator1, class InputIterator2, class BinaryPredicate>
        constexpr bool equal_helper(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate binaryPredicate)
        {
            for (; first1 != last1; ++first1, ++first2)
            {
                if (!binaryPredicate(*first1, *first2))
                {
                    return false;
                }
            }

            return true;
        }

        template<class InputIterator1, class InputIterator2, class BinaryPredicate>
        constexpr bool equal_helper(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2,
            BinaryPredicate binaryPredicate, AZStd::input_iterator_tag, AZStd::input_iterator_tag)
        {
            for (; first1 != last1 && first2 != last2; ++first1, ++first2)
            {
                if (!binaryPredicate(*first1, *first2))
                {
                    return false;
                }
            }

            return first1 == last1 && first2 == last2;
        }
        template<class InputIterator1, class InputIterator2, class BinaryPredicate>
        constexpr bool equal_helper(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2,
            BinaryPredicate binaryPredicate, AZStd::random_access_iterator_tag, AZStd::random_access_iterator_tag)
        {
            if (AZStd::distance(first1, last1) != AZStd::distance(first2, last2))
            {
                return false;
            }

            return equal_helper<InputIterator1, InputIterator2, AZStd::add_lvalue_reference_t<BinaryPredicate>>(first1, last1, first2, binaryPredicate);
        }
    }

    // mismatch
    template<class InputIterator1, class InputIterator2>
    constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
    {
        return Internal::mismatch_helper(first1, last1, first2, AZStd::equal_to<>{});
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate binaryPredicate)
    {
        return Internal::mismatch_helper<InputIterator1, InputIterator2, AZStd::add_lvalue_reference_t<BinaryPredicate>>(first1, last1, first2, binaryPredicate);
    }

    template<class InputIterator1, class InputIterator2>
    constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
    {
        return Internal::mismatch_helper(first1, last1, first2, last2, AZStd::equal_to<>{});
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    constexpr pair<InputIterator1, InputIterator2> mismatch(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate binaryPredicate)
    {
        return Internal::mismatch_helper<InputIterator1, InputIterator2, AZStd::add_lvalue_reference_t<BinaryPredicate>>(
            first1, last1, first2, last2, binaryPredicate);
    }

    // equal
    template<class InputIterator1, class InputIterator2>
    constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
    {
        return Internal::equal_helper(first1, last1, first2, AZStd::equal_to<>{});
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, BinaryPredicate binaryPredicate)
    {
        return Internal::equal_helper<InputIterator1, InputIterator2, AZStd::add_lvalue_reference_t<BinaryPredicate>>(first1, last1, first2, binaryPredicate);
    }

    template<class InputIterator1, class InputIterator2>
    constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
    {
        return Internal::equal_helper(
            first1, last1, first2, last2, AZStd::equal_to<>{},
            typename AZStd::iterator_traits<InputIterator1>::iterator_category{},
            typename AZStd::iterator_traits<InputIterator2>::iterator_category{});
    }

    template<class InputIterator1, class InputIterator2, class BinaryPredicate>
    constexpr bool equal(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, BinaryPredicate binaryPredicate)
    {
        return Internal::equal_helper<InputIterator1, InputIterator2, AZStd::add_lvalue_reference_t<BinaryPredicate>>(
            first1, last1, first2, last2, binaryPredicate,
            typename AZStd::iterator_traits<InputIterator1>::iterator_category{},
            typename AZStd::iterator_traits<InputIterator2>::iterator_category{});
    }

    // for_each.  Apply a function to every element of a range.
    template <class InputIter, class Function>
    constexpr Function for_each(InputIter first, InputIter last, Function f)
    {
        for (; first != last; ++first)
        {
            f(*first);
        }
        return f;
    }

    // count_if
    template <class InputIter, class Predicate>
    constexpr typename iterator_traits<InputIter>::difference_type
    count_if(InputIter first, InputIter last, Predicate pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        typename iterator_traits<InputIter>::difference_type n = 0;
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                ++n;
            }
        }
        return n;
    }

    //////////////////////////////////////////////////////////////////////////
    // Find
    template<class InputIterator, class ComparableToIteratorValue>
    constexpr InputIterator find(InputIterator first, InputIterator last, const ComparableToIteratorValue& value)
    {
        for (; first != last; ++first)
        {
            if (*first == value)
            {
                break;
            }
        }
        return first;
    }

    template<class InputIterator, class Predicate>
    constexpr InputIterator find_if(InputIterator first, InputIterator last, Predicate pred)
    {
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                break;
            }
        }
        return first;
    }

    template<class InputIterator, class Predicate>
    constexpr InputIterator find_if_not(InputIterator first, InputIterator last, Predicate pred)
    {
        for (; first != last; ++first)
        {
            if (!pred(*first))
            {
                break;
            }
        }
        return first;
    }

    // adjacent_find.
    template <class ForwardIter, class BinaryPredicate>
    constexpr ForwardIter adjacent_find(ForwardIter first, ForwardIter last, BinaryPredicate binary_pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        if (first == last)
        {
            return last;
        }
        ForwardIter next = first;
        while (++next != last)
        {
            if (binary_pred(*first, *next))
            {
                return first;
            }
            first = next;
        }
        return last;
    }
    template <class ForwardIter>
    constexpr ForwardIter adjacent_find(ForwardIter first, ForwardIter last)
    {
        return AZStd::adjacent_find(first, last, AZStd::equal_to<typename iterator_traits<ForwardIter>::value_type>());
    }

    // find_first_of, with and without an explicitly supplied comparison function.
    template <class InputIter, class ForwardIter, class BinaryPredicate>
    constexpr InputIter find_first_of(InputIter first1, InputIter last1, ForwardIter first2, ForwardIter last2, BinaryPredicate comp)
    {
        //DEBUG_CHECK(check_range(first1, last1))
        //DEBUG_CHECK(check_range(first2, last2))
        for (; first1 != last1; ++first1)
        {
            for (ForwardIter iter = first2; iter != last2; ++iter)
            {
                if (comp(*first1, *iter))
                {
                    return first1;
                }
            }
        }
        return last1;
    }

    template <class InputIter, class ForwardIter>
    constexpr InputIter find_first_of(InputIter first1, InputIter last1, ForwardIter first2, ForwardIter last2)
    {
        //DEBUG_CHECK(check_range(first1, last1))
        //DEBUG_CHECK(check_range(first2, last2))
        return AZStd::find_first_of(first1, last1, first2, last2, AZStd::equal_to<typename iterator_traits<InputIter>::value_type>());
    }

    // find_end for forward iterators.
    // find_end for bidirectional iterators.

    //! True if operation returns true for all elements in the range.
    //! True if range is empty.
    template <class InputIter, class UnaryOperation>
    constexpr bool all_of(InputIter first, InputIter last, UnaryOperation operation)
    {
        return AZStd::find_if_not(first, last, operation) == last;
    }

    //! True if operation returns true for any element in the range.
    //! False if the range is empty.
    template <class InputIter, class UnaryOperation>
    constexpr bool any_of(InputIter first, InputIter last, UnaryOperation operation)
    {
        return AZStd::find_if(first, last, operation) != last;
    }

    //! True if operation returns true for no elements in the range.
    //! True if range is empty.
    template <class InputIter, class UnaryOperation>
    constexpr bool none_of(InputIter first, InputIter last, UnaryOperation operation)
    {
        return AZStd::find_if(first, last, operation) == last;
    }

    // transform
    template <class InputIterator, class OutputIterator, class UnaryOperation>
    constexpr OutputIterator  transform(InputIterator first, InputIterator last, OutputIterator result, UnaryOperation operation)
    {
        for (; first != last; ++first, ++result)
        {
            *result = operation(*first);
        }
        return result;
    }
    template <class InputIterator1, class InputIterator2, class OutputIterator, class BinaryOperation>
    constexpr OutputIterator transform(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result, BinaryOperation operation)
    {
        for (; first1 != last1; ++first1, ++first2, ++result)
        {
            *result = operation(*first1, *first2);
        }
        return result;
    }

    template<class ForwardIter, class T >
    constexpr void replace(ForwardIter first, ForwardIter last, const T& old_value, const T& new_value)
    {
        for (; first != last; ++first)
        {
            if (*first == old_value)
            {
                *first = new_value;
            }
        }
    }

    // replace_if, replace_copy, replace_copy_if
    template <class ForwardIter, class Predicate, class T>
    constexpr void replace_if(ForwardIter first, ForwardIter last, Predicate pred, const T& new_value)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                *first = new_value;
            }
        }
    }

    template <class InputIter, class OutputIter, class T>
    constexpr OutputIter replace_copy(InputIter first, InputIter last, OutputIter result, const T& old_value, const T& new_value)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first, ++result)
        {
            *result = *first == old_value ? new_value : *first;
        }
        return result;
    }

    template <class Iterator, class OutputIter, class Predicate, class T>
    constexpr OutputIter replace_copy_if(Iterator first, Iterator last, OutputIter result, Predicate pred, const T& new_value)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first, ++result)
        {
            *result = pred(*first) ? new_value : *first;
        }
        return result;
    }

    // generate and generate_n
    template <class ForwardIter, class Generator>
    constexpr void generate(ForwardIter first, ForwardIter last, Generator gen)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            *first = gen();
        }
    }
    template <class OutputIter, class Size, class Generator>
    constexpr void generate_n(OutputIter first, Size n, Generator gen)
    {
        for (; n > 0; --n, ++first)
        {
            *first = gen();
        }
    }
    // remove, remove_if, remove_copy, remove_copy_if
    template <class InputIter, class OutputIter, class T>
    constexpr OutputIter remove_copy(InputIter first, InputIter last, OutputIter result, const T& val)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            if (!(*first == val))
            {
                *result = *first;
                ++result;
            }
        }
        return result;
    }
    template <class InputIter, class OutputIter, class Predicate>
    constexpr OutputIter remove_copy_if(InputIter first, InputIter last, OutputIter result, Predicate pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        for (; first != last; ++first)
        {
            if (!pred(*first))
            {
                *result = *first;
                ++result;
            }
        }
        return result;
    }

    template <class ForwardIter, class T>
    constexpr ForwardIter remove(ForwardIter first, ForwardIter last, const T& val)
    {
        //DEBUG_CHECK(check_range(first, last))
        first = AZStd::find(first, last, val);
        if (first == last)
        {
            return first;
        }
        else
        {
            ForwardIter next = first;
            return AZStd::remove_copy(++next, last, first, val);
        }
    }

    template <class ForwardIter, class Predicate>
    constexpr ForwardIter remove_if(ForwardIter first, ForwardIter last, Predicate pred)
    {
        //DEBUG_CHECK(check_range(first, last))
        first = AZStd::find_if(first, last, pred);
        if (first == last)
        {
            return first;
        }
        else
        {
            ForwardIter next = first;
            return AZStd::remove_copy_if(++next, last, first, pred);
        }
    }


    // Reverse
    // The std::reverse function will be constexpr as of C++20, for now the std:: versions will be aliased
    // into the AZStd namespace
    using std::reverse;

    // Rotate
    // The std::rotate function will be constexpr in C++20
    // Since AZStd code doesn't need it constexpr at the moment, the std:: version will be used
    using std::rotate;

    // nth-element
    using std::nth_element;

    //////////////////////////////////////////////////////////////////////////
    // Heap
    // \todo move to heap.h
    namespace Internal
    {
        template <class RandomAccessIterator, class Distance, class T>
        constexpr void push_heap(RandomAccessIterator first, Distance holeIndex, Distance topIndex, const T& value)
        {
            Distance parent = (holeIndex - 1) / 2;
            while (holeIndex > topIndex && *(first + parent) < value)
            {
                *(first + holeIndex) = *(first + parent);
                holeIndex = parent;
                parent = (holeIndex - 1) / 2;
            }
            *(first + holeIndex) = value;
        }

        template <class RandomAccessIterator, class Distance, class T, class Compare>
        constexpr void push_heap(RandomAccessIterator first, Distance holeIndex, Distance topIndex, const T& value, Compare comp)
        {
            Distance parent = (holeIndex - 1) / 2;
            while (holeIndex > topIndex && comp(*(first + parent), value))
            {
                *(first + holeIndex) = *(first + parent);
                holeIndex = parent;
                parent = (holeIndex - 1) / 2;
            }
            *(first + holeIndex) = value;
        }

        template <class RandomAccessIterator, class Distance, class T>
        constexpr void adjust_heap(RandomAccessIterator first, Distance holeIndex, Distance length, const T& value)
        {
            Distance topIndex = holeIndex;
            Distance secondChild = 2 * holeIndex + 2;
            while (secondChild < length)
            {
                if (*(first + secondChild) < *(first + (secondChild - 1)))
                {
                    --secondChild;
                }
                *(first + holeIndex) = *(first + secondChild);
                holeIndex = secondChild;
                secondChild = 2 * (secondChild + 1);
            }
            if (secondChild == length)
            {
                *(first + holeIndex) = *(first + (secondChild - 1));
                holeIndex = secondChild - 1;
            }
            AZStd::Internal::push_heap(first, holeIndex, topIndex, value);
        }

        template <class RandomAccessIterator, class Distance, class T, class Compare>
        constexpr void adjust_heap(RandomAccessIterator first, Distance holeIndex, Distance length, const T& value, Compare comp)
        {
            Distance topIndex = holeIndex;
            Distance secondChild = 2 * holeIndex + 2;
            while (secondChild < length)
            {
                if (comp(*(first + secondChild), *(first + (secondChild - 1))))
                {
                    --secondChild;
                }
                *(first + holeIndex) = *(first + secondChild);
                holeIndex = secondChild;
                secondChild = 2 * (secondChild + 1);
            }
            if (secondChild == length)
            {
                *(first + holeIndex) = *(first + (secondChild - 1));
                holeIndex = secondChild - 1;
            }
            AZStd::Internal::push_heap(first, holeIndex, topIndex, value, comp);
        }
    }

    /**
    * \defgroup Heaps Heap functions
    * @{
    */

    /// Pushes values to the heap using AZStd::less predicate. \ref CStd
    template<class RandomAccessIterator>
    constexpr void push_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(last - 1);
        AZStd::Internal::push_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type((last - first) - 1), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), value);
    }
    /// Pushes values to the heap using provided binary predicate Compare. \ref CStd
    template<class RandomAccessIterator, class Compare>
    constexpr void push_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(last - 1);
        AZStd::Internal::push_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type((last - first) - 1), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), value, comp);
    }
    /// Prepares heap for popping a value using AZStd::less predicate. \ref CStd
    template <class RandomAccessIterator>
    constexpr void pop_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
        RandomAccessIterator result = last - 1;
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(result - first), value);
    }
    /// Prepares heap for popping a value using Compare predicate. \ref CStd
    template <class RandomAccessIterator, class Compare>
    constexpr void pop_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        RandomAccessIterator result = last - 1;
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(result - first), value, comp);
    }

    /// [Extension] Same as AZStd::pop_heap using AZStd::less predicate, but allows you to provide iterator where to store the result.
    template <class RandomAccessIterator>
    constexpr void pop_heap(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator result)
    {
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(last - first), value);
    }
    /// [Extension] Same as AZStd::pop_heap using Compare predicate, but allows you to provide iterator where to store the result.
    template <class RandomAccessIterator, class Compare>
    constexpr void pop_heap(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator result, Compare comp)
    {
        typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *result;
        *result = *first;
        AZStd::Internal::adjust_heap(first, typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(0), typename AZStd::iterator_traits<RandomAccessIterator>::difference_type(last - first), value, comp);
    }

    /// Make a heap from an array of values, using AZStd::less predicate. \ref CStd
    template <class RandomAccessIterator>
    constexpr void make_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
        if (last - first < 2)
        {
            return;
        }
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type length = last - first;
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type parent = (length - 2) / 2;

        for (;; )
        {
            typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(first + parent);
            AZStd::Internal::adjust_heap(first, parent, length, value);
            if (parent == 0)
            {
                return;
            }
            --parent;
        }
    }

    /// Make a heap from an array of values, using Compare predicate. \ref CStd
    template <class RandomAccessIterator, class Compare>
    constexpr void make_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        if (last - first < 2)
        {
            return;
        }
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type length = last - first;
        typename AZStd::iterator_traits<RandomAccessIterator>::difference_type parent = (length - 2) / 2;

        for (;; )
        {
            typename AZStd::iterator_traits<RandomAccessIterator>::value_type value = *(first + parent);
            AZStd::Internal::adjust_heap(first, parent, length, value, comp);
            if (parent == 0)
            {
                return;
            }
            --parent;
        }
    }

    /// Preforms a heap sort on a range of values, using AZStd::less predicate. \ref CStd
    template <class RandomAccessIterator>
    constexpr void sort_heap(RandomAccessIterator first, RandomAccessIterator last)
    {
        for (; last - first > 1; --last)
        {
            AZStd::pop_heap(first, last);
        }
    }
    /// Preforms a heap sort on a range of values, using Compare predicate. \ref CStd
    template <class RandomAccessIterator, class Compare>
    constexpr void sort_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
    {
        for (; last - first > 1; --last)
        {
            AZStd::pop_heap(first, last, comp);
        }
    }
    /// @}
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Search
    using std::equal_range;

    // TEMPLATE FUNCTION lower_bound
    template<class ForwardIterator, class T>
    constexpr ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value)
    {
        // find first element not before value, using operator<
        typename iterator_traits<ForwardIterator>::difference_type count = AZStd::distance(first, last);
        typename iterator_traits<ForwardIterator>::difference_type count2{};

        for (; 0 < count; )
        {
            // divide and conquer, find half that contains answer
            count2 = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, count2);

            if (*mid < value)
            {
                first = ++mid;
                count -= count2 + 1;
            }
            else
            {
                count = count2;
            }
        }
        return first;
    }

    template<class ForwardIterator, class T, class Compare>
    constexpr ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
    {
        // find first element not before value, using compare predicate
        typename iterator_traits<ForwardIterator>::difference_type count = AZStd::distance(first, last);
        typename iterator_traits<ForwardIterator>::difference_type count2{};

        for (; 0 < count; )
        {
            // divide and conquer, find half that contains answer
            count2 = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, count2);

            if (comp(*mid, value))
            {
                first = ++mid;
                count -= count2 + 1;
            }
            else
            {
                count = count2;
            }
        }
        return first;
    }


    // TEMPLATE FUNCTION upper_bound
    template<class ForwardIterator, class T>
    constexpr ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& value)
    {
        // find first element that value is before, using operator<
        typename iterator_traits<ForwardIterator>::difference_type count = AZStd::distance(first, last);
        typename iterator_traits<ForwardIterator>::difference_type step{};
        for (; 0 < count; )
        {   // divide and conquer, find half that contains answer
            step = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, step);

            if (!(value < *mid))
            {
                first = ++mid;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }
        return first;
    }
    template<class ForwardIterator, class T, class Compare>
    constexpr ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& value, Compare comp)
    {
        // find first element not before value, using compare predicate
        typename iterator_traits<ForwardIterator>::difference_type count = AZStd::distance(first, last);
        typename iterator_traits<ForwardIterator>::difference_type step{};
        for (; 0 < count; )
        {
            // divide and conquer, find half that contains answer
            step = count / 2;
            ForwardIterator mid = first;
            AZStd::advance(mid, step);

            if (!comp(value, *mid))
            {
                first = ++mid;
                count -= step + 1;
            }
            else
            {
                count = step;
            }
        }
        return first;
    }

    template <class ForwardIterator1, class ForwardIterator2>
    constexpr ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2)
    {
        if (first2 == last2)
        {
            return first1;                 // specified in C++11
        }
        while (first1 != last1)
        {
            ForwardIterator1 it1 = first1;
            ForwardIterator2 it2 = first2;
            while (*it1 == *it2)
            {
                ++it1;
                ++it2;
                if (it2 == last2)
                {
                    return first1;
                }
                if (it1 == last1)
                {
                    return last1;
                }
            }
            ++first1;
        }
        return last1;
    }

    template <class ForwardIterator1, class ForwardIterator2, class Compare>
    constexpr ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2, Compare comp)
    {
        if (first2 == last2)
        {
            return first1;                 // specified in C++11
        }
        while (first1 != last1)
        {
            ForwardIterator1 it1 = first1;
            ForwardIterator2 it2 = first2;
            while (comp(*it1, *it2))
            {
                ++it1;
                ++it2;
                if (it2 == last2)
                {
                    return first1;
                }
                if (it1 == last1)
                {
                    return last1;
                }
            }
            ++first1;
        }
        return last1;
    }

    template <class ForwardIterator>
    constexpr bool is_sorted(ForwardIterator first, ForwardIterator last)
    {
        return is_sorted(first, last, AZStd::less<AZStd::remove_cvref_t<decltype(*first)>>());
    }

    template <class ForwardIterator, class Compare>
    constexpr bool is_sorted(ForwardIterator first, ForwardIterator last, Compare comp)
    {
        if (first == last)
        {
            return true;
        }
        ForwardIterator next = first;
        while (++next != last)
        {
            if (comp(*next, *first))
            {
                return false;
            }
            ++first;
        }
        return true;
    }

    template<class ForwardIterator>
    constexpr ForwardIterator unique(ForwardIterator first, ForwardIterator last)
    {
        return unique(first, last, AZStd::equal_to<>{});
    }

    template<class ForwardIterator, class BinaryPredicate>
    constexpr ForwardIterator unique(ForwardIterator first, ForwardIterator last, BinaryPredicate pred)
    {
        if (first == last)
        {
            return last;
        }

        ForwardIterator result = first;
        while (++first != last)
        {
            if (!pred(*result, *first) && ++result != first)
            {
                *result = AZStd::move(*first);
            }
        }
        return ++result;
    }

    using std::binary_search;

    // todo search_n
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // set_difference
    template <class Compare, class InputIterator1, class InputIterator2, class OutputIterator>
    constexpr OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, Compare comp)
    {
        while (first1 != last1)
        {
            if (first2 == last2)
            {
                return AZStd::copy(first1, last1, result);
            }

            if (comp(*first1, *first2))
            {
                *result = *first1;
                ++result;
                ++first1;
            }
            else
            {
                if (!comp(*first2, *first1))
                {
                    ++first1;
                }
                ++first2;
            }
        }
        return result;
    }

    template <class InputIterator1, class InputIterator2, class OutputIterator>
    constexpr OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result)
    {
        return AZStd::set_difference(first1, last1, first2, last2, result,
            AZStd::less<AZStd::common_type_t<typename iterator_traits<InputIterator1>::value_type,
                                             typename iterator_traits<InputIterator2>::value_type>>());
    }
    //////////////////////////////////////////////////////////////////////////

    template<class InputIterator1, class InputIterator2>
    constexpr bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2)
    {
        for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        {
            if (*first1 < *first2)
            {
                return true;
            }
            if (*first2 < *first1)
            {
                return false;
            }
        }
        return first1 == last1 && first2 != last2;
    }

    constexpr bool lexicographical_compare(const unsigned char* first1, const unsigned char* last1, const unsigned char* first2, const unsigned char* last2)
    {
        ptrdiff_t len1 = last1 - first1;
        ptrdiff_t len2 = last2 - first2;
        int res = __builtin_memcmp(first1, first2, len1 < len2 ? len1 : len2);
        return (res < 0 || (res == 0 && len1 < len2));
    }

    template<class InputIterator1, class InputIterator2, class Compare>
    constexpr bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, Compare comp)
    {
        for (; first1 != last1 && first2 != last2; ++first1, ++first2)
        {
            if (comp(*first1, *first2))
            {
                return true;
            }
            if (comp(*first2, *first1))
            {
                return false;
            }
        }
        return first1 == last1 && first2 != last2;
    }

    //////////////////////////////////////////////////////////////////////////
    /**
    * Endian swapping templates
    * note you can specialize any type anywhere in the code if you fell like it.
    */
    template<typename T, AZStd::size_t size>
    struct endian_swap_impl
    {
        // this function is implemented for each specialization.
        static constexpr void swap_data(T& data);
    };

    // specialization for 1 byte type (don't swap)
    template<typename T>
    struct endian_swap_impl<T, 1>
    {
        static constexpr void swap_data(T& data)  { (void)data; }
    };

    // specialization for 2 byte type
    template<typename T>
    struct endian_swap_impl<T, 2>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)
        {
            union SafeCast
            {
                T               m_data;
                unsigned short  m_ushort;
            };
            SafeCast* sc = (SafeCast*)&data;
            unsigned short x = sc->m_ushort;
            sc->m_ushort = (x >> 8) | (x << 8);
        }
    };

    // specialization for 4 byte type
    template<typename T>
    struct endian_swap_impl<T, 4>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)
        {
            union SafeCast
            {
                T               m_data;
                unsigned int    m_uint;
            };
            SafeCast* sc = (SafeCast*)&data;
            unsigned int x = sc->m_uint;
            sc->m_uint = (x >> 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x << 24);
        }
    };

#   define AZ_UINT64_CONST(_Value) _Value
#   define AZ_INT64_CONST(_Value) _Value

    template<typename T>
    struct endian_swap_impl<T, 8>
    {
        static AZ_FORCE_INLINE void swap_data(T& data)
        {
            union SafeCast
            {
                T           m_data;
                AZ::u64     m_uint64;
            };
            SafeCast* sc = (SafeCast*)&data;
            AZ::u64 x = sc->m_uint64;
            sc->m_uint64 = (x >> 56) |
                ((x << 40) & AZ_UINT64_CONST(0x00FF000000000000)) |
                ((x << 24) & AZ_UINT64_CONST(0x0000FF0000000000)) |
                ((x << 8)  & AZ_UINT64_CONST(0x000000FF00000000)) |
                ((x >> 8)  & AZ_UINT64_CONST(0x00000000FF000000)) |
                ((x >> 24) & AZ_UINT64_CONST(0x0000000000FF0000)) |
                ((x >> 40) & AZ_UINT64_CONST(0x000000000000FF00)) |
                (x << 56);
        }
    };

    template<typename T>
    AZ_FORCE_INLINE void endian_swap(T& data)
    {
        endian_swap_impl<T, sizeof(T)>::swap_data(data);
    }

    template<typename Iterator>
    AZ_FORCE_INLINE void endian_swap(Iterator first, Iterator last)
    {
        for (; first != last; ++first)
        {
            AZStd::endian_swap(*first);
        }
    }
    //
    //////////////////////////////////////////////////////////////////////////
}
