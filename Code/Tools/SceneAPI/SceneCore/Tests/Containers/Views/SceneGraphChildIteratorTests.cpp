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
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            namespace Views
            {
                class SceneGraphChildIteratorTest
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
                        SceneGraph::NodeIndex indexD = m_graph.AddSibling(indexC, "D", AZStd::make_shared<DataTypes::MockIGraphObject>(4));

                        SceneGraph::NodeIndex indexE = m_graph.AddChild(indexC, "E", AZStd::make_shared<DataTypes::MockIGraphObject>(5));
                        m_graph.AddSibling(indexE, "F", AZStd::make_shared<DataTypes::MockIGraphObject>(6));

                        m_graph.MakeEndPoint(indexB);
                        m_graph.MakeEndPoint(indexD);
                    }

                    SceneGraph::HierarchyStorageConstData::iterator GetHierarchyNodeA()
                    {
                        SceneGraph::NodeIndex index = m_graph.Find("A");
                        return m_graph.ConvertToHierarchyIterator(index);
                    }

                    SceneGraph m_graph;
                };

                TEST_F(SceneGraphChildIteratorTest, MakeSceneGraphChildIterator_UtilityFunctionProducesSameIteratorAsExplicitlyDeclared_IteratorsAreEqual)
                {
                    auto lhsIterator = MakeSceneGraphChildIterator(m_graph, m_graph.Find("A"), m_graph.GetNameStorage().begin(), true);
                    auto rhsIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>(m_graph, GetHierarchyNodeA(), m_graph.GetNameStorage().begin(), true);
                    EXPECT_EQ(lhsIterator, rhsIterator);
                }

                TEST_F(SceneGraphChildIteratorTest, MakeSceneGraphChildIterator_NodeAndHierarchyVersions_IteratorsAreIdentical)
                {
                    SceneGraph::NodeIndex index = m_graph.Find("A");
                    auto hierarchy = m_graph.ConvertToHierarchyIterator(index);

                    auto indexIterator = MakeSceneGraphChildIterator(m_graph, index, m_graph.GetNameStorage().begin(), true);
                    auto hierarchyIterator = MakeSceneGraphChildIterator(m_graph, hierarchy, m_graph.GetNameStorage().begin(), true);
                    EXPECT_EQ(indexIterator, hierarchyIterator);
                }

                TEST_F(SceneGraphChildIteratorTest, MakeSceneGraphChildIterator_ConstructingFromNodeWithoutChildren_ReturnsEndIterator)
                {
                    auto iterator = MakeSceneGraphChildIterator(m_graph, m_graph.Find("A.C.E"), m_graph.GetNameStorage().begin(), true);
                    auto endIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>();
                    EXPECT_EQ(iterator, endIterator);
                }

                TEST_F(SceneGraphChildIteratorTest, MakeSceneGraphChildView_UtilityFunctionProducesSameIteratorAsExplicitlyDeclared_IteratorsAreEqual)
                {
                    SceneGraph::NodeIndex index = m_graph.Find("A");
                    auto hierarchy = m_graph.ConvertToHierarchyIterator(index);

                    auto view = MakeSceneGraphChildView(m_graph, hierarchy, m_graph.GetNameStorage().begin(), true);
                    auto beginIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>(m_graph, hierarchy, m_graph.GetNameStorage().begin(), true);
                    auto endIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>();

                    EXPECT_EQ(view.begin(), beginIterator);
                    EXPECT_EQ(view.end(), endIterator);
                }

                TEST_F(SceneGraphChildIteratorTest, MakeSceneGraphChildView_NodeAndHierarchyVersions_IteratorsAreIdentical)
                {
                    SceneGraph::NodeIndex index = m_graph.Find("A");
                    auto hierarchy = m_graph.ConvertToHierarchyIterator(index);

                    auto indexView = MakeSceneGraphChildView(m_graph, index, m_graph.GetNameStorage().begin(), true);
                    auto hierarchyView = MakeSceneGraphChildView(m_graph, hierarchy, m_graph.GetNameStorage().begin(), true);

                    EXPECT_EQ(indexView.begin(), hierarchyView.begin());
                    EXPECT_EQ(indexView.end(), hierarchyView.end());
                }

                TEST_F(SceneGraphChildIteratorTest, EmptyGraph_CanDetectEmptyGraph_BeginAndEndIteratorAreEqual)
                {
                    SceneGraph emptyGraph;

                    auto beginIterator = MakeSceneGraphChildIterator(emptyGraph, emptyGraph.GetHierarchyStorage().begin(), emptyGraph.GetNameStorage().begin(), true);
                    auto endIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>();
                    EXPECT_EQ(beginIterator, endIterator);
                }

                TEST_F(SceneGraphChildIteratorTest, EmptyGraph_CanDetectEmptyGraphFromView_BeginAndEndIteratorAreEqual)
                {
                    SceneGraph emptyGraph;

                    auto view = MakeSceneGraphChildView(emptyGraph, emptyGraph.GetHierarchyStorage().begin(), emptyGraph.GetNameStorage().begin(), true);
                    EXPECT_EQ(view.begin(), view.end());
                }

                TEST_F(SceneGraphChildIteratorTest, Dereference_DereferencingThroughStarAndArrowOperator_ValuesAreEqual)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphChildIterator(m_graph, m_graph.Find("A"), valueIterator, true);
                    EXPECT_STREQ(iterator->GetPath(), (*iterator).GetPath());
                }

                TEST_F(SceneGraphChildIteratorTest, IncrementOperator_ListAllChildren_IteratorGivesAllChildNodes)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphChildIterator(m_graph, m_graph.Find("A"), valueIterator, true);
                    EXPECT_STREQ("A.B", iterator->GetPath());
                    EXPECT_STREQ("A.C", (++iterator)->GetPath());
                    EXPECT_STREQ("A.D", (++iterator)->GetPath());
                }

                TEST_F(SceneGraphChildIteratorTest, Dereference_ProvidedIteratorMovedToFirstChildIfRootIterator_ReturnedFirstChildName)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphChildIterator(m_graph, m_graph.Find("A.C"), valueIterator, true);
                    EXPECT_STREQ("A.C.E", (*iterator).GetPath());
                }

                TEST_F(SceneGraphChildIteratorTest, Dereference_ProvidedIteratorMovedToFirstChildIfNotRootIterator_ReturnedFirstChildName)
                {
                    SceneGraph::NodeIndex index = m_graph.Find("A.C");
                    ASSERT_TRUE(index.IsValid()); // Name has been entered in the graph so should be found.
                    auto valueIterator = m_graph.ConvertToNameIterator(index);
                    ASSERT_NE(m_graph.GetNameStorage().end(), valueIterator); // Correct iterator should be found.

                    auto iterator = MakeSceneGraphChildIterator(m_graph, index, valueIterator, false);
                    EXPECT_STREQ("A.C.E", (*iterator).GetPath());
                }

                TEST_F(SceneGraphChildIteratorTest, IncrementOperator_MovedPastLastChild_ReturnsEndIterator)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphChildIterator(m_graph, m_graph.Find("A"), valueIterator, true);
                    ++iterator; // A.B
                    ++iterator; // A.C
                    ++iterator; // A.D
                    EXPECT_EQ(SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator>(), iterator);
                }

                TEST_F(SceneGraphChildIteratorTest, IncrementOperator_ListNodesChildren_IteratorGivesCOnly)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphChildIterator<AcceptNodesOnly>(m_graph, m_graph.Find("A"), valueIterator, true);
                    auto endIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator, AcceptNodesOnly>();
                    EXPECT_STREQ("A.C", iterator->GetPath());
                    EXPECT_EQ(endIterator, ++iterator);
                }

                TEST_F(SceneGraphChildIteratorTest, IncrementOperator_ListEndPoint_IteratorGivesBandD)
                {
                    auto valueIterator = m_graph.GetNameStorage().begin();
                    auto iterator = MakeSceneGraphChildIterator<AcceptEndPointsOnly>(m_graph, m_graph.Find("A"), valueIterator, true);
                    auto endIterator = SceneGraphChildIterator<SceneGraph::NameStorage::const_iterator, AcceptEndPointsOnly>();
                    EXPECT_STREQ("A.B", iterator->GetPath());
                    EXPECT_STREQ("A.D", (++iterator)->GetPath());
                    EXPECT_EQ(endIterator, ++iterator);
                }

                TEST_F(SceneGraphChildIteratorTest, ValueIterator_NonSceneGraphIterator_ExternalIteratorValuesMatchSceneGraphValues)
                {
                    // Commonly containers in the scene graph will be used, but it is possible to specify other containers that shadow
                    //      the scene graph but don't belong to it. This test checks if this works correctly by comparing the values
                    //      stored in the scene graph with the same values stored in an external container.
                    //      (See constructor of SceneGraphChildIterator for more details on arguments.)
                    AZStd::vector<int> values = { 0, 1, 2, 3, 4, 5, 6 };

                    auto sceneView = MakeSceneGraphChildView(m_graph, m_graph.Find("A"), m_graph.GetContentStorage().begin(), true);
                    auto valuesView = MakeSceneGraphChildView(m_graph, m_graph.Find("A"), values.begin(), true);

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

                TEST_F(SceneGraphChildIteratorTest, Algorithms_RangedForLoop_AllChildNodesTouchedAndExitingLoop)
                {
                    AZStd::vector<const char*> expectedName =
                    {
                        "A.B",
                        "A.C",
                        "A.D"
                    };
                    size_t index = 0;

                    auto sceneView = MakeSceneGraphChildView(m_graph, m_graph.Find("A"), m_graph.GetNameStorage().begin(), true);
                    for (auto& it : sceneView)
                    {
                        ASSERT_LT(index, expectedName.size());
                        EXPECT_STREQ(expectedName[index], it.GetPath());
                        index++;
                    }
                    EXPECT_EQ(expectedName.size(), index);
                }

                TEST_F(SceneGraphChildIteratorTest, Algorithms_Find_AlgorithmFindsRequestedName)
                {
                    auto sceneView = MakeSceneGraphChildView(m_graph, m_graph.Find("A"), m_graph.GetNameStorage().begin(), true);
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

                TEST_F(SceneGraphChildIteratorTest, Algorithms_Copy_AllValuesCopiedToNewArray)
                {
                    AZStd::vector<AZStd::string> names(3);

                    auto sceneView = MakeSceneGraphChildView(m_graph, m_graph.Find("A"), m_graph.GetNameStorage().begin(), true);
                    auto convertView = MakeConvertView(sceneView, 
                        [](const SceneGraph::Name& name) -> const char*
                        {
                            return name.GetPath();
                        });

                    AZStd::copy(convertView.begin(), convertView.end(), names.begin());

                    EXPECT_STREQ("A.B", names[0].c_str());
                    EXPECT_STREQ("A.C", names[1].c_str());
                    EXPECT_STREQ("A.D", names[2].c_str());
                }
            } // Views
        } // Containers
    } // SceneAPI
} // AZ
