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
#include <QHeaderView>
#include <QMessageBox>

#include <Tests/UI/AnimGraphUIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanRemoveMotionFromMotionSet)
    {
        RecordProperty("test_case_id", "C24255734");

        auto motionSetPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetPlugin) << "No motion sets plugin found";

        EMStudio::MotionSetManagementWindow* managementWindow = motionSetPlugin->GetManagementWindow();
        ASSERT_TRUE(managementWindow) << "No motion sets management window found";

        EMStudio::MotionSetWindow* motionSetWindow = motionSetPlugin->GetMotionSetWindow();
        ASSERT_TRUE(motionSetWindow) << "No motion set window found";

        // Check there aren't any motion sets yet.
        const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        EXPECT_EQ(numMotionSets, 0);

        // Find the action to create a new motion set and press it.
        QWidget* addMotionSetButton = GetWidgetWithNameFromNamedToolbar(managementWindow, "MotionSetManagementWindow.ToolBar", "MotionSetManagementWindow.ToolBar.AddNewMotionSet");
        ASSERT_TRUE(addMotionSetButton);
        QTest::mouseClick(addMotionSetButton, Qt::LeftButton);

        // Check there is now a motion set.
        const size_t numMotionSetsAfterCreate = EMotionFX::GetMotionManager().GetNumMotionSets();
        ASSERT_EQ(numMotionSetsAfterCreate, 1);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);

        // Ensure new motion set is selected.
        motionSetPlugin->SetSelectedSet(motionSet);

        // It should be empty at the moment.
        const size_t numMotions = motionSet->GetNumMotionEntries();
        EXPECT_EQ(numMotions, 0);

        // Find the action to add a motion to the set and press it.
        QWidget* addMotionButton = GetWidgetWithNameFromNamedToolbar(motionSetWindow, "MotionSetWindow.ToolBar", "MotionSetWindow.ToolBar.AddANewEntry");
        ASSERT_TRUE(addMotionButton);
        QTest::mouseClick(addMotionButton, Qt::LeftButton);

        // There should now be a motion.
        const size_t numMotionsAfterCreate = motionSet->GetNumMotionEntries();
        ASSERT_EQ(numMotionsAfterCreate, 1);

        AZStd::unordered_map<AZStd::string, MotionSet::MotionEntry*> motions = motionSet->GetMotionEntries();

        // The newly created motion should be called "<undefined>".
        MotionSet::MotionEntry* motion = motions["<undefined>"];
        ASSERT_TRUE(motion) << "no \"<undefined>\" motion found";

        // Ensure the new motion is added to the table.
        motionSetWindow->ReInit();

        // Now we need to remove it again.
        EMStudio::MotionSetTableWidget* table = motionSetWindow->findChild< EMStudio::MotionSetTableWidget*>("EMFX.MotionSetWindow.TableWidget");
        ASSERT_TRUE(table);
        ASSERT_EQ(table->rowCount(), 1);

        // Select the motion in the table.
        QList<QTableWidgetItem *> items = table->findItems("<undefined>", Qt::MatchExactly);
        items[0]->setSelected(true);

        // Set up a watcher to press the ok button in the Really Delete dialog.
        ModalPopupHandler reallyDeletehandler;
        reallyDeletehandler.WaitForPopupPressSpecificButton<QMessageBox*>("EMFX.MotionSet.RemoveMotionMessageBox.YesButton");

        // Pop up the context menu and select the Remove action.
        ModalPopupHandler menuHandler;
        menuHandler.ShowContextMenuAndTriggerAction(motionSetWindow, "EMFX.MotionSetTableWidget.RemoveSelectedMotionsAction", 3000, nullptr);

        // Make sure any changes filter through to the widget.
        motionSetWindow->ReInit();

        // The motion should now be gone.
        items = table->findItems("<undefined>", Qt::MatchExactly);
        ASSERT_EQ(table->rowCount(), 0);

        motions = motionSet->GetMotionEntries();
        ASSERT_EQ(table->rowCount(), 0);
    }

    TEST_F(UIFixture, CanRemoveSingleMotionFromMotionSetWithMultipleMotions)
    {
        RecordProperty("test_case_id", "C15105117");

        auto motionSetPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetPlugin) << "No motion sets plugin found";

        EMStudio::MotionSetManagementWindow* managementWindow = motionSetPlugin->GetManagementWindow();
        ASSERT_TRUE(managementWindow) << "No motion sets management window found";

        EMStudio::MotionSetWindow* motionSetWindow = motionSetPlugin->GetMotionSetWindow();
        ASSERT_TRUE(motionSetWindow) << "No motion set window found";

        // Check there aren't any motion sets yet.
        const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        EXPECT_EQ(numMotionSets, 0);

        // Find the action to create a new motion set and press it.
        QWidget* addMotionSetButton = GetWidgetWithNameFromNamedToolbar(managementWindow, "MotionSetManagementWindow.ToolBar", "MotionSetManagementWindow.ToolBar.AddNewMotionSet");
        ASSERT_TRUE(addMotionSetButton);
        QTest::mouseClick(addMotionSetButton, Qt::LeftButton);

        // Check there is now a motion set.
        const size_t numMotionSetsAfterCreate = EMotionFX::GetMotionManager().GetNumMotionSets();
        ASSERT_EQ(numMotionSetsAfterCreate, 1);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);

        // Ensure new motion set is selected.
        motionSetPlugin->SetSelectedSet(motionSet);

        // It should be empty at the moment.
        const size_t numMotions = motionSet->GetNumMotionEntries();
        EXPECT_EQ(numMotions, 0);

        // Find the action to add a motion to the set and press it twice.
        QWidget* addMotionButton = GetWidgetWithNameFromNamedToolbar(motionSetWindow, "MotionSetWindow.ToolBar", "MotionSetWindow.ToolBar.AddANewEntry");
        ASSERT_TRUE(addMotionButton);
        QTest::mouseClick(addMotionButton, Qt::LeftButton);
        QTest::mouseClick(addMotionButton, Qt::LeftButton);

        // There should now be two motion.
        const size_t numMotionsAfterCreate = motionSet->GetNumMotionEntries();
        ASSERT_EQ(numMotionsAfterCreate, 2);

        AZStd::unordered_map<AZStd::string, MotionSet::MotionEntry*> motions = motionSet->GetMotionEntries();

        // The newly created motions should be called "<undefined>".
        MotionSet::MotionEntry* motion = motions["<undefined>"];
        ASSERT_TRUE(motion) << "no \"<undefined>\" motion found";

        // Ensure the new motion is added to the table.
        motionSetWindow->ReInit();

        // Now we need to remove one of them.
        EMStudio::MotionSetTableWidget* table = motionSetWindow->findChild< EMStudio::MotionSetTableWidget*>("EMFX.MotionSetWindow.TableWidget");
        ASSERT_TRUE(table);
        ASSERT_EQ(table->rowCount(), 2);

        // Select the motion in the table.
        QList<QTableWidgetItem *> items = table->findItems("<undefined>", Qt::MatchExactly);
        items[0]->setSelected(true);

        // Set up a watcher to press the ok button in the Really Delete dialog.
        ModalPopupHandler reallyDeletehandler;
        reallyDeletehandler.WaitForPopupPressSpecificButton<QMessageBox*>("EMFX.MotionSet.RemoveMotionMessageBox.YesButton");

        // Pop up the context menu and select the Remove action.
        ModalPopupHandler menuHandler;
        menuHandler.ShowContextMenuAndTriggerAction(motionSetWindow, "EMFX.MotionSetTableWidget.RemoveSelectedMotionsAction", 3000, nullptr);

        // Make sure any changes filter through to the widget.
        motionSetWindow->ReInit();

        // The motion should now be gone leaving the other.
        items = table->findItems("<undefined>", Qt::MatchExactly);
        ASSERT_EQ(table->rowCount(), 1);

        motions = motionSet->GetMotionEntries();
        ASSERT_EQ(table->rowCount(), 1);
    }
} // namespace EMotionFX
