/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>

#include <AzCore/std/iterator/iterator_primitives.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/typetraits/is_convertible.h>
#include <AzCore/std/typetraits/remove_cv.h>
#include <AzCore/std/typetraits/void_t.h>
#include <AzCore/std/utils.h>

#include <iterator>

namespace AZStd
{
    // Everything unless specified is based on C++ 20 (lib.iterators).

    /// Identifying tag for input iterators.
    using input_iterator_tag = std::input_iterator_tag;
    /// Identifying tag for output iterators.
    using output_iterator_tag = std::output_iterator_tag;
    /// Identifying tag for forward iterators.
    using forward_iterator_tag = std::forward_iterator_tag;
    /// Identifying tag for bidirectional iterators.
    using bidirectional_iterator_tag = std::bidirectional_iterator_tag;
    /// Identifying tag for random-access iterators.
    using random_access_iterator_tag = std::random_access_iterator_tag;
    /// Identifying tag for contagious iterators
    struct contiguous_iterator_tag
        : public random_access_iterator_tag {};


    /// Add the default_sentinel struct from C++20
    struct default_sentinel_t {};
    inline constexpr default_sentinel_t default_sentinel{};
}

namespace AZStd::Internal
{
    template <typename Iterator, typename = void>
    inline constexpr bool has_iterator_type_aliases_v = false;

    template <typename Iterator>
    inline constexpr bool has_iterator_type_aliases_v<Iterator, void_t<
        typename Iterator::iterator_category,
        typename Iterator::value_type,
        typename Iterator::difference_type,
        typename Iterator::pointer,
        typename Iterator::reference>
    > = true;

    // Iterator iterator_category alias must be one of the iterator category tags
    template <typename Iterator, bool>
    struct iterator_traits_category_tags
    {};

    template <typename Iterator>
    struct iterator_traits_category_tags<Iterator, true>
    {
        using difference_type = typename Iterator::difference_type;
        using value_type = typename Iterator::value_type;
        using pointer = typename Iterator::pointer;
        using reference = typename Iterator::reference;
        using iterator_category = typename Iterator::iterator_category;
    };

    template <typename Iterator, bool>
    struct iterator_traits_type_aliases
    {};

    // Iterator must have all five type aliases
    template <typename Iterator>
    struct iterator_traits_type_aliases<Iterator, true>
        : iterator_traits_category_tags<Iterator,
        is_convertible_v<typename Iterator::iterator_category, input_iterator_tag>
        || is_convertible_v<typename Iterator::iterator_category, output_iterator_tag>>
    {};
}

namespace AZStd
{
    /**
     * Default iterator traits struct.
    */
    template <class Iterator, class>
    struct iterator_traits
        : Internal::iterator_traits_type_aliases<Iterator, Internal::has_iterator_type_aliases_v<Iterator>>
    {
        // Internal type alias meant to indicate that this is the primary template
        using _is_primary_template = iterator_traits;
    };

    /**
     * Default STL pointer specializations use random_access_iterator as a category.
     * We do refine this as being a continuous iterator.
     */
    template <class T>
    struct iterator_traits<T*>
    {
        using difference_type = ptrdiff_t;
        using value_type = remove_cv_t<T>;
        using pointer = T*;
        using reference = T&;
        using iterator_category = random_access_iterator_tag;
        using iterator_concept = contiguous_iterator_tag;
    };
}

namespace AZStd
{
    // Alias the C++ standard reverse_iterator into the AZStd:: namespace
    using std::reverse_iterator;
    using std::make_reverse_iterator;

    // Alias the C++ standard insert iterators into the AZStd:: namespace
    using std::back_insert_iterator;
    using std::back_inserter;
    using std::front_insert_iterator;
    using std::front_inserter;
    using std::insert_iterator;
    using std::inserter;

#if !defined(__cpp_lib_concepts)
    // In order for pre C++20 back_inserter_iterator, front_inserter_iterator and insert_iterator
    // to work with the range algorithms which require a weakly_incrementable iterator
    // the difference_type type alias must not be void as it is in C++17
    // https://en.cppreference.com/w/cpp/iterator/back_insert_iterator
    // We workaround this by specializing AZStd::iterator_traits for these types
    // to provide a difference_type type alias
    template<class Container>
    struct iterator_traits<back_insert_iterator<Container>>
    {
        using difference_type = ptrdiff_t;
    };

    template<class Container>
    struct iterator_traits<front_insert_iterator<Container>>
    {
        using difference_type = ptrdiff_t;
    };
    template<class Container>
    struct iterator_traits<insert_iterator<Container>>
    {
        using difference_type = ptrdiff_t;
    };

#endif

    enum iterator_status_flag
    {
        isf_none = 0x00,     ///< Iterator is invalid.
        isf_valid = 0x01,     ///< Iterator point to a valid element in container, we can't dereference of all them. For instance the end() is a valid iterator, but you can't access it.
        isf_can_dereference = 0x02,     ///< We can dereference this iterator (it points to a valid element).
        isf_current = 0x04
    };

    //////////////////////////////////////////////////////////////////////////
    // Advance
     // Alias the std::advance function into the AZStd namespace now that C++17 has made it constexpr
    using std::advance;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Distance functions.
    // Alias the std::distance function into the AZStd namespace
    // It is constexpr as of C++17
    using std::distance;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // next prev utils
    // Alias the std::next and std::prev functions into the AZStd namespace
    // Both functions are constexpr as of C++17
    using std::next;
    using std::prev;

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //  (c)begin (c)end, (cr)begin, (cr)end utils
    // Alias all the begin and end iterator functions into the AZStd namespacew
    // They are constexpr as of C++17
    using std::begin;
    using std::cbegin;
    using std::end;
    using std::cend;

    using std::rbegin;
    using std::crbegin;
    using std::rend;
    using std::crend;

    // Alias the C++17 size, empty and data function into the AZStd namespace
    // All of the functions are constexpr
    using std::size;
    using std::empty;
    using std::data;

    namespace Debug
    {
        // Keep macros around for backwards compatibility
        #define AZSTD_POINTER_ITERATOR_PARAMS(_Pointer)         _Pointer
        #define AZSTD_CHECKED_ITERATOR(_IteratorImpl, _Param)    _Param
        #define AZSTD_CHECKED_ITERATOR_2(_IteratorImpl, _Param1, _Param2) _Param1, _Param2
        #define AZSTD_GET_ITER(_Iterator)                       _Iterator
    } // namespace Debug

    namespace Internal
    {
        // AZStd const and non-const iterators are written to be binary compatible, i.e. iterator inherits from const_iterator and only add methods. This we "const casting"
        // between them is a matter of reinterpretation. In general avoid this cast as much as possible, as it's a bad practice, however it's true for all iterators.
        template<class Iterator, class ConstIterator>
        inline Iterator ConstIteratorCast(ConstIterator& iter)
        {
            static_assert((AZStd::is_base_of<ConstIterator, Iterator>::value), "For this cast to work Iterator should derive from ConstIterator");
            static_assert(sizeof(ConstIterator) == sizeof(Iterator), "For this cast to work ConstIterator and Iterator should be binarily identical");
            return *reinterpret_cast<Iterator*>(&iter);
        }
    }
} // namespace AZStd

// Include the AZStd::move_iterator implementation after alias in the std:: iterator names
#include <AzCore/std/iterator/move_iterator.h>
