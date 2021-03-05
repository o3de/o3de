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

        const AZ::u32 numPlugins = pluginManager->GetNumPlugins();

        int visiblePlugins = 0;

        for (AZ::u32 pluginIndex = 0; pluginIndex < numPlugins; pluginIndex++)
        {
            EMStudio::EMStudioPlugin* plugin = pluginManager->GetPlugin(pluginIndex);

            if (plugin->GetPluginType() == EMStudio::EMStudioPlugin::PLUGINTYPE_INVISIBLE)
            {
                continue;
            }

            TestViewMenuItem(viewMenu, plugin->GetName());
            visiblePlugins++;
        }

        ASSERT_EQ(visiblePlugins, numActions) << "View menu action count != number of visible plugins.";
    }
} // namespace EMotionFX
