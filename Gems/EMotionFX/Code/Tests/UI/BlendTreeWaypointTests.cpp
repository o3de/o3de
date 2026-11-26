/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeConnection.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>

namespace EMotionFX
{
    // In a blend tree a reroute is a waypoint on the connection: the editor bends the wire, the blend tree is unchanged.
    class BlendTreeWaypointTest : public AnimGraphUIFixture
    {
    protected:
        void SetUp() override
        {
            AnimGraphUIFixture::SetUp();
            m_graph = CreateAnimGraph();
            ASSERT_NE(m_graph, nullptr);
            auto* manager = CommandSystem::GetCommandManager();
            AZStd::string result;

            auto createNode = [this, manager, &result](const AZ::TypeId& type, const char* parent, const char* name, int x)
            {
                return manager->ExecuteCommand(AZStd::string::format(
                    "AnimGraphCreateNode -animGraphID %u -type %s -parentName %s -name %s -xPos %d -yPos 0",
                    m_graph->GetID(), type.ToString<AZStd::string>().c_str(), parent, name, x), result);
            };
            ASSERT_TRUE(createNode(azrtti_typeid<BlendTree>(), "Root", "Tree", 0)) << result.c_str();
            ASSERT_TRUE(createNode(azrtti_typeid<BlendTreeFloatConstantNode>(), "Tree", "Weight", 0)) << result.c_str();
            ASSERT_TRUE(createNode(azrtti_typeid<BlendTreeBlend2Node>(), "Tree", "BlendA", 400)) << result.c_str();
            ASSERT_TRUE(createNode(azrtti_typeid<BlendTreeBlend2Node>(), "Tree", "BlendB", 400)) << result.c_str();

            for (const char* target : { "BlendA", "BlendB" })
            {
                ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
                    "AnimGraphCreateConnection -animGraphID %u -sourceNode Weight -targetNode %s -sourcePort 0 -targetPort %d",
                    m_graph->GetID(), target, BlendTreeBlend2Node::INPUTPORT_WEIGHT), result)) << result.c_str();
            }

            AnimGraphNode* weight = m_graph->RecursiveFindNodeByName("Weight");
            m_blendA = m_graph->RecursiveFindNodeByName("BlendA");
            m_blendB = m_graph->RecursiveFindNodeByName("BlendB");
            m_connectionA = m_blendA->FindConnection(weight, 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            m_connectionB = m_blendB->FindConnection(weight, 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            ASSERT_NE(m_connectionA, nullptr);
            ASSERT_NE(m_connectionB, nullptr);

            EMStudio::AnimGraphModel& model = m_animGraphPlugin->GetAnimGraphModel();
            model.Focus(model.FindFirstModelIndex(m_graph->RecursiveFindNodeByName("Tree")));
            m_nodeGraph = GetActiveNodeGraph();
            ASSERT_NE(m_nodeGraph, nullptr);

            m_nodeConnectionA = FindNodeConnection(m_connectionA);
            m_nodeConnectionB = FindNodeConnection(m_connectionB);
            ASSERT_NE(m_nodeConnectionA, nullptr);
            ASSERT_NE(m_nodeConnectionB, nullptr);
        }

        EMStudio::NodeConnection* FindNodeConnection(BlendTreeConnection* connection)
        {
            for (const QModelIndex& modelIndex : m_animGraphPlugin->GetAnimGraphModel().FindModelIndexes(connection))
            {
                if (EMStudio::NodeConnection* nodeConnection = m_nodeGraph->FindNodeConnection(modelIndex))
                {
                    return nodeConnection;
                }
            }
            return nullptr;
        }

        void Select(const AZStd::vector<EMStudio::NodeConnection*>& connections)
        {
            QItemSelection selection;
            for (EMStudio::NodeConnection* connection : connections)
            {
                selection.select(connection->GetModelIndex(), connection->GetModelIndex());
            }
            m_animGraphPlugin->GetAnimGraphModel().GetSelectionModel().select(selection, QItemSelectionModel::ClearAndSelect);
        }

        void StartDrag()
        {
            CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, m_original);
            m_graph->SetDirtyFlag(false);
            m_nodeGraph->StartRepositionWaypoint(m_nodeConnectionA, 0, m_connectionA->GetWaypoints());
            m_nodeConnectionA->SetWaypoint(0, m_dragged[0]);
        }

        AnimGraph* m_graph = nullptr;
        AnimGraphNode* m_blendA = nullptr;
        AnimGraphNode* m_blendB = nullptr;
        BlendTreeConnection* m_connectionA = nullptr;
        BlendTreeConnection* m_connectionB = nullptr;
        EMStudio::NodeGraph* m_nodeGraph = nullptr;
        EMStudio::NodeConnection* m_nodeConnectionA = nullptr;
        EMStudio::NodeConnection* m_nodeConnectionB = nullptr;
        const AZStd::vector<AZ::Vector2> m_original{ AZ::Vector2(200.0f, 100.0f) };
        const AZStd::vector<AZ::Vector2> m_dragged{ AZ::Vector2(250.0f, 150.0f) };
    };

    TEST_F(BlendTreeWaypointTest, ConnectionReadsItsWaypointsAndRoutesThroughThem)
    {
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, m_original);

        ASSERT_NE(m_nodeConnectionA->GetWaypoints(), nullptr);
        EXPECT_EQ(*m_nodeConnectionA->GetWaypoints(), m_original);
        EXPECT_EQ(m_nodeConnectionA->FindWaypoint(QPoint(201, 101)), size_t{ 0 });
        EXPECT_EQ(m_nodeConnectionA->FindWaypoint(QPoint(260, 100)), InvalidIndex);

        size_t waypointIndex = InvalidIndex;
        EXPECT_EQ(m_nodeGraph->FindWaypoint(QPoint(200, 100), waypointIndex), m_nodeConnectionA);
        EXPECT_EQ(waypointIndex, size_t{ 0 });

        AZStd::vector<QPoint> path;
        m_nodeConnectionA->CalcPolyline(path);
        ASSERT_EQ(path.size(), size_t{ 3 });
        EXPECT_EQ(path[1], QPoint(200, 100));
        EXPECT_TRUE(m_nodeConnectionA->CalcRect().contains(QPoint(200, 100)));
    }

    TEST_F(BlendTreeWaypointTest, FinishedDragIsOneUndoableCommand)
    {
        StartDrag();
        const AZStd::vector<AZ::Vector2> oldWaypoints = m_nodeGraph->GetRepositionedWaypointOldValues();
        m_nodeGraph->StopRepositionWaypoint();
        m_blendGraphWidget->CommitWaypoints(m_nodeConnectionA, oldWaypoints);

        EXPECT_EQ(m_connectionA->GetWaypoints(), m_dragged);
        EXPECT_TRUE(m_graph->GetDirtyFlag());

        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(m_connectionA->GetWaypoints(), m_original);
    }

    TEST_F(BlendTreeWaypointTest, CancelWaypointDragRestoresOriginal)
    {
        StartDrag();
        m_blendGraphWidget->DeleteSelectedItems();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_connectionA->GetWaypoints(), m_original);
        EXPECT_FALSE(m_graph->GetDirtyFlag());
    }

    TEST_F(BlendTreeWaypointTest, UndoDuringWaypointDragKeepsCommandResult)
    {
        StartDrag();
        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_TRUE(m_connectionA->GetWaypoints().empty());
    }

    TEST_F(BlendTreeWaypointTest, ShortcutAddsAndRemovesAWaypoint)
    {
        Select({ m_nodeConnectionA });
        ASSERT_TRUE(m_blendGraphWidget->CanInsertWaypointOnSelectedConnections());
        EXPECT_FALSE(m_blendGraphWidget->CanRemoveWaypointFromSelectedConnections());

        m_blendGraphWidget->InsertWaypointOnSelectedConnections();
        ASSERT_EQ(m_connectionA->GetWaypoints().size(), size_t{ 1 });
        EXPECT_TRUE(m_connectionB->GetWaypoints().empty());

        ASSERT_TRUE(m_blendGraphWidget->CanRemoveWaypointFromSelectedConnections());
        m_blendGraphWidget->RemoveWaypointFromSelectedConnections();
        EXPECT_TRUE(m_connectionA->GetWaypoints().empty());
    }

    TEST_F(BlendTreeWaypointTest, FannedOutConnectionsShareOneWaypoint)
    {
        Select({ m_nodeConnectionA, m_nodeConnectionB });
        m_blendGraphWidget->InsertWaypointOnSelectedConnections();

        ASSERT_EQ(m_connectionA->GetWaypoints().size(), size_t{ 1 });
        EXPECT_EQ(m_connectionA->GetWaypoints(), m_connectionB->GetWaypoints());

        // Both bend points came from one command group, so one undo removes them together.
        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_TRUE(m_connectionA->GetWaypoints().empty());
        EXPECT_TRUE(m_connectionB->GetWaypoints().empty());
    }

    TEST_F(BlendTreeWaypointTest, RemovingTheTargetNodeCancelsTheDrag)
    {
        StartDrag();
        AZStd::string result;
        auto* manager = CommandSystem::GetCommandManager();
        ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
            "AnimGraphRemoveNode -animGraphID %u -name BlendA", m_graph->GetID()), result)) << result.c_str();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_nodeGraph->GetRepositionedWaypointConnection(), nullptr);
    }
} // namespace EMotionFX
