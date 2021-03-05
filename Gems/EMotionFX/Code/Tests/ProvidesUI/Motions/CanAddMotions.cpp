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

#include <QPushButton>
#include <QAction>
#include <QtTest>
#include <QTableWidget>
#include <qtoolbar.h>

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <Source/Integration/Assets/MotionAsset.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetPicker/AssetPickerDialog.h>

namespace EMotionFX
{
    
    TEST_F(UIFixture, CanAddMotions)
    {
        /*
        Test Case: C1559124
        Can Add Motions
        Imports a motion using commands and ensures that both the UI and the Motion Manager are updated porperly.
        */

        RecordProperty("test_case_id", "C1559124");

        const QString assetName = "rin_idle";  // Asset name to appear in table
        const AZStd::string motionCmd = AZStd::string::format("ImportMotion -filename @devroot@/Gems/EMotionFX/Code/Tests/TestAssets/Rin/rin_idle.motion");

        auto motionWindowPlugin = static_cast<EMStudio::MotionWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionWindowPlugin) << "Could not find the Motion Window Plugin";
        QTableWidget* table = qobject_cast<QTableWidget*>(FindTopLevelWidget("EMFX.MotionListWindow.MotionTable"));
        ASSERT_TRUE(table) << "Could not find the Motion Table :'(";

        // Make sure no motions exists yet
        EXPECT_EQ(GetMotionManager().GetNumMotions(), 0) << "Expected to have no motions for the Manager";
        EXPECT_EQ(table->rowCount(), 0) << "Expected the table to have no rows yet";

        // Run command to import motion
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(motionCmd, result)) << result.c_str();
        }

        // Assert motion was added
        ASSERT_EQ(GetMotionManager().GetNumMotions(), 1) << "Expected to have 1 motion for the Manager";
        ASSERT_EQ(table->rowCount(), 1) << "Expected the table to have 1 row";

        // Assert asset name is in table
        QTableWidgetItem* item = table->item(0, 0);
        ASSERT_TRUE(item) << "First table entry is invalid (unexpectedly)";
        EXPECT_EQ(item->text(), assetName) << "Asset name DOES NOT match table entry";

    }
} // namespace EMotionFX
