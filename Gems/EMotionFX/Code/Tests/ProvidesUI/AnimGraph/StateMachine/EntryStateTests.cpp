/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <gtest/gtest.h>

#include <QAction>
#include <QtTest/QtTest>
#include <qtoolbar.h>
#include <QWidget>
#include <QComboBox>
#include <qrect.h>

#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanSetEntryState)
    {
        // This test checks that you can set the entry state of motion nodes in an animgraph
        RecordProperty("test_case_id", "C1559146");

        AnimGraph* animGraph = nullptr;
        EMStudio::AnimGraphPlugin* animGraphPlugin = nullptr;
        {
            // Set up an empty animgraph with two motion nodes
            const AZ::u32 animGraphId = 64;
            MCore::CommandGroup group;

            // Create empty anim graph, add a motion, entry, hub, and blend tree node.
            group.AddCommandString(AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 200 -yPos 200 -name motionNodeA",
                animGraphId, azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str()));
            group.AddCommandString(AZStd::string::format("AnimGraphCreateNode -animGraphID %d -type %s -parentName Root -xPos 0 -yPos 0 -name motionNodeB",
                animGraphId, azrtti_typeid<AnimGraphMotionNode>().ToString<AZStd::string>().c_str()));

            // Run Commands
            AZStd::string commandResult;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();

            // Get useful components
            animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
            ASSERT_NE(animGraphPlugin, nullptr) << "Anim graph plugin not found.";

            animGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
            ASSERT_NE(animGraph, nullptr) << "Cannot find newly created anim graph.";
        }

        // Create Needed Objects
        const AnimGraphMotionNode* motionNodeA = static_cast<AnimGraphMotionNode*>(animGraph->RecursiveFindNodeByName("motionNodeA"));
        const AnimGraphMotionNode* motionNodeB = static_cast<AnimGraphMotionNode*>(animGraph->RecursiveFindNodeByName("motionNodeB"));
        EMStudio::BlendGraphWidget* graphWidget = animGraphPlugin->GetGraphWidget();
        const EMStudio::GraphNode* graphNodeForMotionNodeA = graphWidget->GetActiveGraph()->FindGraphNode(motionNodeA);
        const EMStudio::GraphNode* graphNodeForMotionNodeB = graphWidget->GetActiveGraph()->FindGraphNode(motionNodeB);

        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(motionNodeA->GetParentNode());

        // Ensure that motionNodeA is the entry node
        ASSERT_EQ(motionNodeA->GetId(), stateMachine->GetEntryStateId()) << "motionNodeA does not start as set entry state.";

        // Select motionNodeB
        EMStudio::NodeGraph* nodeGraph = graphWidget->GetActiveGraph();
        nodeGraph->FitGraphOnScreen(graphWidget->geometry().width(), graphWidget->geometry().height(), graphWidget->GetMousePos(), true);
        const QRect nodeRect = graphNodeForMotionNodeB->GetFinalRect();
        QTest::mouseClick(static_cast<QWidget*>(graphWidget), Qt::LeftButton, Qt::NoModifier, nodeRect.center());
        ASSERT_EQ(nodeGraph->GetSelectedAnimGraphNodes().size(), 1) << "No node is selected";
        ASSERT_EQ(nodeGraph->GetSelectedAnimGraphNodes()[0]->GetNameString(), "motionNodeB") << "Motion Node B was not selected";

        // Right click on motionNodeB
        const AZStd::vector<EMotionFX::AnimGraphNode*> selectedAnimGraphNodes = nodeGraph->GetSelectedAnimGraphNodes();
        graphWidget->OnContextMenuEvent(graphWidget, nodeRect.center(), graphWidget->LocalToGlobal(nodeRect.center()), animGraphPlugin, selectedAnimGraphNodes, true, false, animGraphPlugin->GetActionFilter());

        // Click "Set As Entry State" button
        QAction* setAsEntryPointButton = UIFixture::GetNamedAction(graphWidget, "Set As Entry State");
        ASSERT_TRUE(setAsEntryPointButton) << "Set as entry state button not found.";
        setAsEntryPointButton->trigger();

        // Verify motionNodeA is no longer the entry node
        ASSERT_NE(motionNodeA->GetId(), stateMachine->GetEntryStateId()) << "motionNodeA is still set as entry state.";

        // Verify motionNodeB is the entry node
        ASSERT_EQ(motionNodeB->GetId(), stateMachine->GetEntryStateId()) << "motionNodeB is not set as entry state.";

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}
