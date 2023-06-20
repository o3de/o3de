/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/NodeHierarchyWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/SkeletonModel.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>

#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    using CanSeeJointsFixture = UIFixture;

    TEST_F(CanSeeJointsFixture, CanSeeOpenGLAndNodesTab)
    {
        const int numJoints = 5;
        RecordProperty("test_case_id", "C16019759");

        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<PlaneActorWithJoints>(5, "JointTestsActor");

        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        // Change the Editor mode to Character
        EMStudio::GetMainWindow()->ApplicationModeChanged("Character");

        // Find the skeletonOutlinerPlugin
        auto skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(
            EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        EXPECT_TRUE(skeletonOutliner) << "SkeletonOutlinerPlugin plugin not found!";

        // Select the newly created actor instance
        AZStd::string result;
        AZStd::string command = AZStd::string::format("Select -actorInstanceID %u", actorInstance->GetID());
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, result));

        QTreeView* treeView =
            skeletonOutliner->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        EXPECT_TRUE(treeView) << "Skeleton Outliner Hierarchy not found";

        // Select the node containing the mesh
        const QAbstractItemModel* model = treeView->model();

        const QModelIndex rootJointIndex = model->index(0, SkeletonModel::COLUMN_NAME);
        EXPECT_TRUE(rootJointIndex.isValid()) << "Character Node not found";
        EXPECT_TRUE(rootJointIndex.data(Qt::DisplayRole) == "Character");

        QModelIndex rootItem = model->index(0, SkeletonModel::COLUMN_NAME, model->index(0, 0));
        EXPECT_TRUE(rootItem.isValid()) << "Root node not found";
        EXPECT_TRUE(rootItem.data(Qt::DisplayRole) == "rootJoint");

        for (int i = 1; i < numJoints; ++i)
        {
            rootItem = rootItem.model()->index(0, SkeletonModel::COLUMN_NAME, rootItem);
            EXPECT_TRUE(rootItem.data(Qt::DisplayRole) == AZStd::string::format("joint%i", i).c_str());
        }

        actorInstance->Destroy();
    }
} // namespace EMotionFX
