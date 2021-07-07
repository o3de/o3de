/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <QPushButton>
#include <QAction>
#include <QtTest>
#include <Tests/UI/AnimGraphUIFixture.h>
#include <Tests/ProvidesUI/AnimGraph/PreviewMotionFixture.h>

namespace EMotionFX
{
    TEST_F(PreviewMotionFixture, PreviewMotionTests)
    {
        AnimGraph* animGraph = CreateAnimGraph();
        EXPECT_NE(animGraph, nullptr) << "Cannot find newly created anim graph.";

        auto nodeGraph = GetActiveNodeGraph();
        EXPECT_TRUE(nodeGraph) << "Node graph not found.";

        // Serialize motion node members, the motion id for rin_idle.
        EMotionFX::AnimGraphMotionNode* tempMotionNode = aznew EMotionFX::AnimGraphMotionNode();
        tempMotionNode->SetMotionIds({ "rin_idle" });
        AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::SerializeMembersExcept(tempMotionNode, {});
        
        EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(EMStudio::AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        ASSERT_TRUE(currentNode) << "No current AnimGraphNode found";
        CommandSystem::CreateAnimGraphNode(/*commandGroup=*/nullptr, currentNode->GetAnimGraph(), azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", currentNode, 0, 0, serializedMotionNode.GetValue());

        // Check motion node has been created.
        AnimGraphMotionNode* motionNode = static_cast<AnimGraphMotionNode*>(animGraph->GetNode(1));
        EXPECT_NE(motionNode, nullptr) << "Cannot find newly created motion node.";

        nodeGraph->SelectAllNodes();
        const AZStd::vector<EMotionFX::AnimGraphNode*> selectedAnimGraphNodes = nodeGraph->GetSelectedAnimGraphNodes();
        m_blendGraphWidget->OnContextMenuEvent(m_blendGraphWidget, QPoint(0, 0), QPoint(0, 0), m_animGraphPlugin, selectedAnimGraphNodes, true, false, m_animGraphPlugin->GetActionFilter());

        // Check that Preview Motion action is available in the context menu.
        QMenu* selectedNodeContextMenu = m_blendGraphWidget->findChild<QMenu*>("BlendGraphWidget.SelectedNodeMenu");
        ASSERT_TRUE(selectedNodeContextMenu) << "Selected node context menu was not found.";
        QAction* previewMotionAction = GetNamedAction(selectedNodeContextMenu, QString("Preview rin_idle"));
        ASSERT_TRUE(previewMotionAction) << "Preview motion action not found.";
        delete tempMotionNode;
    }
} // end namespace EMotionFX
