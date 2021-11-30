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

#include <Tests/UI/UIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanAddJointAndChildren)
    {
        /*
        Test Case: C13048819
        Can Add Joint And Children
        Creates an actor, navigates the context menu to invoke "Add Joint and Children", then validates that it functions correctly.
        */

        RecordProperty("test_case_id", "C13048819");

        // Create an actor
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 5, "SimpleActor");
        ActorInstance* actorInstance = ActorInstance::Create(actorAsset->GetActor());
        const Actor* actor = actorAsset->GetActor();

        // Open simulated objects layout
        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        // Execute command to select actor instance
        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(AZStd::string{ "Select -actorInstanceID " } +AZStd::to_string(actorInstance->GetID()), result)) << result.c_str();
        }

        auto simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not found!";

        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        const QModelIndex rootJointIndex = model->index(0, 0);
        ASSERT_TRUE(rootJointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(rootJointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        // Open context menu
        treeView->scrollTo(rootJointIndex);
        const QRect rect = treeView->visualRect(rootJointIndex);
        ASSERT_TRUE(rect.isValid());
        {
            QContextMenuEvent cme(QContextMenuEvent::Mouse, rect.center(), treeView->viewport()->mapTo(treeView->window(), rect.center()));
            QSpontaneKeyEvent::setSpontaneous(&cme);
            QApplication::instance()->notify(
                treeView->viewport(),
                &cme
            );
        }

        // "Simulated object" -> "Add joints and children" -> "<New simulated object>"
        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* simulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(simulatedObjectAction, contextMenu, "Add to simulated object"));
        QMenu* simulatedObjectMenu = simulatedObjectAction->menu();
        EXPECT_TRUE(simulatedObjectMenu);
        QAction* newSimulatedObjectAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(newSimulatedObjectAction, simulatedObjectMenu, "New simulated object..."));

        // Handle the add children dialog box.
        ModalPopupHandler messageBoxPopupHandler;
        messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::Yes);
        newSimulatedObjectAction->trigger();

        EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("EMFX.SimulatedObjectActionManager.SimulatedObjectDialog"));
        ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

        inputDialog->SetText("Joint and Children Simulated Object");
        inputDialog->accept();

        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
        const auto simulatedObject = actor->GetSimulatedObjectSetup()->GetSimulatedObject(0);
        EXPECT_STREQ(simulatedObject->GetName().c_str(), "Joint and Children Simulated Object");
        EXPECT_EQ(simulatedObject->GetNumSimulatedRootJoints(), 1);
        EXPECT_EQ(simulatedObject->GetNumSimulatedJoints(), 5);
        EXPECT_STREQ(actor->GetSkeleton()->GetNode(simulatedObject->GetSimulatedJoint(0)->GetSkeletonJointIndex())->GetName(), "rootJoint");
        EXPECT_STREQ(actor->GetSkeleton()->GetNode(simulatedObject->GetSimulatedRootJoint(0)->GetSkeletonJointIndex())->GetName(), "rootJoint");

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        actorInstance->Destroy();
    }
} // namespace EMotionFX
