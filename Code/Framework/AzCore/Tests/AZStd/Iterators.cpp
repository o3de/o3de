/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/utils.h>


namespace UnitTest
{
    class Iterators
        : public ScopedAllocatorSetupFixture
    {
    };

    template<typename Container>
    void Test_WrapperFunctions_MutableIterator_MutableContainer()
    {
        Container int_container = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};

        typename Container::iterator iter_begin = AZStd::begin(int_container);
        EXPECT_EQ(*iter_begin, 0);
        EXPECT_EQ(*AZStd::next(iter_begin), 1);
        EXPECT_EQ(*AZStd::next(iter_begin, 2), 2);
        ++iter_begin;
        EXPECT_EQ(*iter_begin, 1);

        typename Container::iterator iter_end = end(int_container);
        EXPECT_EQ(iter_end, int_container.end());
        EXPECT_EQ(*AZStd::prev(iter_end), 9);
        EXPECT_EQ(*AZStd::prev(iter_end, 2), 8);
        --iter_end;
        EXPECT_EQ(*iter_end, 9);

        typename Container::reverse_iterator iter_rbegin = AZStd::rbegin(int_container);
        EXPECT_EQ(*iter_rbegin, 9);
        EXPECT_EQ(*AZStd::next(iter_rbegin), 8);
        EXPECT_EQ(*AZStd::next(iter_rbegin, 2), 7);
        ++iter_rbegin;
        EXPECT_EQ(*iter_rbegin, 8);

        typename Container::reverse_iterator iter_rend = AZStd::rend(int_container);
        EXPECT_EQ(*AZStd::prev(iter_rend), 0);
        EXPECT_EQ(*AZStd::prev(iter_rend, 2), 1);
        --iter_rend;
        EXPECT_EQ(*iter_rend, 0);

