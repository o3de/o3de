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
#include <Editor/InputDialogValidatable.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioPlugin.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/InputDialogValidatable.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

#include <MCore/Source/MemoryManager.h>

namespace EMotionFX
{
    class CanUseLayoutMenuFixture
        : public MenuUIFixture
    {
    public:
        void SetUp() override
        {
            MenuUIFixture::SetUp();

            m_saveLayoutFileName = GenerateNewLayoutFilename();
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            if (QFile::exists(GetLayoutFilePath(GetLayoutFileName())))
            {
                QFile::remove(GetLayoutFilePath(GetLayoutFileName()));
            }

            MenuUIFixture::TearDown();
        }

        QString GetLayoutFilePath(const QString& layoutFileName) const
        {
            return QString("%1/%2.layout").arg(GetLayoutFileDirectory(), layoutFileName);
        }

        QString GetLayoutFileDirectory() const
        {
            return QDir{ QString(MysticQt::GetDataDir().c_str()) }.filePath("Layouts");
        }

        QString GetLayoutFileName()
        {
            return m_saveLayoutFileName;
        }

        QString GenerateNewLayoutFilename()
        {
            // Find a temporary layout name that doesn't already exist.
            bool foundFilename = false;
            int fileIndex = 0;

            while (!foundFilename)
            {
                const QString fileName = QString("TestLayout%1").arg(fileIndex);
                const QString filepath = GetLayoutFilePath(fileName);

                // If the file already exists, ask to overwrite or not.
                if (!QFile::exists(filepath))
                {
                    return fileName;
                }
                fileIndex++;
            }

            return nullptr;
        }

        bool ComparePluginLists(const EMStudio::PluginManager::PluginVector& plugins1, const EMStudio::PluginManager::PluginVector& plugins2)
        {
            // Returns false if the lists are the same.
            if (plugins1.size() != plugins2.size())
            {
                return true;
            }

            for (size_t pluginIndex = 0; pluginIndex < plugins1.size(); pluginIndex++)
            {
                if (plugins1[pluginIndex] != plugins2[pluginIndex])
                {
                    return true;
                }
            }

            return false;
        }

        void TestSaveLayoutMenuItem(const QMenu* layoutsMenu)
        {
            QAction* action = FindMenuAction(layoutsMenu, "Save Current", "LayoutsMenu");
            ASSERT_TRUE(layoutsMenu) << "Unable to find 'Save Current' menu option.";

            // Open the save dialog.
            action->trigger();

            // Set the save name and press OK.
            EMStudio::InputDialogValidatable* inputDialog = EMStudio::GetLayoutManager()->GetSaveLayoutNameDialog();
            inputDialog->SetText(GetLayoutFileName());
            QList<QPushButton*> dialogButtons = inputDialog->findChildren<QPushButton*>();
            for (QPushButton* dialogButton : dialogButtons)
            {
                if (dialogButton->text() == "OK")
                {
                    QTest::mouseClick(dialogButton, Qt::LeftButton);
                    break;
                }
            }

            // Check the layout file now exists
            const bool fileExists = QFile::exists(GetLayoutFilePath(GetLayoutFileName()));
            ASSERT_TRUE(fileExists) << "Failed to create layout save file";
        }

        void TestSelectNamedLayoutMenuItem(const QMenu* layoutsMenu, const QString& layoutName)
        {
            // Find the select action in the layouts menu, Specify the parent so that the corresponding remove option isn't discovered.
            QAction* menuAction = FindMenuAction(layoutsMenu, layoutName, "LayoutsMenu");
            ASSERT_TRUE(menuAction) << "Select layout menu item " << layoutName.toUtf8().data() << " not found.";

            // Close all plugins so we can check the load has done something.
            CloseAllPlugins();

            // Get currently active plugins;
            const EMStudio::PluginManager::PluginVector plugins = EMStudio::GetPluginManager()->GetActivePlugins();

            // Select new layout
            menuAction->trigger();

            const EMStudio::PluginManager::PluginVector newPlugins = EMStudio::GetPluginManager()->GetActivePlugins();

            // We're not sure what plugins should be opened, just make sure it's opened something.
            const bool pluginsChanged = ComparePluginLists(plugins, newPlugins);
            ASSERT_TRUE(pluginsChanged) << "Select layout " << layoutName.toUtf8().data() << " failed.";
        }

        void TestSelectLayoutMenuItems(const QMenu* layoutsMenu)
        {
            // Find all the layout files, there should be a menu item for each.
            QDir directory(GetLayoutFileDirectory());
            QStringList layoutFiles = directory.entryList(QStringList() << "*.layout" << "*.layout", QDir::Files);

            for (QString fileName : layoutFiles)
            {
                QString layoutName = fileName.replace(".layout", "");
                TestSelectNamedLayoutMenuItem(layoutsMenu, layoutName);
            }
        }

        void TestRemoveMenuItems(const QMenu* layoutsMenu)
        {
            // Only test removing the item we created: trying to remove others if they are read only will result in an assert in the error message.
            QAction* removeAction = FindMenuAction(layoutsMenu, GetLayoutFileName(), "RemoveMenu");
            ASSERT_TRUE(removeAction) << "No remove menu item found for layout " << GetLayoutFileName().toUtf8().data();

            removeAction->trigger();

            // Press "Yes" in the do you really want to delete this dialog
            QMessageBox* dialog = EMStudio::GetMainWindow()->GetRemoveLayoutDialog();
            ASSERT_TRUE(dialog);

            const QList<QPushButton*> dialogButtons = dialog->findChildren<QPushButton*>();
            for (QPushButton* dialogButton : dialogButtons)
            {
                if (dialog->buttonRole(dialogButton) == QMessageBox::YesRole)
                {
                    QTest::mouseClick(dialogButton, Qt::LeftButton);
                }
            }

            // Check the file is now gone.
            bool layoutFileExists = QFile::exists(GetLayoutFilePath(GetLayoutFileName()));
            ASSERT_FALSE(layoutFileExists) << "Remove layout menu option failed";
        }

    private:
        QString m_saveLayoutFileName;
    };

#if AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_EDITOR_TESTS
    TEST_F(CanUseLayoutMenuFixture, DISABLED_CanUseLayoutMenu)
#else
    TEST_F(CanUseLayoutMenuFixture, CanUseLayoutMenu)
#endif // AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_EDITOR_TESTS
    {
        RecordProperty("test_case_id", "C1698603");

        // Find the Layouts menu.
        QMenu* layoutsMenu = FindMainMenuWithName("LayoutsMenu");
        ASSERT_TRUE(layoutsMenu) << "Unable to find layouts menu.";

        // First test the save current item, so that it's included in the select and remove tests.
        TestSaveLayoutMenuItem(layoutsMenu);

        TestSelectLayoutMenuItems(layoutsMenu);

        TestRemoveMenuItems(layoutsMenu);
    }
} // namespace EMotionFX
