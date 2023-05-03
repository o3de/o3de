/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QAction>
#include <QtTest>
#include <QWidget>
#include <qrect.h>

#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <Tests/UI/AnimGraphUIFixture.h>

namespace QTest
{
    extern int Q_TESTLIB_EXPORT defaultMouseDelay();
} // namespace QTest

namespace EMotionFX
{
    TEST_F(AnimGraphUIFixture, CanAddTransitionCondition)
    {
        // This test checks that you can edit a transition between nodes of an animgraph
        RecordProperty("test_case_id", "C21948785");

        // Set up animgraph
        EMotionFX::AnimGraph* animGraph = AnimGraphUIFixture::CreateAnimGraph();
        {
            MCore::CommandGroup group;

            // Add two motion nodes to animgraph
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 200 -name motionNodeA",
                animGraph->GetID(), azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 0 -yPos 0 -name motionNodeB",
                animGraph->GetID(), azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str()));

            // Run Commands
            AZStd::string commandResult;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();
        }

        // Create Needed Objects
        AnimGraphMotionNode* motionNodeA = static_cast<AnimGraphMotionNode*>(animGraph->RecursiveFindNodeByName("motionNodeA"));
        AnimGraphMotionNode* motionNodeB = static_cast<AnimGraphMotionNode*>(animGraph->RecursiveFindNodeByName("motionNodeB"));
        EMStudio::NodeGraph* nodeGraph = AnimGraphUIFixture::GetActiveNodeGraph();
        const EMStudio::GraphNode* graphNodeForMotionNode0 = nodeGraph->FindGraphNode(motionNodeA);
        const EMStudio::GraphNode* graphNodeForMotionNode1 = nodeGraph->FindGraphNode(motionNodeB);
        EMStudio::AnimGraphModel& model = m_animGraphPlugin->GetAnimGraphModel();
        
        // Resize blend graph window to show both nodes
        m_blendGraphWidget->resize(500, 500);
        m_animGraphPlugin->GetViewWidget()->ZoomSelected();
        
        // Add transition between two nodes
        const QPoint begin(graphNodeForMotionNode0->GetFinalRect().topRight() + QPoint(-2, 2));
        const QPoint end(graphNodeForMotionNode1->GetFinalRect().topLeft() + QPoint(2, 2));
        
        QTest::mousePress(static_cast<QWidget*>(m_blendGraphWidget), Qt::LeftButton, Qt::NoModifier, begin);
        {
            // QTest::mouseMove uses QCursor::setPos to generate a MouseMove
            QMouseEvent moveEvent(QMouseEvent::MouseMove, end, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            moveEvent.setTimestamp(QTest::lastMouseTimestamp += QTest::defaultMouseDelay());
            QApplication::instance()->notify(m_blendGraphWidget, &moveEvent);
        }
        QTest::mouseRelease(static_cast<QWidget*>(m_blendGraphWidget), Qt::LeftButton, Qt::NoModifier, end);

        // Ensure the transition was added to the root state machine
        ASSERT_EQ(animGraph->GetRootStateMachine()->GetNumTransitions(), 1);

        // Ensure the transition is in the AnimGraphModel
        AnimGraphStateTransition* transition = animGraph->GetRootStateMachine()->GetTransition(0);
        
        ASSERT_TRUE(transition) << "Transition was made incorrectly";
        const QModelIndexList matches = model.match(
            /* starting index = */ model.index(0, 0, model.index(0, 0)),
            /* role = */ EMStudio::AnimGraphModel::ROLE_POINTER,
            /* value = */ QVariant::fromValue(static_cast<void*>(transition)),
            /* hits =*/ 1,
            Qt::MatchExactly
        );
        ASSERT_EQ(matches.size(), 1);

        // Ensure that the transition has not condinditions on creation
        ASSERT_EQ(transition->GetNumConditions(), 0) << "Transition starts with conditions";

        // Select Transition
        QPoint transitionCenter;
        transitionCenter.setX((((end.x() - begin.x()) / 2) + begin.x()));
        transitionCenter.setY((((end.y() - begin.y()) / 2) + begin.y()));
        QTest::mouseClick(static_cast<QWidget*>(m_blendGraphWidget), Qt::LeftButton, Qt::NoModifier, transitionCenter);

        // Make sure transition was selected
        ASSERT_EQ(nodeGraph->GetSelectedNodeConnections().size(), 1) << "Transition was not selected";

        // Find and "click" on Add Condition button
        EMStudio::AddConditionButton* addConditionButton = m_animGraphPlugin->GetAttributesWindow()->GetAddConditionButton();
        ASSERT_TRUE(addConditionButton) << "Add Condition button was not found";
        addConditionButton->OnCreateContextMenu();

        // Add a specific condition
        QAction* addStateConditionButton = AnimGraphUIFixture::GetNamedAction(m_animGraphPlugin->GetAttributesWindow(), "State Condition");
        ASSERT_TRUE(addStateConditionButton) << "'State Condition' button was not found";
        addStateConditionButton->trigger();

        // Verify that the condition has been added to the transition.
        ASSERT_EQ(transition->GetNumConditions(), 1) << "Transition doesn't have one condition";

        QApplication::processEvents();

    }
}
