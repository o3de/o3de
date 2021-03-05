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
#include <QWidget>
#include <QAction>
#include <QtTest>
#include <QTreeWidget>
#include <QToolBar>

#include <Tests/UI/UIFixture.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetManagementWindow.h>


namespace EMotionFX
{
    TEST_F(UIFixture, CanCreateMotionSet)
    {
        /*
        Test Rail ID: C16735973
        Overview: Create a Motion Set using the Toolbar plus (+) icon
        Expected Result: When the button to create a Motion Set is pressed, a motion set should be created and added into the UI.
        */

        RecordProperty("test_case_id", "C16735973");

        // Get pointers to useful UI objects
        auto motionSetsWindowPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(
            EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID)
        );
        ASSERT_TRUE(motionSetsWindowPlugin) << "Motion Sets Plugin could not be found";
        EMStudio::MotionSetManagementWindow* managerWindow = motionSetsWindowPlugin->GetManagementWindow();
        ASSERT_TRUE(managerWindow) << "Motion Sets Manager could not be found";
        QTreeWidget* treeWidget = managerWindow->findChild<QTreeWidget*>("EMFX.MotionSetManagementWindow.MotionSetsTree");
        ASSERT_TRUE(treeWidget) << "Motion Set Manager's Tree Widget could not be found";
        QAbstractItemModel* model = treeWidget->model();
        ASSERT_TRUE(model) << "Tree Widget's Data Model could not be found";

        // Validate state before creating a new motion set
        ASSERT_EQ(GetMotionManager().GetNumMotionSets(), 0) << "Expected exactly zero motion sets";
        ASSERT_EQ(treeWidget->topLevelItemCount(), 0) << "Expected exactly 0 TopLevelItems in TreeWidget";
        ASSERT_FALSE(model->index(0, 0).isValid()) << "Tree Model index(0, 0) is expected to not exist yet";

        // Find and click the plus (+) icon on the toolbar (to create new motion set)
        // Found through the Manager Window's Toolbar's Actions
        QToolBar* toolBar = managerWindow->findChild<QToolBar*>("MotionSetManagementWindow.ToolBar");
        ASSERT_TRUE(toolBar) << "Motion Set Management ToolBar could not be found";
        QWidget* newMotionSetButton = GetWidgetFromToolbar(toolBar, "Add new motion set");
        
        ASSERT_TRUE(newMotionSetButton) << "Could not find new motion set button. Did the text description change?";
        QTest::mouseClick(newMotionSetButton, Qt::LeftButton);

        // Refresh the window to make sure the new MotionSet will show in the TreeWidget
        managerWindow->ReInit();

        // Validate state after clicking "add motionset" button
        ASSERT_EQ(GetMotionManager().GetNumMotionSets(), 1) << "Not exactly one motion set found";
        ASSERT_EQ(treeWidget->topLevelItemCount(), 1) << "Not exacxtly one TopLevelItem in TreeWidget";
        ASSERT_TRUE(model->index(0, 0).isValid()) << "Tree Model index(0, 0) is expected to be valid";

    }
}
