/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <AzCore/std/typetraits/is_array.h>
#include <AzCore/std/typetraits/is_assignable.h>
#include <AzCore/std/typetraits/is_constructible.h>
#include <AzCore/std/typetraits/is_destructible.h>
#include <AzCore/std/typetraits/is_trivially_copyable.h>
#include <AzCore/std/typetraits/is_void.h>
#include <AzCore/std/utils.h> // AZStd::addressof

namespace AZStd
{
    //! Bring the names of uninitialized_default_construct and
    //! uninitialized_default_construct_n into the AZStd namespace
    using std::uninitialized_default_construct;
    using std::uninitialized_default_construct_n;

    //! uninitialized_value_construct and uninitialized_value_construct_n
    //! are now brought into scope of the AZStd namespace
    using std::uninitialized_value_construct;
    using std::uninitialized_value_construct_n;
}

namespace AZStd::Internal
{
    /**
     * Internal code should be used only by AZSTD. These are as flat as possible (less function calls) implementation, that
     * should be fast and easy in debug. You can call this functions, but it's better to use the specialized one in the
     * algorithms.
     */
     //////////////////////////////////////////////////////////////////////////
     /**
      * Type has trivial destructor. We don't call it.
      */
    template <class InputIterator, class ValueType = iter_value_t<InputIterator>, bool = is_trivially_destructible_v<ValueType>>
    struct destroy
    {
        static constexpr void range(InputIterator first, InputIterator last) { (void)first; (void)last; }
        static constexpr void single(InputIterator iter) { (void)iter; }
    };
    /**
        * Calls the destructor on a range of objects, defined by start and end forward iterators.
        */
    template<class InputIterator, class ValueType>
    struct destroy<InputIterator, ValueType, false>
    {
        static constexpr void range(InputIterator first, InputIterator last)
        {
            for (; first != last; ++first)
            {
                first->~ValueType();
            }
        }

        static constexpr void single(InputIterator iter)
        {
            (void)iter;
            iter->~ValueType();
        }
    };
}

namespace AZStd
{
    //! Implements the C++20 version of destroy_at
    //! If the element type T is not an array type it invokes the destructor on that object
    //! Otherwise it recursively destructs the elements of the array as if by calling
    //! AZStd::destroy(AZStd::begin(*ptr), AZStd::end(*ptr))
    template <typename T>
    constexpr void destroy_at(T* ptr)
    {
        if constexpr (AZStd::is_array_v<T>)
        {
            for (auto& element : *ptr)
            {
                AZStd::destroy_at(AZStd::addressof(element));
            }
        }
        else
        {
            ptr->~T();
        }
    }
    //! Implements the C++17 destroy function which works on a range of elements
    //! The functions accepts a first and last iterator
    //! and invokes the destructor on all element in the iterator range
    template <typename ForwardIt>
    constexpr void destroy(ForwardIt first, ForwardIt last)
    {
        for (; first != last; ++first)
        {
            AZStd::destroy_at(AZStd::addressof(*first));
        }
    }

    //! Implements the C++17 destroy_n function
    //! The function accepts a forward iterator and a number of elements
    //! and invokes the destructor on all the elements in the range
    //! Returns an iterator past the last element destructed
    template <typename ForwardIt, size_t N>
    constexpr ForwardIt destroy_n(ForwardIt first, size_t numElements)
    {
        for (; numElements > 0; ++first, --numElements)
        {
            AZStd::destroy_at(AZStd::addressof(*first));
        }
        return first;
    }
}

namespace AZStd::Internal
{
    /**
     * Default object construction.
     */
    // placement new isn't a core constant expression therefore it cannot be used in a constexpr function
    template<class InputIterator, class ValueType = iter_value_t<InputIterator>,
        bool = is_trivially_constructible_v<ValueType>>
        struct construct
    {
        static constexpr void range(InputIterator first, InputIterator last)
        {
            range(first, last, ValueType());
        }
        static constexpr void range(InputIterator first, InputIterator last, const ValueType& value)
        {
            for (; first != last; ++first)
            {
                *first = value;
            }
        }
        static constexpr void single(InputIterator iter)
        {
            single(iter, ValueType());
        }
        static constexpr void single(InputIterator iter, const ValueType& value)
        {
            *iter = value;
        }

