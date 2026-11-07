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
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateGraphNode.h>
#include <QApplication>
#include <QFocusEvent>

namespace EMotionFX
{
    class WaypointDragTest : public AnimGraphUIFixture
    {
    protected:
        void SetUp() override
        {
            AnimGraphUIFixture::SetUp();
            m_graph = CreateAnimGraph();
            AnimGraph* graph = m_graph;
            ASSERT_NE(graph, nullptr);
            auto* manager = CommandSystem::GetCommandManager();
            AZStd::string result;
            const AZStd::string type = azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>();

            for (const char* name : {"First", "Second"})
            {
                ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
                    "AnimGraphCreateNode -animGraphID %u -type %s -parentName Root -name %s",
                    graph->GetID(), type.c_str(), name), result)) << result.c_str();
            }

            ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
                "AnimGraphCreateConnection -animGraphID %u -sourceNode First -targetNode Second -sourcePort 0 -targetPort 0 -transitionType %s",
                graph->GetID(), azrtti_typeid<AnimGraphStateTransition>().ToString<AZStd::string>().c_str()), result)) << result.c_str();

            m_transition = graph->RecursiveFindTransitionById(AnimGraphConnectionId::CreateFromString(result));
            ASSERT_NE(m_transition, nullptr);
            CommandSystem::AdjustTransitionWaypoints(m_transition, original);

            m_nodeGraph = GetActiveNodeGraph();
            auto* nodeGraph = m_nodeGraph;
            ASSERT_NE(nodeGraph, nullptr);
            m_connection = nodeGraph->FindStateConnection(m_animGraphPlugin->GetAnimGraphModel().FindFirstModelIndex(m_transition));
            ASSERT_NE(m_connection, nullptr);
        }

        void StartDrag()
        {
            m_graph->SetDirtyFlag(false);
            m_nodeGraph->StartRepositionWaypoint(m_connection, 0, m_transition->GetWaypoints());
            m_transition->SetWaypoint(0, AZ::Vector2(200.0f, 200.0f));
        }

        AnimGraph* m_graph = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
        EMStudio::NodeGraph* m_nodeGraph = nullptr;
        EMStudio::StateConnection* m_connection = nullptr;
        const AZStd::vector<AZ::Vector2> original{AZ::Vector2(100.0f, 100.0f)};
    };

    TEST_F(WaypointDragTest, CancelWaypointDragRestoresOriginal)
    {
        StartDrag();
        m_blendGraphWidget->DeleteSelectedItems();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_transition->GetWaypoints(), original);
        EXPECT_FALSE(m_graph->GetDirtyFlag());
    }

    TEST_F(WaypointDragTest, SwitchingGraphCancelsWaypointDrag)
    {
        StartDrag();
        m_blendGraphWidget->SetActiveGraph(nullptr);

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_transition->GetWaypoints(), original);
        EXPECT_FALSE(m_graph->GetDirtyFlag());

        m_blendGraphWidget->SetActiveGraph(m_nodeGraph);
        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
    }

    TEST_F(WaypointDragTest, LosingFocusCancelsWaypointDrag)
    {
        StartDrag();
        QFocusEvent event(QEvent::FocusOut);
        QApplication::sendEvent(m_blendGraphWidget, &event);

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_transition->GetWaypoints(), original);
        EXPECT_FALSE(m_graph->GetDirtyFlag());
    }

    TEST_F(WaypointDragTest, UndoDuringWaypointDragKeepsCommandResult)
    {
        StartDrag();
        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_TRUE(m_transition->GetWaypoints().empty());
    }

    TEST_F(WaypointDragTest, UndoTransitionCreationEndsWaypointDrag)
    {
        AZStd::string result;
        auto* manager = CommandSystem::GetCommandManager();
        ASSERT_TRUE(manager->Undo(result)) << result.c_str();
        m_transition->SetWaypoints(original);
        const auto transitionId = m_transition->GetId();
        StartDrag();
        ASSERT_TRUE(manager->Undo(result)) << result.c_str();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_nodeGraph->GetRepositionedWaypointConnection(), nullptr);
        EXPECT_EQ(m_graph->RecursiveFindTransitionById(transitionId), nullptr);

        ASSERT_TRUE(manager->Redo(result)) << result.c_str();
        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
    }

    TEST_F(WaypointDragTest, RemovingStateCancelsWaypointDragBeforeDeletingConnection)
    {
        StartDrag();
        AZStd::string result;
        auto* manager = CommandSystem::GetCommandManager();
        ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
            "AnimGraphRemoveNode -animGraphID %u -name Second", m_graph->GetID()), result)) << result.c_str();

        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_nodeGraph->GetRepositionedWaypointConnection(), nullptr);
        ASSERT_TRUE(manager->Undo(result)) << result.c_str();
        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
    }

}
