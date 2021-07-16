/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QWidget>
#include <QWidgetList>
#include <QAction>
#include <QtTest>
#include <QTreeWidget>
#include <QMessageBox>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetManagementWindow.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanRemoveMotionSet)
    {
        RecordProperty("test_case_id", "C24255735");

        // Add new motion set.
        EXPECT_EQ(GetMotionManager().GetNumMotionSets(), 0) << "No motion set should be present yet.";
        const char* motionSetName = "TestMotionSet";
        const AZStd::string commandString = AZStd::string::format("CreateMotionSet -name \"%s\"", motionSetName);
        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        EXPECT_EQ(GetMotionManager().GetNumMotionSets(), 1) << "Exactly one motion set should be present.";

        // Select the motion set.
        MotionSet* motionSet = GetMotionManager().GetMotionSet(0);
        motionSet->SetDirtyFlag(false);
        auto motionSetsPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetsPlugin) << "Motion Sets Plugin could not be found";
        EMStudio::MotionSetManagementWindow* motionSetWindow = motionSetsPlugin->GetManagementWindow();
        motionSetWindow->ReInit();
        motionSetsPlugin->SetSelectedSet(motionSet);

        // Get the rect of the selected motion set tree widget item.
        ASSERT_TRUE(motionSetWindow) << "Expected a valid motion set management window.";
        const QTreeWidget* treeWidget = motionSetWindow->findChild<QTreeWidget*>("EMFX.MotionSetManagementWindow.MotionSetsTree");
        ASSERT_TRUE(treeWidget) << "Expected a valid motion set tree widget.";
        QTreeWidgetItem* motionSetItem = treeWidget->invisibleRootItem()->child(0);
        EXPECT_TRUE(motionSetItem) << "Tree widget item for motion set not found.";
        const QRect rect = treeWidget->visualItemRect(motionSetItem);

        // Bring up context menu for the selected motion set.
        BringUpContextMenu(treeWidget, rect);
        QMenu* contextMenu = motionSetWindow->findChild<QMenu*>("EMFX.MotionSetManagementWindow.ContextMenu");
        EXPECT_TRUE(contextMenu) << "No context menu available";

        // Remove selected motion sets.
        QAction* removeSelectedAction = nullptr;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(removeSelectedAction, contextMenu, "Remove selected"))
            << "Cannot find remove selected motion set context menu entry.";

        // Modal pop-up window requesting to remove the motions from the project entirely or only from the motion set comes up. 
        {
            ModalPopupHandler messageBoxPopupHandler;
            messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::Yes);
            removeSelectedAction->trigger();
        }

        motionSetWindow->ReInit();       

        // Data verification.
        EXPECT_EQ(GetMotionManager().GetNumMotionSets(), 0) << "No motion set should be present anymore.";
        EXPECT_EQ(treeWidget->topLevelItemCount(), 0) << "Expected an empty tree widget.";
    }
} // namespace EMotionFX