        template<class ... InputArguments>
        static void single(InputIterator iter, InputArguments&& ... inputArguments)
        {
            ::new (static_cast<void*>(&*iter))ValueType(AZStd::forward<InputArguments>(inputArguments) ...);
        }
    };

    template<class InputIterator, class ValueType>
    struct construct<InputIterator, ValueType, false>
    {
        static void range(InputIterator first, InputIterator last)
        {
            for (; first != last; ++first)
            {
                ::new (&*first) ValueType();
            }
        }

        static void range(InputIterator first, InputIterator last, const ValueType& value)
        {
            for (; first != last; ++first)
            {
                ::new (&*first) ValueType(value);
            }
        }

        template<class ... InputArguments>
        static void single(InputIterator iter, InputArguments&& ... inputArguments)
        {
            ::new (&*iter) ValueType(AZStd::forward<InputArguments>(inputArguments) ...);
        }
    };
    //////////////////////////////////////////////////////////////////////////
}

namespace AZStd
{
    //! C++20 implementation of construct_at
    //! Invokes placement new on the supplied address
    //! Constraints: Only available when the expression
    //! `new (declval<void*>()) T(declval<Args>()...)` is well-formed
    template <typename T, typename... Args>
    constexpr auto construct_at(T* ptr, Args&&... args)
        -> decltype(new (AZStd::declval<void*>()) T(AZStd::forward<Args>(args)...), (T*)nullptr)
    {
        return ::new (ptr) T(AZStd::forward<Args>(args)...);
    }
}


namespace AZStd::Internal
{

    //////////////////////////////////////////////////////////////////////////
    // Sequence copy. If we use optimized version we use memcpy.
    /**
    * Class to determine if we have apply fast copy. There are 2 conditions
    * - trivial copy ctor.
    * - all iterators satisfy the C++20 are contiguous iterator concept
    */
    template<class Out, class = void>
    constexpr bool indirectly_trivially_copyable = false;
    template<class Out>
    constexpr bool indirectly_trivially_copyable<Out,
        enable_if_t<indirectly_readable<Out>>> = is_trivially_copyable_v<iter_value_t<Out>>;

    template<class InputIterator, class ResultIterator>
    using is_fast_copy = bool_constant<indirectly_trivially_copyable<ResultIterator>
        && contiguous_iterator<InputIterator>
        && contiguous_iterator<ResultIterator>
        >;

    template<class InputIterator, class ResultIterator>
    constexpr bool is_fast_copy_v = is_fast_copy<InputIterator, ResultIterator>::value;


    // is_fast_copy argument is no longer used.
    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator copy(InputIterator first, InputIterator last, ForwardIterator result, bool)
    {
        if constexpr (is_fast_copy_v<InputIterator, ForwardIterator>)
        {
            // Specialized copy for contiguous iterators which are trivially copyable
            size_t numElements = last - first;
            if (numElements > 0)
            {
#if az_has_builtin_memcpy
                static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Size of value types must match for a trivial copy");
                __builtin_memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
#else
                if (az_builtin_is_constant_evaluated())
                {
                    for (; first != last; ++result, ++first)
                    {
                        *result = *first;
                    }

                    return result;
                }
                else
                {
                    static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Size of value types must match for a trivial copy");
                    AZ_Assert((static_cast<const void*>(&*result) < static_cast<const void*>(&*first))
                        || (static_cast<const void*>(&*result) >= static_cast<const void*>(&*first + numElements)),
                        "AZStd::copy memory overlaps use AZStd::copy_backward!");
                    ::memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
                }
#endif
            }
            return result + numElements;
        }
        else
        {
            for (; first != last; ++result, ++first)
            {
                *result = *first;
            }

            return result;
        }
    }


