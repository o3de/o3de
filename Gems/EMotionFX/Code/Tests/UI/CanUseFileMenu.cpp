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
#include <QMenu>
#include <QApplication>
#include <QGridLayout>
#include <QDir>
#include <QCheckBox>

#include <Tests/UI/MenuUIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/EMStudioSDK/Source/ResetSettingsDialog.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>


namespace EMotionFX
{
    class CanUseFileMenuUIFixture
        : public MenuUIFixture
    {
    public:

        void SetUp() override
        {
            MenuUIFixture::SetUp();
            
            GetEMotionFX().InitAssetFolderPaths();

            m_animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
            ASSERT_TRUE(m_animGraphPlugin);

            EMStudio::GetManager()->SetSkipSourceControlCommands(true);
        }

        void TearDown() override
        {
            QDir(GetAssetSaveFolder()).removeRecursively();

            MenuUIFixture::TearDown();
        }

        QString GetAssetSaveFolder() const
        {
            auto testAssetsPath = AZ::IO::Path(GetEMotionFX().GetAssetCacheFolder()) / "tmptestassets";
            QString dataDir = QString::fromUtf8(testAssetsPath.c_str(), aznumeric_cast<int>(testAssetsPath.Native().size()));

            if (!QDir(dataDir).exists())
            {
                QDir().mkdir(dataDir);
            }

            return dataDir;
        }

        QString GenerateTempAssetFile(const QString& filenamebase, const QString& extension) const
        {
            bool foundFilename = false;
            int fileIndex = 0;
            QString baseDir = GetAssetSaveFolder();

            while (!foundFilename)
            {
                const QString filename = QString("%1_%2").arg(filenamebase).arg(fileIndex);
                const QString filepath = QString("%1/%2.%3").arg(baseDir, filename, extension);

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

        QString GenerateTempActorFilename() const
        {
            return GenerateTempAssetFile("tmpactor", "actor");
        }

        QString GenerateTempMotionSetFilename() const
        {
            return GenerateTempAssetFile("tmpmotionset", "motionset");
        }

        QString GenerateTempMotionFilename() const
        {
            return GenerateTempAssetFile("tmpmotion", "motion");
        }
            
        void CreateAnimGraph()
        {
            if (!AnimGraphExists())
            {
                m_animGraphPlugin->GetViewWidget()->OnCreateAnimGraph();
                ASSERT_TRUE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to create AnimGraph.";
            }
        }

        void CreateActor()
        {
            if (EMotionFX::GetActorManager().GetNumActorInstances() == 0)
            {
                AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
                AZ::Data::Asset<Integration::ActorAsset> actorAsset =
                    TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 2, "CanAddSimulatedObjectWithJointsActor");
                ActorInstance::Create(actorAsset->GetActor());

                EXPECT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 1) << "Failed to create actor set for reset test.";
            }
        }

        void CreateAndSaveActor(const char* filename) const
        {
            if (EMotionFX::GetActorManager().GetNumActorInstances() == 0)
            {
                AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
                AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(
                    actorAssetId, 2, "CanAddSimulatedObjectWithJointsActor");
                ActorInstance::Create(actorAsset->GetActor());

                EXPECT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 1) << "Failed to create actor set for reset test.";
                actorAsset->GetActor()->SetFileName(filename);

                AZStd::string stringFilename = filename;
                ExporterLib::SaveActor(stringFilename, actorAsset->GetActor(), MCore::Endian::ENDIAN_LITTLE);
            }
        }

        void LoadActor(const char* filename, const bool replaceScene)
        {
            ModalPopupHandler saveDirtyPopupHandler;

            saveDirtyPopupHandler.WaitForPopupPressDialogButton<EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Ok);

