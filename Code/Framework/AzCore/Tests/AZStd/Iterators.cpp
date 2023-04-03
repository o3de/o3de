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
#include <AzCore/std/iterator/common_iterator.h>
#include <AzCore/std/iterator/const_iterator.h>
#include <AzCore/std/iterator/counted_iterator.h>
#include <AzCore/std/iterator/move_sentinel.h>
#include <AzCore/std/iterator/unreachable_sentinel.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/utils.h>


namespace UnitTest
{
    class Iterators
        : public LeakDetectionFixture
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

    namespace IteratorInternal
    {
        struct TestIterator
        {
            using difference_type = ptrdiff_t;
            using value_type = int;
            using pointer = void;
            using reference = int&;
            using iterator_category = AZStd::bidirectional_iterator_tag;
            using iterator_concept = AZStd::bidirectional_iterator_tag;

            struct TestOperatorArrow
            {
                bool m_boolValue{};
            };

            int& operator*()
            {
                return m_value;
            }
            const int& operator*() const
            {
                return m_value;
            }
            TestOperatorArrow* operator->()
            {
                return &m_operatorArrow;
            }
            const TestOperatorArrow* operator->() const
            {
                return &m_operatorArrow;
            }
            TestIterator& operator++()
            {
                ++m_value;
                return *this;
            }
            TestIterator operator++(int)
            {
                auto oldThis = *this;
                m_value++;
                return oldThis;
            }
            TestIterator& operator--()
            {
                --m_value;
                return *this;
            }
            TestIterator operator--(int)
            {
                auto oldThis = *this;
                m_value--;
                return oldThis;
            }
            bool operator==(const TestIterator& other) const
            {
                return m_value == other.m_value;
            }
            bool operator!=(const TestIterator& other) const
            {
                return m_value != other.m_value;
            }

            static constexpr int EndValue()
            {
                return 10;
            }
            int m_value{};
            TestOperatorArrow m_operatorArrow;
        };
        struct TestSentinel
        {
            int m_end = TestIterator::EndValue();
        };

        bool operator==(const TestIterator& iter, const TestSentinel& sen)
        {
            return iter.m_value == sen.m_end;
        }
        bool operator==(const TestSentinel& sen, const TestIterator& iter)
        {
            return operator==(iter, sen);
        }
        bool operator!=(const TestIterator& iter, const TestSentinel& sen)
        {
            return !operator==(iter, sen);
        }
        bool operator!=(const TestSentinel& sen, const TestIterator& iter)
        {
            return !operator==(iter, sen);
        }

        ptrdiff_t operator-(const TestIterator& iter, const TestSentinel& sen)
        {
            return iter.m_value - sen.m_end;
        }
        ptrdiff_t operator-(const TestSentinel& sen, const TestIterator& iter)
        {
            return sen.m_end - iter.m_value;
        }
    }

    TEST_F(Iterators, CommonIterator_CanWrapIteratorWithDifferentSentinelType_Succeeds)
    {
        using CommonTestIterator = AZStd::common_iterator<IteratorInternal::TestIterator, IteratorInternal::TestSentinel>;
        CommonTestIterator testIter{ IteratorInternal::TestIterator{} };
        constexpr CommonTestIterator testSen{ IteratorInternal::TestSentinel{} };
        ASSERT_NE(testSen, testIter);
        EXPECT_EQ(IteratorInternal::TestIterator::EndValue(), AZStd::ranges::distance(testIter, testSen));
        EXPECT_EQ(0, *testIter);
        ++testIter;
        EXPECT_EQ(1, *testIter);
        AZStd::ranges::advance(testIter, testSen);
        EXPECT_EQ(testSen, testIter);
    }

    TEST_F(Iterators, CommonIterator_OperatorArrow_Compiles)
    {
        using CommonTestIterator = AZStd::common_iterator<IteratorInternal::TestIterator, IteratorInternal::TestSentinel>;
        CommonTestIterator testIter{ IteratorInternal::TestIterator{} };
        EXPECT_FALSE(testIter.operator->()->m_boolValue);
    }

    TEST_F(Iterators, UnreachableSentinel_DoesNotCompareEqualToAnyIterator)
    {
        char testString[] = "123";
        EXPECT_NE(AZStd::unreachable_sentinel, AZStd::ranges::begin(testString));
        EXPECT_NE(AZStd::unreachable_sentinel, AZStd::ranges::end(testString));
        EXPECT_NE(AZStd::ranges::begin(testString), AZStd::unreachable_sentinel);
        EXPECT_NE(AZStd::ranges::end(testString), AZStd::unreachable_sentinel);
    }

    TEST_F(Iterators, MoveIterator_SupportsCpp17IteratorTraits_Compiles)
    {
        using MoveIterator = AZStd::move_iterator<int*>;
        using MoveIteratorTraits = AZStd::iterator_traits<MoveIterator>;
        static_assert(AZStd::Internal::sfinae_trigger_v<typename MoveIteratorTraits::value_type>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename MoveIteratorTraits::difference_type>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename MoveIteratorTraits::iterator_category>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename MoveIteratorTraits::pointer>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename MoveIteratorTraits::reference>);
    }

