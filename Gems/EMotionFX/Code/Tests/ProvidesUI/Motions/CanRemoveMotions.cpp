/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
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

        const QString assetName = "rin_idle";  // Asset name to appear in table
        const AZStd::string motionCmd = "ImportMotion -filename @exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion";

        auto motionWindowPlugin = static_cast<EMStudio::MotionWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionWindowPlugin) << "Could not find the Motion Window Plugin";

        QTableWidget* table = qobject_cast<QTableWidget*>(FindTopLevelWidget("EMFX.MotionListWindow.MotionTable"));
        ASSERT_TRUE(table) << "Could not find the Motion Table";

        // Make sure no motions exists yet
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected to have no motions for the Manager";
        EXPECT_EQ(table->rowCount(), 0) << "Expected the table to have no rows yet";

        // Run command to import motion
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(motionCmd, result)) << result.c_str();
        }

        // Assert motion was added
        ASSERT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected to have 1 motions for the Manager";
        ASSERT_EQ(table->rowCount(), 1) << "Expected the table to have 1 row";

        // Add another motion
        const QString assetJumpName = "rin_jump";  // Asset name to appear in table
        const AZStd::string motionJumpCmd = "ImportMotion -filename @exefolder@/Test.Assets/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_jump.motion";

        // Run command to import motion
        {
            AZStd::string jump_results;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(motionJumpCmd, jump_results)) << jump_results.c_str();
        }

        // Assert motion was added
        ASSERT_EQ(GetMotionManager().GetNumMotions(), 2) << "Expected to have 2 motions for the Manager";
        ASSERT_EQ(table->rowCount(), 2) << "Expected the table to have 2 rows";

        // Assert asset name is in table
        QTableWidgetItem* item = table->item(0, 0);
        ASSERT_TRUE(item) << "First table entry is invalid (unexpectedly)";
        EXPECT_EQ(item->text(), assetName) << "Asset name does not match table entry";

        // Select the motion in the table.
        item->setSelected(true);

        // Remove the first motion
        ModalPopupHandler menuHandler;
        menuHandler.ShowContextMenuAndTriggerAction(table, "EMFX.MotionListWindow.RemoveSelectionMotionsAction", 3000, nullptr);
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected to have 1 motion for the Manager";
        EXPECT_EQ(table->rowCount(), 1) << "Expected the table to have 1 row after removal";

        // Assert asset name is in table
        item = table->item(0, 0);
        ASSERT_TRUE(item) << "First table entry is invalid (unexpectedly)";
        EXPECT_EQ(item->text(), assetJumpName) << "Asset name does not match table entry";
        
        // Select the motion in the table.
        item->setSelected(true);

        // Remove the second motion
        menuHandler.ShowContextMenuAndTriggerAction(table, "EMFX.MotionListWindow.RemoveSelectionMotionsAction", 3000, nullptr);
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected to have 0 motions for the Manager";
        EXPECT_EQ(table->rowCount(), 0) << "Expected the table to have 0 row after removal";
    }
} // namespace EMotionFX
