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
#include <Tests/UI/MenuUIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <Editor/ColliderContainerWidget.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectColliderWidget.h>
#include <Editor/InputDialogValidatable.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/PhysicsSetupUtils.h>
#include <Editor/ReselectingTreeView.h>

#include <AzQtComponents/Components/Widgets/CardHeader.h>

namespace EMotionFX
{
    class CanAddSimulatedObjectFixture
        : public UIFixture
    {
    public:
        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            UIFixture::TearDown();
        }

        void RecursiveGetAllChildren(QTreeView* treeView, const QModelIndex& index, QModelIndexList& outIndicies)
        {
            outIndicies.push_back(index);
            for (int i = 0; i < treeView->model()->rowCount(index); ++i)
            {
                RecursiveGetAllChildren(treeView, treeView->model()->index(i, 0, index), outIndicies);
            }
        }

        void CreateSimulateObject(const char* objectName)
        {
            // Select the newly created actor
            {
                AZStd::string result;
                EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorID 0", result)) << result.c_str();
            }

            // Change the Editor mode to Simulated Objects
            EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

            // Find the Simulated Object Manager and its button
            m_simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
            ASSERT_TRUE(m_simulatedObjectWidget) << "Simulated Object plugin not found!";

            QPushButton* addSimulatedObjectButton = m_simulatedObjectWidget->GetDockWidget()->findChild<QPushButton*>("addSimulatedObjectButton");

            // Send the left button click directly to the button
            QTest::mouseClick(addSimulatedObjectButton, Qt::LeftButton);

            // In the Input Dialog set the name of the object and close the dialog
            EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("EMFX.SimulatedObjectActionManager.SimulatedObjectDialog"));
            ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

            inputDialog->SetText(objectName);
            inputDialog->accept();

            // There is one and only one simulated objects
            ASSERT_EQ(m_actorAsset->GetActor()->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
            // Check it has the correct name
            EXPECT_STREQ(m_actorAsset->GetActor()->GetSimulatedObjectSetup()->GetSimulatedObject(0)->GetName().c_str(), objectName);

        }

        void AddCapsuleColliderToJointIndex(int index)
        {
            m_skeletonTreeView->selectionModel()->clearSelection();

            // Find the indexed joint in the TreeView and select it
            SelectIndexes(m_indexList, m_skeletonTreeView, index, index);

            // Open the Right Click Context Menu
            const QRect rect = m_skeletonTreeView->visualRect(m_indexList[index-1]);
            EXPECT_TRUE(rect.isValid());
            BringUpContextMenu(m_skeletonTreeView, rect);

            QMenu* contextMenu = m_skeletonOutliner->GetDockWidget()->findChild<QMenu*>("contextMenu");

            // Trace down the sub Menus to Add Collider and select it
            QAction* simulatedObjectColliderAction = GetNamedAction(m_skeletonOutliner->GetDockWidget(), "Add collider");
            ASSERT_TRUE(simulatedObjectColliderAction);
            QMenu* simulatedObjectColliderMenu = simulatedObjectColliderAction->menu();

            const QList<QAction*> addSelectedColliderMenuActions = simulatedObjectColliderMenu->actions();
            auto addCapsuleColliderAction = AZStd::find_if(addSelectedColliderMenuActions.begin(), addSelectedColliderMenuActions.end(), [](const QAction* action) {
                return action->text() == "Capsule";
            });
            ASSERT_NE(addCapsuleColliderAction, addSelectedColliderMenuActions.end());

            size_t numCapsuleColliders = PhysicsSetupUtils::CountColliders(
                m_actorAsset->GetActor(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);

            (*addCapsuleColliderAction)->trigger();

            // Delete the context menu, otherwise there it will still be around during this frame as the Qt event loop has not been run.
            delete contextMenu;

            const size_t numCapsuleCollidersAfterAdd = PhysicsSetupUtils::CountColliders(
                m_actorAsset->GetActor(), PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);

            ASSERT_EQ(numCapsuleCollidersAfterAdd, numCapsuleColliders + 1);

            m_skeletonTreeView->selectionModel()->clearSelection();
        }

    protected:
        AZ::Data::Asset<Integration::ActorAsset> m_actorAsset;
        EMotionFX::SimulatedObjectWidget* m_simulatedObjectWidget = nullptr;

        EMotionFX::SkeletonOutlinerPlugin* m_skeletonOutliner = nullptr;
        ReselectingTreeView* m_skeletonTreeView = nullptr;
        const QAbstractItemModel* m_skeletonModel;
        QModelIndexList m_indexList;
    };

