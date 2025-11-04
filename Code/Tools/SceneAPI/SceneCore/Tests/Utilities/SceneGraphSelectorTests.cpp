/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/ManifestBase/MockISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockIMeshData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            using ::testing::NiceMock;
            using ::testing::Return;
            using ::testing::ReturnRef;
            using ::testing::Eq;
            using ::testing::Invoke;
            using ::testing::_;
            using ::testing::Matcher;

            class SceneGraphSelectorTest
                : public ::testing::Test
            {
            public:
                virtual ~SceneGraphSelectorTest() = default;

                void SetUp()
                {
                    /*---------------------------------------\
                    |      Root                              |
                    |       |                                |
                    |       A                                |
                    |     / | \                              |
                    |    B  C  D                             |
                    |           \                            |
                    |            E                           |
                    |                                        |
                    \---------------------------------------*/

                    Containers::SceneGraph::NodeIndex root = m_graph.GetRoot();
                    Containers::SceneGraph::NodeIndex indexA = m_graph.AddChild(root, "A");
                    Containers::SceneGraph::NodeIndex indexB = m_graph.AddChild(indexA, "B");
                    Containers::SceneGraph::NodeIndex indexC = m_graph.AddSibling(indexB, "C");
                    Containers::SceneGraph::NodeIndex indexD = m_graph.AddSibling(indexC, "D");
                    m_graph.AddChild(indexD, "E");
                }

                void CreateMeshGroup(AZStd::vector<AZStd::string>& selectedNodes, AZStd::vector<AZStd::string>& unselectedNodes)
                {
                    EXPECT_CALL(m_testNodeSelectionList, GetSelectedNodeCount())
                        .WillRepeatedly(Return(selectedNodes.size()));
                    EXPECT_CALL(m_testNodeSelectionList, GetUnselectedNodeCount())
                        .WillRepeatedly(Return(unselectedNodes.size()));

                    auto enumerateSelectedNodesInvoke =
                        [&selectedNodes](const DataTypes::ISceneNodeSelectionList::EnumerateNodesCallback& callback)
                    {
                        for (auto& node : selectedNodes)
                        {
                            if (!callback(node))
                            {
                                break;
                            }
                        }
                    };
                    EXPECT_CALL(m_testNodeSelectionList, EnumerateSelectedNodes(_))
                        .WillRepeatedly(Invoke(enumerateSelectedNodesInvoke));

                    auto enumerateUnselectedNodesInvoke =
                        [&unselectedNodes](const DataTypes::ISceneNodeSelectionList::EnumerateNodesCallback& callback)
                    {
                        for (auto& node : unselectedNodes)
                        {
                            if (!callback(node))
                            {
                                break;
                            }
                        }
                    };
                    EXPECT_CALL(m_testNodeSelectionList, EnumerateUnselectedNodes(_))
                        .WillRepeatedly(Invoke(enumerateUnselectedNodesInvoke));

                    auto isSelectedNodeInvoke = [&selectedNodes](const AZStd::string& name)
                    {
                        return (AZStd::find(selectedNodes.begin(), selectedNodes.end(), name) != selectedNodes.end());
                    };
                    EXPECT_CALL(m_testNodeSelectionList, IsSelectedNode(_)).WillRepeatedly(Invoke(isSelectedNodeInvoke));

                    auto selectedNodeInvoke = [&selectedNodes](const AZStd::string& name)
                        {
                            selectedNodes.push_back(name);
                        };
                    EXPECT_CALL(m_testNodeSelectionList, AddSelectedNode(_))
                        .WillRepeatedly(Invoke(selectedNodeInvoke));

                    auto removeNodeInvoke = [&unselectedNodes](const AZStd::string& name)
                        {
                            unselectedNodes.push_back(name);
                        };
                    EXPECT_CALL(m_testNodeSelectionList, RemoveSelectedNode(Matcher<const AZStd::string&>(_)))
                        .WillRepeatedly(Invoke(removeNodeInvoke));

                    auto clearSelectedNodes = [&selectedNodes]()
                        {
                            selectedNodes.clear();
                        };
                    EXPECT_CALL(m_testNodeSelectionList, ClearSelectedNodes())
                        .WillRepeatedly(Invoke(clearSelectedNodes));

                    auto clearUnselectedNodes = [&unselectedNodes]()
                        {
                            unselectedNodes.clear();
                        };
                    EXPECT_CALL(m_testNodeSelectionList, ClearUnselectedNodes())
                        .WillRepeatedly(Invoke(clearUnselectedNodes));
                }

                static bool IsValidTestNodeType([[maybe_unused]] const Containers::SceneGraph& graph, [[maybe_unused]] Containers::SceneGraph::NodeIndex& index)
                {
                    return true;
                }

                static bool IsValidTestNodeTypeSomeInvalid(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex& index)
                {
                    if (strcmp(graph.GetNodeName(index).GetPath(), "") == 0 || strcmp(graph.GetNodeName(index).GetPath(), "A.D") == 0)
                    {
                        return false;
                    }
                    else
                    {
                        return true;
                    }
                }

            protected:
                using TestNodeSelectionList = DataTypes::MockISceneNodeSelectionList;

                Containers::SceneGraph m_graph;
                NiceMock<TestNodeSelectionList> m_testNodeSelectionList;
            };

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_EmptySelectedAndEmptyUnselectedNodes_NoTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 0);
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_OnlySelectedRootNode_AllNodesInTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A" };
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 5);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.B", targetNodes[1].c_str());
                EXPECT_STREQ("A.C", targetNodes[2].c_str());
                EXPECT_STREQ("A.D", targetNodes[3].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[4].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_OnlyUnselectedRootNode_NoTargetNode)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes = { "A" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_TRUE(targetNodes.empty());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_NonemptySelectedIncludingRootNodeAndEmptyUnselectedNodes_AllNodesInTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B", "A.D" };
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 5);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.B", targetNodes[1].c_str());
                EXPECT_STREQ("A.C", targetNodes[2].c_str());
                EXPECT_STREQ("A.D", targetNodes[3].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[4].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_NonemptySelectedExcludingRootNodeAndEmptyUnselectedNodes_NodeAandADandADE)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A.B", "A.D" };
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 3);
                EXPECT_STREQ("A.B", targetNodes[0].c_str());
                EXPECT_STREQ("A.D", targetNodes[1].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[2].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_EmptySelectedAndNonemptyUnselectedNodesIncludingRootNode_NoTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes = { "A", "A.B", "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_TRUE(targetNodes.empty());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_EmptySelectedAndNonemptyUnselectedNodesExcludingRootNode_NodeAandAC)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes = { "A.B", "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 2);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.C", targetNodes[1].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_DuplicateNodeRemovedFromSelected_MissNodeADotD)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B", "A.C", "A.D", "A.D.E" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 4);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.B", targetNodes[1].c_str());
                EXPECT_STREQ("A.C", targetNodes[2].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[3].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_SelectedParentNodeUnselectedChildNode_NodeAandAB)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.C", "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 2);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.B", targetNodes[1].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_UnselectedParentNodeSelectedChildNode_NodeACandADandADE)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A.C", "A.D" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A", "A.B" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 3);
                EXPECT_STREQ("A.C", targetNodes[0].c_str());
                EXPECT_STREQ("A.D", targetNodes[1].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[2].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_SelectedParentUnselectedChild_ParentNodeInTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A.D" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.D.E" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_NE(targetNodes.end(), AZStd::find(targetNodes.begin(), targetNodes.end(), "A.D"));
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_UnselectedParentSelectedChild_ChildNodeInTargetNodesButNotParent)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A.D.E" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.end(), AZStd::find(targetNodes.begin(), targetNodes.end(), "A.D"));
                EXPECT_NE(targetNodes.end(), AZStd::find(targetNodes.begin(), targetNodes.end(), "A.D.E"));
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_GrandparentNodeSelectedParentNodeUnknown_GrandchildNodeInTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.B", "A.C" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 3);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.D", targetNodes[1].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[2].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_GrandparentNodeUnselectedParentNodeUnknown_GrandchildNodeNotInTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A.B", "A.C" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 2);
                EXPECT_STREQ("A.B", targetNodes[0].c_str());
                EXPECT_STREQ("A.C", targetNodes[1].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_OverlappedSelectedAndUnselectedNodes_OverlappedNodeNotInTargetNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B", "A.D" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.B", "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeType);
                EXPECT_EQ(targetNodes.size(), 2);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.C", targetNodes[1].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_OnlySelectedRootNodeSomeInvalidNodes_NodeAandABandACandADE)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A" };
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeTypeSomeInvalid);
                EXPECT_EQ(targetNodes.size(), 4);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.B", targetNodes[1].c_str());
                EXPECT_STREQ("A.C", targetNodes[2].c_str());
                EXPECT_STREQ("A.D.E", targetNodes[3].c_str());
            }

            TEST_F(SceneGraphSelectorTest, GenerateTargetNodes_NonemptySelectedandUnselectedNodseSomeInvalidNodes_NodeAandAC)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.B", "A.D.E"};
                CreateMeshGroup(selectedNodes, unselectedNodes);
                auto targetNodes = SceneGraphSelector::GenerateTargetNodes(m_graph, m_testNodeSelectionList, IsValidTestNodeTypeSomeInvalid);
                EXPECT_EQ(targetNodes.size(), 2);
                EXPECT_STREQ("A", targetNodes[0].c_str());
                EXPECT_STREQ("A.C", targetNodes[1].c_str());
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_EmptySelection_AllNodesInSelected)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(0, selectedNodes.size());
                EXPECT_EQ(5, unselectedNodes.size());
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_UnselectedNodeA_AllNodesInUnselected)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes = { "A" };
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(0, selectedNodes.size());
                EXPECT_EQ(5, unselectedNodes.size());
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_NonemptySelectedExcludingRootNodeAndEmptyUnselectedNodes__ABandADandADEFoundInSelectedNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A.B", "A.D" };
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(3, selectedNodes.size());
                EXPECT_EQ(2, unselectedNodes.size());
                EXPECT_STREQ("A.B", selectedNodes[0].c_str());
                EXPECT_STREQ("A.D", selectedNodes[1].c_str());
                EXPECT_STREQ("A.D.E", selectedNodes[2].c_str());
                EXPECT_STREQ("A", unselectedNodes[0].c_str());
                EXPECT_STREQ("A.C", unselectedNodes[1].c_str());
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_EmptySelectedAndNonemptyUnselectedNodesIncludingRootNode_AllNodesInUnselected)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes = { "A", "A.B", "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(0, selectedNodes.size());
                EXPECT_EQ(5, unselectedNodes.size());
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_EmptySelectedAndNonemptyUnselectedNodesExcludingRootNode_AandACFoundInSelectedNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes;
                AZStd::vector<AZStd::string> unselectedNodes = { "A.B", "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(2, selectedNodes.size());
                EXPECT_EQ(3, unselectedNodes.size());
                EXPECT_STREQ("A", selectedNodes[0].c_str());
                EXPECT_STREQ("A.C", selectedNodes[1].c_str());
                EXPECT_STREQ("A.B", unselectedNodes[0].c_str());
                EXPECT_STREQ("A.D", unselectedNodes[1].c_str());
                EXPECT_STREQ("A.D.E", unselectedNodes[2].c_str());
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_DuplicateEntryRemovedFromSelected_ADotDNotFoundInSelectedNodes)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B", "A.C", "A.D", "A.D.E" };
                AZStd::vector<AZStd::string> unselectedNodes = { "A.D" };
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(4, selectedNodes.size());
                EXPECT_EQ(1, unselectedNodes.size());
                EXPECT_EQ(selectedNodes.end(), AZStd::find(selectedNodes.begin(), selectedNodes.end(), "A.D"));
                EXPECT_NE(unselectedNodes.end(), AZStd::find(unselectedNodes.begin(), unselectedNodes.end(), "A.D"));
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_InvalidEntryInSelected_InvalidEntryRemoved)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B", "X", "A.C", "A.D", "A.D.E" };
                AZStd::vector<AZStd::string> unselectedNodes;
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(selectedNodes.end(), AZStd::find(selectedNodes.begin(), selectedNodes.end(), "X"));
            }

            TEST_F(SceneGraphSelectorTest, UpdateNodeSelection_InvalidEntryInUnselected_InvalidEntryRemoved)
            {
                AZStd::vector<AZStd::string> selectedNodes = { "A", "A.B", "A.C", "A.D", "A.D.E" };
                AZStd::vector<AZStd::string> unselectedNodes = { "X" };
                CreateMeshGroup(selectedNodes, unselectedNodes);

                SceneGraphSelector::UpdateNodeSelection(m_graph, m_testNodeSelectionList);
                EXPECT_EQ(unselectedNodes.end(), AZStd::find(unselectedNodes.begin(), unselectedNodes.end(), "X"));
            }
        }
    }
}