    TEST_F(Iterators, MoveSentinel_ComparableToMoveIterator_Succeeds)
    {
        AZStd::array testStringArray{ AZStd::string("Hello"), AZStd::string("World") };
        auto moveIterator = AZStd::make_move_iterator(testStringArray.begin());
        auto moveSentinel = AZStd::move_sentinel<decltype(testStringArray)::iterator>(testStringArray.end());

        AZStd::array<AZStd::string, 2> movedStringArray{};
        for (size_t i = 0; moveIterator != moveSentinel; ++moveIterator, ++i)
        {
            movedStringArray[i] = *moveIterator;
        }

        EXPECT_EQ(moveSentinel, moveIterator);
        EXPECT_EQ(moveIterator, moveSentinel);
        EXPECT_THAT(movedStringArray, ::testing::ElementsAre(AZStd::string_view("Hello"), AZStd::string_view("World")));
        EXPECT_THAT(testStringArray, ::testing::ElementsAre(AZStd::string_view(), AZStd::string_view()));
    }

    TEST_F(Iterators, ConstIterator_ProvidesOnlyConstantReferencesToValueType_Succeeds)
    {
        AZStd::string testString("Hello World");
        using ConstIterator = AZStd::const_iterator<typename decltype(testString)::iterator>;
        using ConstSentinel = AZStd::const_sentinel<typename decltype(testString)::iterator>;
        ConstIterator iter = testString.begin();
        ConstSentinel sen = testString.end();

        static_assert(AZStd::is_const_v<AZStd::remove_reference_t<decltype(*iter)>>);
        AZStd::string result;
        for (; iter != sen; ++iter)
        {
            result += *iter;
        }

        EXPECT_EQ(testString, result);
    }

    TEST_F(Iterators, ConstIterator_SupportsCpp17IteratorTraits_Compiles)
    {
        // Indicates whether the iterator type can be used with C++17 style iterator traits
        // when using algorithms from std namespace
        using ConstIterator = AZStd::const_iterator<typename AZStd::string::iterator>;
        using ConstIteratorTraits = AZStd::iterator_traits<ConstIterator>;
        static_assert(AZStd::Internal::sfinae_trigger_v<typename ConstIteratorTraits::value_type>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename ConstIteratorTraits::difference_type>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename ConstIteratorTraits::iterator_category>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename ConstIteratorTraits::pointer>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename ConstIteratorTraits::reference>);
    }

    TEST_F(Iterators, CountedIterator_CanIterate_UntilDefaultSentinel)
    {
        AZStd::string testString("Hello World");
        AZStd::counted_iterator<typename decltype(testString)::iterator> testCountedIter(testString.begin(), 5);

        AZStd::string result;
        for (; testCountedIter != AZStd::default_sentinel; ++testCountedIter)
        {
            result += *testCountedIter;
        }

        EXPECT_EQ("Hello", result);
        EXPECT_EQ(AZStd::default_sentinel, testCountedIter);
        ASSERT_EQ(testString.begin() + 5, testCountedIter.base());
        EXPECT_EQ(0, testCountedIter.count());

        auto copyIter = testCountedIter - 5;
        copyIter[3] = 'j';
        EXPECT_EQ('H', *copyIter);
        EXPECT_EQ('j', *(copyIter + 3));
    }

    TEST_F(Iterators, CountedIterator_CompareableAgainstOtherCountedIterator)
    {
        AZStd::string_view testString("Hello World");
        AZStd::counted_iterator<typename decltype(testString)::iterator> testCountedIter(testString.begin(), 5);
        AZStd::counted_iterator<typename decltype(testString)::iterator> testCountedIter2(testString.begin(), 5);

        EXPECT_EQ(testCountedIter, testCountedIter2);
        ++testCountedIter;
        EXPECT_NE(testCountedIter, testCountedIter2);
        EXPECT_LT(testCountedIter2, testCountedIter);
        EXPECT_GT(testCountedIter, testCountedIter2);
        EXPECT_LE(testCountedIter2, testCountedIter);
        EXPECT_GE(testCountedIter, testCountedIter2);
        EXPECT_EQ(4, testCountedIter.count());
    }

    TEST_F(Iterators, CountedIterator_SupportsCpp17IteratorTraits_Compiles)
    {
        // Indicates whether the iterator type can be used with C++17 style iterator traits
        // when using algorithms from std namespace
        using CountedIterator = AZStd::counted_iterator<typename AZStd::string::iterator>;
        using CountedIteratorTraits = AZStd::iterator_traits<CountedIterator>;
        static_assert(AZStd::Internal::sfinae_trigger_v<typename CountedIteratorTraits::value_type>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename CountedIteratorTraits::difference_type>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename CountedIteratorTraits::iterator_category>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename CountedIteratorTraits::pointer>);
        static_assert(AZStd::Internal::sfinae_trigger_v<typename CountedIteratorTraits::reference>);
    }
}