    TEST_F(CanAddSimulatedObjectFixture, CanAddSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048820");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 1, "CanAddSimulatedObjectActor");

        CreateSimulateObject("New simulated object");
    }

    TEST_F(UIFixture, CanAddSimulatedObjectWithJoints)
    {
        RecordProperty("test_case_id", "C13048818");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 2, "CanAddSimulatedObjectWithJointsActor");
        Actor* actor = actorAsset->GetActor();
        ActorInstance* actorInstance = ActorInstance::Create(actor);

        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(AZStd::string{"Select -actorInstanceID "} + AZStd::to_string(actorInstance->GetID()), result)) << result.c_str();
        }

        auto simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not found!";

        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        const QModelIndex rootJointIndex = model->index(0, 0, model->index(0, 0));
        ASSERT_TRUE(rootJointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(rootJointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        treeView->scrollTo(rootJointIndex);
        BringUpContextMenu(treeView, treeView->visualRect(rootJointIndex));

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

        EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("EMFX.SimulatedObjectActionManager.SimulatedObjectDialog"));
        ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

        inputDialog->SetText("New simulated object");
        inputDialog->accept();

        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 1);
        const auto simulatedObject = actor->GetSimulatedObjectSetup()->GetSimulatedObject(0);
        EXPECT_STREQ(simulatedObject->GetName().c_str(), "New simulated object");
        EXPECT_EQ(simulatedObject->GetNumSimulatedRootJoints(), 1);
        EXPECT_EQ(simulatedObject->GetNumSimulatedJoints(), 1);
        EXPECT_STREQ(actor->GetSkeleton()->GetNode(simulatedObject->GetSimulatedJoint(0)->GetSkeletonJointIndex())->GetName(), "rootJoint");
        EXPECT_STREQ(actor->GetSkeleton()->GetNode(simulatedObject->GetSimulatedRootJoint(0)->GetSkeletonJointIndex())->GetName(), "rootJoint");

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        actorInstance->Destroy();
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddSimulatedObjectWithJointsAndName)
    {
        RecordProperty("test_case_id", "C13048820a");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 5, "CanAddSimulatedObjectActor");
        Actor* actor = m_actorAsset->GetActor();

        CreateSimulateObject("sim1");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        // Find the 3rd joint in the TreeView and select it
        const QModelIndex jointIndex = model->index(0, 3, model->index(0, 0));
        ASSERT_TRUE(jointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(jointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        treeView->scrollTo(jointIndex);

        // Open the Right Click Context Menu
        const QRect rect = treeView->visualRect(jointIndex);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(treeView, rect);

        // Trace down the sub Menus to <New simulated object> and select it
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
        messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::No);

        newSimulatedObjectAction->trigger();

        // Set the name in the Dialog Box and test it.
        EMStudio::InputDialogValidatable* inputDialog = qobject_cast<EMStudio::InputDialogValidatable*>(FindTopLevelWidget("EMFX.SimulatedObjectActionManager.SimulatedObjectDialog"));
        ASSERT_NE(inputDialog, nullptr) << "Cannot find input dialog.";

        inputDialog->SetText("sim2");
        inputDialog->accept();

        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 2);
        const auto simulatedObject = actor->GetSimulatedObjectSetup()->GetSimulatedObject(1);
        EXPECT_STREQ(simulatedObject->GetName().c_str(), "sim2");
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddColliderToSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048816");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 5, "CanAddSimulatedObjectActor");
        Actor* actor = m_actorAsset->GetActor();

        CreateSimulateObject("sim1");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        const QAbstractItemModel* model = treeView->model();

        // Find the 3rd joint in the TreeView and select it
        const QModelIndex jointIndex = model->index(0, 3, model->index(0, 0));
        ASSERT_TRUE(jointIndex.isValid()) << "Unable to find a model index for the root joint of the actor";

        treeView->selectionModel()->select(jointIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        treeView->scrollTo(jointIndex);

        // Open the Right Click Context Menu
        const QRect rect = treeView->visualRect(jointIndex);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(treeView, rect);

        // Trace down the sub Menus to Add Collider and select it
        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* addSelectedAddColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addSelectedAddColliderAction, contextMenu, "Add collider"));
        QMenu* addSelectedColliderMenu = addSelectedAddColliderAction->menu();
        QAction* addCapsuleColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addCapsuleColliderAction, addSelectedColliderMenu, "Capsule"));

        size_t numCapsuleColliders = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        EXPECT_EQ(numCapsuleColliders, 0);

        addCapsuleColliderAction->trigger();

        // Check that a collider has been added.
        size_t numCollidersAfterAddCapsule = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        ASSERT_EQ(numCollidersAfterAddCapsule, numCapsuleColliders + 1) << "Capsule collider not added.";

        QAction* addSphereColliderAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addSphereColliderAction, addSelectedColliderMenu, "Sphere"));

        const size_t numSphereColliders = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        EXPECT_EQ(numSphereColliders, 0);

        addSphereColliderAction->trigger();

        // Check that a second collider has been added.
        const size_t numCollidersAfterAddSphere = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        ASSERT_EQ(numCollidersAfterAddSphere, numSphereColliders + 1) << "Sphere collider not added.";
    }

    TEST_F(CanAddSimulatedObjectFixture, CanRemoveSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048821");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 1, "CanRemoveSimulatedObjectActor");
        Actor* actor = m_actorAsset->GetActor();

        CreateSimulateObject("TestObject1");

        // Get the Simulated Object and find the model relating to its treeview
        EMotionFX::SimulatedObjectWidget* simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        ASSERT_TRUE(simulatedObjectWidget);
        QTreeView* treeView = simulatedObjectWidget->GetDockWidget()->findChild<QTreeView*>("EMFX.SimulatedObjectWidget.TreeView");
        ASSERT_TRUE(treeView);
        const SimulatedObjectModel* model = reinterpret_cast<SimulatedObjectModel*>(treeView->model());
        const QModelIndex index = model->index(0, 0);

        treeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->scrollTo(index);

        BringUpContextMenu(treeView, treeView->visualRect(index));
        QMenu* contextMenu = simulatedObjectWidget->GetDockWidget()->findChild<QMenu*>("EMFX.SimulatedObjectWidget.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* removeObjectAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(removeObjectAction, contextMenu, "Remove object"));
        removeObjectAction->trigger();

        ASSERT_EQ(actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects(), 0);
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddColliderToSimulatedObjectFromInspector)
    {
        RecordProperty("test_case_id", "C20385259");
        
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 5, "CanAddSimulatedObjectActor");
        Actor* actor = m_actorAsset->GetActor();

        CreateSimulateObject("sim1");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        ASSERT_TRUE(skeletonOutliner);
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        ASSERT_TRUE(treeView);
        const QAbstractItemModel* model = treeView->model();

        QModelIndexList indexList;
        RecursiveGetAllChildren(treeView, model->index(0, 0, model->index(0, 0)), indexList);

        SelectIndexes(indexList, treeView, 3, 3);

        QDockWidget* simulatedObjectInspectorDock = EMStudio::GetMainWindow()->findChild<QDockWidget*>("EMFX.SimulatedObjectWidget.SimulatedObjectInspectorDock");
        ASSERT_TRUE(simulatedObjectInspectorDock);
        QPushButton* addColliderButton = simulatedObjectInspectorDock->findChild<QPushButton*>("EMFX.SimulatedObjectColliderWidget.AddColliderButton");
        ASSERT_TRUE(addColliderButton);
        // Send the left button click directly to the button
        QTest::mouseClick(addColliderButton, Qt::LeftButton);

        const size_t numCapsuleColliders =
            PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        EXPECT_EQ(numCapsuleColliders, 0);

        const size_t numSphereColliders =
            PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        EXPECT_EQ(numSphereColliders, 0);

        QMenu* contextMenu = addColliderButton->findChild<QMenu*>("EMFX.AddColliderButton.ContextMenu");
        EXPECT_TRUE(contextMenu);

        QAction* addCapsuleAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addCapsuleAction, contextMenu, "Add capsule"));
        addCapsuleAction->trigger();
        const size_t numCollidersAfterAddCapsule =
            PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Capsule);
        ASSERT_EQ(numCollidersAfterAddCapsule, numCapsuleColliders + 1) << "Capsule collider not added.";

        QAction* addSphereAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(addSphereAction, contextMenu, "Add sphere"));
        addSphereAction->trigger();
        const size_t numCollidersAfterAddSphere =
            PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider, false, Physics::ShapeType::Sphere);
        ASSERT_EQ(numCollidersAfterAddSphere, numSphereColliders + 1) << "Sphere collider not added.";
    }

    TEST_F(CanAddSimulatedObjectFixture, CanAddMultipleJointsToSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048818");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 7, "CanAddSimulatedObjectActor");
        Actor* actor = m_actorAsset->GetActor();

        CreateSimulateObject("ANY");

        // Get the Skeleton Outliner and find the model relating to its treeview
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        ASSERT_TRUE(skeletonOutliner) << "Can't find SkeletonOutlinerPlugin";
        QTreeView* treeView = skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        ASSERT_TRUE(treeView) << "Skeleton Treeview not found";
        const QAbstractItemModel* model = treeView->model();

        QModelIndexList indexList;
        RecursiveGetAllChildren(treeView, model->index(0, 0, model->index(0, 0)), indexList);

        SelectIndexes(indexList, treeView, 3, 5);

        // Open the Right Click Context Menu
        const QRect rect = treeView->visualRect(indexList[4]);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(treeView, rect);

        // Trace down the sub Menus to <New simulated object> and select it
        QMenu* contextMenu = skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu) << "Context Menu not found";
        QAction* simulatedObjectAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(simulatedObjectAction, contextMenu, "Add to simulated object"));
        QMenu* simulatedObjectMenu = simulatedObjectAction->menu();
        EXPECT_TRUE(simulatedObjectMenu) << "Simulated Object Menu not found";
        QAction* newSimulatedObjectAction;
        ASSERT_TRUE(UIFixture::GetActionFromContextMenu(newSimulatedObjectAction, simulatedObjectMenu, "ANY")) << "Can't find named simulated object in menu";

        // Handle the add children dialog box.
        ModalPopupHandler messageBoxPopupHandler;
        messageBoxPopupHandler.WaitForPopupPressDialogButton<QMessageBox*>(QDialogButtonBox::No);
        newSimulatedObjectAction->trigger();

        const EMotionFX::SimulatedObject* simulatedObject = actor->GetSimulatedObjectSetup()->GetSimulatedObject(0);
        EXPECT_EQ(simulatedObject->GetNumSimulatedRootJoints(), 1);
        EXPECT_EQ(simulatedObject->GetNumSimulatedJoints(), 3);
    }
    TEST_F(CanAddSimulatedObjectFixture, CanRemoveColliderFromSimulatedObject)
    {
        RecordProperty("test_case_id", "C13048817");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        m_actorAsset = TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 5, "CanAddSimulatedObjectActor");
        Actor* actor = m_actorAsset->GetActor();

        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        CreateSimulateObject("sim1");

        m_skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        m_skeletonTreeView = m_skeletonOutliner->GetDockWidget()->findChild<ReselectingTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        m_skeletonModel = m_skeletonTreeView->model();

        m_indexList.clear();

        m_skeletonTreeView->RecursiveGetAllChildren(m_skeletonModel->index(0, 0, m_skeletonModel->index(0, 0)), m_indexList);

        // Add colliders to two joints.
        AddCapsuleColliderToJointIndex(3);
        AddCapsuleColliderToJointIndex(4);

        const size_t numCollidersAfterAdd = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider);
        EXPECT_EQ(numCollidersAfterAdd, 2);

        m_indexList.clear();

        m_skeletonTreeView->RecursiveGetAllChildren(m_skeletonModel->index(0, 0, m_skeletonModel->index(0, 0)), m_indexList);

        // Reselect joint 3 and pop up the context menu for it.
        m_skeletonTreeView->selectionModel()->clearSelection();
        SelectIndexes(m_indexList, m_skeletonTreeView, 3, 3);

        // Open the Right Click Context Menu
        const QRect rect = m_skeletonTreeView->visualRect(m_indexList[2]);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(m_skeletonTreeView, rect);

        const QList<QMenu*> contextMenus = m_skeletonOutliner->GetDockWidget()->findChildren<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_NE(contextMenus.size(), 0) << "Unable to find Skeketon Outliner context menu.";

        // There will be several existing menus, as the Qt event loop has not yet been run, so we need to find the latest and use that.
        QMenu* contextMenu = *(contextMenus.end() - 1);

        QAction* removeAction = contextMenu->findChild<QAction*>("EMFX.SimulatedObjectWidget.RemoveCollidersAction");
        ASSERT_TRUE(removeAction);
        
        removeAction->trigger();

        // Check that one of the colliders is now gone.
        const size_t numCollidersAfterFirstRemove = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider);
        ASSERT_EQ(numCollidersAfterFirstRemove, numCollidersAfterAdd - 1) << "RemoveCollider action in Simulated Object Inspector failed.";

        // Now do the same thing using the Simulated Object Inspector context menu.
        const SimulatedObjectColliderWidget* simulatedObjectColliderWidget = GetSimulatedObjectColliderWidget();
        ASSERT_TRUE(simulatedObjectColliderWidget) << "SimulatedObjectColliderWidget not found.";

        // Select the second collider that was made earlier.
        m_skeletonTreeView->selectionModel()->clearSelection();
        SelectIndexes(m_indexList, m_skeletonTreeView, 4, 4);

        const ColliderContainerWidget* colliderContainerWidget = simulatedObjectColliderWidget->findChild< ColliderContainerWidget*>();
        ASSERT_TRUE(colliderContainerWidget) << "ColliderContainerWidget not found.";

        // Get the collider widget card from the container.
        const ColliderWidget* colliderWidget = colliderContainerWidget->findChild<ColliderWidget*>();
        ASSERT_TRUE(colliderWidget) << "ColliderWidget not found.";

        const AzQtComponents::CardHeader* cardHeader = colliderWidget->findChild<AzQtComponents::CardHeader*>();
        ASSERT_TRUE(cardHeader) << "ColliderWidget CardHeader not found.";

        const QFrame* frame = cardHeader->findChild<QFrame*>("Background");
        ASSERT_TRUE(frame) << "ColliderWidget CardHeader Background Frame not found.";

        QPushButton* contextMenubutton = frame->findChild< QPushButton*>("ContextMenu");
        ASSERT_TRUE(contextMenubutton) << "ColliderWidget ContextMenu not found.";

        // Pop up the context menu.
        QTest::mouseClick(contextMenubutton, Qt::LeftButton);

        // Find the delete collider button and press it.
        const QMenu* collilderWidgetContextMenu = colliderWidget->findChild<QMenu*>("EMFX.ColliderContainerWidget.ContextMenu");

        QAction* delAction = collilderWidgetContextMenu->findChild<QAction*>("EMFX.ColliderContainerWidget.DeleteColliderAction");

        delAction->trigger();

        // Check that we have the number of colliders we started we expect.
        const size_t numCollidersAfterSecondRemove = PhysicsSetupUtils::CountColliders(actor, PhysicsSetup::SimulatedObjectCollider);
        ASSERT_EQ(numCollidersAfterSecondRemove, numCollidersAfterAdd - 2);
    }

} // namespace EMotionFX
