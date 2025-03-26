/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#define _SCL_SECURE_NO_WARNINGS

#include <AzTest/AzTest.h>

#include <AzCore/std/createdestroy.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Tests/Containers/Views/IteratorTestsBase.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                template<typename CollectionType>
                class FilterIteratorBasicTests
                    : public IteratorTypedTestsBase<CollectionType>
                {
                public:
                    FilterIteratorBasicTests()
                    {
                        MakeComparePredicate(0);
                    }
                    ~FilterIteratorBasicTests() override = default;

                    using BaseIteratorReference = typename IteratorTypedTestsBase<CollectionType>::BaseIteratorReference;
                    template<typename T>
                    void MakeComparePredicate(T compareValue)
                    {
                        m_testPredicate = [compareValue](const BaseIteratorReference value) -> bool
                            {
                                return value >= compareValue;
                            };
                    }

                    template<typename T>
                    void MakeNotEqualPredicate(T compareValue)
                    {
                        m_testPredicate = [compareValue](const BaseIteratorReference value) -> bool
                            {
                                return value != compareValue;
                            };
                    }

                    AZStd::function<bool(const BaseIteratorReference)> m_testPredicate;
                };

                TYPED_TEST_SUITE_P(FilterIteratorBasicTests);

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Constructor_InputIsEmptyValidBaseIterator_NoCrash)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    FilterIterator<BaseIterator>
                    testIterator(this->m_testCollection.begin(), this->m_testCollection.end(),
                        this->m_testPredicate);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Constructor_MovesForwardBasedOnPredicate_ExpectSkipFirstEntryAndReturnSecond)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(1);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    lhsIterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    EXPECT_EQ(1, *lhsIterator);
                }

                // Increment operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPreIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    ++iterator;

                    EXPECT_EQ(1, *iterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPreIncrement_MoveOneSkippingOne_ReturnsTheThirdValue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(1);

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    ++iterator;

                    EXPECT_EQ(2, *iterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPostIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    iterator++;

                    EXPECT_EQ(1, *iterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorPostIncrement_MoveOneSkippingOne_ReturnsTheThirdValue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(1);

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    iterator++;

                    EXPECT_EQ(2, *iterator);
                }

                // Equals equals operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesAll_ReturnsFalse)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);
                    FilterIterator<BaseIterator>
                    lhsIterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    FilterIterator<BaseIterator>
                    rhsIterator(this->GetBaseIterator(1), this->m_testCollection.end(),
                        this->m_testPredicate);
                    EXPECT_NE(lhsIterator, rhsIterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesPart_ReturnsTrue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(1);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    lhsIterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    FilterIterator<BaseIterator>
                    rhsIterator(this->GetBaseIterator(1), this->m_testCollection.end(),
                        this->m_testPredicate);
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorEqualsEquals_SkippingAllEntriesMatchesWithEndIterator_FullySkippedIteratorIsSameAsEnd)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(3);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    FilterIterator<BaseIterator>
                    iterator(this->m_testCollection.begin(), this->m_testCollection.end(),
                        this->m_testPredicate);
                    FilterIterator<BaseIterator>
                    endIterator(this->m_testCollection.end(), this->m_testCollection.end(),
                        this->m_testPredicate);
                    EXPECT_EQ(iterator, endIterator);
                }

                // Not equals operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorNotEquals_DifferentObjects_ReturnsTrue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    FilterIterator<BaseIterator>
                    lhsIterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    FilterIterator<BaseIterator>
                    rhsIterator(this->GetBaseIterator(1), this->m_testCollection.end(),
                        this->m_testPredicate);

                    EXPECT_TRUE(lhsIterator != rhsIterator);
                }

                // Star operator
                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorStar_GetValueByDereferencingIterator_ExpectFirstValueInArray)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    AddElement(this->m_testCollection, 0);

                    FilterIterator<BaseIterator>
                    lhsIterator(this->GetBaseIterator(0), this->m_testCollection.end(),
                        this->m_testPredicate);
                    EXPECT_EQ(0, *lhsIterator);
                }

                // Decrement operator
                template<typename Container, typename = typename AZStd::iterator_traits<typename Container::iterator>::iterator_category>
                struct OperatorPreDecrement
                {
                    int operator()([[maybe_unused]] FilterIteratorBasicTests<Container>& test, [[maybe_unused]] int iteratorOffset, int expectedResult)
                    {
                        return expectedResult;
                    }
                };

                template<typename Container>
                struct OperatorPreDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                    int operator()(FilterIteratorBasicTests<Container>& test, int iteratorOffset, [[maybe_unused]] int expectedResult)
                    {
                        FilterIterator<typename Container::iterator>
                        iterator(test.GetBaseIterator(iteratorOffset), test.m_testCollection.begin(), test.m_testCollection.end(),
                            test.m_testPredicate);

                        --iterator;
                        return *iterator;
                    };
                };

                template<typename Container>
                struct OperatorPreDecrement<Container, AZStd::random_access_iterator_tag>
                    : public OperatorPreDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                template<typename Container>
                struct OperatorPreDecrement<Container, AZStd::contiguous_iterator_tag>
                    : public OperatorPreDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                template<typename Container, typename = typename AZStd::iterator_traits<typename Container::iterator>::iterator_category>
                struct OperatorPostDecrement
                {
                    int operator()([[maybe_unused]] FilterIteratorBasicTests<Container>& test, [[maybe_unused]] int iteratorOffset, int expectedResult)
                    {
                        return expectedResult;
                    }
                };

                template<typename Container>
                struct OperatorPostDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                    int operator()(FilterIteratorBasicTests<Container>& test, int iteratorOffset, [[maybe_unused]] int expectedResult)
                    {
                        FilterIterator<typename Container::iterator>
                            iterator(test.GetBaseIterator(iteratorOffset), test.m_testCollection.begin(), test.m_testCollection.end(),
                                test.m_testPredicate);

                        iterator--;
                        return *iterator;
                    };
                };

                template<typename Container>
                struct OperatorPostDecrement<Container, AZStd::random_access_iterator_tag>
                    : public OperatorPostDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                template<typename Container>
                struct OperatorPostDecrement<Container, AZStd::contiguous_iterator_tag>
                    : public OperatorPostDecrement<Container, AZStd::bidirectional_iterator_tag>
                {
                };

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveOneUnfilteredElementDown_ReturnsTheFirstValue)
                {
                    using CollectionType = typename IteratorTypedTestsBase<TypeParam>::CollectionType;

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);
                    EXPECT_EQ(0, OperatorPreDecrement<CollectionType>()(*this, 1, 0));
                    EXPECT_EQ(0, OperatorPostDecrement<CollectionType>()(*this, 1, 0));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveOneFilteredElementDown_ReturnsTheFirstValue)
                {
                    using CollectionType = typename IteratorTypedTestsBase<TypeParam>::CollectionType;

                    this->MakeNotEqualPredicate(1);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);
                    EXPECT_EQ(0, OperatorPreDecrement<CollectionType>()(*this, 2, 0));
                    EXPECT_EQ(0, OperatorPostDecrement<CollectionType>()(*this, 2, 0));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveDownToLastFilteredElement_ExpectToStayOnCurrentElement)
                {
                    using CollectionType = typename IteratorTypedTestsBase<TypeParam>::CollectionType;

                    this->MakeNotEqualPredicate(0);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);
                    EXPECT_EQ(1, OperatorPreDecrement<CollectionType>()(*this, 1, 1));
                    EXPECT_EQ(1, OperatorPostDecrement<CollectionType>()(*this, 1, 1));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    OperatorDecrement_MoveOneUnfilteredElementDownFromEnd_ReturnsTheSecondValue)
                {
                    using CollectionType = typename IteratorTypedTestsBase<TypeParam>::CollectionType;

                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);
                    EXPECT_EQ(1, OperatorPreDecrement<CollectionType>()(*this, 2, 1));
                    EXPECT_EQ(1, OperatorPostDecrement<CollectionType>()(*this, 2, 1));
                }

                // Filtered Elements
                TYPED_TEST_P(FilterIteratorBasicTests,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(2);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);
                    AddElement(this->m_testCollection, 3);
                    AZStd::unordered_set<int> expectedElements = {2, 3};
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(this->m_testCollection.begin(), this->m_testCollection.end(),
                            this->m_testPredicate);
                    for (auto it : view)
                    {
                        if (expectedElements.find(it) != expectedElements.end())
                        {
                            expectedElements.erase(it);
                        }
                    }
                    EXPECT_TRUE(expectedElements.empty());
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    MakeFilterView_InputIteratorNotStartsAtBegin_CorrectFilteredElements)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(2);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);
                    AddElement(this->m_testCollection, 3);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    AZStd::unordered_set<int> expectedElements = { 2, 3 };
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(this->GetBaseIterator(1), this->m_testCollection.end(),
                            this->m_testPredicate);
                    for (auto it : view)
                    {
                        if (expectedElements.find(it) != expectedElements.end())
                        {
                            expectedElements.erase(it);
                        }
                    }
                    EXPECT_TRUE(expectedElements.empty());
                }

                // Algorithms
                TYPED_TEST_P(FilterIteratorBasicTests,
                    Algorithms_CopyFilteredContainer_ContainerIsCopiedWithoutFilteredValue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeNotEqualPredicate(1);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    View<FilterIterator<BaseIterator> > view = MakeFilterView(this->GetBaseIterator(0), this->m_testCollection.end(),
                            this->m_testPredicate);

                    AZStd::vector<int> target;
                    AZStd::copy(view.begin(), view.end(), AZStd::back_inserter(target));

                    ASSERT_EQ(2, target.size());
                    EXPECT_EQ(0, target[0]);
                    EXPECT_EQ(2, target[1]);
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Algorithms_FillFilteredContainer_AllValuesAreSetTo42ExceptFilteredValue)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeNotEqualPredicate(1);
                    AddElement(this->m_testCollection, 0);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);

                    ReorderToMatchIterationWithAddition(this->m_testCollection);

                    View<FilterIterator<BaseIterator> > view = MakeFilterView(this->GetBaseIterator(0), this->m_testCollection.end(),
                            this->m_testPredicate);

                    AZStd::generate_n(view.begin(), 2, [](){ return 42; });

                    EXPECT_EQ(42, *this->GetBaseIterator(0));
                    EXPECT_EQ(1, *this->GetBaseIterator(1));
                    EXPECT_EQ(42, *this->GetBaseIterator(2));
                }

                TYPED_TEST_P(FilterIteratorBasicTests,
                    Algorithms_PartialSortCopyFilteredContainer_AllValuesLargerOrEqualThan10AreCopiedAndSorted)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(10);
                    AddElement(this->m_testCollection, 18);
                    AddElement(this->m_testCollection, 42);
                    AddElement(this->m_testCollection, 36);
                    AddElement(this->m_testCollection, 9);
                    AddElement(this->m_testCollection, 88);
                    AddElement(this->m_testCollection, 3);

                    AZStd::vector<int> results;
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(this->m_testCollection.begin(), this->m_testCollection.end(), this->m_testPredicate);

                    AZStd::copy(view.begin(), view.end(), AZStd::back_inserter(results));
                    for (auto val : results)
                    {
                        EXPECT_GE(val, 10);
                    }
                }

                REGISTER_TYPED_TEST_SUITE_P(FilterIteratorBasicTests,
                    Constructor_InputIsEmptyValidBaseIterator_NoCrash,
                    OperatorStar_GetValueByDereferencingIterator_ExpectFirstValueInArray,
                    Constructor_MovesForwardBasedOnPredicate_ExpectSkipFirstEntryAndReturnSecond,
                    OperatorPreIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue,
                    OperatorPreIncrement_MoveOneSkippingOne_ReturnsTheThirdValue,
                    OperatorPostIncrement_MoveOneUnfilteredElementUp_ReturnsTheSecondValue,
                    OperatorPostIncrement_MoveOneSkippingOne_ReturnsTheThirdValue,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesAll_ReturnsFalse,
                    OperatorEqualsEquals_DifferentlyInitializedObjectsPredicatePassesPart_ReturnsTrue,
                    OperatorEqualsEquals_SkippingAllEntriesMatchesWithEndIterator_FullySkippedIteratorIsSameAsEnd,
                    OperatorNotEquals_DifferentObjects_ReturnsTrue,
                    OperatorDecrement_MoveOneUnfilteredElementDown_ReturnsTheFirstValue,
                    OperatorDecrement_MoveOneFilteredElementDown_ReturnsTheFirstValue,
                    OperatorDecrement_MoveDownToLastFilteredElement_ExpectToStayOnCurrentElement,
                    OperatorDecrement_MoveOneUnfilteredElementDownFromEnd_ReturnsTheSecondValue,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements,
                    MakeFilterView_InputIteratorNotStartsAtBegin_CorrectFilteredElements,
                    Algorithms_CopyFilteredContainer_ContainerIsCopiedWithoutFilteredValue,
                    Algorithms_FillFilteredContainer_AllValuesAreSetTo42ExceptFilteredValue,
                    Algorithms_PartialSortCopyFilteredContainer_AllValuesLargerOrEqualThan10AreCopiedAndSorted
                    );

                INSTANTIATE_TYPED_TEST_SUITE_P(CommonTests,
                    FilterIteratorBasicTests,
                    BasicCollectionTypes);

                // Map iterator tests
                template<typename CollectionType>
                class FilterIteratorMapTests
                    : public IteratorTypedTestsBase<CollectionType>
                {
                public:
                    FilterIteratorMapTests()
                    {
                        this->MakeComparePredicate(0);
                    }
                    ~FilterIteratorMapTests() override = default;

                protected:
                    using BaseIteratorReference = typename IteratorTypedTestsBase<CollectionType>::BaseIteratorReference;
                    template<typename T>
                    void MakeComparePredicate(T compareValue)
                    {
                        m_testPredicate = [compareValue](const BaseIteratorReference value) -> bool
                            {
                                return value.first >= compareValue;
                            };
                    }
                    AZStd::function<bool(const BaseIteratorReference)> m_testPredicate;
                };

                TYPED_TEST_SUITE_P(FilterIteratorMapTests);

                TYPED_TEST_P(FilterIteratorMapTests,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements)
                {
                    using BaseIterator = typename IteratorTypedTestsBase<TypeParam>::BaseIterator;

                    this->MakeComparePredicate(2);
                    AddElement(this->m_testCollection, 1);
                    AddElement(this->m_testCollection, 2);
                    AddElement(this->m_testCollection, 3);
                    AZStd::unordered_set<int> expectedElements = { 2, 3 };
                    View<FilterIterator<BaseIterator> > view = MakeFilterView(this->m_testCollection.begin(), this->m_testCollection.end(),
                            this->m_testPredicate);
                    for (auto it : view)
                    {
                        if (expectedElements.find(it.first) != expectedElements.end())
                        {
                            expectedElements.erase(it.first);
                        }
                    }
                    EXPECT_TRUE(expectedElements.empty());
                }

                REGISTER_TYPED_TEST_SUITE_P(FilterIteratorMapTests,
                    MakeFilterView_InputIsIterator_CorrectFilteredElements
                    );

                INSTANTIATE_TYPED_TEST_SUITE_P(CommonTests,
                    FilterIteratorMapTests,
                    MapCollectionTypes);

                // Added as separate test to avoid having to write another set of specialized templates.
                TEST(FilterIteratorTests, Algorithms_ReverseFilteredContainer_ValuesReversedExceptValuesLargerThan50)
                {
                    AZStd::vector<int> values = { 18, 7, 62, 63, 14 };

                    View<FilterIterator<AZStd::vector<int>::iterator> > view = MakeFilterView(values.begin(), values.end(),
                            [](int value) -> bool
                            {
                                return value < 50;
                            });

                    AZStd::reverse(view.begin(), view.end());

                    EXPECT_EQ(values[0], 14);
                    EXPECT_EQ(values[1], 7);
                    EXPECT_EQ(values[2], 62);
                    EXPECT_EQ(values[3], 63);
                    EXPECT_EQ(values[4], 18);
                }
            }
        }
    }
}

#undef _SCL_SECURE_NO_WARNINGS
