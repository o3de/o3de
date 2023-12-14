/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/typetraits/is_same.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/sort.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Tests/Containers/Views/IteratorTestsBase.h>

namespace AZ::SceneAPI::Containers::Views::Internal
{
    TEST(PairIteratorCategoryTests, Declaration_SameCategory_TwoIteratorsHaveEqualCategory)
    {
        using Iterator = AZStd::vector<int>::iterator;
        using CategoryInfo = PairIteratorCategory<Iterator, Iterator>;

        EXPECT_TRUE(CategoryInfo::s_sameCategory);
        EXPECT_TRUE(CategoryInfo::s_firstIteratorCategoryIsBaseOfSecondIterator);
        EXPECT_TRUE(CategoryInfo::s_SecondIteratorCategoryIsBaseOfFirstIterator);
        bool isSameCategory = AZStd::is_same<AZStd::iterator_traits<Iterator>::iterator_category, CategoryInfo::Category>::value;
        EXPECT_TRUE(isSameCategory);
    }

    TEST(PairIteratorCategoryTests, Declaration_DifferentCategoryWithFirstHighest_NotTheSameCategoryAndPicksLowestCategory)
    {
        using IteratorHigh = AZStd::vector<int>::iterator;
        using IteratorLow = AZStd::list<int>::iterator;
        using CategoryInfo = PairIteratorCategory<IteratorHigh, IteratorLow>;

        EXPECT_FALSE(CategoryInfo::s_sameCategory);
        EXPECT_FALSE(CategoryInfo::s_firstIteratorCategoryIsBaseOfSecondIterator);
        EXPECT_TRUE(CategoryInfo::s_SecondIteratorCategoryIsBaseOfFirstIterator);
        bool isSameCategory = AZStd::is_same<AZStd::iterator_traits<IteratorLow>::iterator_category, CategoryInfo::Category>::value;
        EXPECT_TRUE(isSameCategory);
    }

    TEST(PairIteratorCategoryTests, Declaration_DifferentCategoryWithFirstLowest_NotTheSameCategoryAndPicksLowestCategory)
    {
        using IteratorHigh = AZStd::vector<int>::iterator;
        using IteratorLow = AZStd::list<int>::iterator;
        using CategoryInfo = PairIteratorCategory<IteratorLow, IteratorHigh>;

        EXPECT_FALSE(CategoryInfo::s_sameCategory);
        EXPECT_TRUE(CategoryInfo::s_firstIteratorCategoryIsBaseOfSecondIterator);
        EXPECT_FALSE(CategoryInfo::s_SecondIteratorCategoryIsBaseOfFirstIterator);
        bool isSameCategory = AZStd::is_same<AZStd::iterator_traits<IteratorLow>::iterator_category, CategoryInfo::Category>::value;
        EXPECT_TRUE(isSameCategory);
    }
} // namespace AZ::SceneAPI::Containers::Views::Internal

namespace AZ::SceneAPI::Containers::Views
{
    template<typename CollectionType>
    class PairIteratorTests
        : public IteratorTypedTestsBase<CollectionType>
    {
    public:
        void AddElementPair(int first, int second)
        {
            AddElement(m_firstContainer, first);
            AddElement(m_secondContainer, second);
        }

        PairIteratorTests()
        {
            AddElementPair(42, 88);
            AddElementPair(142, 188);
        }
        ~PairIteratorTests() override = default;

        void Clear()
        {
            m_firstContainer.clear();
            m_secondContainer.clear();
        }

        CollectionType m_firstContainer;
        CollectionType m_secondContainer;
    };

    TYPED_TEST_CASE_P(PairIteratorTests);