    // Copy backward.
    template <class BidirectionalIterator1, class BidirectionalIterator2>
    constexpr BidirectionalIterator2 copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result, bool)
    {
        if constexpr (is_fast_copy_v<BidirectionalIterator1, BidirectionalIterator2>)
        {
            // Specialized copy for contiguous iterators which are trivially copyable
            size_t numElements = last - first;
            if (numElements > 0)
            {
#if az_has_builtin_memmove
                static_assert(sizeof(iter_value_t<BidirectionalIterator1>) == sizeof(iter_value_t<BidirectionalIterator2>), "Size of value types must match for a trivial copy");
                result -= numElements;
                __builtin_memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<BidirectionalIterator1>));
#else
                if (az_builtin_is_constant_evaluated())
                {
                    while (first != last)
                    {
                        *--result = *--last;
                    }

                    return result;
                }
                else
                {
                    static_assert(sizeof(iter_value_t<BidirectionalIterator1>) == sizeof(iter_value_t<BidirectionalIterator2>), "Size of value types must match for a trivial copy");
                    result -= numElements;
                    AZ_Assert(((&*result + numElements) <= &*first) || ((&*result + numElements) > (&*first + numElements)), "AZStd::copy_backward memory overlaps use AZStd::copy!");
                    ::memmove(&*result, &*first, numElements * sizeof(iter_value_t<BidirectionalIterator1>));
                }
#endif
            }
            return result;
        }
        else
        {
            while (first != last)
            {
                *--result = *--last;
            }

            return result;
        }
    }

    template <class BidirectionalIterator1, class ForwardIterator>
    constexpr ForwardIterator reverse_copy(BidirectionalIterator1 first, BidirectionalIterator1 last, ForwardIterator dest)
    {
        while (last != first)
        {
            *(dest++) = *(--last);
        }

        return dest;
    }
    //////////////////////////////////////////////////////////////////////////
}

namespace AZStd
{
    /**
    * Specialized algorithms 20.4.4. We extend that by adding faster specialized versions when we have trivial assign type.
    */
    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result, bool)
    {
        // Specialized copy for contiguous iterators which are trivially copyable
        if constexpr (Internal::is_fast_copy_v<InputIterator, ForwardIterator>)
        {
            size_t numElements = last - first;
            if (numElements > 0)
            {
#if az_has_builtin_memcpy
                static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Value type sizes must match for a trivial copy");
                __builtin_memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
#else
                if (az_builtin_is_constant_evaluated())
                {
                    for (; first != last; ++result, ++first)
                    {
                        construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(result)), *first);
                    }

                    return result;
                }
                else
                {
                    static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Value type sizes must match for a trivial copy");
                    ::memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
                }
#endif
            }
            return result + numElements;
        }
        else
        {
            for (; first != last; ++result, ++first)
            {
                construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(result)), *first);
            }

            return result;
        }
    }


    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result)
    {
        return uninitialized_copy(first, last, result, {});
    }

    // 25.3.1 Copy
    template<class InputIterator, class OutputIterator>
    constexpr OutputIterator copy(InputIterator first, InputIterator last, OutputIterator result)
    {
        return Internal::copy(first, last, result, {});
    }

    template <class BidirectionalIterator, class OutputIterator>
    constexpr OutputIterator reverse_copy(BidirectionalIterator first, BidirectionalIterator last, OutputIterator dest)
    {
        return Internal::reverse_copy(first, last, dest);
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2 copy_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        return Internal::copy_backward(first, last, result, {});
    }
}

namespace AZStd::Internal
{
    //////////////////////////////////////////////////////////////////////////
    // Sequence move
    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator move(InputIterator first, InputIterator last, ForwardIterator result, bool)
    {
        // Specialized copy for contiguous iterators which are trivially copyable
        if constexpr (is_fast_copy_v<InputIterator, ForwardIterator>)
        {
            size_t numElements = last - first;
            if (numElements > 0)
            {
#if az_has_builtin_memmove
                static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Size of value types must match for a trivial copy");
                __builtin_memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
#else
                if (az_builtin_is_constant_evaluated())
                {
                    for (; first != last; ++result, ++first)
                    {
                        *result = ::AZStd::move(*first);
                    }
                    return result;
                }
                else
                {
                    static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Size of value types must match for a trivial copy");
                    AZ_Assert((static_cast<const void*>(&*result) < static_cast<const void*>(&*first))
                        || (static_cast<const void*>(&*result) >= static_cast<const void*>(&*first + numElements)),
                        "AZStd::move memory overlaps use AZStd::move_backward!");
                    ::memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
                }
#endif
            }
            return result + numElements;
        }
        else
        {
            for (; first != last; ++result, ++first)
            {
                *result = ::AZStd::move(*first);
            }
            return result;
        }
    }


