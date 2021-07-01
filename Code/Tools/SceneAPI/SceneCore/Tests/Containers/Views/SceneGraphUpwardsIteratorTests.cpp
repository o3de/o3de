/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/*
* This suite of tests focus on the unique features the SceneGraphUpwardsIterator
* adds as an iterator. The basic functionality and iterator conformity is tested
* in the Iterator Conformity Tests (see IteratorConformityTests.cpp).
*/

#include <AzTest/AzTest.h>
#include <algorithm>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                class SceneGraphUpwardsIteratorTest
                    : public ::testing::Test
                {
                public:
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
                        SceneGraph::NodeIndex index = m_graph.Find("A.C.E");
                        return m_graph.ConvertToHierarchyIterator(index);
                    }

                    SceneGraph m_graph;
                };

                TEST_F(SceneGraphUpwardsIteratorTest, MakeSceneGraphUpwardsIterator_UtilityFunctionProducesSameIteratorAsExplicitlyDeclared_IteratorsAreEqual)
                {
                    auto lhsIterator = MakeSceneGraphUpwardsIterator(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    auto rhsIterator = SceneGraphUpwardsIterator<SceneGraph::NameStorage::const_iterator>(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                TEST_F(SceneGraphUpwardsIteratorTest, MakeSceneGraphUpwardsIterator_NodeAndHierarchyVersions_IteratorsAreIdentical)
                {
                    SceneGraph::NodeIndex index = m_graph.Find("A.C.E");
                    auto hierarchy = m_graph.ConvertToHierarchyIterator(index);

                    auto indexIterator = MakeSceneGraphUpwardsIterator(m_graph, index, m_graph.GetNameStorage().begin(), true);
                    auto hierarchyIterator = MakeSceneGraphUpwardsIterator(m_graph, hierarchy, m_graph.GetNameStorage().begin(), true);
                    EXPECT_EQ(indexIterator, hierarchyIterator);
                }

                TEST_F(SceneGraphUpwardsIteratorTest, MakeSceneGraphUpwardsView_UtilityFunctionProducesSameIteratorAsExplicitlyDeclared_IteratorsAreEqual)
                {
                    auto view = MakeSceneGraphUpwardsView(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    auto beginIterator = SceneGraphUpwardsIterator<SceneGraph::NameStorage::const_iterator>(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    auto endIterator = SceneGraphUpwardsIterator<SceneGraph::NameStorage::const_iterator>();

                    EXPECT_EQ(view.begin(), beginIterator);
                    EXPECT_EQ(view.end(), endIterator);
                }

                TEST_F(SceneGraphUpwardsIteratorTest, MakeSceneGraphUpwardsView_NodeAndHierarchyVersions_IteratorsAreIdentical)
                {
                    SceneGraph::NodeIndex index = m_graph.Find("A.C.E");
                    auto hierarchy = m_graph.ConvertToHierarchyIterator(index);

                    auto indexView = MakeSceneGraphUpwardsView(m_graph, index, m_graph.GetNameStorage().begin(), true);
                    auto hierarchyView = MakeSceneGraphUpwardsView(m_graph, hierarchy, m_graph.GetNameStorage().begin(), true);

                    EXPECT_EQ(indexView.begin(), hierarchyView.begin());
                    EXPECT_EQ(indexView.end(), hierarchyView.end());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, EmptyGraph_CanDetectEmptyGraph_BeginAndEndIteratorAreEqual)
                {
                    SceneGraph emptyGraph;

                    auto beginIterator = MakeSceneGraphUpwardsIterator(emptyGraph, emptyGraph.GetHierarchyStorage().begin(), emptyGraph.GetNameStorage().begin(), true);
                    beginIterator++; // Even an empty graph will have at least a root entry.
                    auto endIterator = SceneGraphUpwardsIterator<SceneGraph::NameStorage::const_iterator>();
                    EXPECT_EQ(beginIterator, endIterator);
                }

                TEST_F(SceneGraphUpwardsIteratorTest, EmptyGraph_CanDetectEmptyGraphFromView_BeginAndEndIteratorAreEqual)
                {
                    SceneGraph emptyGraph;

                    auto view = MakeSceneGraphUpwardsView(emptyGraph, emptyGraph.GetHierarchyStorage().begin(), emptyGraph.GetNameStorage().begin(), true);
                    EXPECT_EQ(++view.begin(), view.end());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, Dereference_GetRootIteratorValue_ReturnsRelativeValueFromGivenValueIterator)
                {
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetRootHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    EXPECT_STREQ("", (*iterator).GetPath());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, Dereference_GetDeepestIteratorValue_ReturnsRelativeValueFromGivenValueIterator)
                {
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    EXPECT_STREQ("A.C.E", (*iterator).GetPath());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, Dereference_ValueIteratorNotSyncedWithHierarchyIteratorIfNotRequested_ReturnedValueMatchesOriginalValueIterator)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin() + 2;
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetDeepestHierarchyIterator(), valueIterator, false);
                    EXPECT_STREQ((*valueIterator).GetPath(), (*iterator).GetPath());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, Dereference_DereferencingThroughStarAndArrowOperator_ValuesAreEqual)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetDeepestHierarchyIterator(), valueIterator, true);
                    EXPECT_STREQ(iterator->GetPath(), (*iterator).GetPath());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, IncrementOperator_MoveUpTheTree_IteratorReturnsParentOfPreviousIteration)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetDeepestHierarchyIterator(), valueIterator, true);
                    EXPECT_STREQ("A.C.E", iterator->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    EXPECT_STREQ("A", (++iterator)->GetPath());
                    EXPECT_STREQ("", (++iterator)->GetPath());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, IncrementOperator_MovePastRoot_ReturnsEndIterator)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetRootHierarchyIterator(), valueIterator, true);
                    iterator++;
                    EXPECT_EQ(SceneGraphUpwardsIterator<SceneGraph::NameStorage::const_iterator>(), iterator);
                }

                TEST_F(SceneGraphUpwardsIteratorTest, GetHierarchyIterator_MatchesWithNodeInformationAfterMove_NameEqualToNodeIndexedName)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphUpwardsIterator(m_graph, GetDeepestHierarchyIterator(), valueIterator, true);
                    iterator++;

                    SceneGraph::HierarchyStorageConstIterator hierarchyIterator = iterator.GetHierarchyIterator();
                    SceneGraph::NodeIndex index = m_graph.ConvertToNodeIndex(hierarchyIterator);

                    EXPECT_STREQ(m_graph.GetNodeName(index).GetPath(), iterator->GetPath());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, ValueIterator_NonSceneGraphIterator_ExternalIteratorValuesMatchSceneGraphValues)
                {
                    // Commonly containers in the scene graph will be used, but it is possible to specify other containers that shadow
                    //      the scene graph but don't belong to it. This test checks if this works correctly by comparing the values
                    //      stored in the scene graph with the same values stored in an external container.
                    //      (See constructor of SceneGraphUpwardsIterator for more details on arguments.)
                    AZStd::vector<int> values = { 0, 1, 2, 3, 4, 5, 6 };

                    auto sceneView = MakeSceneGraphUpwardsView(m_graph, GetDeepestHierarchyIterator(), m_graph.GetContentStorage().begin(), true);
                    auto valuesView = MakeSceneGraphUpwardsView(m_graph, GetDeepestHierarchyIterator(), values.begin(), true);

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

                TEST_F(SceneGraphUpwardsIteratorTest, Algorithms_RangedForLoop_AllParentNodesTouchedAndExitingLoop)
                {
                    AZStd::vector<const char*> expectedName =
                    {
                        "A.C.E",
                        "A.C",
                        "A",
                        ""
                    };
                    size_t index = 0;

                    auto sceneView = MakeSceneGraphUpwardsView(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    for (auto& it : sceneView)
                    {
                        ASSERT_LT(index, expectedName.size());
                        EXPECT_STREQ(expectedName[index], it.GetPath());
                        index++;
                    }
                    EXPECT_EQ(expectedName.size(), index);
                }

                TEST_F(SceneGraphUpwardsIteratorTest, Algorithms_Find_AlgorithmFindsRequestedName)
                {
                    auto sceneView = MakeSceneGraphUpwardsView(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    auto convertView = MakeConvertView(sceneView,
                        [](const SceneGraph::Name& name) -> const char*
                        {
                            return name.GetPath();
                        });
                    // Needs AZStd::string for comparing otherwise two pointers will be compared instead of string content.
                    auto result = AZStd::find(convertView.begin(), convertView.end(), AZStd::string("A.C"));

                    auto compare = m_graph.ConvertToHierarchyIterator(m_graph.Find("A.C"));

                    EXPECT_EQ(compare, result.GetBaseIterator().GetHierarchyIterator());
                }

                TEST_F(SceneGraphUpwardsIteratorTest, Algorithms_Copy_AllValuesCopiedToNewArray)
                {
                    AZStd::vector<AZStd::string> names(4);

                    auto sceneView = MakeSceneGraphUpwardsView(m_graph, GetDeepestHierarchyIterator(), m_graph.GetNameStorage().begin(), true);
                    auto convertView = MakeConvertView(sceneView,
                        [](const SceneGraph::Name& name) -> const char*
                        {
                            return name.GetPath();
                        });

                    AZStd::copy(convertView.begin(), convertView.end(), names.begin());

                    EXPECT_STREQ("A.C.E", names[0].c_str());
                    EXPECT_STREQ("A.C", names[1].c_str());
                    EXPECT_STREQ("A", names[2].c_str());
                    EXPECT_STREQ("", names[3].c_str());
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ
