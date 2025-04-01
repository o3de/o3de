/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QModelIndex>
#include <QPushButton>
#include <QtTest>

#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>

#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>

namespace EMotionFX
{
    TEST_F(AnimGraphUIFixture, CanAddReferenceNode)
    {
        RecordProperty("test_case_id", "C21948788");

        auto animGraph = CreateAnimGraph();
        ASSERT_TRUE(animGraph) << "Failed to create AnimGraph";

        auto nodeGraph = GetActiveNodeGraph();
        ASSERT_TRUE(nodeGraph) << "NodeGraph not found";

        EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(EMStudio::AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        ASSERT_TRUE(currentNode) << "No current AnimGraphNode found";

        // Launch the node graph context menu.
        const AZStd::vector<EMotionFX::AnimGraphNode*> selectedAnimGraphNodes = nodeGraph->GetSelectedAnimGraphNodes();
        m_blendGraphWidget->OnContextMenuEvent(m_blendGraphWidget, QPoint(0, 0), QPoint(0, 0), m_animGraphPlugin, selectedAnimGraphNodes, true, false, m_animGraphPlugin->GetActionFilter());

        // Add the Reference node through the context menu.
        auto* tree = UIFixture::GetFirstChildOfType<GraphCanvas::NodePaletteTreeView>(m_blendGraphWidget);
        ASSERT_TRUE(tree);
        const QModelIndex idx = UIFixture::GetIndexFromName(tree, "Reference");
        ASSERT_TRUE(idx.isValid());
        // Selection should spawn the node
        tree->setCurrentIndex(idx);

        // Check the expected node now exists.
        size_t numNodesAfter = currentNode->GetNumChildNodes();
        EXPECT_EQ(1, numNodesAfter);

        AnimGraphNode* newNode = currentNode->GetChildNode(0);
        ASSERT_STREQ(newNode->GetName(), "Reference0");

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
} // namespace EMotionFX
