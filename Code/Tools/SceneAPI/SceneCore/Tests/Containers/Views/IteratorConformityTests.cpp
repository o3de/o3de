/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/*
* This collection of tests aim to test if the given Iterator conforms to iterator
* concepts. It will not test the functionality that's unique to the iterator, for
* this consider creating additional tests. If an iterator passes this collection
* of tests it's reasonable to assume it can be used with standard algorithms and
* utilities as any other iterator.
*/

#include <AzTest/AzTest.h>

#if defined(AZ_COMPILER_MSVC)

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/typetraits/is_pointer.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                inline static float Converter(int& value)
                {
                    return static_cast<float>(value);
                }

                class VectorIteratorContext
                    : public ::testing::Test
                {
                public:
                    using Iterator = AZStd::vector<int>::iterator;
                    virtual ~VectorIteratorContext() = default;

                    virtual void SetUp()
                    {
                        m_source.push_back(10);
                        m_source.push_back(20);
                        m_source.push_back(30);
                    }

                    Iterator Construct()
                    {
                        return m_source.begin();
                    }

                    AZStd::vector<int> m_source;
                };

                class ListIteratorContext
                    : public ::testing::Test
                {
                public:
                    using Iterator = AZStd::list<int>::iterator;
                    virtual ~ListIteratorContext() = default;

                    virtual void SetUp()
                    {
                        m_source.push_back(10);
                        m_source.push_back(20);
                        m_source.push_back(30);
                    }

                    Iterator Construct()
                    {
                        return m_source.begin();
                    }

                    AZStd::list<int> m_source;
                };

                class ForwardListIteratorContext
                    : public ::testing::Test
                {
                public:
                    using Iterator = AZStd::forward_list<int>::iterator;
                    virtual ~ForwardListIteratorContext() = default;

                    virtual void SetUp()
                    {
                        m_source.push_front(10);
                        m_source.push_front(20);
                        m_source.push_front(30);
                    }

                    Iterator Construct()
                    {
                        return m_source.begin();
                    }

                    AZStd::forward_list<int> m_source;
                };

                template<typename ContainerContext>
                class ConvertIteratorContext
                    : public ContainerContext
                {
                public:
                    using Iterator = ConvertIterator<typename ContainerContext::Iterator, float>;
                    ~ConvertIteratorContext() override = default;

                    Iterator Construct()
                    {
                        return MakeConvertIterator(ContainerContext::Construct(), Converter);
                    }
                };

                template<typename ContainerContext>
                class FilterIteratorContext
                    : public ContainerContext
                {
                public:
                    using Iterator = FilterIterator<typename ContainerContext::Iterator>;
                    ~FilterIteratorContext() override = default;

                    Iterator Construct()
                    {
                        return Iterator(this->m_source.begin(), this->m_source.end(),
                            [](int) -> bool
                            {
                                return true;
                            });
                    }
                };

                template<typename ContainerContext>
                class PairIteratorContext
                    : public ContainerContext
                {
                public:
                    using Iterator = PairIterator<typename ContainerContext::Iterator, typename ContainerContext::Iterator>;
                    ~PairIteratorContext() override = default;

                    Iterator Construct()
                    {
                        return Iterator(ContainerContext::Construct(), ContainerContext::Construct());
                    }
                };

                class SceneGraphBaseContext
                    : public ::testing::Test
                {
                public:
                    virtual ~SceneGraphBaseContext() = 0;

                    virtual void SetUp()
                    {
                        /*---------------------------------------\
                        |      Root                              |
                        |       |                                |
                        |       A                                |
                        |     / | \                              |
                        |    B  C  D                             |
                        |      / \                               |
                        |     E   F                              |
                        \---------------------------------------*/

                        //Build up the graph
                        SceneGraph::NodeIndex root = m_graph.GetRoot();
                        SceneGraph::NodeIndex indexA = m_graph.AddChild(root, "A");
                        SceneGraph::NodeIndex indexB = m_graph.AddChild(indexA, "B");
                        SceneGraph::NodeIndex indexC = m_graph.AddSibling(indexB, "C");
                        m_graph.AddSibling(indexC, "D");

                        SceneGraph::NodeIndex indexE = m_graph.AddChild(indexC, "E");
                        m_graph.AddSibling(indexE, "F");
                    }

                    SceneGraph m_graph;
                };

                SceneGraphBaseContext::~SceneGraphBaseContext()
                {
                }

                class SceneGraphUpwardsIteratorContext
                    : public SceneGraphBaseContext
                {
                public:
                    using Iterator = SceneGraphUpwardsIterator<SceneGraph::NameStorage::const_iterator>;
                    ~SceneGraphUpwardsIteratorContext() override = default;

                    Iterator Construct()
                    {
                        SceneGraph::NodeIndex index = m_graph.Find("A.C.E");
                        return Iterator(m_graph, m_graph.ConvertToHierarchyIterator(index), m_graph.GetNameStorage().begin(), true);
                    }
                };

                class SceneGraphChildIteratorContext
                    : public SceneGraphBaseContext
                {
                public:
                    using Iterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>;
                    ~SceneGraphChildIteratorContext() override = default;

                    Iterator Construct()
                    {
                        SceneGraph::NodeIndex index = m_graph.Find("A");
                        return Iterator(m_graph, m_graph.ConvertToHierarchyIterator(index), m_graph.GetNameStorage().begin(), true);
                    }
                };

                class SceneGraphDownwardsIteratorContext_DepthFirst
                    : public SceneGraphBaseContext
                {
                public:
                    using Iterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, DepthFirst>;
                    ~SceneGraphDownwardsIteratorContext_DepthFirst() override = default;

                    Iterator Construct()
                    {
                        SceneGraph::NodeIndex index = m_graph.Find("A");
                        return Iterator(m_graph, m_graph.ConvertToHierarchyIterator(index), m_graph.GetNameStorage().begin(), true);
                    }
                };

                class SceneGraphDownwardsIteratorContext_BreadthFirst
                    : public SceneGraphBaseContext
                {
                public:
                    using Iterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, BreadthFirst>;
                    ~SceneGraphDownwardsIteratorContext_BreadthFirst() override = default;

                    Iterator Construct()
                    {
                        SceneGraph::NodeIndex index = m_graph.Find("A");
                        return Iterator(m_graph, m_graph.ConvertToHierarchyIterator(index), m_graph.GetNameStorage().begin(), true);
                    }
                };


                /***********************************************
                *    All Iterator category
                ************************************************/
                namespace AllIterators
                {
                    template<typename T>
                    class AllContext
                        : public T
                    {
                    public:
                        using Iterator = typename T::Iterator;
                        ~AllContext() override = default;
                    };

                    TYPED_TEST_CASE_P(AllContext);

                    // Constructor
                    TYPED_TEST_P(AllContext, Constructor_CanBeConstructedAndDestructed_DoesNotCrash)
                    {
                        this->Construct();
                    }

                    // Copy constructor
                    TYPED_TEST_P(AllContext, CopyConstructor_CanBeCopyConstructedExplicit_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        Iterator other(iterator);
                        (void)other;
                    }

                    TYPED_TEST_P(AllContext, CopyConstructor_CanBeCopyConstructedImplicit_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        Iterator other = iterator;
                        (void)other;
                    }

                    // Assignment operator
                    TYPED_TEST_P(AllContext, AssignmentOperator_CanBeAssignedToOtherIterator_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();
                        lhsIterator = rhsIterator;
                    }

                    TYPED_TEST_P(AllContext, AssignmentOperator_CanBeChainAssigned_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iteratorFirst = this->Construct();
                        Iterator iteratorSecond = this->Construct();
                        Iterator iteratorThird = this->Construct();
                        iteratorFirst = iteratorSecond = iteratorThird = this->Construct();
                    }

                    // Post Increment Operator
                    TYPED_TEST_P(AllContext, PostIncrementOperator_IterateOneStep_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        iterator++;
                    }

                    TYPED_TEST_P(AllContext, PostIncrementOperator_ReturnsIterator_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        iterator++;
                    }

                    // Pre Increment Operator
                    TYPED_TEST_P(AllContext, PreIncrementOperator_IterateOneStep_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        ++iterator;
                    }

                    TYPED_TEST_P(AllContext, PreIncrementOperator_ReturnsIterator_DoesNotCrash)
                    {
                        using Iterator = typename AllContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        ++iterator;
                    }

                    REGISTER_TYPED_TEST_CASE_P(AllContext,
                        Constructor_CanBeConstructedAndDestructed_DoesNotCrash,
                        CopyConstructor_CanBeCopyConstructedExplicit_DoesNotCrash,
                        CopyConstructor_CanBeCopyConstructedImplicit_DoesNotCrash,
                        AssignmentOperator_CanBeAssignedToOtherIterator_DoesNotCrash,
                        AssignmentOperator_CanBeChainAssigned_DoesNotCrash,
                        PostIncrementOperator_IterateOneStep_DoesNotCrash,
                        PostIncrementOperator_ReturnsIterator_DoesNotCrash,
                        PreIncrementOperator_IterateOneStep_DoesNotCrash,
                        PreIncrementOperator_ReturnsIterator_DoesNotCrash
                        );
                }

                /***********************************************
                *    Input Iterator category
                ************************************************/
                namespace InputIterators
                {
                    template<typename T>
                    class InputContext
                        : public T
                    {
                    public:
                        using Iterator = typename T::Iterator;
                        ~InputContext() override = default;
                    };

                    TYPED_TEST_CASE_P(InputContext);

                    // Equal operator
                    TYPED_TEST_P(InputContext, EqualsOperator_IteratorComparedWithSelf_SameIteratorObject)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        EXPECT_TRUE(iterator == iterator);
                    }

                    TYPED_TEST_P(InputContext, EqualsOperator_IdenticallyConstructedIterators_IteratorsAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();
                        EXPECT_TRUE(lhsIterator == rhsIterator);
                    }

                    TYPED_TEST_P(InputContext, EqualsOperator_DifferentIterators_IteratorsAreNotEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator intermediate = this->Construct();
                        Iterator rhsIterator = ++intermediate;
                        EXPECT_FALSE(lhsIterator == rhsIterator);
                    }

                    // Not equal operator
                    TYPED_TEST_P(InputContext, NotEqualsOperator_IteratorComparedWithSelf_InstanceNotNotEqualToItself)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        EXPECT_FALSE(iterator != iterator);
                    }

                    TYPED_TEST_P(InputContext, NotEqualsOperator_IdenticallyConstructedIterators_IteratorsAreNotNotEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();
                        EXPECT_FALSE(lhsIterator != rhsIterator);
                    }

                    TYPED_TEST_P(InputContext, NotEqualsOperator_DifferentIterators_IteratorsAreNotEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();
                        ++rhsIterator;
                        EXPECT_TRUE(lhsIterator != rhsIterator);
                    }

                    // Dereference operator
                    TYPED_TEST_P(InputContext, DereferenceOperator_IteratorCanBeDereferenced_DoesNotCrash)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        *iterator;
                    }

                    TYPED_TEST_P(InputContext, DereferenceOperator_TwoNewlyCreatedIteratorsReferenceTheSameValue_ValuesAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();
                        EXPECT_EQ(*lhsIterator, *rhsIterator);
                    }

                    TYPED_TEST_P(InputContext, DereferenceOperator_ExplicitlyCopiedIteratorsHaveTheSameValue_ValuesAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator(lhsIterator);
                        EXPECT_EQ(*lhsIterator, *rhsIterator);
                    }

                    TYPED_TEST_P(InputContext, DereferenceOperator_ImplicitlyCopiedIteratorsHaveTheSameValue_ValuesAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = lhsIterator;
                        EXPECT_EQ(*lhsIterator, *rhsIterator);
                    }

                    template<typename T>
                    void TestArrowOperator(T iterator, AZStd::true_type)
                    {
                        EXPECT_EQ(*iterator, *iterator);
                    }

                    template<typename T>
                    void TestArrowOperator(T iterator, AZStd::false_type)
                    {
                        EXPECT_EQ(*iterator, *iterator.operator->());
                    }

                    // Arrow operator
                    TYPED_TEST_P(InputContext, ArrowOperator_DereferencesToSameValueAsDereferenceOperator_ValuesAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        TestArrowOperator(this->Construct(), typename AZStd::is_pointer<Iterator>::type());
                    }

                    // Post increment operator - additional
                    TYPED_TEST_P(InputContext, PostIncrementOperator_IncrementedIteratorIsNotTheSameAsTheOriginal_IteratorsDiffers)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;
                        original++;
                        EXPECT_NE(copy, original);
                    }

                    TYPED_TEST_P(InputContext, PostIncrementOperator_OperatorReturnsOriginalIterator_ReturnedIteratorEqualsOriginalAndNotIncremented)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;
                        Iterator returned = original++;

                        EXPECT_EQ(copy, returned);
                        EXPECT_NE(original, returned);
                    }

                    // Pre increment operator - additional
                    TYPED_TEST_P(InputContext, PreIncrementOperator_IncrementedIteratorIsNotTheSameAsTheOriginal_IteratorsDiffers)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;
                        ++original;
                        EXPECT_NE(copy, original);
                    }

                    TYPED_TEST_P(InputContext, PreIncrementOperator_OperatorReturnsIncrementedIterator_ReturnedIteratorEqualsIncrementedAndNotOriginal)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;
                        Iterator returned = ++original;

                        EXPECT_NE(copy, returned);
                        EXPECT_EQ(original, returned);
                    }

                    // Copy constructor - additional
                    TYPED_TEST_P(InputContext, CopyConstructor_CanBeCopyConstructedExplicit_IteratorsAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator(lhsIterator);
                        EXPECT_EQ(lhsIterator, rhsIterator);
                    }

                    TYPED_TEST_P(InputContext, CopyConstructor_CanBeCopyConstructedImplicit_IteratorsAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = lhsIterator;
                        EXPECT_EQ(lhsIterator, rhsIterator);
                    }

                    // Assignment operator - additional
                    TYPED_TEST_P(InputContext, AssignmentOperator_CanBeAssignedToOtherIterator_IteratorsAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();
                        lhsIterator = rhsIterator;
                        EXPECT_EQ(lhsIterator, rhsIterator);
                    }

                    TYPED_TEST_P(InputContext, AssignmentOperator_CanBeChainAssigned_IteratorsAreEqual)
                    {
                        using Iterator = typename InputContext<TypeParam>::Iterator;

                        Iterator iteratorFirst = this->Construct();
                        Iterator iteratorSecond = this->Construct();
                        Iterator iteratorThird = this->Construct();

                        iteratorFirst = iteratorSecond = iteratorThird = this->Construct();
                        Iterator reference = this->Construct();

                        EXPECT_EQ(iteratorFirst, reference);
                        EXPECT_EQ(iteratorSecond, reference);
                        EXPECT_EQ(iteratorThird, reference);

                        EXPECT_EQ(iteratorFirst, iteratorSecond);
                        EXPECT_EQ(iteratorSecond, iteratorThird);
                        EXPECT_EQ(iteratorFirst, iteratorThird);
                    }

                    REGISTER_TYPED_TEST_CASE_P(InputContext,
                        EqualsOperator_IteratorComparedWithSelf_SameIteratorObject,
                        EqualsOperator_IdenticallyConstructedIterators_IteratorsAreEqual,
                        EqualsOperator_DifferentIterators_IteratorsAreNotEqual,
                        NotEqualsOperator_IteratorComparedWithSelf_InstanceNotNotEqualToItself,
                        NotEqualsOperator_IdenticallyConstructedIterators_IteratorsAreNotNotEqual,
                        NotEqualsOperator_DifferentIterators_IteratorsAreNotEqual,
                        DereferenceOperator_IteratorCanBeDereferenced_DoesNotCrash,
                        DereferenceOperator_TwoNewlyCreatedIteratorsReferenceTheSameValue_ValuesAreEqual,
                        DereferenceOperator_ExplicitlyCopiedIteratorsHaveTheSameValue_ValuesAreEqual,
                        DereferenceOperator_ImplicitlyCopiedIteratorsHaveTheSameValue_ValuesAreEqual,
                        ArrowOperator_DereferencesToSameValueAsDereferenceOperator_ValuesAreEqual,
                        PostIncrementOperator_IncrementedIteratorIsNotTheSameAsTheOriginal_IteratorsDiffers,
                        PostIncrementOperator_OperatorReturnsOriginalIterator_ReturnedIteratorEqualsOriginalAndNotIncremented,
                        PreIncrementOperator_IncrementedIteratorIsNotTheSameAsTheOriginal_IteratorsDiffers,
                        PreIncrementOperator_OperatorReturnsIncrementedIterator_ReturnedIteratorEqualsIncrementedAndNotOriginal,
                        CopyConstructor_CanBeCopyConstructedExplicit_IteratorsAreEqual,
                        CopyConstructor_CanBeCopyConstructedImplicit_IteratorsAreEqual,
                        AssignmentOperator_CanBeAssignedToOtherIterator_IteratorsAreEqual,
                        AssignmentOperator_CanBeChainAssigned_IteratorsAreEqual
                        );
                }

                /***********************************************
                *    Forward Iterator category
                ************************************************/
                namespace ForwardIterators
                {
                    template<typename T>
                    class ForwardContext
                        : public T
                    {
                    public:
                        using Iterator = typename T::Iterator;
                        ~ForwardContext() override = default;
                    };

                    TYPED_TEST_CASE_P(ForwardContext);

                    // Default constructor
                    TYPED_TEST_P(ForwardContext, DefaultConstructor_CanExplicityConstructed_DoesNotCrash)
                    {
                        using Iterator = typename ForwardContext<TypeParam>::Iterator;

                        Iterator iterator = Iterator();
                        (void)iterator;
                    }

                    TYPED_TEST_P(ForwardContext, DefaultConstructor_CanImplicityConstructed_DoesNotCrash)
                    {
                        using Iterator = typename ForwardContext<TypeParam>::Iterator;

                        Iterator iterator;
                        (void)iterator;
                    }

                    // Multi pass
                    TYPED_TEST_P(ForwardContext, MultiPass_DereferencingMultipleTimes_ValueBeforeAndAfterIncrementingIsTheSame)
                    {
                        using Iterator = typename ForwardContext<TypeParam>::Iterator;

                        Iterator a = this->Construct();
                        Iterator b = a;
                        auto aValue = *a++;
                        auto bValue = *b;
                        EXPECT_EQ(aValue, bValue);
                    }

                    REGISTER_TYPED_TEST_CASE_P(ForwardContext,
                        DefaultConstructor_CanExplicityConstructed_DoesNotCrash,
                        DefaultConstructor_CanImplicityConstructed_DoesNotCrash,
                        MultiPass_DereferencingMultipleTimes_ValueBeforeAndAfterIncrementingIsTheSame
                        );
                }

                /***********************************************
                *    Bidirectional Iterator category
                ************************************************/
                namespace BidirectionalIterators
                {
                    template<typename T>
                    class BidirectionalContext
                        : public T
                    {
                    public:
                        using Iterator = typename T::Iterator;
                        ~BidirectionalContext() override = default;
                    };

                    TYPED_TEST_CASE_P(BidirectionalContext);

                    // Post Decrement Operator
                    TYPED_TEST_P(BidirectionalContext, PostDecrementOperator_IterateOneStep_DoesNotCrash)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        iterator++;
                        iterator--;
                    }

                    TYPED_TEST_P(BidirectionalContext, PostDecrementOperator_ReturnsIterator_DoesNotCrash)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        iterator++;
                        iterator--;
                    }

                    TYPED_TEST_P(BidirectionalContext, PostDecrementOperator_IteratorReturnsOriginalIterator_OriginalIteratorMatchesCopiedValueAndNotMoveIterator)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        original++;
                        Iterator copy = original;
                        Iterator returned = original--;

                        EXPECT_EQ(copy, returned);
                        EXPECT_NE(original, returned);
                    }

                    TYPED_TEST_P(BidirectionalContext, PostDecrementOperator_IncrementedIteratorReturnsToSamePoint_PreMovedIteratorIsSameAsPostMoved)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;
                        original++;
                        original--;
                        EXPECT_EQ(copy, original);
                        EXPECT_EQ(*copy, *original);
                    }

                    // Pre Decrement Operator
                    TYPED_TEST_P(BidirectionalContext, PreDecrementOperator_IterateOneStep_DoesNotCrash)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        ++iterator;
                        --iterator;
                    }

                    TYPED_TEST_P(BidirectionalContext, PreDecrementOperator_ReturnsIterator_DoesNotCrash)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        ++iterator;
                    }

                    TYPED_TEST_P(BidirectionalContext, PreDecrementOperator_IteratorReturnsMovedIterator_OriginalIteratorDoesNotMatchCopiedValueButMovedIteratorDoes)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = ++original;
                        Iterator returned = --original;

                        EXPECT_NE(copy, returned);
                        EXPECT_EQ(original, returned);
                    }

                    TYPED_TEST_P(BidirectionalContext, PreDecrementOperator_IncrementedIteratorReturnsToSamePoint_PreMovedIteratorIsSameAsPostMoved)
                    {
                        using Iterator = typename BidirectionalContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;
                        ++original;
                        --original;
                        EXPECT_EQ(copy, original);
                        EXPECT_EQ(*copy, *original);
                    }

                    REGISTER_TYPED_TEST_CASE_P(BidirectionalContext,
                        PostDecrementOperator_IterateOneStep_DoesNotCrash,
                        PostDecrementOperator_ReturnsIterator_DoesNotCrash,
                        PostDecrementOperator_IteratorReturnsOriginalIterator_OriginalIteratorMatchesCopiedValueAndNotMoveIterator,
                        PostDecrementOperator_IncrementedIteratorReturnsToSamePoint_PreMovedIteratorIsSameAsPostMoved,
                        PreDecrementOperator_IterateOneStep_DoesNotCrash,
                        PreDecrementOperator_ReturnsIterator_DoesNotCrash,
                        PreDecrementOperator_IteratorReturnsMovedIterator_OriginalIteratorDoesNotMatchCopiedValueButMovedIteratorDoes,
                        PreDecrementOperator_IncrementedIteratorReturnsToSamePoint_PreMovedIteratorIsSameAsPostMoved
                        );
                }

                /***********************************************
                *    Random Access Iterator category
                ************************************************/
                namespace RandomAccessIterators
                {
                    template<typename T>
                    class RandomAccessContext
                        : public T
                    {
                    public:
                        using Iterator = typename T::Iterator;
                        ~RandomAccessContext() override = default;
                    };

                    TYPED_TEST_CASE_P(RandomAccessContext);

                    // Difference subtract operator
                    TYPED_TEST_P(RandomAccessContext, DifferenceSubtractOperator_DifferenceWithItself_DifferenceIsZero)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator iterator = this->Construct();
                        size_t difference = iterator - iterator;

                        EXPECT_EQ(0, difference);
                    }

                    TYPED_TEST_P(RandomAccessContext, DifferenceSubtractOperator_DifferenceBetweenIteratorsAtSamePosition_DifferenceIsZero)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator rhsIterator = this->Construct();
                        Iterator lhsIterator = this->Construct();
                        size_t difference = lhsIterator - rhsIterator;

                        EXPECT_EQ(0, difference);
                    }

                    TYPED_TEST_P(RandomAccessContext, DifferenceSubtractOperator_DifferenceBetweenIteratorsAtDifferentPositions_DifferenceIsOne)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator rhsIterator = this->Construct();
                        Iterator intermediate = this->Construct();
                        Iterator lhsIterator = ++intermediate;
                        size_t difference = lhsIterator - rhsIterator;

                        EXPECT_EQ(1, difference);
                    }

                    // Arithmetic add operator
                    TYPED_TEST_P(RandomAccessContext, ArithmeticAddOperator_IteratorMovesTwoPlacesUp_ExplicitlyIncrementedIteratorAtSameLocation)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 2;

                        original++;
                        original++;

                        EXPECT_EQ(original, moved);
                    }

                    // Arithmetic subtract operator
                    TYPED_TEST_P(RandomAccessContext, ArithmeticSubtractOperator_IteratorMovesTwoPlacesDown_CopyIteratorAtSameLocation)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;

                        original++;
                        original++;
                        Iterator moved = original - 2;

                        EXPECT_EQ(copy, moved);
                    }

                    TYPED_TEST_P(RandomAccessContext, ArithmeticSubtractOperator_DifferenceBetweenTwoIterators_DifferenceIsTwo)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 2;

                        EXPECT_EQ(2, moved - original);
                    }

                    // Arithmetic add-equal operator
                    TYPED_TEST_P(RandomAccessContext, ArithmeticAddEqualOperator_IteratorMovesTwoPlacesUp_ExplicitlyIncrementedIteratorAtSameLocation)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original;

                        original++;
                        original++;
                        moved += 2;

                        EXPECT_EQ(original, moved);
                    }

                    // Arithmetic subtract-equal operator
                    TYPED_TEST_P(RandomAccessContext, ArithmeticSubtractEqualOperator_IteratorMovesTwoPlacesDown_CopyIteratorAtSameLocation)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;

                        original++;
                        original++;
                        original -= 2;

                        EXPECT_EQ(copy, original);
                    }

                    // Smaller than operator
                    TYPED_TEST_P(RandomAccessContext, SmallerThanOperator_OriginalIteratorIsSmallerThanIncrementedIterator_OrignalIsSmallerInBothDirections)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 1;

                        EXPECT_TRUE(original < moved);
                        EXPECT_FALSE(moved < original);
                    }

                    // Larger than operator
                    TYPED_TEST_P(RandomAccessContext, LargerThanOperator_MovedIteratorIsLargerThanOriginalIterator_MovedIsLargerInBothDirections)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 1;

                        EXPECT_TRUE(moved > original);
                        EXPECT_FALSE(original > moved);
                    }

                    // Smaller than equal operator
                    TYPED_TEST_P(RandomAccessContext, SmallerThanEqualOperator_OriginalIteratorIsSmallerThanIncrementedIterator_OrignalIsSmallerInBothDirections)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 1;

                        EXPECT_TRUE(original <= moved);
                        EXPECT_FALSE(moved <= original);
                    }

                    TYPED_TEST_P(RandomAccessContext, SmallerThanEqualOperator_SameIteratorsCompareAsEqual_OriginalAndCopyCompareAsTrue)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;

                        EXPECT_TRUE(original <= copy);
                    }

                    TYPED_TEST_P(RandomAccessContext, SmallerThanEqualOperator_NewIteratorsCompareAsEqual_FirstAndSecondCreatedIteratorsCompareEqual)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();

                        EXPECT_TRUE(lhsIterator <= rhsIterator);
                    }

                    // Larger than equal operator
                    TYPED_TEST_P(RandomAccessContext, LargerThanEqualOperator_MovedIteratorIsLargerThanOriginalIterator_MovedIsLargerInBothDirections)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 1;

                        EXPECT_TRUE(moved >= original);
                        EXPECT_FALSE(original >= moved);
                    }

                    TYPED_TEST_P(RandomAccessContext, LargerThanEqualOperator_SameIteratorsCompareAsEqual_OriginalAndCopyCompareAsTrue)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator copy = original;

                        EXPECT_TRUE(original >= copy);
                    }

                    TYPED_TEST_P(RandomAccessContext, LargerThanEqualOperator_NewIteratorsCompareAsEqual_FirstAndSecondCreatedIteratorsCompareEqual)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator lhsIterator = this->Construct();
                        Iterator rhsIterator = this->Construct();

                        EXPECT_TRUE(lhsIterator >= rhsIterator);
                    }

                    // Index operator
                    TYPED_TEST_P(RandomAccessContext, IndexOperator_IndexValueMatchesDereferencedValue_StoredValuesAreTheSame)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        EXPECT_EQ(*original, original[0]);
                    }

                    TYPED_TEST_P(RandomAccessContext, IndexOperator_IndexValueMatchesDereferencedValueAtOffset_StoredValuesAreTheSame)
                    {
                        using Iterator = typename RandomAccessContext<TypeParam>::Iterator;

                        Iterator original = this->Construct();
                        Iterator moved = original + 1;

                        EXPECT_EQ(*moved, original[1]);
                    }

                    REGISTER_TYPED_TEST_CASE_P(RandomAccessContext,
                        DifferenceSubtractOperator_DifferenceWithItself_DifferenceIsZero,
                        DifferenceSubtractOperator_DifferenceBetweenIteratorsAtSamePosition_DifferenceIsZero,
                        DifferenceSubtractOperator_DifferenceBetweenIteratorsAtDifferentPositions_DifferenceIsOne,
                        ArithmeticAddOperator_IteratorMovesTwoPlacesUp_ExplicitlyIncrementedIteratorAtSameLocation,
                        ArithmeticSubtractOperator_IteratorMovesTwoPlacesDown_CopyIteratorAtSameLocation,
                        ArithmeticSubtractOperator_DifferenceBetweenTwoIterators_DifferenceIsTwo,
                        ArithmeticAddEqualOperator_IteratorMovesTwoPlacesUp_ExplicitlyIncrementedIteratorAtSameLocation,
                        ArithmeticSubtractEqualOperator_IteratorMovesTwoPlacesDown_CopyIteratorAtSameLocation,
                        SmallerThanOperator_OriginalIteratorIsSmallerThanIncrementedIterator_OrignalIsSmallerInBothDirections,
                        LargerThanOperator_MovedIteratorIsLargerThanOriginalIterator_MovedIsLargerInBothDirections,
                        SmallerThanEqualOperator_OriginalIteratorIsSmallerThanIncrementedIterator_OrignalIsSmallerInBothDirections,
                        SmallerThanEqualOperator_SameIteratorsCompareAsEqual_OriginalAndCopyCompareAsTrue,
                        SmallerThanEqualOperator_NewIteratorsCompareAsEqual_FirstAndSecondCreatedIteratorsCompareEqual,
                        LargerThanEqualOperator_MovedIteratorIsLargerThanOriginalIterator_MovedIsLargerInBothDirections,
                        LargerThanEqualOperator_SameIteratorsCompareAsEqual_OriginalAndCopyCompareAsTrue,
                        LargerThanEqualOperator_NewIteratorsCompareAsEqual_FirstAndSecondCreatedIteratorsCompareEqual,
                        IndexOperator_IndexValueMatchesDereferencedValue_StoredValuesAreTheSame,
                        IndexOperator_IndexValueMatchesDereferencedValueAtOffset_StoredValuesAreTheSame
                        );
                }

                /***********************************************
                *    Test Execution
                ************************************************/
                namespace AllIterators
                {
                    using BaseGroup = ::testing::Types<VectorIteratorContext, ListIteratorContext, ForwardListIteratorContext>;
                    using BaseExtGroup = ::testing::Types<
                        ConvertIteratorContext<VectorIteratorContext>, ConvertIteratorContext<ListIteratorContext>, ConvertIteratorContext<ForwardListIteratorContext>,
                        FilterIteratorContext<VectorIteratorContext>, FilterIteratorContext<ListIteratorContext>, FilterIteratorContext<ForwardListIteratorContext>,
                        PairIteratorContext<VectorIteratorContext>, PairIteratorContext<ListIteratorContext>, PairIteratorContext<ForwardListIteratorContext>> ;
                    using GraphGroup = ::testing::Types<
                        SceneGraphUpwardsIteratorContext, SceneGraphChildIteratorContext,
                        SceneGraphDownwardsIteratorContext_DepthFirst, SceneGraphDownwardsIteratorContext_BreadthFirst>;
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Base, AllContext, BaseGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_BaseExt, AllContext, BaseExtGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Graph, AllContext, GraphGroup);
                }
                namespace InputIterators
                {
                    using BaseGroup = ::testing::Types<VectorIteratorContext, ListIteratorContext, ForwardListIteratorContext>;
                    using BaseExtGroup = ::testing::Types<
                        ConvertIteratorContext<VectorIteratorContext>, ConvertIteratorContext<ListIteratorContext>, ConvertIteratorContext<ForwardListIteratorContext>,
                        FilterIteratorContext<VectorIteratorContext>, FilterIteratorContext<ListIteratorContext>, FilterIteratorContext<ForwardListIteratorContext>,
                        PairIteratorContext<VectorIteratorContext>, PairIteratorContext<ListIteratorContext>, PairIteratorContext<ForwardListIteratorContext>> ;
                    using GraphGroup = ::testing::Types<
                        SceneGraphUpwardsIteratorContext, SceneGraphChildIteratorContext,
                        SceneGraphDownwardsIteratorContext_DepthFirst, SceneGraphDownwardsIteratorContext_BreadthFirst>;
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Base, InputContext, BaseGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_BaseExt, InputContext, BaseExtGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Graph, InputContext, GraphGroup);
                }
                namespace ForwardIterators
                {
                    using BaseGroup = ::testing::Types<VectorIteratorContext, ListIteratorContext, ForwardListIteratorContext>;
                    using BaseExtGroup = ::testing::Types <
                        ConvertIteratorContext<VectorIteratorContext>, ConvertIteratorContext<ListIteratorContext>, ConvertIteratorContext<ForwardListIteratorContext>,
                        FilterIteratorContext<VectorIteratorContext>, FilterIteratorContext<ListIteratorContext>, FilterIteratorContext<ForwardListIteratorContext>,
                        PairIteratorContext<VectorIteratorContext>, PairIteratorContext<ListIteratorContext>, PairIteratorContext < ForwardListIteratorContext >> ;
                    using GraphGroup = ::testing::Types<
                        SceneGraphUpwardsIteratorContext, SceneGraphChildIteratorContext,
                        SceneGraphDownwardsIteratorContext_DepthFirst, SceneGraphDownwardsIteratorContext_BreadthFirst>;
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Base, ForwardContext, BaseGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_BaseExt, ForwardContext, BaseExtGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Graph, ForwardContext, GraphGroup);
                }
                namespace BidirectionalIterators
                {
                    using BaseGroup = ::testing::Types<VectorIteratorContext, ListIteratorContext>;
                    using BaseExtGroup = ::testing::Types<
                            ConvertIteratorContext<VectorIteratorContext>, ConvertIteratorContext<ListIteratorContext>,
                            FilterIteratorContext<VectorIteratorContext>, FilterIteratorContext<ListIteratorContext>,
                            PairIteratorContext<VectorIteratorContext>, PairIteratorContext<ListIteratorContext>>;
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Base, BidirectionalContext, BaseGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_BaseExt, BidirectionalContext, BaseExtGroup);
                }
                namespace RandomAccessIterators
                {
                    using BaseGroup = ::testing::Types<VectorIteratorContext>;
                    using BaseExtGroup = ::testing::Types<ConvertIteratorContext<VectorIteratorContext>, PairIteratorContext<VectorIteratorContext>>;
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_Base, RandomAccessContext, BaseGroup);
                    INSTANTIATE_TYPED_TEST_CASE_P(IteratorConformityTests_BaseExt, RandomAccessContext, BaseExtGroup);
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ

#endif // AZ_COMPILER_MSVC