    // For generic iterators, move is the same as copy.
    template <class BidirectionalIterator1, class BidirectionalIterator2>
    constexpr BidirectionalIterator2 move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result, bool)
    {
        // Specialized copy for contiguous iterators which are trivially copyable
        if constexpr (is_fast_copy_v<BidirectionalIterator1, BidirectionalIterator1>)
        {
            size_t numElements = last - first;
            if (numElements > 0)
            {
#if az_has_builtin_memmove
                static_assert(sizeof(iter_value_t<BidirectionalIterator1>) == sizeof(iter_value_t<BidirectionalIterator2>), "Size of value types must match for a trivial copy");
                result -= numElements;
                __builtin_memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<BidirectionalIterator1>));
#else
                if (az_builtin_is_constant_evaluated())
                {
                    while (first != last)
                    {
                        *--result = ::AZStd::move(*--last);
                    }
                    return result;
                }
                else
                {
                    static_assert(sizeof(iter_value_t<BidirectionalIterator1>) == sizeof(iter_value_t<BidirectionalIterator2>), "Size of value types must match for a trivial copy");
                    result -= numElements;
                    AZ_Assert((static_cast<const void*>(&*result + numElements) <= static_cast<const void*>(&*first))
                        || (static_cast<const void*>(&*result + numElements) > static_cast<const void*>(&*first + numElements)),
                        "AZStd::move_backward memory overlaps use AZStd::move!");
                    ::memmove(to_address(result), to_address(first), numElements * sizeof(iter_value_t<BidirectionalIterator1>));
                }
#endif
            }
            return result;
        }
        else
        {
            while (first != last)
            {
                *--result = ::AZStd::move(*--last);
            }
            return result;
        }
    }

    template <class InputIterator, class ForwardIterator>
    constexpr ForwardIterator uninitialized_move(InputIterator first, InputIterator last, ForwardIterator result, bool)
    {
        // Specialized copy for contiguous iterators which are trivially copyable
        if constexpr (is_fast_copy_v<InputIterator, ForwardIterator>)
        {
            size_t numElements = last - first;
            if (numElements > 0)
            {
#if az_has_builtin_memcpy
                static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Value type sizes must match for a trivial copy");
                __builtin_memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
#else
                if (az_builtin_is_constant_evaluated())
                {
                    for (; first != last; ++result, ++first)
                    {
                        construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(result)), ::AZStd::move(*first));
                    }

                    return result;
                }
                else
                {
                    static_assert(sizeof(iter_value_t<InputIterator>) == sizeof(iter_value_t<ForwardIterator>), "Value type sizes must match for a trivial copy");
                    ::memcpy(to_address(result), to_address(first), numElements * sizeof(iter_value_t<InputIterator>));
                }
#endif
            }
            return result + numElements;
        }
        else
        {
            for (; first != last; ++result, ++first)
            {
                construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(result)), ::AZStd::move(*first));
            }

            return result;
        }
    }
    // end of sequence move.
    //////////////////////////////////////////////////////////////////////////
}

namespace AZStd
{
    //! Implements the C++17 uninitialized_move function
    //! The functions accepts two input iterators and an output iterator
    //! It performs an AZStd::move on each in in the range of the input iterator
    //! and stores the result in location pointed by the output iterator
    template <typename InputIt, typename ForwardIt>
    ForwardIt uninitialized_move(InputIt first, InputIt last, ForwardIt result)
    {
        return AZStd::Internal::uninitialized_move(first, last, result, {});
    }
    // 25.3.2 Move
    template<class InputIterator, class OutputIterator>
    OutputIterator move(InputIterator first, InputIterator last, OutputIterator result)
    {
        return AZStd::Internal::move(first, last, result, {});
    }

    template<class BidirectionalIterator1, class BidirectionalIterator2>
    BidirectionalIterator2 move_backward(BidirectionalIterator1 first, BidirectionalIterator1 last, BidirectionalIterator2 result)
    {
        return AZStd::Internal::move_backward(first, last, result, {});
    }
}

