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
#include <QTableWidget>
#include <qtoolbar.h>

#include <Tests/UI/UIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Source/Integration/Assets/MotionAsset.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetPicker/AssetPickerDialog.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanRemoveMotions)
    {
        /*
        Test Case: C1559123
        Can remove Motions
        Imports a motion using commands and ensures that both the UI and the Motion Manager are updated porperly then removes it
        */
        RecordProperty("test_case_id", "C1559123");

        AZStd::string command;
        AZStd::string result;
        const char* assetName = "rin_idle";  // Asset name to appear in table
        const char* filename = "@gemroot:EMotionFX@/Code/Tests/TestAssets/Rin/rin_idle.motion";

        auto motionSetsWindowPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetsWindowPlugin) << "Could not find the Motion Sets Window Plugin";
        EMStudio::MotionSetWindow* motionSetWindow = motionSetsWindowPlugin->GetMotionSetWindow();
        ASSERT_TRUE(motionSetWindow) << "Could not find the Motion Sets Window.";
        EMStudio::MotionSetTableWidget* tableWidget = motionSetWindow->GetTableWidget();
        ASSERT_TRUE(tableWidget) << "Could not find the motion set table widget.";

        // Make sure no motions exists yet
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected to have no motions for the Manager";
        EXPECT_EQ(tableWidget->rowCount(), 0) << "Expected the table to have no rows yet";

        ASSERT_EQ(GetMotionManager().GetNumMotionSets(), 1) << "Expected the editor to automatically create a default motion set.";
        MotionSet* motionSet = GetMotionManager().GetMotionSet(0);

        auto AddMotion = [motionSet](const char* assetName, const char* filename)
        {
            AZStd::string result;
            const AZStd::string command = AZStd::string::format("ImportMotion -filename %s", filename);
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, result)) << result.c_str();

            MCore::CommandGroup commandGroup("Add new motion set entry");
            CommandSystem::AddMotionSetEntry(motionSet->GetID(), assetName, {}, filename, &commandGroup);
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        };

        AddMotion(assetName, filename);
        ASSERT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected to have 1 motion for the Manager";
        ASSERT_EQ(motionSet->GetNumMotionEntries(), 1) << "Expected the newly added motion entry to be there.";
        ASSERT_EQ(tableWidget->rowCount(), 1) << "Expected the table to have 1 row";

        // Add another motion
        const char* assetJumpName = "rin_jump";  // Asset name to appear in table
        const char* motionJumpFilename = "@exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_jump.motion";
        AddMotion(assetJumpName, motionJumpFilename);
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 2);
        EXPECT_EQ(motionSet->GetNumMotionEntries(), 2);
        EXPECT_EQ(tableWidget->rowCount(), 2);

        // Assert asset name is in table
        QTableWidgetItem* item = tableWidget->item(0, 1);
        ASSERT_TRUE(item) << "First table entry is invalid (unexpectedly)";
        EXPECT_EQ(item->text(), assetName) << "Asset name does not match table entry";

        // Select the motion in the table.
        item->setSelected(true);

        // Remove the first motion
        ModalPopupHandler menuHandler;
        menuHandler.ShowContextMenuAndTriggerAction(tableWidget, "EMFX.MotionSetTableWidget.RemoveSelectedMotionsAction", 3000, nullptr);
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected to have 1 motion for the Manager";
        EXPECT_EQ(tableWidget->rowCount(), 1) << "Expected the table to have 1 row after removal";

        // Assert asset name is in table
        item = tableWidget->item(0, 1);
        ASSERT_TRUE(item) << "First table entry is invalid (unexpectedly)";
        EXPECT_EQ(item->text(), assetJumpName) << "Asset name does not match table entry";
        
        // Select the motion in the table.
        item->setSelected(true);

        // Remove the second motion
        menuHandler.ShowContextMenuAndTriggerAction(tableWidget, "EMFX.MotionSetTableWidget.RemoveSelectedMotionsAction", 3000, nullptr);
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected to have 0 motions for the Manager";
        EXPECT_EQ(tableWidget->rowCount(), 0) << "Expected the table to have 0 row after removal";
    }
} // namespace EMotionFX
