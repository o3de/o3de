/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QAction>
#include <QtTest>

#include <Tests/UI/MenuUIFixture.h>
#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>

namespace EMotionFX
{
    TEST_F(AnimGraphUIFixture, CanUseEditMenu)
    {
        RecordProperty("test_case_id", "C1698602 ");

        // Find the Edit menu.
        QMenu* editMenu = MenuUIFixture::FindMainMenuWithName("EMFX.MainWindow.EditMenu");
        ASSERT_TRUE(editMenu) << "Unable to find edit menu.";

        EMotionFX::AnimGraph*animGraph = CreateAnimGraph();
        ASSERT_TRUE(animGraph);

        EMStudio::NodeGraph* nodeGraph = GetActiveNodeGraph();
        ASSERT_TRUE(nodeGraph);

        EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(EMStudio::AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        ASSERT_TRUE(currentNode) << "No current AnimGraphNode found";

        // Create an anim graph node, so we have something to undo.
        CommandSystem::CreateAnimGraphNode(/*commandGroup=*/nullptr, animGraph, azrtti_typeid<EMotionFX::AnimGraphReferenceNode>(), "Reference", currentNode, 0, 0);
        
        // Check the expected node now exists.
        uint32 numNodes = currentNode->GetNumChildNodes();
        EXPECT_EQ(1, numNodes);

        // Undo.
        QAction* undoAction = MenuUIFixture::FindMenuActionWithObjectName(editMenu, "EMFX.MainWindow.UndoAction", "EMFX.MainWindow.EditMenu");
        ASSERT_TRUE(undoAction);
        undoAction->trigger();

        const uint32 numNodesAfterUndo = currentNode->GetNumChildNodes();
        ASSERT_EQ(numNodesAfterUndo, numNodes - 1);

        // Redo.
        QAction* redoAction = MenuUIFixture::FindMenuActionWithObjectName(editMenu, "EMFX.MainWindow.RedoAction", "EMFX.MainWindow.EditMenu");
        ASSERT_TRUE(redoAction);
        redoAction->trigger();

        const uint32 numNodesAfterRedo = currentNode->GetNumChildNodes();
        ASSERT_EQ(numNodesAfterRedo, numNodesAfterUndo + 1);
    }
} // namespace EMotionFX
