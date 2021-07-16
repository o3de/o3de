/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <iostream>
#include <cstdlib>
#include <QAction>
#include <QtTest>
#include <QWidget>
#include <QRect>
#include <QComboBox>

#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <Tests/UI/AnimGraphUIFixture.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <Source/Editor/ObjectEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

namespace QTest
{
    extern int Q_TESTLIB_EXPORT defaultMouseDelay();
} // namespace QTest

namespace EMotionFX
{
    TEST_F(AnimGraphUIFixture, CanEditTransition)
    {
        // This test checks that you can edit a transition between nodes of an animgraph
        RecordProperty("test_case_id", "C21948785");

        // Set up animgraph
        EMotionFX::AnimGraph* animGraph = AnimGraphUIFixture::CreateAnimGraph();
        {
            // Place two motion nodes
            MCore::CommandGroup group;

            // Create empty anim graph, add a motion, entry, hub, and  blend tree node.
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
        EMStudio::AttributesWindow* attributesWindow = m_animGraphPlugin->GetAttributesWindow();

        // Add transition between two nodes
        m_blendGraphWidget->resize(500, 500);
        m_animGraphPlugin->GetViewWidget()->ZoomSelected();

        const QPoint begin(graphNodeForMotionNode0->GetFinalRect().topRight() + QPoint(-2, 2));
        const QPoint end(graphNodeForMotionNode1->GetFinalRect().topLeft() + QPoint(2, 2));

        QTest::mousePress(static_cast<QWidget*>(m_blendGraphWidget), Qt::LeftButton, Qt::NoModifier, begin);
        {
            // QTest::mouseMove uses QCursor::setPos to generate a MouseMove
            // event to send to the resulting widget. This won't happen if the
            // widget isn't visible. So we need to send the event directly.
            QMouseEvent moveEvent(QMouseEvent::MouseMove, end, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            moveEvent.setTimestamp(QTest::lastMouseTimestamp += QTest::defaultMouseDelay());
            QApplication::instance()->notify(m_blendGraphWidget, &moveEvent);
        }
        QTest::mouseRelease(static_cast<QWidget*>(m_blendGraphWidget), Qt::LeftButton, Qt::NoModifier, end);

        // Ensure the transition was added to the root state machine
        ASSERT_EQ(animGraph->GetRootStateMachine()->GetNumTransitions(), 1);

        // Ensure the transition is in the AnimGraphModel
        AnimGraphStateTransition* transition = animGraph->GetRootStateMachine()->GetTransition(0);
        const QModelIndexList matches = model.match(
            /* starting index = */ model.index(0, 0, model.index(0, 0)),
            /* role = */ EMStudio::AnimGraphModel::ROLE_POINTER,
            /* value = */ QVariant::fromValue(static_cast<void*>(transition)),
            /* hits =*/ 1,
            Qt::MatchExactly
        );
        ASSERT_EQ(matches.size(), 1);

        // Select Transition
        QPoint transitionCenter;
        transitionCenter.setX((((end.x() - begin.x()) / 2) + begin.x()));
        transitionCenter.setY((((end.y() - begin.y()) / 2) + begin.y()));

        QTest::mouseClick(static_cast<QWidget*>(m_blendGraphWidget), Qt::LeftButton, Qt::NoModifier, transitionCenter);

        // Make sure transition was selected
        ASSERT_EQ(nodeGraph->GetSelectedNodeConnections().size(), 1) << "Transition was not selected";

        ASSERT_EQ(transition->GetSyncMode(), 0) << "Transition did not start with the expected sync mode";
        ASSERT_EQ(transition->GetInterpolationType(), 0) << "Transition did not start with the expected interpolation type";

        using WidgetListItem = AZStd::pair<AzToolsFramework::InstanceDataNode*, AzToolsFramework::PropertyRowWidget*>;
        using WidgetList = AZStd::unordered_map<AzToolsFramework::InstanceDataNode*, AzToolsFramework::PropertyRowWidget*>;
        
        auto objectEditor = attributesWindow->findChild<ObjectEditor*>("EMFX.AttributesWindow.ObjectEditor");
        auto propertyEditor = objectEditor->findChild<AzToolsFramework::ReflectedPropertyEditor*>("PropertyEditor");
        const WidgetList& list = propertyEditor->GetWidgets();

        // Look for PropertyRowWidget for "Sync mode"
        AzToolsFramework::PropertyRowWidget* syncPropertyRow = nullptr;
        for (const WidgetListItem& item : list) {
            if (item.second->objectName() == "Sync mode")
            {
                syncPropertyRow = item.second;
            }
        }
        ASSERT_TRUE(syncPropertyRow) << "Could not find the 'Sync mode' PropertyRowWidget";

        // Edit value in property combo box
        auto comboBox = reinterpret_cast<QComboBox*>(syncPropertyRow->GetChildWidget()->children()[1]);
        ASSERT_TRUE(comboBox) << "Sync Mode Combo box not found";
        comboBox->setCurrentIndex(1);

        // Look for PropertyRowWidget for "Interpolation"
        AzToolsFramework::PropertyRowWidget* interpolationPropertyRow = nullptr;
        for (const WidgetListItem& item : list) {
            if (item.second->objectName() == "Interpolation")
            {
                interpolationPropertyRow = item.second;
            }
        }
        ASSERT_TRUE(interpolationPropertyRow) << "Could not find the 'Interpolation' PropertyRowWidget";

        // Edit value in property combo box
        auto comboBox2 = reinterpret_cast<QComboBox*>(interpolationPropertyRow->GetChildWidget()->children()[1]);
        ASSERT_TRUE(comboBox2) << "Sync Mode Combo box not found";
        comboBox2->setCurrentIndex(1);

        //Check that the transition has been edited properly
        ASSERT_EQ(transition->GetSyncMode(), 1) << "Transition did not start with the expected sync mode";
        ASSERT_EQ(transition->GetInterpolationType(), 1) << "Transition did not start with the expected interpolation type";

        QApplication::processEvents();
    }
} // namespace EMotionFX
