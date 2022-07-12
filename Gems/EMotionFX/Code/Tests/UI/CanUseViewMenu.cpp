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
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

namespace EMotionFX
{
    class CanUseViewMenuFixture
        : public MenuUIFixture
    {
    public:

        void TestShowPlugin(const QMenu* viewMenu, const QString& pluginName) const
        {
            QAction* action = FindMenuAction(viewMenu, pluginName, "ViewMenu");
            ASSERT_TRUE(action) << "Unable to find view menu item " << pluginName.toUtf8().data();

            if (action->isChecked())
            {
                return;
            }

            const EMStudio::PluginManager::PluginVector pluginsBefore = EMStudio::GetPluginManager()->GetActivePlugins();

            action->trigger();

            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            const EMStudio::PluginManager::PluginVector pluginsAfter = EMStudio::GetPluginManager()->GetActivePlugins();

            ASSERT_EQ(pluginsAfter.size(), pluginsBefore.size() + 1) << "Failed to open plugin with view menu option " << action->text().toUtf8().data();

            // Need to get the action again to check that the checked status has flipped.
            action = FindMenuAction(viewMenu, pluginName, "ViewMenu");
            ASSERT_TRUE(action);
            ASSERT_TRUE(action->isChecked()) << "View menu option not checked after opening " << action->text().toUtf8().data();
        }

        void TestHidePlugin(const QMenu* viewMenu, const QString& pluginName) const
        {
            QAction* action = FindMenuAction(viewMenu, pluginName, "ViewMenu");
            ASSERT_TRUE(action) << "Unable to find view menu item " << pluginName.toUtf8().data();

            if (!action->isChecked())
            {
                return;
            }

            const EMStudio::PluginManager::PluginVector pluginsBefore = EMStudio::GetPluginManager()->GetActivePlugins();

            action->trigger();

            const EMStudio::PluginManager::PluginVector pluginsAfter = EMStudio::GetPluginManager()->GetActivePlugins();

            ASSERT_EQ(pluginsAfter.size(), pluginsBefore.size() - 1) << "Failed to close plugin with view menu option " << action->text().toUtf8().data();

            // Need to get the action again to check that the checked status has flipped.
            action = FindMenuAction(viewMenu, pluginName, "ViewMenu");
            ASSERT_TRUE(action);
            ASSERT_TRUE(!action->isChecked()) << "View menu option still checked after closing " << action->text().toUtf8().data();
        }

        void TestViewMenuItem(const QMenu* viewMenu, const QString& pluginName)
        {
            QAction* action = FindMenuAction(viewMenu, pluginName, "ViewMenu");
            ASSERT_TRUE(action) << "Unable to find view menu item " << pluginName.toUtf8().data();

            if (action->isChecked())
            {
                TestHidePlugin(viewMenu, pluginName);
                TestShowPlugin(viewMenu, pluginName);
            }
            else
            {
                TestShowPlugin(viewMenu, pluginName);
                TestHidePlugin(viewMenu, pluginName);
            }
        }
    };

    TEST_F(CanUseViewMenuFixture, CanUseViewMenu)
    {
        RecordProperty("test_case_id", "C1698604");

        CloseAllPlugins();

        // Find the View menu.
        QMenu* viewMenu = FindMainMenuWithName("ViewMenu");
        ASSERT_TRUE(viewMenu) << "Unable to find view menu.";

        EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();

        QList<QAction*> actions = viewMenu->findChildren<QAction*>();
        int numActions = actions.size() - 1;// -1 as we don't want to include the view menu action itself.

        int visiblePlugins = 0;

        const EMStudio::PluginManager::PluginVector& registeredPlugins = pluginManager->GetRegisteredPlugins();
        for (EMStudio::EMStudioPlugin* plugin : registeredPlugins)
        {

            TestViewMenuItem(viewMenu, plugin->GetName());
            visiblePlugins++;
        }

        ASSERT_EQ(visiblePlugins, numActions) << "View menu action count != number of visible plugins.";
    }
} // namespace EMotionFX
