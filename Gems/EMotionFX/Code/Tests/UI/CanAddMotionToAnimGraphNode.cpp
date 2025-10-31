/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QPushButton>
#include <QAction>
#include <QtTest>
#include <QComboBox>

#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>

#include <Editor/PropertyWidgets/MotionSetMotionIdHandler.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphEditor.h>

namespace EMotionFX
{
    TEST_F(AnimGraphUIFixture, CanAddMotionToAnimGraphNode)
    {
        RecordProperty("test_case_id", "C2187169");

        // Create a motion set and add a motion to it.
        auto motionSetPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetPlugin) << "No motion sets plugin found";

        const EMStudio::MotionSetManagementWindow* managementWindow = motionSetPlugin->GetManagementWindow();
        ASSERT_TRUE(managementWindow) << "No motion sets management window found";

        const EMStudio::MotionSetWindow* motionSetWindow = motionSetPlugin->GetMotionSetWindow();
        ASSERT_TRUE(motionSetWindow) << "No motion set window found";

        // Check there aren't any motion sets yet.
        const size_t oldNumMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();

        // Find the action to create a new motion set and press it.
        QWidget* addMotionSetButton = GetWidgetWithNameFromNamedToolbar(managementWindow, "MotionSetManagementWindow.ToolBar", "MotionSetManagementWindow.ToolBar.AddNewMotionSet");
        ASSERT_TRUE(addMotionSetButton) << "Unable to find Add Motion Set button.";
        QTest::mouseClick(addMotionSetButton, Qt::LeftButton);

        // Make sure the new motion set has  been created.
        ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), oldNumMotionSets + 1) << "Failed to create motion set.";

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(oldNumMotionSets);

        // Ensure new motion set is selected.
        motionSetPlugin->SetSelectedSet(motionSet);

        // It should be empty at the moment.
        const size_t numMotions = motionSet->GetNumMotionEntries();
        EXPECT_EQ(numMotions, 0);

        // Find the action to add a motion to the set and press it.
        QWidget* addMotionButton = GetWidgetWithNameFromNamedToolbar(motionSetWindow, "MotionSetWindow.ToolBar", "MotionSetWindow.ToolBar.AddANewEntry");
        ASSERT_TRUE(addMotionButton) << "No Add Motion to Motion Set button found";
        QTest::mouseClick(addMotionButton, Qt::LeftButton);

        // There should now be a new more motion.
        const size_t numMotionsAfterCreate = motionSet->GetNumMotionEntries();
        ASSERT_EQ(numMotionsAfterCreate, numMotions + 1) << "Failed to create new motion.";

        AZStd::unordered_map<AZStd::string, MotionSet::MotionEntry*> motions = motionSet->GetMotionEntries();

        // The newly created motion should be called "<undefined>".
        MotionSet::MotionEntry* motion = motions["<undefined>"];
        ASSERT_TRUE(motion) << "no \"<undefined>\" motion found";

        // Create a motion node in the anim graph.
        EMotionFX::AnimGraph* animGraph = CreateAnimGraph();
        ASSERT_TRUE(animGraph) << "Failed to find anim graph";

        AnimGraphNode* node = AddNodeToAnimGraph(animGraph, "Motion");
        ASSERT_TRUE(node) << "Failed to find create motion node in anim graph.";

        AnimGraphMotionNode* motionNode = azdynamic_cast<AnimGraphMotionNode*>(node);

        size_t numMotionsInNode = motionNode->GetNumMotions();
        EXPECT_EQ(numMotionsInNode, 0);

        EMStudio::NodeGraph* nodeGraph = GetActiveNodeGraph();

        // Find the corresponding GraphNode and click on it to select it.
        nodeGraph->SelectAllNodes();
        AZStd::vector<EMStudio::GraphNode*> nodes = nodeGraph->GetSelectedGraphNodes();
        ASSERT_NE(nodes.size(), 0) << "Failed to select motion node in anim graph.";

        EMStudio::GraphNode* newNode = nodes[0];
        QRect nodeLocalRect = newNode->GetFinalRect();
        QTest::mouseClick(m_blendGraphWidget, Qt::LeftButton, {}, nodeLocalRect.center());

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        // RPE should have now set up the attributes editor.
        EMStudio::AttributesWindow* attributesWindow = m_animGraphPlugin->GetAttributesWindow();
        ASSERT_TRUE(attributesWindow) << "Failed to find AttributesWindow.";
        EMotionFX::AnimGraphEditor* animGraphEditor = attributesWindow->GetAnimGraphEditor();
        ASSERT_TRUE(animGraphEditor) << "Failed to find AnimGraphEditor in AttributesWindow.";

        // Select our motion set in the combo widget (index 0 is the select motion set instruction text).
        QComboBox* combo = animGraphEditor->GetMotionSetComboBox();
        ASSERT_TRUE(combo) << "Unable to get MotionSetComboBox from AnimGraphEditor.";
        combo->setCurrentIndex(1);

        // Find the picker add button in the attributes window and press it.
        const  MotionSetMotionIdPicker* idPicker = attributesWindow->findChild<MotionSetMotionIdPicker*>();
        ASSERT_TRUE(idPicker) << "Failed to find MotionSetMotionIdPicker in AttributesWindow.";

        // Click the add motion button.
        QPushButton* pickerButton = idPicker->findChild<QPushButton*>("EMFX.MotionSetMotionIdPicker.PickButton");
        ASSERT_TRUE(pickerButton) << "Failed to find PickButton in MotionSetMotionIdPicker.";

        QTest::mouseClick(pickerButton, Qt::LeftButton);

        // The motion picker dialog should now be open.
        EMStudio::MotionSetSelectionWindow* pickWindow = idPicker->findChild< EMStudio::MotionSetSelectionWindow*>();
        ASSERT_TRUE(pickWindow) << "Failed to find MotionSetSelectionWindow.";

        pickWindow->GetHierarchyWidget()->SelectItemsWithText("<undefined>");

        // Select the motion we created earlier and press the OK button.
        pickWindow->GetHierarchyWidget()->SelectItemsWithText("<undefined>");
        QPushButton* okButton = pickWindow->findChild<QPushButton*>("EMFX.MotionSetSelectionWindow.Ok");
        ASSERT_TRUE(okButton) << "Failed to find OK button in MotionSetSelectionWindow.";

        QTest::mouseClick(okButton, Qt::LeftButton);

        // The motion should now have been pushed back to the node: check it.
        const size_t numMotionsInNodeAfter = motionNode->GetNumMotions();
        ASSERT_EQ(numMotionsInNodeAfter, numMotionsInNode + 1) << "Failed to add motion to motion node.";

        const char* motionName = motionNode->GetMotionId(0);
        ASSERT_STREQ(motionName, "<undefined>") << "Failed to find added motion in motion node";

        // Delete the created node.
        QTest::keyClick(m_blendGraphWidget, Qt::Key_Delete);
    }
} // namespace EMotionFX