    TYPED_TEST_P(PairIteratorTests, MakePairIterator_BuildFromTwoSeparateIterators_StoredIteratorsMatchTheGivenIterators)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        EXPECT_EQ(iterator.GetFirstIterator(), this->m_firstContainer.begin());
        EXPECT_EQ(iterator.GetSecondIterator(), this->m_secondContainer.begin());
    }

    TYPED_TEST_P(PairIteratorTests, MakePairIterator_BuildFromTwoSeparateIterators_FirstAndSecondInContainersCanBeAccessedThroughIterator)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        auto first = AZStd::get<0>(*iterator);
        auto second = AZStd::get<1>(*iterator);
        EXPECT_EQ(first, *this->m_firstContainer.begin());
        EXPECT_EQ(second, *this->m_secondContainer.begin());
    }

    TYPED_TEST_P(PairIteratorTests, MakePairView_CreateFromIterators_IteratorsInViewMatchExplicitlyCreatedIterators)
    {
        auto begin = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        auto end = MakePairIterator(this->m_firstContainer.end(), this->m_secondContainer.end());

        auto view = MakePairView(this->m_firstContainer.begin(), this->m_firstContainer.end(), this->m_secondContainer.begin(), this->m_secondContainer.end());
        EXPECT_EQ(view.begin(), begin);
        EXPECT_EQ(view.end(), end);
    }

    TYPED_TEST_P(PairIteratorTests, MakePairView_CreateFromViews_IteratorsInViewMatchExplicitlyCreatedIterators)
    {
        auto firstView = MakeView(this->m_firstContainer.begin(), this->m_firstContainer.end());
        auto secondView = MakeView(this->m_secondContainer.begin(), this->m_secondContainer.end());

        auto begin = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        auto end = MakePairIterator(this->m_firstContainer.end(), this->m_secondContainer.end());

        auto view = MakePairView(firstView, secondView);
        EXPECT_EQ(view.begin(), begin);
        EXPECT_EQ(view.end(), end);
    }

    TYPED_TEST_P(PairIteratorTests, OperatorStar_DereferencingChangesFirst_FirstChangeIsPassedToContainer)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        AZStd::get<0>(*iterator) = 4;

        EXPECT_EQ(4, *this->m_firstContainer.begin());
    }

    TYPED_TEST_P(PairIteratorTests, OperatorStar_DereferencingChangesSecond_SecondsChangeIsPassedToContainer)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        AZStd::get<1>(*iterator) = 4;

        EXPECT_EQ(4, *this->m_secondContainer.begin());
    }


    TYPED_TEST_P(PairIteratorTests, OperatorArrow_DereferencingChangesFirst_FirstChangeIsPassedToContainer)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        AZStd::get<0>(*iterator.operator->()) = 4;

        EXPECT_EQ(4, *this->m_firstContainer.begin());
    }

    TYPED_TEST_P(PairIteratorTests, OperatorArrow_DereferencingChangesSecond_SecondsChangeIsPassedToContainer)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        AZStd::get<1>(*iterator.operator->()) = 4;

        EXPECT_EQ(4, *this->m_secondContainer.begin());
    }

    TYPED_TEST_P(PairIteratorTests, PreIncrementOperator_IncrementingMovesBothIterators_BothStoredIteratorsMoved)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        ++iterator;

        auto cmpFirst = this->m_firstContainer.begin();
        auto cmpSecond = this->m_secondContainer.begin();
        ++cmpFirst;
        ++cmpSecond;

        EXPECT_EQ(iterator.GetFirstIterator(), cmpFirst);
        EXPECT_EQ(iterator.GetSecondIterator(), cmpSecond);
    }

    TYPED_TEST_P(PairIteratorTests, PostIncrementOperator_IncrementingMovesBothIterators_BothStoredIteratorsMoved)
    {
        auto iterator = MakePairIterator(this->m_firstContainer.begin(), this->m_secondContainer.begin());
        iterator++;

        auto cmpFirst = this->m_firstContainer.begin();
        auto cmpSecond = this->m_secondContainer.begin();
        ++cmpFirst;
        ++cmpSecond;

        EXPECT_EQ(iterator.GetFirstIterator(), cmpFirst);
        EXPECT_EQ(iterator.GetSecondIterator(), cmpSecond);
    }

    TYPED_TEST_P(PairIteratorTests, Algorithms_Generate_FirstContainerFilledWithTheFirstAndSecondContainerFilledWithSecondInGivenPair)
    {
        this->Clear();
        this->m_firstContainer.resize(10);
        this->m_secondContainer.resize(10);

        auto view = MakePairView(this->m_firstContainer.begin(), this->m_firstContainer.end(), this->m_secondContainer.begin(), this->m_secondContainer.end());
        AZStd::generate(view.begin(), view.end(),
            []() -> AZStd::ranges::range_value_t<decltype(view)>
        {
            using ValueType = AZStd::ranges::range_value_t<decltype(view)>;
            return ValueType{ 3, 9 };
        });

        for (auto it : this->m_firstContainer)
        {
            EXPECT_EQ(3, it);
        }
        for (auto it : this->m_secondContainer)
        {
            EXPECT_EQ(9, it);
        }
    }

    REGISTER_TYPED_TEST_CASE_P(PairIteratorTests,
        MakePairIterator_BuildFromTwoSeparateIterators_StoredIteratorsMatchTheGivenIterators,
        MakePairIterator_BuildFromTwoSeparateIterators_FirstAndSecondInContainersCanBeAccessedThroughIterator,
        MakePairView_CreateFromIterators_IteratorsInViewMatchExplicitlyCreatedIterators,
        MakePairView_CreateFromViews_IteratorsInViewMatchExplicitlyCreatedIterators,
        OperatorStar_DereferencingChangesFirst_FirstChangeIsPassedToContainer,
        OperatorStar_DereferencingChangesSecond_SecondsChangeIsPassedToContainer,
        OperatorArrow_DereferencingChangesFirst_FirstChangeIsPassedToContainer,
        OperatorArrow_DereferencingChangesSecond_SecondsChangeIsPassedToContainer,
        PreIncrementOperator_IncrementingMovesBothIterators_BothStoredIteratorsMoved,
        PostIncrementOperator_IncrementingMovesBothIterators_BothStoredIteratorsMoved,
        Algorithms_Generate_FirstContainerFilledWithTheFirstAndSecondContainerFilledWithSecondInGivenPair);

    INSTANTIATE_TYPED_TEST_CASE_P(CommonTests, PairIteratorTests, BasicCollectionTypes);

    // The following tests are done as standalone tests as not all iterators support this functionality
    TEST(PairIteratorTest, PreDecrementIterator_DecrementingMovesBothIterators_BothStoredIteratorsMoved)
    {
        AZStd::vector<int> firstContainer = { 42, 142 };
        AZStd::vector<int> secondContainer = { 88, 188 };

        auto iterator = MakePairIterator(firstContainer.begin(), secondContainer.begin());
        ++iterator;
        --iterator;

        EXPECT_EQ(iterator.GetFirstIterator(), firstContainer.begin());
        EXPECT_EQ(iterator.GetSecondIterator(), secondContainer.begin());
    }

    TEST(PairIteratorTest, PostDecrementIterator_DecrementingMovesBothIterators_BothStoredIteratorsMoved)
    {
        AZStd::vector<int> firstContainer = { 42, 142 };
        AZStd::vector<int> secondContainer = { 88, 188 };

        auto iterator = MakePairIterator(firstContainer.begin(), secondContainer.begin());
        ++iterator;
        iterator--;

        EXPECT_EQ(iterator.GetFirstIterator(), firstContainer.begin());
        EXPECT_EQ(iterator.GetSecondIterator(), secondContainer.begin());
    }

    TEST(PairIteratorTest, Algorithms_Sort_BothListSortedByFirstThenSecondAndPairsNotBroken)
    {
        AZStd::vector<int> firstContainer = { 105, 106, 101, 104, 103, 108 };
        AZStd::vector<int> secondContainer = { 205, 206, 201, 204, 203, 208 };

        auto view = MakePairView(firstContainer.begin(), firstContainer.end(), secondContainer.begin(), secondContainer.end());
        AZStd::sort(view.begin(), view.end());

        EXPECT_EQ(AZStd::get<0>(*view.begin()) + 100, AZStd::get<1>(*view.begin()));
        for (auto it = view.begin() + 1; it != view.end(); ++it)
        {
            auto previousIt = it - 1;
            EXPECT_LT(AZStd::get<0>(*previousIt), AZStd::get<0>(*it));
            EXPECT_EQ(AZStd::get<0>(*it) + 100, AZStd::get<1>(*it));
        }
    }

    TEST(PairIteratorTest, Algorithms_Reverse_SecondsAreInDescendingOrder)
    {
        AZStd::vector<int> firstContainer = { 1, 2, 3, 4, 5 };
        AZStd::vector<int> secondContainer = { 1, 2, 3, 4, 5 };

        auto view = MakePairView(firstContainer.begin(), firstContainer.end(), secondContainer.begin(), secondContainer.end());

        AZStd::ranges::reverse(view.begin(), view.end());

        for (auto it = view.begin() + 1; it != view.end(); ++it)
        {
            auto previousIt = it - 1;
            EXPECT_GT(*previousIt, *it);
        }
    }
} // AZ::SceneAPI::Containers::Views
