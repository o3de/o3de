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
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>

namespace EMotionFX
{
    static QAction* FindBlendTreeMenuAction(const QMenu& menu, const char* text)
    {
        for (QAction* action : menu.actions())
        {
            if (action->text() == QString(text))
            {
                return action;
            }
        }
        return nullptr;
    }

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

    TEST_F(BlendTreeWaypointTest, BentWireIsOnlyHitNearItsLine)
    {
        // Bent down into a U, the wire's path encloses a large area between its line and its two ends.
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, { AZ::Vector2(200.0f, 300.0f), AZ::Vector2(300.0f, 300.0f) });
        const QRect everywhere(-100000, -100000, 200000, 200000);
        m_nodeGraph->FindGraphNode(m_graph->RecursiveFindNodeByName("Weight"))->Update(everywhere, QPoint());
        m_nodeGraph->FindGraphNode(m_blendA)->Update(everywhere, QPoint());
        m_nodeConnectionA->Update(everywhere, QPoint());
        m_nodeConnectionA->UpdatePainterPath();
        ASSERT_TRUE(m_nodeConnectionA->GetPainterPath().contains(QPointF(250.0, 150.0)));

        EXPECT_FALSE(m_nodeConnectionA->CheckIfIsCloseTo(QPoint(250, 150)));
        EXPECT_FALSE(m_nodeConnectionA->Intersects(QRect(240, 140, 20, 20)));
        EXPECT_TRUE(m_nodeConnectionA->CheckIfIsCloseTo(QPoint(250, 300)));
    }

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
        const AZStd::vector<EMStudio::RepositionedWaypoint> drags = m_nodeGraph->GetRepositionedWaypoints();
        m_nodeGraph->StopRepositionWaypoint();
        m_blendGraphWidget->CommitWaypoints(drags);

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

    TEST_F(BlendTreeWaypointTest, AltDraggingAWaypointBranchesANewWireFromIt)
    {
        const AZStd::vector<AZ::Vector2> pins{ AZ::Vector2(150.0f, 50.0f), AZ::Vector2(200.0f, 100.0f) };
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, pins);

        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(AZStd::string::format(
            "AnimGraphCreateNode -animGraphID %u -type %s -parentName Tree -name BlendC -xPos 400 -yPos 300",
            m_graph->GetID(), azrtti_typeid<BlendTreeBlend2Node>().ToString<AZStd::string>().c_str()), result)) << result.c_str();
        AnimGraphNode* blendC = m_graph->RecursiveFindNodeByName("BlendC");
        EMStudio::GraphNode* blendCGraphNode = m_nodeGraph->FindGraphNode(blendC);
        ASSERT_NE(blendCGraphNode, nullptr);

        // Alt-press on the second pin, the way the user grabs it.
        const QPoint local = m_nodeGraph->GetTransform().map(QPoint(200, 100));
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(local), QPointF(m_blendGraphWidget->mapToGlobal(local)),
            Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
        QApplication::sendEvent(m_blendGraphWidget, &press);

        ASSERT_TRUE(m_nodeGraph->GetIsCreatingConnection());
        EXPECT_FALSE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_nodeGraph->GetCreateConnectionNode(), m_nodeConnectionA->GetSourceNode());
        EXPECT_EQ(m_nodeGraph->GetCreateConnectionPortNr(), m_nodeConnectionA->GetOutputPortNr());
        EXPECT_EQ(m_nodeGraph->GetCreateConnectionWaypoints(), pins);

        // Dropping it on a port makes an ordinary connection from the original output, bent through the pins.
        m_blendGraphWidget->OnCreateConnection(m_nodeGraph->GetCreateConnectionPortNr(), m_nodeGraph->GetCreateConnectionNode(), false,
            BlendTreeBlend2Node::INPUTPORT_WEIGHT, blendCGraphNode, true, m_nodeGraph->GetCreateConnectionStartOffset(), QPoint());
        m_nodeGraph->StopCreateConnection();
        EXPECT_TRUE(m_nodeGraph->GetCreateConnectionWaypoints().empty());

        const BlendTreeConnection* branch =
            blendC->FindConnection(m_graph->RecursiveFindNodeByName("Weight"), 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
        ASSERT_NE(branch, nullptr);
        EXPECT_EQ(branch->GetWaypoints(), pins);
        EXPECT_EQ(m_connectionA->GetWaypoints(), pins);
    }

    TEST_F(BlendTreeWaypointTest, AltPressOnAnEarlierPinTakesOnlyThePinsBeforeIt)
    {
        const AZStd::vector<AZ::Vector2> pins{ AZ::Vector2(150.0f, 50.0f), AZ::Vector2(200.0f, 100.0f) };
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, pins);

        const QPoint local = m_nodeGraph->GetTransform().map(QPoint(150, 50));
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(local), QPointF(m_blendGraphWidget->mapToGlobal(local)),
            Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
        QApplication::sendEvent(m_blendGraphWidget, &press);

        ASSERT_TRUE(m_nodeGraph->GetIsCreatingConnection());
        EXPECT_EQ(m_nodeGraph->GetCreateConnectionWaypoints(), AZStd::vector<AZ::Vector2>{ pins[0] });
        m_nodeGraph->StopCreateConnection();
    }

    TEST_F(BlendTreeWaypointTest, PlainPressOnAWaypointStillMovesIt)
    {
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, m_original);

        const QPoint local = m_nodeGraph->GetTransform().map(QPoint(200, 100));
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(local), QPointF(m_blendGraphWidget->mapToGlobal(local)),
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(m_blendGraphWidget, &press);

        EXPECT_TRUE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_FALSE(m_nodeGraph->GetIsCreatingConnection());
        m_nodeGraph->CancelRepositionWaypoint();
    }

    TEST_F(BlendTreeWaypointTest, SharedPinMovesAndUndoesForEveryWire)
    {
        Select({ m_nodeConnectionA, m_nodeConnectionB });
        m_blendGraphWidget->InsertWaypointOnSelectedConnections();
        ASSERT_EQ(m_connectionA->GetWaypoints().size(), size_t{ 1 });
        const AZStd::vector<AZ::Vector2> pinned = m_connectionA->GetWaypoints();

        // Grabbing the pin on one wire drags the same pin on the other wire too.
        m_nodeGraph->StartRepositionWaypoint(m_nodeConnectionA, 0, pinned);
        ASSERT_EQ(m_nodeGraph->GetRepositionedWaypoints().size(), size_t{ 2 });
        m_nodeGraph->MoveRepositionedWaypoints(m_dragged[0]);
        EXPECT_EQ(m_connectionA->GetWaypoints(), m_dragged);
        EXPECT_EQ(m_connectionB->GetWaypoints(), m_dragged);

        const AZStd::vector<EMStudio::RepositionedWaypoint> drags = m_nodeGraph->GetRepositionedWaypoints();
        m_nodeGraph->StopRepositionWaypoint();
        m_blendGraphWidget->CommitWaypoints(drags);
        EXPECT_EQ(m_connectionA->GetWaypoints(), m_dragged);
        EXPECT_EQ(m_connectionB->GetWaypoints(), m_dragged);

        // One move is one undo step for both wires.
        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(m_connectionA->GetWaypoints(), pinned);
        EXPECT_EQ(m_connectionB->GetWaypoints(), pinned);
    }

    TEST_F(BlendTreeWaypointTest, CancellingASharedPinDragRestoresEveryWire)
    {
        Select({ m_nodeConnectionA, m_nodeConnectionB });
        m_blendGraphWidget->InsertWaypointOnSelectedConnections();
        const AZStd::vector<AZ::Vector2> pinned = m_connectionA->GetWaypoints();

        m_nodeGraph->StartRepositionWaypoint(m_nodeConnectionB, 0, pinned);
        m_nodeGraph->MoveRepositionedWaypoints(m_dragged[0]);
        m_nodeGraph->CancelRepositionWaypoint();

        EXPECT_EQ(m_connectionA->GetWaypoints(), pinned);
        EXPECT_EQ(m_connectionB->GetWaypoints(), pinned);
    }

    TEST_F(BlendTreeWaypointTest, PinsOnlyShareWithinOneOutput)
    {
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, m_original);
        CommandSystem::AdjustConnectionWaypoints(m_blendB, m_connectionB, m_original);

        AZStd::vector<AZStd::pair<EMStudio::NodeConnection*, size_t>> shared;
        m_nodeGraph->FindSharedWaypoints(m_nodeConnectionA, 0, shared);
        ASSERT_EQ(shared.size(), size_t{ 2 });
        EXPECT_EQ(shared[0].first, m_nodeConnectionA);
        EXPECT_EQ(shared[1].first, m_nodeConnectionB);

        // A pin somewhere else on the other wire is not the same pin.
        CommandSystem::AdjustConnectionWaypoints(m_blendB, m_connectionB, m_dragged);
        m_nodeGraph->FindSharedWaypoints(m_nodeConnectionA, 0, shared);
        EXPECT_EQ(shared.size(), size_t{ 1 });
    }

    TEST_F(BlendTreeWaypointTest, WireBranchedFromAPinSharesIt)
    {
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, m_original);

        // Branch from A's pin into B's weight input, which replaces B's direct wire.
        m_nodeGraph->StartCreateConnection(m_nodeConnectionA->GetOutputPortNr(), false, m_nodeConnectionA->GetSourceNode(),
            m_nodeConnectionA->GetSourceNode()->GetOutputPort(m_nodeConnectionA->GetOutputPortNr()), QPoint());
        m_nodeGraph->SetCreateConnectionWaypoints(m_original);
        m_blendGraphWidget->OnCreateConnection(m_nodeConnectionA->GetOutputPortNr(), m_nodeConnectionA->GetSourceNode(), false,
            BlendTreeBlend2Node::INPUTPORT_WEIGHT, m_nodeGraph->FindGraphNode(m_blendB), true, QPoint(), QPoint());
        m_nodeGraph->StopCreateConnection();

        const BlendTreeConnection* branch =
            m_blendB->FindConnection(m_graph->RecursiveFindNodeByName("Weight"), 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
        ASSERT_NE(branch, nullptr);
        EXPECT_EQ(branch->GetWaypoints(), m_original);

        // Moving the pin from the original wire takes the branch along.
        m_nodeGraph->StartRepositionWaypoint(m_nodeConnectionA, 0, m_original);
        EXPECT_EQ(m_nodeGraph->GetRepositionedWaypoints().size(), size_t{ 2 });
        m_nodeGraph->MoveRepositionedWaypoints(m_dragged[0]);
        EXPECT_EQ(branch->GetWaypoints(), m_dragged);
        m_nodeGraph->CancelRepositionWaypoint();
        EXPECT_EQ(branch->GetWaypoints(), m_original);
    }

    TEST_F(BlendTreeWaypointTest, MenuOffersSharedAndOwnWaypointsForWiresFromOneOutput)
    {
        QMenu menu;
        m_blendGraphWidget->AddWaypointMenuEntries(menu, { m_nodeConnectionA, m_nodeConnectionB }, QPoint(200, 100));

        EXPECT_NE(FindBlendTreeMenuAction(menu, "Add shared waypoint"), nullptr);
        QAction* eachAction = FindBlendTreeMenuAction(menu, "Add waypoint to each");
        ASSERT_NE(eachAction, nullptr);

        eachAction->trigger();
        EXPECT_EQ(m_connectionA->GetWaypoints().size(), size_t{ 1 });
        EXPECT_EQ(m_connectionB->GetWaypoints().size(), size_t{ 1 });
    }

    TEST_F(BlendTreeWaypointTest, WiresFromDifferentOutputsEachGetTheirOwnWaypoint)
    {
        // Feed BlendB from a second constant, so the two selected wires leave different outputs.
        AZStd::string result;
        auto* manager = CommandSystem::GetCommandManager();
        ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
            "AnimGraphCreateNode -animGraphID %u -type %s -parentName Tree -name OtherWeight -xPos 0 -yPos 300",
            m_graph->GetID(), azrtti_typeid<BlendTreeFloatConstantNode>().ToString<AZStd::string>().c_str()), result)) << result.c_str();
        ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
            "AnimGraphRemoveConnection -animGraphID %u -targetNode BlendB -targetPort %d -sourceNode Weight -sourcePort 0",
            m_graph->GetID(), BlendTreeBlend2Node::INPUTPORT_WEIGHT), result)) << result.c_str();
        ASSERT_TRUE(manager->ExecuteCommand(AZStd::string::format(
            "AnimGraphCreateConnection -animGraphID %u -sourceNode OtherWeight -targetNode BlendB -sourcePort 0 -targetPort %d",
            m_graph->GetID(), BlendTreeBlend2Node::INPUTPORT_WEIGHT), result)) << result.c_str();

        BlendTreeConnection* otherConnection = m_blendB->FindConnection(
            m_graph->RecursiveFindNodeByName("OtherWeight"), 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
        ASSERT_NE(otherConnection, nullptr);
        EMStudio::NodeConnection* otherNodeConnection = FindNodeConnection(otherConnection);
        ASSERT_NE(otherNodeConnection, nullptr);

        QMenu menu;
        m_blendGraphWidget->AddWaypointMenuEntries(menu, { m_nodeConnectionA, otherNodeConnection }, QPoint(200, 100));
        EXPECT_EQ(FindBlendTreeMenuAction(menu, "Add shared waypoint"), nullptr);
        QAction* eachAction = FindBlendTreeMenuAction(menu, "Add waypoint to each");
        ASSERT_NE(eachAction, nullptr);

        eachAction->trigger();
        ASSERT_EQ(m_connectionA->GetWaypoints().size(), size_t{ 1 });
        ASSERT_EQ(otherConnection->GetWaypoints().size(), size_t{ 1 });
        EXPECT_NE(m_connectionA->GetWaypoints(), otherConnection->GetWaypoints());

        // The shortcut does the same.
        Select({ m_nodeConnectionA, otherNodeConnection });
        ASSERT_TRUE(m_blendGraphWidget->CanInsertWaypointOnSelectedConnections());
        m_blendGraphWidget->InsertWaypointOnSelectedConnections();
        EXPECT_EQ(m_connectionA->GetWaypoints().size(), size_t{ 2 });
        EXPECT_EQ(otherConnection->GetWaypoints().size(), size_t{ 2 });
    }

    class BlendTreePinnedWireTest : public BlendTreeWaypointTest
    {
    protected:
        void SetUp() override
        {
            BlendTreeWaypointTest::SetUp();
            m_weight = m_graph->RecursiveFindNodeByName("Weight");
            m_weightGraphNode = m_nodeGraph->FindGraphNode(m_weight);
            ASSERT_NE(m_weightGraphNode, nullptr);
        }

        void PressAt(const QPoint& graphPos, Qt::KeyboardModifiers modifiers)
        {
            const QPoint local = m_nodeGraph->GetTransform().map(graphPos);
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(local), QPointF(m_blendGraphWidget->mapToGlobal(local)),
                Qt::LeftButton, Qt::LeftButton, modifiers);
            QApplication::sendEvent(m_blendGraphWidget, &press);
        }

        AnimGraphNode* m_weight = nullptr;
        EMStudio::GraphNode* m_weightGraphNode = nullptr;
        const AZ::Vector2 m_bend{ 150.0f, 250.0f };
        const AZ::Vector2 m_pin{ 300.0f, 400.0f };
    };

    TEST_F(BlendTreePinnedWireTest, AltDropOnEmptySpaceLeavesAPinnedWire)
    {
        m_nodeGraph->StartCreateConnection(0, false, m_weightGraphNode, m_weightGraphNode->GetOutputPort(0), QPoint());
        m_blendGraphWidget->OnCreatePinnedWire(0, m_weightGraphNode, QPoint(300, 400));
        m_nodeGraph->StopCreateConnection();

        ASSERT_EQ(m_weight->GetPinnedWires().size(), size_t{ 1 });
        EXPECT_EQ(m_weight->GetPinnedWires()[0], BlendTreePinnedWire(0, { m_pin }));
        EXPECT_NE(m_nodeGraph->FindPinnedWire(m_weight, 0), nullptr);

        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_TRUE(m_weight->GetPinnedWires().empty());
        EXPECT_EQ(m_nodeGraph->FindPinnedWire(m_weight, 0), nullptr);
    }

    TEST_F(BlendTreePinnedWireTest, DroppingBackOnThePinItCameFromMakesNothing)
    {
        m_weight->SetPinnedWires({ BlendTreePinnedWire(0, { m_pin }) });

        m_nodeGraph->StartCreateConnection(0, false, m_weightGraphNode, m_weightGraphNode->GetOutputPort(0), QPoint());
        m_nodeGraph->SetCreateConnectionWaypoints({ m_pin });
        m_blendGraphWidget->OnCreatePinnedWire(0, m_weightGraphNode, QPoint(300, 400));
        m_nodeGraph->StopCreateConnection();

        EXPECT_EQ(m_weight->GetPinnedWires().size(), size_t{ 1 });
    }

    TEST_F(BlendTreePinnedWireTest, PinnedWireIsFoundAtItsPinAndRoutesToIt)
    {
        m_weight->SetPinnedWires({ BlendTreePinnedWire(0, { m_bend, m_pin }) });

        size_t waypointIndex = InvalidIndex;
        EMStudio::PinnedWire* pinnedWire = m_nodeGraph->FindPinnedWireWaypoint(QPoint(300, 400), waypointIndex);
        ASSERT_NE(pinnedWire, nullptr);
        EXPECT_EQ(waypointIndex, size_t{ 1 });
        EXPECT_EQ(m_nodeGraph->FindWaypoint(QPoint(150, 250), waypointIndex), pinnedWire);
        EXPECT_EQ(waypointIndex, size_t{ 0 });

        AZStd::vector<QPoint> path;
        pinnedWire->CalcPolyline(path);
        ASSERT_EQ(path.size(), size_t{ 3 });
        EXPECT_EQ(path.back(), QPoint(300, 400));
    }

    TEST_F(BlendTreePinnedWireTest, AltDragFromAPinBranchesAWireAndKeepsTheStub)
    {
        m_weight->SetPinnedWires({ BlendTreePinnedWire(0, { m_pin }) });

        PressAt(QPoint(300, 400), Qt::AltModifier);
        ASSERT_TRUE(m_nodeGraph->GetIsCreatingConnection());
        EXPECT_EQ(m_nodeGraph->GetCreateConnectionNode(), m_weightGraphNode);
        EXPECT_EQ(m_nodeGraph->GetCreateConnectionWaypoints(), AZStd::vector<AZ::Vector2>{ m_pin });

        // Drop it on BlendB's weight input, which replaces BlendB's direct wire.
        m_blendGraphWidget->OnCreateConnection(0, m_weightGraphNode, false,
            BlendTreeBlend2Node::INPUTPORT_WEIGHT, m_nodeGraph->FindGraphNode(m_blendB), true, m_nodeGraph->GetCreateConnectionStartOffset(), QPoint());
        m_nodeGraph->StopCreateConnection();

        const BlendTreeConnection* branch = m_blendB->FindConnection(m_weight, 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
        ASSERT_NE(branch, nullptr);
        EXPECT_EQ(branch->GetWaypoints(), AZStd::vector<AZ::Vector2>{ m_pin });
        ASSERT_EQ(m_weight->GetPinnedWires().size(), size_t{ 1 });

        // The stub and the branch share the pin, so moving it from the stub moves both.
        EMStudio::PinnedWire* stub = m_nodeGraph->FindPinnedWire(m_weight, 0);
        ASSERT_NE(stub, nullptr);
        m_nodeGraph->StartRepositionWaypoint(stub, 0, *stub->GetWaypoints());
        EXPECT_EQ(m_nodeGraph->GetRepositionedWaypoints().size(), size_t{ 2 });
        m_nodeGraph->MoveRepositionedWaypoints(m_bend);
        EXPECT_EQ(m_weight->GetPinnedWires()[0].m_waypoints, AZStd::vector<AZ::Vector2>{ m_bend });
        EXPECT_EQ(branch->GetWaypoints(), AZStd::vector<AZ::Vector2>{ m_bend });
        m_nodeGraph->CancelRepositionWaypoint();
        EXPECT_EQ(branch->GetWaypoints(), AZStd::vector<AZ::Vector2>{ m_pin });
    }

    TEST_F(BlendTreePinnedWireTest, BranchFollowsThePinnedWireCurveExactly)
    {
        // A connection bent through the pin, as a branch off the pinned wire is.
        m_weight->SetPinnedWires({ BlendTreePinnedWire(0, { m_pin }) });
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, { m_pin });

        EMStudio::PinnedWire* stub = m_nodeGraph->FindPinnedWire(m_weight, 0);
        ASSERT_NE(stub, nullptr);
        stub->UpdatePainterPath();
        m_nodeConnectionA->UpdatePainterPath();
        const QPainterPath& stubPath = stub->GetPainterPath();
        const QPainterPath& branchPath = m_nodeConnectionA->GetPainterPath();

        // The stub is the branch's path up to the pin: move, step off the port, and one curve that ends exactly on the pin.
        ASSERT_EQ(stubPath.elementCount(), 5);
        ASSERT_GT(branchPath.elementCount(), stubPath.elementCount());
        for (int i = 0; i < stubPath.elementCount(); ++i)
        {
            EXPECT_EQ(QPointF(stubPath.elementAt(i)), QPointF(branchPath.elementAt(i))) << "path element " << i;
        }
        EXPECT_EQ(stubPath.currentPosition(), QPointF(300.0, 400.0));
    }

    TEST_F(BlendTreePinnedWireTest, PlainDragMovesThePinInOneUndoStep)
    {
        m_weight->SetPinnedWires({ BlendTreePinnedWire(0, { m_pin }) });

        PressAt(QPoint(300, 400), Qt::NoModifier);
        ASSERT_TRUE(m_nodeGraph->GetIsRepositioningWaypoint());
        EXPECT_EQ(m_nodeGraph->GetRepositionedWaypointConnection()->GetType(), static_cast<uint32>(EMStudio::PinnedWire::TYPE_ID));
        m_nodeGraph->MoveRepositionedWaypoints(m_bend);

        const AZStd::vector<EMStudio::RepositionedWaypoint> drags = m_nodeGraph->GetRepositionedWaypoints();
        m_nodeGraph->StopRepositionWaypoint();
        m_blendGraphWidget->CommitWaypoints(drags);
        EXPECT_EQ(m_weight->GetPinnedWires()[0].m_waypoints, AZStd::vector<AZ::Vector2>{ m_bend });

        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(m_weight->GetPinnedWires()[0].m_waypoints, AZStd::vector<AZ::Vector2>{ m_pin });
    }

    TEST_F(BlendTreePinnedWireTest, MenuRemovesWaypointsAndThePinnedWireButNotItsBranches)
    {
        m_weight->SetPinnedWires({ BlendTreePinnedWire(0, { m_bend, m_pin }) });
        CommandSystem::AdjustConnectionWaypoints(m_blendA, m_connectionA, { m_bend, m_pin });

        {
            QMenu menu;
            m_blendGraphWidget->AddPinnedWireMenuEntries(menu, m_nodeGraph->FindPinnedWire(m_weight, 0), 0);
            EXPECT_NE(FindBlendTreeMenuAction(menu, "Remove pinned wire"), nullptr);
            QAction* removeWaypoint = FindBlendTreeMenuAction(menu, "Remove waypoint");
            ASSERT_NE(removeWaypoint, nullptr);

            // The bend is shared with the branch, so it goes from both.
            removeWaypoint->trigger();
            EXPECT_EQ(m_weight->GetPinnedWires()[0].m_waypoints, AZStd::vector<AZ::Vector2>{ m_pin });
            EXPECT_EQ(m_connectionA->GetWaypoints(), AZStd::vector<AZ::Vector2>{ m_pin });
        }

        {
            // The pin itself is only removed with the whole pinned wire.
            QMenu menu;
            m_blendGraphWidget->AddPinnedWireMenuEntries(menu, m_nodeGraph->FindPinnedWire(m_weight, 0), 0);
            EXPECT_EQ(FindBlendTreeMenuAction(menu, "Remove waypoint"), nullptr);
            QAction* removePinnedWire = FindBlendTreeMenuAction(menu, "Remove pinned wire");
            ASSERT_NE(removePinnedWire, nullptr);

            removePinnedWire->trigger();
            EXPECT_TRUE(m_weight->GetPinnedWires().empty());
            EXPECT_EQ(m_connectionA->GetWaypoints(), AZStd::vector<AZ::Vector2>{ m_pin });
        }

        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << result.c_str();
        EXPECT_EQ(m_weight->GetPinnedWires().size(), size_t{ 1 });
    }
} // namespace EMotionFX
