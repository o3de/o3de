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

#include <Tests/UI/AnimGraphUIFixture.h>
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
    TEST_F(UIFixture, CanAddMotionToMotionSet)
    {
        RecordProperty("test_case_id", "C1559110");

        auto motionSetPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
        ASSERT_TRUE(motionSetPlugin) << "No motion sets plugin found";

        EMStudio::MotionSetManagementWindow* managementWindow = motionSetPlugin->GetManagementWindow();
        ASSERT_TRUE(managementWindow) << "No motion sets management window found";

        EMStudio::MotionSetWindow* motionSetWindow = motionSetPlugin->GetMotionSetWindow();
        ASSERT_TRUE(motionSetWindow) << "No motion set window found";

        // Check there aren't any motion sets yet.
        uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        EXPECT_EQ(numMotionSets, 0);

        // Find the action to create a new motion set and press it.
        QWidget* addMotionSetButton = GetWidgetWithNameFromNamedToolbar(managementWindow, "MotionSetManagementWindow.ToolBar", "MotionSetManagementWindow.ToolBar.AddNewMotionSet");
        ASSERT_TRUE(addMotionSetButton);
        QTest::mouseClick(addMotionSetButton, Qt::LeftButton);

        // Check there is now a motion set.
        int numMotionSetsAfterCreate = EMotionFX::GetMotionManager().GetNumMotionSets();
        ASSERT_EQ(numMotionSetsAfterCreate, 1);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);

        // Ensure new motion set is selected.
        motionSetPlugin->SetSelectedSet(motionSet);

        // It should be empty at the moment.
        int numMotions = motionSet->GetNumMotionEntries();
        EXPECT_EQ(numMotions, 0);

        // Find the action to add a motion to the set and press it.
        QWidget* addMotionButton = GetWidgetWithNameFromNamedToolbar(motionSetWindow, "MotionSetWindow.ToolBar", "MotionSetWindow.ToolBar.AddANewEntry");
        ASSERT_TRUE(addMotionButton);
        QTest::mouseClick(addMotionButton, Qt::LeftButton);

        // There should now be a motion.
        int numMotionsAfterCreate = motionSet->GetNumMotionEntries();
        ASSERT_EQ(numMotionsAfterCreate, 1);

        AZStd::unordered_map<AZStd::string, MotionSet::MotionEntry*> motions = motionSet->GetMotionEntries();

        // The newly created motion should be called "<undefined>".
        MotionSet::MotionEntry* motion = motions["<undefined>"];
        ASSERT_TRUE(motion) << "no \"<undefined>\" motion found";

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
} // namespace EMotionFX
