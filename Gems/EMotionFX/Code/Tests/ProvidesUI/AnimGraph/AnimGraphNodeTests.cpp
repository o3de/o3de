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
#include <QtTest>
#include <qtoolbar.h>
#include <QWidget>
#include <QComboBox>
#include <qrect.h>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>


namespace EMotionFX
{
    TEST_F(UIFixture, CanAddAnimGraphNode)
    {
        // This test checks that you can add a node to an animgraph
        RecordProperty("test_case_id", "C22083482");

        // Set up Animgraph
        AnimGraph* animGraph = nullptr;
        EMStudio::AnimGraphPlugin* animGraphPlugin = nullptr;
        {
            // Set up an empty animgraph
            const AZ::u32 animGraphId = 64;
            MCore::CommandGroup group;

            // Create empty anim graph
            group.AddCommandString(AZStd::string::format("CreateAnimGraph -animGraphID %d", animGraphId));

            // Run Commands
            AZStd::string commandResult;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, commandResult)) << commandResult.c_str();

            // Get useful components
            animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
            ASSERT_NE(animGraphPlugin, nullptr) << "Anim graph plugin not found.";

            animGraph = GetAnimGraphManager().FindAnimGraphByID(animGraphId);
            ASSERT_NE(animGraph, nullptr) << "Cannot find newly created anim graph.";
        }

        // Grab needed objects
        EMStudio::BlendGraphWidget* graphWidget = animGraphPlugin->GetGraphWidget();
        const AZStd::vector<EMotionFX::AnimGraphNode*> selectedAnimGraphNodes;

        // Check that there is no node before action
        EMotionFX::AnimGraphNode* parentNode = graphWidget->GetActiveGraph()->GetModelIndex().data(EMStudio::AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        ASSERT_EQ(parentNode->GetNumChildNodes(), 0) << "Node was not created according to root node";

        // Nodes to test addition capabilities
        const AZStd::vector<QString> graphNodeTypeNames{"Motion", "Entry", "Hub"};

        for (QString nodeName : graphNodeTypeNames)
        {
            // Right Click on GraphWidget
            const QRect graphRect = graphWidget->rect();
            graphWidget->OnContextMenuEvent(graphWidget, graphRect.center(), graphWidget->LocalToGlobal(graphRect.center()), animGraphPlugin, selectedAnimGraphNodes, true, false, animGraphPlugin->GetActionFilter());

            // Grab the add node action from the graphWidget context menu
            QAction* addNodeAction = UIFixture::GetNamedAction(graphWidget, nodeName);
            ASSERT_TRUE(addNodeAction) << "Add  node action not found.";
            addNodeAction->trigger();
        }

        // Make sure animgraph node was created
        ASSERT_EQ(parentNode->GetNumChildNodes(), graphNodeTypeNames.size()) << "Node was not created according to root node";

        // Make sure animgraph node was created via models
        for (QString nodeName : graphNodeTypeNames)
        {
            const AZStd::string actualName = (nodeName.toStdString() + "0").c_str();
            AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(actualName.c_str());
            ASSERT_NE(animGraphNode, nullptr) << "Motion Node was not found by animpraph object";

            EMStudio::AnimGraphModel& animGraphModel = animGraphPlugin->GetAnimGraphModel();
            const QModelIndex nodeModelIndex = animGraphModel.FindFirstModelIndex(animGraphNode);
            ASSERT_TRUE(nodeModelIndex.isValid()) << "Node was not created according to model";
        }
    
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}