        //verify we can successfully modify the value in a non-const iterator
        *begin(int_container) = 42;
        EXPECT_EQ(*begin(int_container), 42);
    }

    template<typename Container>
    void Test_WrapperFunctions_ConstIterator_MutableContainer()
    {
        Container int_container = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};

        typename Container::const_iterator iter_cbegin = AZStd::cbegin(int_container);
        EXPECT_EQ(*iter_cbegin, 0);
        EXPECT_EQ(*AZStd::next(iter_cbegin), 1);
        EXPECT_EQ(*AZStd::next(iter_cbegin, 2), 2);
        ++iter_cbegin;
        EXPECT_EQ(*iter_cbegin, 1);

        typename Container::const_iterator iter_cend = AZStd::cend(int_container);
        EXPECT_EQ(iter_cend, int_container.cend());
        EXPECT_EQ(*AZStd::prev(iter_cend), 9);
        EXPECT_EQ(*AZStd::prev(iter_cend, 2), 8);
        --iter_cend;
        EXPECT_EQ(*iter_cend, 9);

        typename Container::const_reverse_iterator iter_crbegin = AZStd::crbegin(int_container);
        EXPECT_EQ(*iter_crbegin, 9);
        EXPECT_EQ(*AZStd::next(iter_crbegin), 8);
        EXPECT_EQ(*AZStd::next(iter_crbegin, 2), 7);
        ++iter_crbegin;
        EXPECT_EQ(*iter_crbegin, 8);

        typename Container::const_reverse_iterator iter_crend = AZStd::crend(int_container);
        EXPECT_EQ(*AZStd::prev(iter_crend), 0);
        EXPECT_EQ(*AZStd::prev(iter_crend, 2), 1);
        --iter_crend;
        EXPECT_EQ(*iter_crend, 0);
    }

    template<typename ConstContainer>
    void Test_WrapperFunctions_ConstIterator_ConstContainer()
    {
        const ConstContainer const_int_container = {{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }};

        typename ConstContainer::const_iterator const_iter_begin = AZStd::begin(const_int_container);
        EXPECT_EQ(*const_iter_begin, 10);
        EXPECT_EQ(*AZStd::next(const_iter_begin), 11);
        EXPECT_EQ(*AZStd::next(const_iter_begin, 2), 12);

        typename ConstContainer::const_iterator const_iter_end = AZStd::end(const_int_container);
        EXPECT_EQ(const_iter_end, const_int_container.end());
        EXPECT_EQ(*AZStd::prev(const_iter_end), 19);
        EXPECT_EQ(*AZStd::prev(const_iter_end, 2), 18);
    }

    TEST_F(Iterators, FunctionWrappers_MutableContainers)
    {
        Test_WrapperFunctions_MutableIterator_MutableContainer<AZStd::vector<int>>();
        Test_WrapperFunctions_ConstIterator_MutableContainer<AZStd::vector<int>>();

        Test_WrapperFunctions_MutableIterator_MutableContainer<AZStd::array<int, 10>>();
        Test_WrapperFunctions_ConstIterator_MutableContainer<AZStd::array<int, 10>>();

        Test_WrapperFunctions_MutableIterator_MutableContainer<AZStd::list<int>>();

        Test_WrapperFunctions_MutableIterator_MutableContainer<AZStd::set<int>>();

        //list and set currently don't support const iterator accessors so we can't test them here (yet)
    }

    TEST_F(Iterators, FunctionWrappers_ConstContainers)
    {
        Test_WrapperFunctions_ConstIterator_ConstContainer<const AZStd::vector<int>>();
        Test_WrapperFunctions_ConstIterator_ConstContainer<const AZStd::array<int, 10>>();
        Test_WrapperFunctions_ConstIterator_ConstContainer<const AZStd::list<int>>();
        Test_WrapperFunctions_ConstIterator_ConstContainer<const AZStd::set<int>>();
    }

    TEST_F(Iterators, FunctionWrappers_MutableRawArray)
    {
        int int_array[10] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };

        EXPECT_EQ(*AZStd::begin(int_array), 20);
        EXPECT_EQ(*AZStd::next(AZStd::begin(int_array)), 21);
        EXPECT_EQ(*AZStd::next(AZStd::begin(int_array), 2), 22);

        EXPECT_EQ(AZStd::end(int_array) - AZ_ARRAY_SIZE(int_array), AZStd::begin(int_array));
        EXPECT_EQ(*AZStd::prev(AZStd::end(int_array)), 29);
        EXPECT_EQ(*AZStd::prev(AZStd::end(int_array), 2), 28);

        EXPECT_EQ(*AZStd::rbegin(int_array), 29);
        EXPECT_EQ(*AZStd::next(AZStd::rbegin(int_array)), 28);
        EXPECT_EQ(*AZStd::next(AZStd::rbegin(int_array), 2), 27);

        EXPECT_EQ(*AZStd::prev(AZStd::rend(int_array)), 20);
        EXPECT_EQ(*AZStd::prev(AZStd::rend(int_array), 2), 21);

        EXPECT_EQ(*AZStd::crbegin(int_array), 29);
        EXPECT_EQ(*AZStd::next(AZStd::crbegin(int_array)), 28);
        EXPECT_EQ(*AZStd::next(AZStd::crbegin(int_array), 2), 27);

        EXPECT_EQ(*AZStd::prev(AZStd::crend(int_array)), 20);
        EXPECT_EQ(*AZStd::prev(AZStd::crend(int_array), 2), 21);

        //verify we can successfully modify the value in a non-const iterator
        *AZStd::begin(int_array) = -42;
        EXPECT_EQ(*AZStd::begin(int_array), -42);
    }

    TEST_F(Iterators, FunctionWrappers_ConstRawArray)
    {
        const int const_int_array[10] = { 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 };

        EXPECT_EQ(*AZStd::cbegin(const_int_array), 30);
        EXPECT_EQ(*AZStd::next(AZStd::cbegin(const_int_array)), 31);
        EXPECT_EQ(*AZStd::next(AZStd::cbegin(const_int_array), 2), 32);

        EXPECT_EQ(AZStd::cend(const_int_array) - AZ_ARRAY_SIZE(const_int_array), AZStd::cbegin(const_int_array));
        EXPECT_EQ(*AZStd::prev(AZStd::cend(const_int_array)), 39);
        EXPECT_EQ(*AZStd::prev(AZStd::cend(const_int_array), 2), 38);
    }

    TEST_F(Iterators, IteratorTraits_ResolveAtCompileTime)
    {
        using list_type = AZStd::list<int>;
        constexpr bool list_type_iterator_type_aliases = AZStd::Internal::has_iterator_type_aliases_v<typename list_type::iterator>;
        static_assert(AZStd::is_convertible_v<AZStd::Internal::iterator_traits_type_aliases<typename list_type::iterator, list_type_iterator_type_aliases>::iterator_category,
            AZStd::input_iterator_tag>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::iterator>::iterator_category, AZStd::bidirectional_iterator_tag>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::iterator>::value_type, int>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::iterator>::difference_type, AZStd::ptrdiff_t>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::iterator>::pointer, int*>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::iterator>::reference, int&>);
        static_assert(AZStd::input_iterator<typename list_type::iterator>);
        static_assert(!AZStd::contiguous_iterator<typename list_type::iterator>);

        constexpr bool list_type_const_iterator_type_aliases = AZStd::Internal::has_iterator_type_aliases_v<typename list_type::const_iterator>;
        static_assert(AZStd::is_convertible_v<AZStd::Internal::iterator_traits_type_aliases<typename list_type::iterator, list_type_const_iterator_type_aliases>::iterator_category,
            AZStd::input_iterator_tag>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::iterator_category, AZStd::bidirectional_iterator_tag>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::value_type, int>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::difference_type, AZStd::ptrdiff_t>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::pointer, const int*>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::reference, const int&>);
        static_assert(AZStd::input_iterator<typename list_type::const_iterator>);
        static_assert(!AZStd::contiguous_iterator<typename list_type::const_iterator>);

        using pointer_type = const char*;
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<pointer_type>::iterator_concept, AZStd::contiguous_iterator_tag>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<pointer_type>::iterator_category, AZStd::random_access_iterator_tag>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<pointer_type>::value_type, char>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<pointer_type>::difference_type, AZStd::ptrdiff_t>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<pointer_type>::pointer, const char*>);
        static_assert(AZStd::is_same_v<AZStd::iterator_traits<pointer_type>::reference, const char&>);
        static_assert(AZStd::contiguous_iterator<pointer_type>);
    }
}