namespace AZStd::Internal
{
    //////////////////////////////////////////////////////////////////////////
    // Fill
    /**
    * Helper class to determine if we have apply fast fill. There are 3 conditions
    * - trivial assign
    * - size of type == 1 (chars) to use memset
    * - contiguous iterators
    */
    template<class Out, class = void>
    constexpr bool indirectly_copy_assignable = false;
    template<class Out>
    constexpr bool indirectly_copy_assignable<Out, enable_if_t<indirectly_readable<Out>>> =
        is_trivially_copy_assignable_v<iter_value_t<Out>> && sizeof(iter_value_t<Out>) == 1;

    template<class Iterator>
    using is_fast_fill = bool_constant<indirectly_copy_assignable<Iterator> && contiguous_iterator<Iterator>>;
    template<class Iterator>
    constexpr bool is_fast_fill_v = is_fast_fill<Iterator>::value;

    // The fast fill trait is no longer used
    // It is detected using C++20 concepts now
    template <class ForwardIterator, class T>
    constexpr void fill(ForwardIterator first, ForwardIterator last, const T& value, bool)
    {
        if constexpr (is_fast_fill_v<ForwardIterator>)
        {
            size_t numElements = last - first;
            if (numElements > 0)
            {
                if (az_builtin_is_constant_evaluated())
                {
                    for (; first != last; ++first)
                    {
                        *first = value;
                    }
                }
                else
                {
                    ::memset(to_address(first), reinterpret_cast<const unsigned char&>(value), numElements);
                }
            }
        }
        else
        {
            for (; first != last; ++first)
            {
                *first = value;
            }
        }
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void fill_n(ForwardIterator first, Size numElements, const T& value, bool)
    {
        if constexpr (is_fast_fill_v<ForwardIterator>)
        {
            if (numElements)
            {
                if (az_builtin_is_constant_evaluated())
                {
                    for (; numElements--; ++first)
                    {
                        *first = value;
                    }
                }
                else
                {
                    ::memset(to_address(first), reinterpret_cast<const unsigned char&>(value), numElements);
                }
            }
        }
        else
        {
            for (; numElements--; ++first)
            {
                *first = value;
            }
        }
    }
}

namespace AZStd
{
    template <class ForwardIterator, class T>
    constexpr void fill(ForwardIterator first, ForwardIterator last, const T& value)
    {
        Internal::fill(first, last, value, {});
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void fill_n(ForwardIterator first, Size numElements, const T& value)
    {
        Internal::fill_n(first, numElements, value, {});
    }

    template <class ForwardIterator, class T>
    constexpr void uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& value, bool)
    {
        if constexpr (Internal::is_fast_fill_v<ForwardIterator>)
        {
            size_t numElements = last - first;
            if (numElements > 0)
            {
                if (az_builtin_is_constant_evaluated())
                {
                    for (; first != last; ++first)
                    {
                        construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(first)), value);
                    }
                }
                else
                {
                    ::memset(to_address(first), reinterpret_cast<const unsigned char&>(value), numElements);
                }
            }
        }
        else
        {
            for (; first != last; ++first)
            {
                construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(first)), value);
            }
        }
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void uninitialized_fill(ForwardIterator first, Size numElements, const T& value)
    {
        return uninitialized_fill(first, numElements, value, {});
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value, bool)
    {
        if constexpr (Internal::is_fast_fill_v<ForwardIterator>)
        {
            if (numElements > 0)
            {
                if (az_builtin_is_constant_evaluated())
                {
                    for (; numElements--; ++first)
                    {
                        construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(first)), value);
                    }
                }
                else
                {
                    ::memset(to_address(first), reinterpret_cast<const unsigned char&>(value), numElements);
                }
            }
        }
        else
        {
            for (; numElements--; ++first)
            {
                construct_at(static_cast<iter_value_t<ForwardIterator>*>(to_address(first)), value);
            }
        }
    }

    template <class ForwardIterator, class Size, class T>
    constexpr void uninitialized_fill_n(ForwardIterator first, Size numElements, const T& value)
    {
        return uninitialized_fill_n(first, numElements, value, {});
    }
}
