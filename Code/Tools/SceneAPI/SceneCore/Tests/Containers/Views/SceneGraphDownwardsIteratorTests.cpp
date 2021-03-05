/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

/*
* This suite of tests focus on the unique features the SceneGraphDownwardsIterator
* adds as an iterator. The basic functionality and iterator conformity is tested
* in the Iterator Conformity Tests (see IteratorConformityTests.cpp).
*/

#include <AzTest/AzTest.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                class SceneGraphDownwardsIteratorTest
                    : public ::testing::Test
                {
                public:
                    virtual ~SceneGraphDownwardsIteratorTest() = default;

                    void SetUp()
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
                        m_graph.SetContent(root, AZStd::make_shared<DataTypes::MockIGraphObject>(0));

                        SceneGraph::NodeIndex indexA = m_graph.AddChild(root, "A", AZStd::make_shared<DataTypes::MockIGraphObject>(1));
                        SceneGraph::NodeIndex indexB = m_graph.AddChild(indexA, "B", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
                        SceneGraph::NodeIndex indexC = m_graph.AddSibling(indexB, "C", AZStd::make_shared<DataTypes::MockIGraphObject>(3));
                        m_graph.AddSibling(indexC, "D", AZStd::make_shared<DataTypes::MockIGraphObject>(4));

                        SceneGraph::NodeIndex indexE = m_graph.AddChild(indexC, "E", AZStd::make_shared<DataTypes::MockIGraphObject>(5));
                        m_graph.AddSibling(indexE, "F", AZStd::make_shared<DataTypes::MockIGraphObject>(6));
                    }

                    SceneGraph::HierarchyStorageConstData::iterator GetRootHierarchyIterator()
                    {
                        return m_graph.ConvertToHierarchyIterator(m_graph.GetRoot());
                    }

                    SceneGraph::HierarchyStorageConstData::iterator GetDeepestHierarchyIterator()
                    {
                        SceneGraph::NodeIndex index = m_graph.Find("A.C.F");
                        return m_graph.ConvertToHierarchyIterator(index);
                    }

                    SceneGraph m_graph;
                };

                template<typename T>
                class SceneGraphDownwardsIteratorContext
                    : public SceneGraphDownwardsIteratorTest
                {
                public:
                    using Traversal = T;
                };

                TYPED_TEST_CASE_P(SceneGraphDownwardsIteratorContext);

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, MakeSceneGraphDownwardsIterator_FunctionComparedWithExplicitlyDeclaredIterator_IteratorsAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto lhsIterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    auto rhsIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, MakeSceneGraphDownwardsIterator_ExtendedFunctionComparedWithExplicitlyDeclaredIterator_IteratorsAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto lhsIterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    auto rhsIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, MakeSceneGraphDownwardsIterator_NodeAndHierarchyVersions_IteratorsAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    SceneGraph::NodeIndex index = this->m_graph.Find("A.C");
                    auto hierarchy = this->m_graph.ConvertToHierarchyIterator(index);

                    auto indexIterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, index, this->m_graph.GetNameStorage().begin(), true);
                    auto hierarchyIterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, hierarchy, this->m_graph.GetNameStorage().begin(), true);
                    EXPECT_EQ(indexIterator, hierarchyIterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, MakeSceneGraphDownwardsView_FunctionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto view = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    auto beginIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    auto endIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>();

                    EXPECT_EQ(view.begin(), beginIterator);
                    EXPECT_EQ(view.end(), endIterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, MakeSceneGraphDownwardsView_ExtendedFunctionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto view = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    auto beginIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    auto endIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>();

                    EXPECT_EQ(view.begin(), beginIterator);
                    EXPECT_EQ(view.end(), endIterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, MakeSceneGraphDownwardsView_NodeAndHierarchyVersions_IteratorsInViewsAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    SceneGraph::NodeIndex index = this->m_graph.Find("A.C");
                    auto hierarchy = this->m_graph.ConvertToHierarchyIterator(index);

                    auto indexView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, index, this->m_graph.GetNameStorage().begin(), true);
                    auto hierarchyView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, hierarchy, this->m_graph.GetNameStorage().begin(), true);

                    EXPECT_EQ(indexView.begin(), hierarchyView.begin());
                    EXPECT_EQ(indexView.end(), hierarchyView.end());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Dereference_GetRootIteratorValue_ReturnsRelativeValueFromGivenValueIterator)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    EXPECT_STREQ("", (*iterator).GetPath());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Dereference_GetDeepestIteratorValue_ReturnsRelativeValueFromGivenValueIterator)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetDeepestHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    EXPECT_STREQ("A.C.F", (*iterator).GetPath());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Dereference_ValueIteratorNotSyncedWithHierarchyIteratorIfNotRequested_ReturnedValueMatchesOriginalValueIterator)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto valueIterator = this->m_graph.GetNameStorage().begin() + 2;
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetDeepestHierarchyIterator(), valueIterator, false);
                    EXPECT_STREQ((*valueIterator).GetPath(), (*iterator).GetPath());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Dereference_DereferencingThroughStarAndArrowOperator_ValuesAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto valueIterator = this->m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetDeepestHierarchyIterator(), valueIterator, true);
                    EXPECT_STREQ(iterator->GetPath(), (*iterator).GetPath());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, IncrementOperator_MovePastEnd_ReturnsEndIterator)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto valueIterator = this->m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetDeepestHierarchyIterator(), valueIterator, true);
                    iterator++;
                    auto endIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>();
                    EXPECT_EQ(endIterator, iterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, GetHierarchyIterator_MatchesWithNodeInformationAfterMove_NameEqualToNodeIndexedName)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto valueIterator = this->m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetRootHierarchyIterator(), valueIterator, true);
                    iterator++;

                    SceneGraph::HierarchyStorageConstIterator hierarchyIterator = iterator.GetHierarchyIterator();
                    SceneGraph::NodeIndex index = this->m_graph.ConvertToNodeIndex(hierarchyIterator);

                    EXPECT_STREQ(this->m_graph.GetNodeName(index).GetPath(), iterator->GetPath());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, GetHierarchyIterator_MovePastEnd_GetEmptyDefaultNode)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto valueIterator = this->m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphDownwardsIterator<Traversal>(this->m_graph, this->GetDeepestHierarchyIterator(), valueIterator, true);
                    iterator++;

                    SceneGraph::HierarchyStorageConstIterator hierarchyIterator = iterator.GetHierarchyIterator();
                    EXPECT_EQ(hierarchyIterator, SceneGraph::HierarchyStorageConstIterator());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, EmptyGraph_CanDetectEmptyGraph_BeginPlusOneAndEndIteratorAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    SceneGraph emptyGraph;

                    auto beginIterator = MakeSceneGraphDownwardsIterator<Traversal>(emptyGraph, emptyGraph.GetHierarchyStorage().begin(), emptyGraph.GetNameStorage().begin(), true);
                    beginIterator++; // Even an empty graph will have at least a root entry.
                    auto endIterator = SceneGraphDownwardsIterator<SceneGraph::NameStorage::const_iterator, Traversal>();
                    EXPECT_EQ(beginIterator, endIterator);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, EmptyGraph_CanDetectEmptyGraphFromView_BeginPlusOneAndEndIteratorAreEqual)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    SceneGraph emptyGraph;

                    auto view = MakeSceneGraphDownwardsView<Traversal>(emptyGraph, emptyGraph.GetHierarchyStorage().begin(), emptyGraph.GetNameStorage().begin(), true);
                    EXPECT_EQ(++view.begin(), view.end());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Algorithm_RangeForLoop_CanSuccesfullyRun)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto sceneView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    for (auto& it : sceneView)
                    {
                    }
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, IncrementOperator_TouchesAllNodes_NumberOfIterationStepsMatchesNumberOfNodesInGraph)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    size_t entryCount = this->m_graph.GetHierarchyStorage().end() - this->m_graph.GetHierarchyStorage().begin();
                    size_t localCount = 0;

                    auto sceneView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->GetRootHierarchyIterator(), this->m_graph.GetNameStorage().begin(), true);
                    for (auto& it : sceneView)
                    {
                        localCount++;
                    }
                    EXPECT_EQ(entryCount, localCount);
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, ValueIterator_NonSceneGraphIterator_ExternalIteratorValuesMatchSceneGraphValues)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    // Commonly containers in the scene graph will be used, but it is possible to specify other containers that shadow
                    //      the scene graph but don't belong to it. This test checks if this works correctly by comparing the values
                    //      stored in the scene graph with the same values stored in an external container.
                    //      (See constructor of SceneGraphUpwardsIterator for more details on arguments.)
                    AZStd::vector<int> values = { 0, 1, 2, 3, 4, 5, 6 };

                    auto sceneView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->m_graph.GetContentStorage().begin());
                    auto valuesView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, values.begin());

                    auto sceneIterator = sceneView.begin();
                    auto valuesIterator = valuesView.begin();

                    while (sceneIterator != sceneView.end())
                    {
                        EXPECT_NE(valuesView.end(), valuesIterator);

                        DataTypes::MockIGraphObject* stored = azrtti_cast<DataTypes::MockIGraphObject*>(sceneIterator->get());
                        ASSERT_NE(stored, nullptr);

                        EXPECT_EQ(stored->m_id, *valuesIterator);

                        valuesIterator++;
                        sceneIterator++;
                    }
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Algorithms_Find_AlgorithmFindsRequestedName)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    auto sceneView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    auto convertView = MakeConvertView(sceneView,
                        [](const SceneGraph::Name& name) -> const char*
                        {
                            return name.GetPath();
                        });
                    // Needs AZStd::string for comparing otherwise two pointers will be compared instead of string content.
                    auto result = AZStd::find(convertView.begin(), convertView.end(), AZStd::string("A.C"));
                    auto compare = this->m_graph.ConvertToHierarchyIterator(this->m_graph.Find("A.C"));

                    EXPECT_EQ(compare, result.GetBaseIterator().GetHierarchyIterator());
                }

                TYPED_TEST_P(SceneGraphDownwardsIteratorContext, Algorithms_FindIf_FindsValue3InNodeAdotC)
                {
                    using Traversal = typename SceneGraphDownwardsIteratorContext<TypeParam>::Traversal;
                    SceneGraph::NodeIndex index = this->m_graph.Find("A.C");
                    auto sceneView = MakeSceneGraphDownwardsView<Traversal>(this->m_graph, this->m_graph.GetContentStorage().begin());
                    auto result = AZStd::find_if(sceneView.begin(), sceneView.end(),
                        [](const AZStd::shared_ptr<DataTypes::IGraphObject>& object) -> bool
                        {
                            const DataTypes::MockIGraphObject* value = azrtti_cast<const DataTypes::MockIGraphObject*>(object.get());
                            return value && value->m_id == 3;
                        });
                    ASSERT_NE(sceneView.end(), result);
                   
                    const DataTypes::MockIGraphObject* value = azrtti_cast<const DataTypes::MockIGraphObject*>(result->get());
                    ASSERT_NE(nullptr, value);
                    EXPECT_EQ(3, value->m_id);
                }

                REGISTER_TYPED_TEST_CASE_P(SceneGraphDownwardsIteratorContext,
                    MakeSceneGraphDownwardsIterator_FunctionComparedWithExplicitlyDeclaredIterator_IteratorsAreEqual,
                    MakeSceneGraphDownwardsIterator_ExtendedFunctionComparedWithExplicitlyDeclaredIterator_IteratorsAreEqual,
                    MakeSceneGraphDownwardsIterator_NodeAndHierarchyVersions_IteratorsAreEqual,
                    MakeSceneGraphDownwardsView_FunctionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd,
                    MakeSceneGraphDownwardsView_ExtendedFunctionComparedWithExplicitlyDeclaredIterators_ViewHasEquivalentBeginAndEnd,
                    MakeSceneGraphDownwardsView_NodeAndHierarchyVersions_IteratorsInViewsAreEqual,
                    Dereference_GetRootIteratorValue_ReturnsRelativeValueFromGivenValueIterator,
                    Dereference_GetDeepestIteratorValue_ReturnsRelativeValueFromGivenValueIterator,
                    Dereference_ValueIteratorNotSyncedWithHierarchyIteratorIfNotRequested_ReturnedValueMatchesOriginalValueIterator,
                    Dereference_DereferencingThroughStarAndArrowOperator_ValuesAreEqual,
                    IncrementOperator_MovePastEnd_ReturnsEndIterator,
                    GetHierarchyIterator_MatchesWithNodeInformationAfterMove_NameEqualToNodeIndexedName,
                    GetHierarchyIterator_MovePastEnd_GetEmptyDefaultNode,
                    EmptyGraph_CanDetectEmptyGraph_BeginPlusOneAndEndIteratorAreEqual,
                    EmptyGraph_CanDetectEmptyGraphFromView_BeginPlusOneAndEndIteratorAreEqual,
                    Algorithm_RangeForLoop_CanSuccesfullyRun,
                    IncrementOperator_TouchesAllNodes_NumberOfIterationStepsMatchesNumberOfNodesInGraph,
                    ValueIterator_NonSceneGraphIterator_ExternalIteratorValuesMatchSceneGraphValues,
                    Algorithms_Find_AlgorithmFindsRequestedName,
                    Algorithms_FindIf_FindsValue3InNodeAdotC
                    );

                using Group = ::testing::Types<DepthFirst, BreadthFirst>;
                INSTANTIATE_TYPED_TEST_CASE_P(SceneGraphDownwardsIteratorTests, SceneGraphDownwardsIteratorContext, Group);

                TEST_F(SceneGraphDownwardsIteratorTest, DepthFirst_IncrementOperator_MoveDownTheTree_IteratorReturnsParentOfPreviousIteration)
                {
                    auto iterator = MakeSceneGraphDownwardsIterator<DepthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.E", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.F", (++iterator)->GetPath());
                    EXPECT_STREQ("A.D", (++iterator)->GetPath());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, BreadthFirst_IncrementOperator_MoveDownTheTree_IteratorReturnsParentOfPreviousIteration)
                {
                    auto iterator = MakeSceneGraphDownwardsIterator<BreadthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    EXPECT_STREQ("A.D", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.E", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.F", (++iterator)->GetPath());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, DepthFirst_IncrementOperator_BlockCsChildren_AllNodesIteratedExceptEandF)
                {
                    auto iterator = MakeSceneGraphDownwardsIterator<DepthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    iterator.IgnoreNodeDescendants();
                    EXPECT_STREQ("A.D", (++iterator)->GetPath());
                    ++iterator;
                    EXPECT_EQ(iterator.GetHierarchyIterator(), SceneGraph::HierarchyStorageConstIterator());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, BreadthFirst_IncrementOperator_BlockCsChildren_AllNodesIteratedExceptEandF)
                {
                    auto iterator = MakeSceneGraphDownwardsIterator<BreadthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    iterator.IgnoreNodeDescendants();
                    EXPECT_STREQ("A.D", (++iterator)->GetPath());
                    ++iterator;
                    EXPECT_EQ(iterator.GetHierarchyIterator(), SceneGraph::HierarchyStorageConstIterator());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, BreadthFirst_IncrementOperator_SiblingsAreIgnored_SiblingNodesBandDareNotReturned)
                {
                    SceneGraph::NodeIndex index = this->m_graph.Find("A.C");
                    auto iterator = MakeSceneGraphDownwardsIterator<BreadthFirst>(this->m_graph, index, this->m_graph.GetNameStorage().begin(), true);
                    EXPECT_STREQ("A.C", iterator->GetPath());
                    EXPECT_STREQ("A.C.E", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.F", (++iterator)->GetPath());
                    ++iterator;
                    EXPECT_EQ(iterator.GetHierarchyIterator(), SceneGraph::HierarchyStorageConstIterator());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, DepthFirst_IncrementOperator_SiblingsAreIgnored_SiblingNodesBandDareNotReturned)
                {
                    SceneGraph::NodeIndex index = this->m_graph.Find("A.C");
                    auto iterator = MakeSceneGraphDownwardsIterator<DepthFirst>(this->m_graph, index, this->m_graph.GetNameStorage().begin(), true);
                    EXPECT_STREQ("A.C", iterator->GetPath());
                    EXPECT_STREQ("A.C.E", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.F", (++iterator)->GetPath());
                    ++iterator;
                    EXPECT_EQ(iterator.GetHierarchyIterator(), SceneGraph::HierarchyStorageConstIterator());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, DepthFirst_IncrementOperator_BlockAllChildren_NoNodesListedAfterRoot)
                {
                    auto iterator = MakeSceneGraphDownwardsIterator<DepthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    iterator.IgnoreNodeDescendants();
                    ++iterator;
                    EXPECT_EQ(iterator.GetHierarchyIterator(), SceneGraph::HierarchyStorageConstIterator());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, BreadthFirst_IncrementOperator_BlockAllChildren_NoNodesListedAfterRoot)
                {
                    auto iterator = MakeSceneGraphDownwardsIterator<BreadthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    iterator.IgnoreNodeDescendants();
                    ++iterator;
                    EXPECT_EQ(iterator.GetHierarchyIterator(), SceneGraph::HierarchyStorageConstIterator());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, DepthFirst_Algorithms_Copy_AllValuesCopiedToNewArray)
                {
                    AZStd::vector<AZStd::string> names(7);
                    auto sceneView = MakeSceneGraphDownwardsView<DepthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    auto convertView = MakeConvertView(sceneView,
                        [](const SceneGraph::Name& name) -> AZStd::string
                        {
                            return name.GetPath();
                        }); 
                    AZStd::copy(convertView.begin(), convertView.end(), names.begin());

                    EXPECT_STREQ("", names[0].c_str());
                    EXPECT_STREQ("A", names[1].c_str());
                    EXPECT_STREQ("A.B", names[2].c_str());
                    EXPECT_STREQ("A.C", names[3].c_str());
                    EXPECT_STREQ("A.C.E", names[4].c_str());
                    EXPECT_STREQ("A.C.F", names[5].c_str());
                    EXPECT_STREQ("A.D", names[6].c_str());
                }

                TEST_F(SceneGraphDownwardsIteratorTest, BreadthFirst_Algorithms_Copy_AllValuesCopiedToNewArray)
                {
                    AZStd::vector<AZStd::string> names(7);
                    auto sceneView = MakeSceneGraphDownwardsView<BreadthFirst>(this->m_graph, this->m_graph.GetNameStorage().begin());
                    auto convertView = MakeConvertView(sceneView,
                        [](const SceneGraph::Name& name) -> AZStd::string
                        {
                            return name.GetPath();
                        });
                    AZStd::copy(convertView.begin(), convertView.end(), names.begin());

                    EXPECT_STREQ("", names[0].c_str());
                    EXPECT_STREQ("A", names[1].c_str());
                    EXPECT_STREQ("A.B", names[2].c_str());
                    EXPECT_STREQ("A.C", names[3].c_str());
                    EXPECT_STREQ("A.D", names[4].c_str());
                    EXPECT_STREQ("A.C.E", names[5].c_str());
                    EXPECT_STREQ("A.C.F", names[6].c_str());
                }

                TEST(SceneGraphDownwardsIterator, DepthFirst_IncrementOperator_EdgeCase_NodesAreListedInCorrectOrder)
                {
                    /*---------------------------------------\
                    |      Root                              |
                    |       |                                |
                    |       A                                |
                    |     / |                                |
                    |    B  C                                |
                    |   /  /                                 |
                    |  D  E                                  |
                    |    /                                   |
                    |   F                                    |
                    \---------------------------------------*/

                    SceneGraph graph;
                    SceneGraph::NodeIndex root = graph.GetRoot();
                    
                    SceneGraph::NodeIndex indexA = graph.AddChild(root, "A");
                    SceneGraph::NodeIndex indexB = graph.AddChild(indexA, "B");
                    SceneGraph::NodeIndex indexC = graph.AddSibling(indexB, "C");

                    graph.AddChild(indexB, "D");

                    SceneGraph::NodeIndex indexE = graph.AddChild(indexC, "E");
                    graph.AddChild(indexE, "F");

                    auto iterator = MakeSceneGraphDownwardsIterator<DepthFirst>(graph, graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B.D", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.E", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.E.F", (++iterator)->GetPath());
                }

                TEST(SceneGraphDownwardsIterator, BreadthFirst_IncrementOperator_EdgeCase_NodesAreListedInCorrectOrder)
                {
                    /*---------------------------------------\
                    |      Root                              |
                    |       |                                |
                    |       A                                |
                    |     / |                                |
                    |    B  C                                |
                    |   /  /                                 |
                    |  D  E                                  |
                    |    /                                   |
                    |   F                                    |
                    \---------------------------------------*/

                    SceneGraph graph;
                    SceneGraph::NodeIndex root = graph.GetRoot();
                    
                    SceneGraph::NodeIndex indexA = graph.AddChild(root, "A");
                    SceneGraph::NodeIndex indexB = graph.AddChild(indexA, "B");
                    SceneGraph::NodeIndex indexC = graph.AddSibling(indexB, "C");

                    graph.AddChild(indexB, "D");

                    SceneGraph::NodeIndex indexE = graph.AddChild(indexC, "E");
                    graph.AddChild(indexE, "F");

                    auto iterator = MakeSceneGraphDownwardsIterator<BreadthFirst>(graph, graph.GetNameStorage().begin());
                    EXPECT_STREQ("", iterator->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    EXPECT_STREQ("A.B.D", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.E", (++iterator)->GetPath());
                    EXPECT_STREQ("A.C.E.F", (++iterator)->GetPath());
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ
