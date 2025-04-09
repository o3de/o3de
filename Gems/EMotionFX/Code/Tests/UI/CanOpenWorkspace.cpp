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
#include <QToolBar>
#include <QCheckBox>

#include <Tests/UI/MenuUIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/EMStudioSDK/Source/ResetSettingsDialog.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>

namespace EMotionFX
{
    class CanOpenWorkspaceFixture
        : public MenuUIFixture
    {
    public:
        void SetUp() override
        {
            UIFixture::SetUp();

            GetEMotionFX().InitAssetFolderPaths();

            m_saveDataPath = CreateTempSaveFolder();

            EMStudio::GetManager()->SetSkipSourceControlCommands(true);
        }

        void TearDown() override
        {
            QDir(m_saveDataPath).removeRecursively();

            UIFixture::TearDown();
        }

        QString CreateTempSaveFolder() const
        {
            int fileIndex = 0;
            bool madeDir = false;
            const QString baseDir = GetEMotionFX().GetAssetCacheFolder().c_str();

            while(!madeDir)
            {
                const QString dirname = QString("tmpdata_%1").arg(fileIndex);
                const QString dirpath = QString("%1/%2").arg(baseDir, dirname);

                if (!QDir(dirpath).exists())
                {
                    QDir().mkdir(dirpath);
                    madeDir = true;
                    return dirpath;
                }

                fileIndex++;
            }

            return nullptr;
        }

        QString GenerateTempAssetFile(const QString& filenamebase, const QString& extension) const
        {
            bool foundFilename = false;
            int fileIndex = 0;

            while (!foundFilename)
            {
                const QString filename = QString("%1_%2").arg(filenamebase).arg(fileIndex);
                const QString filepath = QString("%1/%2.%3").arg(m_saveDataPath, filename, extension);

                if (!QFile::exists(filepath))
                {
                    return filepath;
                }
                fileIndex++;
            }

            return nullptr;
        }

        QString GenerateTempAnimGraphFilename() const
        {
            return GenerateTempAssetFile("tmpanimgraph", "animgraph");
        }

        QString GenerateTempWorkspaceFilename() const
        {
            return GenerateTempAssetFile("tmpworkspace", "emfxworkspace");
        }

        QString GenerateTempMotionSetFilename() const
        {
            return GenerateTempAssetFile("tmpmotionset", "motionset");
        }

        void ResetAll()
        {
            // Set up a callback to set the correct checkbox states when the ResetSettings dialog appears.
            WidgetActiveCallback resetSettingsCallback = [](QWidget* widget)
            {
                ASSERT_TRUE(widget) << "Failed to find Reset widget.";

                // Set all checkboxes
                QList<QCheckBox*>checkBoxes = widget->findChildren<QCheckBox*>();
                for (QCheckBox *checkBox : checkBoxes)
                {
                    checkBox->setChecked(true);
                }
                
                // Press the Ok button
                QDialogButtonBox* buttonBox = widget->findChild< QDialogButtonBox*>();
                ASSERT_TRUE(buttonBox) << "Unable to find button box in ResetSettingsDialog";

                QAbstractButton* button = buttonBox->button(QDialogButtonBox::Ok);
                ASSERT_TRUE(button) << "Unable to find Ok button in ResetSettingsDialog";

                QTest::mouseClick(button, Qt::LeftButton);

            };

            // Setup a watcher to handle the save dirty settings dialog by pressing Discard.
            ModalPopupHandler saveDirtyPopupHandler;
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Discard);

            // Setup a handler to select the type to reset and press ok.
            ModalPopupHandler resetSettingsHandler;
            resetSettingsHandler.WaitForPopup<EMStudio::ResetSettingsDialog*>(resetSettingsCallback);

            QMenu* fileMenu = MenuUIFixture::FindMainMenuWithName("EMFX.MainWindow.FileMenu");
            ASSERT_TRUE(fileMenu) << "Unable to find file menu.";

            QAction* resetMenuAction = MenuUIFixture::FindMenuActionWithObjectName(fileMenu, "EMFX.MainWindow.ResetAction", fileMenu->objectName());
            ASSERT_TRUE(resetMenuAction) << "Unable to find reset menu action.";

            resetMenuAction->trigger();

            saveDirtyPopupHandler.WaitForCompletion();
            resetSettingsHandler.WaitForCompletion();

            ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), 1) << "The default motion set should be present.";
            ASSERT_FALSE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to reset AnimGraphs.";
        }

        void GenerateTestWorkspace()
        {
            // AnimGraph
            m_animGraphPlugin->GetViewWidget()->OnCreateAnimGraph();
            ASSERT_TRUE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to create AnimGraph.";

            CreateAnimGraphParameter("TestParam1");
            CreateAnimGraphParameter("TestParam2");
            CreateAnimGraphParameter("TestParam3");

            // Motion
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);

            auto motionSetPlugin = static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
            ASSERT_TRUE(motionSetPlugin) << "No motion sets plugin found";

            motionSetPlugin->SetSelectedSet(motionSet);

            const EMStudio::MotionSetWindow* motionSetWindow = motionSetPlugin->GetMotionSetWindow();
            ASSERT_TRUE(motionSetWindow) << "No motion set window found";

            QWidget* addMotionButton = GetWidgetWithNameFromNamedToolbar(motionSetWindow, "MotionSetWindow.ToolBar", "MotionSetWindow.ToolBar.AddANewEntry");
            ASSERT_TRUE(addMotionButton);
            QTest::mouseClick(addMotionButton, Qt::LeftButton);

            // Check there is now a motion.
            EXPECT_EQ(motionSet->GetNumMotionEntries(), 1);
        }

        void SaveAll()
        {
            // Set filenames for everything we can to avoid system file requestors.
            const QString motionsetFilename = GenerateTempMotionSetFilename();
            EMotionFX::MotionSet *motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
            motionSet->SetFilename(motionsetFilename.toUtf8().constData());

            EMStudio::Workspace* workspace = EMStudio::GetManager()->GetWorkspace();
            m_workspaceSavePath = GenerateTempWorkspaceFilename();
            workspace->SetFilename(m_workspaceSavePath.toUtf8().data());

            const QString animGraphFilename = GenerateTempAnimGraphFilename();
            EMotionFX::AnimGraph* animGraph = m_animGraphPlugin->GetActiveAnimGraph();
            animGraph->SetFileName(animGraphFilename.toUtf8().constData());

            const QMenu* fileMenu = MenuUIFixture::FindMainMenuWithName("EMFX.MainWindow.FileMenu");
            ASSERT_TRUE(fileMenu) << "Unable to find file menu.";

            ModalPopupHandler saveDirtyPopupHandler;
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Ok);

            QAction* saveAllAction = MenuUIFixture::FindMenuActionWithObjectName(fileMenu, "EMFX.MainWindow.SaveAllAction", fileMenu->objectName());
            ASSERT_TRUE(saveAllAction) << "Save All menu item not found";
            ASSERT_TRUE(saveAllAction->isEnabled());

            saveAllAction->trigger();
        }

    protected:
        QString m_saveDataPath;
        QString m_workspaceSavePath;
    };

    TEST_F(CanOpenWorkspaceFixture, CanAddAnimGraph)
    {
        RecordProperty("test_case_id", "C953542");

        GenerateTestWorkspace();

        SaveAll();

        ResetAll();

        //Check everything has gone.
        ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), 1) << "The default motion set should be present.";
        ASSERT_FALSE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to reset AnimGraphs.";

        // Reload the saved workspace and check everything reappears:
        // Discard any unsaved changes
        ModalPopupHandler saveDirtyPopupHandler;
        saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Discard);

        EMStudio::GetMainWindow()->LoadFile(m_workspaceSavePath.toUtf8().constData());

        // Check all the expected items are back
        ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), 1) << "Failed to create MotionSets in workspace load.";
        ASSERT_TRUE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to create AnimGraph in workspace load.""Failed to create parameters in workspace load.";
        ASSERT_EQ(m_animGraphPlugin->GetActiveAnimGraph()->GetNumParameters(), 3) << "Failed to create Parameters in workspace load.";
    }
} // namespace EMotionFX