            EMStudio::GetMainWindow()->LoadActor(filename, replaceScene);
        }

        void CreateMotion()
        {
            if (EMotionFX::GetMotionManager().GetNumMotions() == 0)
            {
                LoadTestMotion();
                ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotions(), 1) << "Failed to create motion for reset test.";
            }
        }

        void DeleteAnimGraph()
        {
            EMotionFX::AnimGraph* animGraph = m_animGraphPlugin->GetActiveAnimGraph();
            GetEventManager().OnDeleteAnimGraph(animGraph);
        }

        bool AnimGraphExists() const
        {
            EMotionFX::AnimGraph* animGraph = m_animGraphPlugin->GetActiveAnimGraph();
            return animGraph;
        }

        void SaveCurrentAnimGraph(const QString& filename)
        {
            // Set the save filename to avoid a file select dialog.
            EMotionFX::AnimGraph* animGraph = m_animGraphPlugin->GetActiveAnimGraph();
            animGraph->SetFileName(filename.toUtf8().constData());

            m_animGraphPlugin->OnFileSave();

            ASSERT_TRUE(QFile::exists(filename)) << "Failed to save AnimGraph.";
        }

        void TestActorMenus(QMenu* fileMenu)
        {
            // Open Actor
            // We can't use the Open Actor menu item as it would involve interacting with a system requestor, so just do what the menu option does internally.

            //Clear any existing actors before we start.
            QAction* resetAction = GetResetMenuAction(fileMenu);
            ASSERT_TRUE(resetAction) << "Reset menu item not found";
            TestResetMenuItem(resetAction, "EMFX.ResetSettingsDialog.Actors");
            ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 0) << "Failed to reset Actors.";

            // Create an actor and save it so we can reload it for the merge step.
            const QString actorFilename = GenerateTempActorFilename();
            CreateAndSaveActor(actorFilename.toUtf8().data());
           
            // Clear out the existing actors so we can tell whether the load works.
            TestResetMenuItem(resetAction, "EMFX.ResetSettingsDialog.Actors");
            ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 0) << "Failed to reset Actors.";

            // Load the actor we just saved, with replaceScene set to true to represent a load.
            LoadActor(actorFilename.toUtf8().data(), true);
            ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 1) << "Failed to load Actor.";

            // Do it again to verify that number of actors stays the same when replaceScene is true.
            LoadActor(actorFilename.toUtf8().data(), true);
            ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 1) << "Failed to load Actor.";

            // Now load again, but with replaceScene set to false, as the merge code does.
            LoadActor(actorFilename.toUtf8().data(), false);
            ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 2) << "Failed to merge Actor.";

            // We can't test Save Selected Actor as we would it would involve mocking source scene handling.

            // Add the filename to the recent actorsa anyway, so we can test that functionality.
            EMStudio::GetMainWindow()->AddRecentActorFile(actorFilename);
          
            // Check for the file saved in the Save test to be listed in the recent actors submenu.
            const QList <QMenu*> recentMenus = fileMenu->findChildren<QMenu*>("EMFX.MainWindow.RecentFilesMenu");
            QMenu* recentActorsMenu = nullptr;
            for (QMenu* recentMenu : recentMenus)
            {
                if (recentMenu->title() == "Recent Actors")
                {
                    recentActorsMenu = recentMenu;
                }
            }

            ASSERT_TRUE(recentActorsMenu) << "Unable to find recent actors menu.";

            const QList<QAction*> actions = recentActorsMenu->findChildren<QAction *>();
            QAction* recentAction = nullptr;

            for (QAction* action : actions)
            {
                if (IsActionRecentlySavedActor(action->text(), actorFilename))
                {
                    recentAction = action;
                }
            }

            ASSERT_TRUE(recentAction) << "Recent action for last saved actor not found.";

            recentAction->trigger();

            ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 1) << "Failed to load recent Actor.";

            QAction* resetRecentAction = recentActorsMenu->findChild<QAction *>("EMFX.RecentFiles.ResetRecentFilesAction");
            ASSERT_TRUE(resetRecentAction) << "Reset recent actors action not found.";

            resetRecentAction->trigger();

            const QList<QAction*> actionsAfterReset = recentActorsMenu->findChildren<QAction *>();
            ASSERT_EQ(actionsAfterReset.size(), 1) << "Failed to reset recent items menu.";
        }

        bool IsActionRecentlySavedActor(const QString& actionTitle, const QString& actorFilename) const
        {
            if (actionTitle.isEmpty())
            {
                return false;
            }

            QFileInfo fileInfo(actionTitle);
            QString fileName = fileInfo.fileName();

            // Remove the shortcut at the start.
            const int shortcutLen = fileName.indexOf(" ");
            fileName = fileName.mid(shortcutLen + 1);

            return actorFilename.endsWith(fileName);
        }

        void TestSaveWorkspaceMenuOption([[maybe_unused]] QMenu* fileMenu, const QString& workspaceFilename)
        {
            EMStudio::Workspace* workspace = EMStudio::GetManager()->GetWorkspace();
            ASSERT_TRUE(workspace) << "Current workspace not found";

            // Create an anim graph so that there is unsaved data
            CreateAnimGraph();

            // The workspace needs to have a file to save to, as we can't interact with the SaveAs dialog.
            workspace->SetFilename(workspaceFilename.toUtf8().constData());

            // If we try to save now, we'll be asked to select a save file for the anim graph, we need to provide one to avoid that.
            const QString animGraphFilename = GenerateTempAnimGraphFilename();
            SaveCurrentAnimGraph(animGraphFilename);

            // Pretend editing the anim graph
            EMotionFX::AnimGraph* animGraph = m_animGraphPlugin->GetActiveAnimGraph();
            animGraph->SetDirtyFlag(true);

            // Skip the motion set.
            GetMotionManager().GetMotionSet(0)->SetDirtyFlag(false);

            // Prepare a watcher to press the ok button when the SaveDirtySettingsWindow appears.
            ModalPopupHandler saveDirtyPopupHandler;

            saveDirtyPopupHandler.WaitForPopupPressDialogButton<EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Ok);

            EMStudio::GetMainWindow()->OnFileSaveWorkspace();

            ASSERT_TRUE(saveDirtyPopupHandler.GetSeenTargetWidget()) << "Expected SaveDirtySettingsWindow not found.";

            ASSERT_TRUE(QFile::exists(workspaceFilename)) << "Workspace save failed.";
        }

        bool IsActionRecentlySavedWorkspace(const QString& actionTitle) const
        {
            if (actionTitle.isEmpty())
            {
                return false;
            }

            QFileInfo fileInfo(actionTitle);
            QString fileName = fileInfo.fileName();

            // Remove the shortcut at the start.
            const int shortcutLen = fileName.indexOf(" ");
            fileName = fileName.mid(shortcutLen + 1);

            return m_lastSavedWorkspaceFilename.endsWith(fileName);
        }

        void TestNewWorkspaceMenuOption(QMenu* fileMenu)
        {
            // Create an anim graph so that there is unsaved data
            CreateAnimGraph();

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Test 1. Select New Workspace, press cancel in the SaveDirtySettingsWindow.
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Start waiting for the "do you want to save" dialog, then press Cancel.
            ModalPopupHandler saveDirtyPopupHandler;
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Cancel);

            // Trigger the new workspace menu option
            QAction* newWorkspaceAction = MenuUIFixture::FindMenuActionWithObjectName(fileMenu, "EMFX.MainWindow.NewWorkspaceAction", "EMFX.MainWindow.FileMenu");
            ASSERT_TRUE(newWorkspaceAction);
            newWorkspaceAction->trigger();

            // If the dialog does not appear, we might get here before the callback is triggered, so check that it was.
            ASSERT_TRUE(saveDirtyPopupHandler.GetSeenTargetWidget()) << "Expected SaveDirtySettingsWindow not found.";

            // The dialog should now be gone but the anim graph we made should still exist.
            ASSERT_FALSE(QApplication::activeModalWidget()) << "SaveDirtySettingsWindow failed to close.";

            ASSERT_TRUE(AnimGraphExists()) << "AnimGraph not found.";

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Test 2. Select New Workspace, press Discard in the SaveDirtySettingsWindow.
            // Then press No in the new workspace confirmation dialog.
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Start waiting for the "do you want to save" dialog, then press Discard.
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Discard);

            // Start waiting for the "really make new workspace?" confirmation popup, then press No.
            ModalPopupHandler messageBoxPopupHandler;
            messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::No);

            newWorkspaceAction->trigger();

            saveDirtyPopupHandler.WaitForCompletion();
            messageBoxPopupHandler.WaitForCompletion();

            // If the dialog does not appear, we might get here before the callback is triggered, so check that it was.
            ASSERT_TRUE(saveDirtyPopupHandler.GetSeenTargetWidget()) << "Expected SaveDirtySettingsWindow not found.";

            ASSERT_TRUE(messageBoxPopupHandler.GetSeenTargetWidget()) << "Expected QMessageBox not found.";

            // The dialog should now be gone and the anim graph we made should still be there.
            ASSERT_FALSE(QApplication::activeModalWidget()) << "SaveDirtySettingsWindow failed to close.";

            ASSERT_TRUE(AnimGraphExists()) << "AnimGraph not found.";

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Test 3. Select New Workspace, press discard in the SaveDirtySettingsWindow.
            // Then press Yes in the new workspace confirmation dialog.
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // Start waiting for the "do you want to save" dialog, then press Discard.
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Discard);

            // Start waiting for the "really make new workspace?" confirmation popup, then press Yes.
            messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::Yes);

            newWorkspaceAction->trigger();

            // If the dialog does not appear, we might get here before the callback is triggered, so check that it did.
            ASSERT_TRUE(saveDirtyPopupHandler.GetSeenTargetWidget()) << "Expected SaveDirtySettingsWindow not found.";
            ASSERT_TRUE(messageBoxPopupHandler.GetSeenTargetWidget()) << "Expected QMessageBox not found.";

            // There should be no active popup.
            ASSERT_FALSE(QApplication::activeModalWidget()) << "SaveDirtySettingsWindow failed to close.";

            // The AnimGraph should be removed.
            ASSERT_FALSE(AnimGraphExists()) << "AnimGraph not removed.";
        }

        void TestRecentWorkspacesMenuOption(QMenu* fileMenu)
        {
            // Remove the AnimGraph so that we can tell the workspace has been reloaded correctly.
            DeleteAnimGraph();
            ASSERT_FALSE(AnimGraphExists()) << "AnimGraph not removed.";

            // Check for the file saved in the Save test to be listed.
            const QList <QMenu*> recentMenus = fileMenu->findChildren<QMenu*>("EMFX.MainWindow.RecentFilesMenu");
            QMenu* recentWorkspacesMenu = nullptr;
            for (QMenu* recentMenu : recentMenus)
            {
                if (recentMenu->title() == "Recent Workspaces")
                {
                    recentWorkspacesMenu = recentMenu;
                }
            }

            ASSERT_TRUE(recentWorkspacesMenu) << "Unable to find recent workspaces menu.";

            const QList<QAction*> actions = recentWorkspacesMenu->findChildren<QAction *>();
            QAction* recentAction = nullptr;

            for (QAction* action : actions)
            {
                if (IsActionRecentlySavedWorkspace(action->text()))
                {
                    recentAction = action;
                }
            }

            ASSERT_TRUE(recentAction) << "Recent action for last saved workspace not found.";

            // Now try to use the action to reload the workspace. 

            // As we've deleted the AnimGraph, we'll be asked about saving changes, discard them.
            ModalPopupHandler saveDirtyPopupHandler;
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Discard);

            recentAction->trigger();

            ASSERT_TRUE(AnimGraphExists()) << "AnimGraph not found after reloading recent workspace.";

            // Test the clear recent items action.
            QAction* resetRecentAction = recentWorkspacesMenu->findChild<QAction *>("EMFX.RecentFiles.ResetRecentFilesAction");
            ASSERT_TRUE(resetRecentAction) << "Reset recent workspaces action not found.";

            resetRecentAction->trigger();

            const QList<QAction*> actionsAfterReset = recentWorkspacesMenu->findChildren<QAction *>();
            ASSERT_EQ(actionsAfterReset.size(), 1) << "Failed to reset workspaces items menu.";
        }

        void CreateDataForResetTest()
        {
            CreateActor();
            CreateMotion();
            CreateAnimGraph();
        }

        void TestResetMenuItem(QAction* resetMenuAction, const QString& resetItemName)
        {
            // Set up a callback to set the correct checkbox states when the ResetSettings dialog appears.
            WidgetActiveCallback resetSettingsCallback = [resetItemName](QWidget* widget)
            {
                ASSERT_TRUE(widget) << "Failed to find Reset widget.";

                if (resetItemName == "*")
                {
                    // Set all checkboxes
                    QList<QCheckBox*>checkBoxes = widget->findChildren<QCheckBox*>();
                    for (QCheckBox *checkBox : checkBoxes)
                    {
                        checkBox->setChecked(true);
                    }
                }
                else
                {
                    // Reset all checkboxes
                    QList<QCheckBox*>checkBoxes = widget->findChildren<QCheckBox*>();
                    for (QCheckBox *checkBox : checkBoxes)
                    {
                        QString on = checkBox->objectName();
                        checkBox->setChecked(false);
                    }

                    // Just tick the one we want
                    QCheckBox* checkBox = widget->findChild<QCheckBox*>(resetItemName);
                    ASSERT_TRUE(checkBox) << "Failed to find reset item checkbox ";
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

            resetMenuAction->trigger();

            saveDirtyPopupHandler.WaitForCompletion();
            resetSettingsHandler.WaitForCompletion();
        }

        QString GetTestMotionFileName() const
        {
            AZStd::string resolvedAssetPath = this->ResolvePath("@gemroot:EMotionFX@/Code/Tests/TestAssets/Rin/rin_idle.motion");
            return QString::fromUtf8(resolvedAssetPath.data(), aznumeric_cast<int>(resolvedAssetPath.size()));
        }

        void LoadTestMotion()
        {
            const QString testFile = GetTestMotionFileName();
            ASSERT_TRUE(QFile::exists(testFile)) << "Failed to find motion file asset.";

            AZStd::vector<AZStd::string> motionFilenames;
            motionFilenames.push_back(testFile.toUtf8().data());
            CommandSystem::LoadMotionsCommand(motionFilenames);
        }

        QAction* GetResetMenuAction(const QMenu* fileMenu) const
        {
            return MenuUIFixture::FindMenuActionWithObjectName(fileMenu, "EMFX.MainWindow.ResetAction", fileMenu->objectName());
        }

        void TestResetMenuItem(QMenu* fileMenu)
        {            
            QAction* resetAction = GetResetMenuAction(fileMenu);
            ASSERT_TRUE(resetAction) << "Reset menu item not found";

            ASSERT_TRUE(resetAction->isEnabled()) << "Reset menu action is disabled.";

            // Make one of everything to reset
            CreateDataForResetTest();

            // Reset them one at a time:
            {
                // Actors
                TestResetMenuItem(resetAction, "EMFX.ResetSettingsDialog.Actors");
                ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 0) << "Failed to reset Actors.";

                // Motions
                TestResetMenuItem(resetAction, "EMFX.ResetSettingsDialog.Motions");
                ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotions(), 0) << "Failed to reset Motions.";

                // Motion Sets
                TestResetMenuItem(resetAction, "EMFX.ResetSettingsDialog.MotionSets");
                ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), 1) << "Failed to reset MotionSets. Default motion set should be present.";

                // AnimGraphs
                TestResetMenuItem(resetAction, "EMFX.ResetSettingsDialog.AnimGraphs");
                ASSERT_FALSE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to reset AnimGraphs.";
            }

            // The reset menu item should be disabled as there is nothing to reset.
            ASSERT_FALSE(resetAction->isEnabled()) << "Reset menu action is enabled after resetting all items.";

            // Recreate test data
            CreateDataForResetTest();

            // Reset them all at once
            {
                TestResetMenuItem(resetAction, "*");
                ASSERT_EQ(EMotionFX::GetActorManager().GetNumActorInstances(), 0) << "Failed to reset Actors.";
                ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotions(), 0) << "Failed to reset Motions.";
                ASSERT_EQ(EMotionFX::GetMotionManager().GetNumMotionSets(), 1) << "Failed to reset MotionSets. Default motion set should be present.";
                ASSERT_FALSE(m_animGraphPlugin->GetActiveAnimGraph()) << "Failed to reset AnimGraphs.";
            }
        }

        void TestSaveAllMenuItem(QMenu* fileMenu)
        {
            // Use the test reset menu item to ensure everything is cleared out.
            QAction* resetAction = GetResetMenuAction(fileMenu);
            ASSERT_TRUE(resetAction) << "Reset menu item not found";
            TestResetMenuItem(resetAction, "*");

            // Make new sets of all data types, give them a unique filename and set them as dirty.
            CreateAnimGraph();
            const QString animGraphFilename = GenerateTempAnimGraphFilename();
            EMotionFX::AnimGraph* animGraph = m_animGraphPlugin->GetActiveAnimGraph();
            animGraph->SetFileName(animGraphFilename.toUtf8().constData());
            animGraph->SetDirtyFlag(true);

            const QString motionsetFilename = GenerateTempMotionSetFilename();
            EMotionFX::MotionSet *motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
            motionSet->SetFilename(motionsetFilename.toUtf8().constData());
            motionSet->SetDirtyFlag(true);

            // Don't create an actor or motion as we can't save that due to source scene requirements.

            EMStudio::Workspace* workspace = EMStudio::GetManager()->GetWorkspace();
            const QString workspaceFilename = GenerateTempWorkspaceFilename();
            workspace->SetFilename(workspaceFilename.toUtf8().data());
            workspace->SetDirtyFlag(true);

            ModalPopupHandler saveDirtyPopupHandler;
            saveDirtyPopupHandler.WaitForPopupPressDialogButton< EMStudio::SaveDirtySettingsWindow*>(QDialogButtonBox::Ok);

            QAction* saveAllAction = MenuUIFixture::FindMenuActionWithObjectName(fileMenu, "EMFX.MainWindow.SaveAllAction", fileMenu->objectName());
            ASSERT_TRUE(saveAllAction) << "Save All menu item not found";
            ASSERT_TRUE(saveAllAction->isEnabled());

            saveAllAction->trigger();

            ASSERT_TRUE(QFile::exists(animGraphFilename)) << "Failed to save AnimGraph in SaveAll action.";
            ASSERT_TRUE(QFile::exists(motionsetFilename)) << "Failed to save MotionSet in SaveAll action.";
            ASSERT_TRUE(QFile::exists(workspaceFilename)) << "Failed to save Workspace in SaveAll action.";
        }

        void TestWorkspaceMenuItems(QMenu* fileMenu)
        {
            m_lastSavedWorkspaceFilename = GenerateTempWorkspaceFilename();

            TestNewWorkspaceMenuOption(fileMenu);

            // We can't test Open Workspace as it requires interacting with a system file dialog.

            TestSaveWorkspaceMenuOption(fileMenu, m_lastSavedWorkspaceFilename);

            // We can't test Save As as it requires interacting with a system file dialog.

            TestRecentWorkspacesMenuOption(fileMenu);

         }

        private:
            EMStudio::AnimGraphPlugin* m_animGraphPlugin = nullptr;

            QString m_lastSavedWorkspaceFilename;
    };

    TEST_F(CanUseFileMenuUIFixture, CanUseFileMenu)
    {
        RecordProperty("test_case_id", "C1698601");
        RecordProperty("test_case_id", "C16302183");
        RecordProperty("test_case_id", "C1698617");

        // Find the File menu.
        QMenu* fileMenu = MenuUIFixture::FindMainMenuWithName("EMFX.MainWindow.FileMenu");
        ASSERT_TRUE(fileMenu) << "Unable to find file menu.";

        TestWorkspaceMenuItems(fileMenu);

        TestResetMenuItem(fileMenu);

        // Temporarily disable loading actor test.
        // This is because the importer command now load actor asset instead of reading from disk. We do not want to add dependency to the asset processor
        // in this test.
        // TestActorMenus(fileMenu);

        TestSaveAllMenuItem(fileMenu);
    }
} // namespace EMotionFX
