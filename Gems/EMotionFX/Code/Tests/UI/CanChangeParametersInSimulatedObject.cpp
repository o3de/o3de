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

#include <AzToolsFramework/UI/PropertyEditor/PropertyCheckBoxCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <Editor/ObjectEditor.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/ReselectingTreeView.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    class CanChangeParametersInSimulatedObjectFixture
        : public UIFixture
    {
      public:
    };

    TEST_F(CanChangeParametersInSimulatedObjectFixture, CanChangeParametersInSimulatedObject)
    {
        RecordProperty("test_case_id", "C14519563");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 7, "CanAddToSimulatedObjectActor");
        ActorInstance* actorInstance = ActorInstance::Create(actorAsset->GetActor());

        // Change the Editor mode to Simulated Objects
        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        // Select the newly created actor instance
        AZStd::string result;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(
            AZStd::string{"Select -actorInstanceID "} + AZStd::to_string(actorInstance->GetID()), result)) << result.c_str();

        EMotionFX::SkeletonOutlinerPlugin* skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(
            EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        EXPECT_TRUE(skeletonOutliner);
        ReselectingTreeView* skeletonTreeView =
            skeletonOutliner->GetDockWidget()->findChild<ReselectingTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        EXPECT_TRUE(skeletonTreeView);
        const QAbstractItemModel* skeletonModel = skeletonTreeView->model();

        QModelIndexList indexList;
        skeletonTreeView->RecursiveGetAllChildren(skeletonModel->index(0, 0, skeletonModel->index(0, 0)), indexList);
        EXPECT_EQ(indexList.size(), 7);

        SelectIndexes(indexList, skeletonTreeView, 2, 4);

        // Bring up the contextMenu so we can add joints to the simulated object
        const QRect rect = skeletonTreeView->visualRect(indexList[3]);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(skeletonTreeView, rect);

        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* addSelectedJointAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addSelectedJointAction, contextMenu, "Add to simulated object"));
        QMenu* addSelectedJointMenu = addSelectedJointAction->menu();
        EXPECT_TRUE(addSelectedJointMenu);
        QAction* newSimulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(newSimulatedObjectAction, addSelectedJointMenu, "New simulated object..."));

        // Handle the add children dialog box.
        ModalPopupHandler messageBoxPopupHandler;
        messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::No);
        newSimulatedObjectAction->trigger();

        EMStudio::InputDialogValidatable* inputDialog =
            qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("EMFX.SimulatedObjectActionManager.SimulatedObjectDialog"));
        ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

        inputDialog->SetText("TestObj");
        inputDialog->accept();

        // Find the Simulated Object Manager and its button
        EMotionFX::SimulatedObjectWidget* simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(
            EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not found!";

        ReselectingTreeView* treeView =
            simulatedObjectWidget->GetDockWidget()->findChild<ReselectingTreeView*>("EMFX.SimulatedObjectWidget.TreeView");
        ASSERT_TRUE(treeView);
        const SimulatedObjectModel* model = reinterpret_cast<SimulatedObjectModel*>(treeView->model());

        indexList.clear();
        treeView->RecursiveGetAllChildren(model->index(0, 0), indexList);
        EXPECT_EQ(indexList.size(), 4);

        SelectIndexes(indexList, treeView, 1, 3);

        // Check all joints have a false AutoExclusion
        for (int i = 1; i <= 3; ++i)
        {
            SimulatedJoint* joint = indexList[i].data(SimulatedObjectModel::ROLE_JOINT_PTR).value<SimulatedJoint*>();
            EXPECT_FALSE(joint->IsGeometricAutoExclusion());
        }

        const SimulatedJointWidget* simulatedJointWidget = simulatedObjectWidget->GetSimulatedJointWidget();
        EXPECT_TRUE(simulatedJointWidget) << "SimulatedJointWidget not found.";

        ObjectEditor* objectEditor = simulatedJointWidget->findChild<ObjectEditor*>("EMFX.SimulatedJointWidget.SimulatedJointEditor");
        EXPECT_TRUE(objectEditor) << "Can't find Object Editor";
        AzToolsFramework::ReflectedPropertyEditor* propertyEditor =
            objectEditor->findChild<AzToolsFramework::ReflectedPropertyEditor*>("PropertyEditor");
        EXPECT_TRUE(propertyEditor) << "Can't find RPE";

        AzToolsFramework::PropertyRowWidget* checkBoxWidget = reinterpret_cast<AzToolsFramework::PropertyRowWidget*>(
            GetNamedPropertyRowWidgetFromReflectedPropertyEditor(propertyEditor, "Geometric auto exclude"));
        EXPECT_TRUE(checkBoxWidget) << "Geometric Auto Exclude not found";

        AzToolsFramework::PropertyCheckBoxCtrl* checkBoxCtrl =
            reinterpret_cast<AzToolsFramework::PropertyCheckBoxCtrl*>(checkBoxWidget->GetChildWidget());
        EXPECT_TRUE(checkBoxCtrl);
        QCheckBox* checkBox = checkBoxCtrl->GetCheckBox();
        EXPECT_TRUE(checkBox) << "CheckBox not found";
        checkBox->click();

        // Check all joints have a true AutoExclusion
        for (int i = 1; i <= 3; ++i)
        {
            SimulatedJoint* joint = indexList[i].data(SimulatedObjectModel::ROLE_JOINT_PTR).value<SimulatedJoint*>();
            EXPECT_TRUE(joint->IsGeometricAutoExclusion());
        }

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        actorInstance->Destroy();
    }
}   // namespace EMotionFX
