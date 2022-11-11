/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "SkeletonOutlinerTestFixture.h"
#include <Editor/ColliderHelpers.h>
#include <Editor/ObjectEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyDoubleSpinCtrl.hxx>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/SimpleActors.h>

#include <QApplication>
#include <QTest>

namespace EMotionFX
{
    TEST_F(SkeletonOutlinerTestFixture, AddClothCollider)
    {
        const int numJoints = 6;
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "ClothColliderTestsActor");

        CreateSkeletonAndModelIndices();

        EXPECT_EQ(m_indexList.size(), numJoints);

        // Find the 3rd joint after the RootJoint in the TreeView and select it
        SelectIndexes(m_indexList, m_treeView, 3, 3);

        auto* treeView = GetAddCollidersTreeView();
        auto index = treeView->model()->index(0, 0, treeView->model()->index(0, 0));
        treeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->clicked(index);


        auto* widget = GetJointPropertyWidget();
        auto* mw = new QMainWindow;
        mw->setFixedHeight(900);
        mw->layout()->addWidget(widget);
        widget->setFixedHeight(500);

        mw->show();
        for(int i = 0; i < 10000; i++)
            QApplication::processEvents();

        // Check the node is in the ragdoll
        EXPECT_TRUE(ColliderHelpers::NodeHasClothCollider(m_indexList[3]));
    }

    TEST_F(SkeletonOutlinerTestFixture, ChangeClothColliderValue)
    {
        EMStudio::GetMainWindow()->ApplicationModeChanged("Physics");
        const int numJoints = 6;
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "ClothColliderTestsActor");

        CreateSkeletonAndModelIndices();

        EXPECT_EQ(m_indexList.size(), numJoints);

        // Find the 3rd joint after the RootJoint in the TreeView and select it
        SelectIndexes(m_indexList, m_treeView, 3, 3);

        auto* treeView = GetAddCollidersTreeView();
        auto index = treeView->model()->index(0, 0, treeView->model()->index(0, 0));
        treeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->clicked(index);

        // Check the node is in the ragdoll
        EXPECT_TRUE(ColliderHelpers::NodeHasClothCollider(m_indexList[3]));

        // get the widget
        //auto* widget = GetJointPropertyWidget()->findChild<ClothJointWidget*>("EMotionFX.ClothJointWidget");
        auto* widget = GetJointPropertyWidget();
        auto* mw = new QMainWindow;
        mw->setFixedHeight(900);

        widget->setFixedHeight(800);
        // widget->show();
        mw->layout()->addWidget(widget);
        mw->show();
        // get a value widget
        auto propertyEditor = widget->findChild<AzToolsFramework::ReflectedPropertyEditor*>("PropertyEditor");

        // Get list of all PropertyRowWidgets (and their InstanceDataNodes)
        const auto list = propertyEditor->GetWidgets();
        ASSERT_GT(list.size(), 0) << "Did not find any PropertyRowWidgets";

        // Look for PropertyRowWidget for "Name"
        AzToolsFramework::PropertyRowWidget* propertyRow = nullptr;
        for (const auto& item : list)
        {
            if (item.second->objectName() == "Height")
            {
                propertyRow = item.second;
            }
        }

        QApplication::processEvents();
        // change it
        auto *lineEdit = static_cast<AzToolsFramework::PropertyDoubleSpinCtrl*>(propertyRow->GetChildWidget());
        //AZ_Assert(lineEdit) << "Did not find Editing handle";
        lineEdit->setValue(3.89);
        lineEdit->editingFinished();

        // watch the world burn
        while (true)
            QApplication::processEvents();

    }
} // namespace EMotionFX
