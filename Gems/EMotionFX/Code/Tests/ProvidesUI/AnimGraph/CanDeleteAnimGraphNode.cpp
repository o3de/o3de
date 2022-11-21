/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QApplication>
#include <QAction>
#include <QtTest>

#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>

#include <EMotionFX/Source/AnimGraphBindPoseNode.h>

namespace EMotionFX {

    TEST_F(AnimGraphUIFixture, CanDeleteAnimGraphNode)
    {
        /*
        Test Case: C22083484
        Can Delete AnimGraph Node.
        Tests the UI functionality for deleting an existing AnimGraph Node via the right-click context menu.
        */

        RecordProperty("test_case_id", "C22083484");

        AnimGraph* activeAnimGraph = CreateAnimGraph();

        // Create a node to delete later
        AnimGraphNode* node = CreateAnimGraphNode(azrtti_typeid<AnimGraphBindPoseNode>().ToString<AZStd::string>(), "-parentName Root -xPos 1 -yPos 1");
        ASSERT_TRUE(node) << "Node was not created.";

        // Verify no nodes are selecetd yet
        EXPECT_EQ(m_blendGraphWidget->CalcNumSelectedNodes(), 0) << "Expected to have exactly zero (0) selected nodes";
        EXPECT_EQ(m_blendGraphWidget->GetActiveGraph()->GetSelectedAnimGraphNodes().size(), 0) << "Expected to have zero (0) items selected in selection model";

        // Select the new Graph Node (via left mouse click)
        QRect nodeRect = m_blendGraphWidget->GetActiveGraph()->FindGraphNode(node)->GetRect();
        QPoint localPoint = nodeRect.center();
        QTest::mouseClick(m_blendGraphWidget, Qt::LeftButton, Qt::NoModifier, localPoint);

        // Verify our node was selected
        EXPECT_EQ(m_blendGraphWidget->CalcNumSelectedNodes(), 1) << "Expected to have exactly one (1) selected node";
        EXPECT_EQ(m_blendGraphWidget->GetActiveGraph()->GetSelectedAnimGraphNodes().size(), 1) << "Not exactly one selection made";

        // Open context menu
        m_blendGraphWidget->OnContextMenuEvent(m_blendGraphWidget, localPoint, m_blendGraphWidget->LocalToGlobal(localPoint), m_animGraphPlugin, m_blendGraphWidget->GetActiveGraph()->GetSelectedAnimGraphNodes(), true, false, m_animGraphPlugin->GetActionFilter());

        // Find Action for deleting node
        QAction* deleteAction = GetNamedAction(m_animGraphPlugin->GetViewWidget(), FromStdString(EMStudio::AnimGraphPlugin::s_deleteSelectedNodesShortcutName));
        ASSERT_TRUE(deleteAction) << "Could not find the '" <<
            std::string(EMStudio::AnimGraphPlugin::s_deleteSelectedNodesShortcutName.data(), EMStudio::AnimGraphPlugin::s_deleteSelectedNodesShortcutName.size()) 
            << "' action in the context menu";

        // Trigger delete
        const size_t nodeCount = activeAnimGraph->GetNumNodes();
        deleteAction->trigger();

        // Verify Result
        EXPECT_EQ(nodeCount - 1, activeAnimGraph->GetNumNodes()) << "Delete Action from Context menu did not remove exactly 1 Node from AnimGraph";
        EXPECT_EQ(m_blendGraphWidget->CalcNumSelectedNodes(), 0) << "BlendGraphWidget still has the deleted node selected";
        EXPECT_EQ(m_blendGraphWidget->GetActiveGraph()->GetSelectedAnimGraphNodes().size(), 0) << "Selection model not cleared";
    }
}
