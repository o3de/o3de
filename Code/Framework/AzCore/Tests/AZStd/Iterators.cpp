/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/iterator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/utils.h>

using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    class Iterators
        : public AllocatorsFixture
    {
    };

    template<typename Container>
    void Test_WrapperFunctions_MutableIterator_MutableContainer()
    {
        Container int_container = {{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }};

        typename Container::iterator iter_begin = begin(int_container);
        EXPECT_EQ(*iter_begin, 0);
        EXPECT_EQ(*next(iter_begin), 1);
        EXPECT_EQ(*next(iter_begin, 2), 2);
        ++iter_begin;
        EXPECT_EQ(*iter_begin, 1);

        typename Container::iterator iter_end = end(int_container);
        EXPECT_EQ(iter_end, int_container.end());
        EXPECT_EQ(*prev(iter_end), 9);
        EXPECT_EQ(*prev(iter_end, 2), 8);
        --iter_end;
        EXPECT_EQ(*iter_end, 9);

        typename Container::reverse_iterator iter_rbegin = rbegin(int_container);
        EXPECT_EQ(*iter_rbegin, 9);
        EXPECT_EQ(*next(iter_rbegin), 8);
        EXPECT_EQ(*next(iter_rbegin, 2), 7);
        ++iter_rbegin;
        EXPECT_EQ(*iter_rbegin, 8);

        typename Container::reverse_iterator iter_rend = rend(int_container);
        EXPECT_EQ(*prev(iter_rend), 0);
        EXPECT_EQ(*prev(iter_rend, 2), 1);
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

        typename Container::const_iterator iter_cbegin = cbegin(int_container);
        EXPECT_EQ(*iter_cbegin, 0);
        EXPECT_EQ(*next(iter_cbegin), 1);
        EXPECT_EQ(*next(iter_cbegin, 2), 2);
        ++iter_cbegin;
        EXPECT_EQ(*iter_cbegin, 1);

        typename Container::const_iterator iter_cend = cend(int_container);
        EXPECT_EQ(iter_cend, int_container.cend());
        EXPECT_EQ(*prev(iter_cend), 9);
        EXPECT_EQ(*prev(iter_cend, 2), 8);
        --iter_cend;
        EXPECT_EQ(*iter_cend, 9);

        typename Container::const_reverse_iterator iter_crbegin = crbegin(int_container);
        EXPECT_EQ(*iter_crbegin, 9);
        EXPECT_EQ(*next(iter_crbegin), 8);
        EXPECT_EQ(*next(iter_crbegin, 2), 7);
        ++iter_crbegin;
        EXPECT_EQ(*iter_crbegin, 8);

        typename Container::const_reverse_iterator iter_crend = crend(int_container);
        EXPECT_EQ(*prev(iter_crend), 0);
        EXPECT_EQ(*prev(iter_crend, 2), 1);
        --iter_crend;
        EXPECT_EQ(*iter_crend, 0);
    }

    template<typename ConstContainer>
    void Test_WrapperFunctions_ConstIterator_ConstContainer()
    {
        const ConstContainer const_int_container = {{ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 }};

        typename ConstContainer::const_iterator const_iter_begin = begin(const_int_container);
        EXPECT_EQ(*const_iter_begin, 10);
        EXPECT_EQ(*next(const_iter_begin), 11);
        EXPECT_EQ(*next(const_iter_begin, 2), 12);

        typename ConstContainer::const_iterator const_iter_end = end(const_int_container);
        EXPECT_EQ(const_iter_end, const_int_container.end());
        EXPECT_EQ(*prev(const_iter_end), 19);
        EXPECT_EQ(*prev(const_iter_end, 2), 18);
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

        EXPECT_EQ(*begin(int_array), 20);
        EXPECT_EQ(*next(begin(int_array)), 21);
        EXPECT_EQ(*next(begin(int_array), 2), 22);

        EXPECT_EQ(end(int_array) - AZ_ARRAY_SIZE(int_array), begin(int_array));
        EXPECT_EQ(*prev(end(int_array)), 29);
        EXPECT_EQ(*prev(end(int_array), 2), 28);

        EXPECT_EQ(*rbegin(int_array), 29);
        EXPECT_EQ(*next(rbegin(int_array)), 28);
        EXPECT_EQ(*next(rbegin(int_array), 2), 27);

        EXPECT_EQ(*prev(rend(int_array)), 20);
        EXPECT_EQ(*prev(rend(int_array), 2), 21);

        EXPECT_EQ(*crbegin(int_array), 29);
        EXPECT_EQ(*next(crbegin(int_array)), 28);
        EXPECT_EQ(*next(crbegin(int_array), 2), 27);

        EXPECT_EQ(*prev(crend(int_array)), 20);
        EXPECT_EQ(*prev(crend(int_array), 2), 21);

        //verify we can successfully modify the value in a non-const iterator
        *begin(int_array) = -42;
        EXPECT_EQ(*begin(int_array), -42);
    }

    TEST_F(Iterators, FunctionWrappers_ConstRawArray)
    {
        const int const_int_array[10] = { 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 };

        EXPECT_EQ(*cbegin(const_int_array), 30);
        EXPECT_EQ(*next(cbegin(const_int_array)), 31);
        EXPECT_EQ(*next(cbegin(const_int_array), 2), 32);

        EXPECT_EQ(cend(const_int_array) - AZ_ARRAY_SIZE(const_int_array), cbegin(const_int_array));
        EXPECT_EQ(*prev(cend(const_int_array)), 39);
        EXPECT_EQ(*prev(cend(const_int_array), 2), 38);
    }

    TEST_F(Iterators, IteratorTraits_ResolveAtCompileTime)
    {
        using list_type = AZStd::list<int>;
        static_assert(AZStd::Internal::has_iterator_category_v<typename list_type::iterator>);
        static_assert(AZStd::Internal::has_iterator_type_aliases_v<typename list_type::iterator>);
        constexpr bool list_type_iterator_type_aliases = AZStd::Internal::has_iterator_type_aliases_v<typename list_type::iterator>;
        static_assert(AZStd::is_convertible_v<AZStd::Internal::iterator_traits_type_aliases<typename list_type::iterator, list_type_iterator_type_aliases>::iterator_category,
            AZStd::input_iterator_tag>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::iterator>::iterator_category, bidirectional_iterator_tag>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::iterator>::value_type, int>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::iterator>::difference_type, AZStd::ptrdiff_t>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::iterator>::pointer, int*>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::iterator>::reference, int&>);
        static_assert(AZStd::Internal::is_input_iterator_v<typename list_type::iterator>);
        static_assert(!AZStd::Internal::has_iterator_concept_v<AZStd::iterator_traits<typename list_type::iterator>>);
        static_assert(!AZStd::Internal::satisfies_contiguous_iterator_concept_v<typename list_type::iterator>);

        static_assert(AZStd::Internal::has_iterator_category_v<typename list_type::const_iterator>);
        static_assert(AZStd::Internal::has_iterator_type_aliases_v<typename list_type::const_iterator>);
        constexpr bool list_type_const_iterator_type_aliases = AZStd::Internal::has_iterator_type_aliases_v<typename list_type::const_iterator>;
        static_assert(AZStd::is_convertible_v<AZStd::Internal::iterator_traits_type_aliases<typename list_type::iterator, list_type_const_iterator_type_aliases>::iterator_category,
            AZStd::input_iterator_tag>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::iterator_category, bidirectional_iterator_tag>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::value_type, int>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::difference_type, AZStd::ptrdiff_t>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::pointer, const int*>);
        static_assert(is_same_v<AZStd::iterator_traits<typename list_type::const_iterator>::reference, const int&>);
        static_assert(AZStd::Internal::is_input_iterator_v<typename list_type::const_iterator>);
        static_assert(!AZStd::Internal::has_iterator_concept_v<AZStd::iterator_traits<typename list_type::const_iterator>>);
        static_assert(!AZStd::Internal::satisfies_contiguous_iterator_concept_v<typename list_type::const_iterator>);

        using pointer_type = const char*;
        static_assert(AZStd::Internal::has_iterator_category_v<AZStd::iterator_traits<pointer_type>>);
        static_assert(AZStd::Internal::has_iterator_type_aliases_v<AZStd::iterator_traits<pointer_type>>);
        static_assert(is_same_v<AZStd::iterator_traits<pointer_type>::iterator_concept, contiguous_iterator_tag>);
        static_assert(is_same_v<AZStd::iterator_traits<pointer_type>::iterator_category, random_access_iterator_tag>);
        static_assert(is_same_v<AZStd::iterator_traits<pointer_type>::value_type, char>);
        static_assert(is_same_v<AZStd::iterator_traits<pointer_type>::difference_type, AZStd::ptrdiff_t>);
        static_assert(is_same_v<AZStd::iterator_traits<pointer_type>::pointer, const char*>);
        static_assert(is_same_v<AZStd::iterator_traits<pointer_type>::reference, const char&>);
        static_assert(AZStd::Internal::has_iterator_concept_v<AZStd::iterator_traits<pointer_type>>);
        static_assert(AZStd::Internal::satisfies_contiguous_iterator_concept_v<pointer_type>);
    }
}
