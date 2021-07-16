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
#include <QMessageBox>

#include <Tests/UI/MenuUIFixture.h>
#include <Tests/UI/AnimGraphUIFixture.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

namespace EMotionFX
{

    TEST_F(MenuUIFixture, CanUseHelpMenu)
    {
        RecordProperty("test_case_id", "C1698605");

        // For the open autosave/settings folders, we can't check whether they have succeeded without OS dependent code,
        // so we just check the options and folders exist.

        const QMenu* helpMenu = MenuUIFixture::FindMainMenuWithName("EMFX.MainWindow.HelpMenu");
        ASSERT_TRUE(helpMenu) << "Unable to find help menu.";

        const QMenu* foldersMenu = MenuUIFixture::FindMenuWithName(helpMenu, "EMFX.MainWindow.FoldersMenu");
        ASSERT_TRUE(helpMenu) << "Unable to find folders menu.";

        const QAction* autosaveAction = FindMenuAction(foldersMenu, "Open autosave folder", "EMFX.MainWindow.FoldersMenu");
        ASSERT_TRUE(autosaveAction) << "Unable to find autosave folder menu item.";

        const QAction* settingsAction = FindMenuAction(foldersMenu, "Open settings folder", "EMFX.MainWindow.FoldersMenu");
        ASSERT_TRUE(settingsAction) << "Unable to find settings folder menu item.";

        const QString autosaveFolder = EMStudio::GetManager()->GetAutosavesFolder().c_str();
        ASSERT_TRUE(QDir(autosaveFolder).exists()) << "Unable to find autosave folder ";

        const QString settingsFolder = EMStudio::GetManager()->GetAppDataFolder().c_str();
        ASSERT_TRUE(QDir(settingsFolder).exists()) << "Unable to find settings folder ";
    }
} // namespace EMotionFX
