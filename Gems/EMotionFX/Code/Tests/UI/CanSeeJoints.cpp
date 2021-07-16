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
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeWindowPlugin.h>
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

        // Find the NodeWindowPlugin
        auto nodeWindow = static_cast<EMStudio::NodeWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::NodeWindowPlugin::CLASS_ID));
        EXPECT_TRUE(nodeWindow) << "NodeWidow plugin not found!";

        // Select the newly created actor instance
        AZStd::string result;
        AZStd::string command = AZStd::string::format("Select -actorInstanceID %u", actorInstance->GetID());
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(command, result));

        QTreeWidget* treeWidget = nodeWindow->GetDockWidget()->findChild<EMStudio::NodeHierarchyWidget*>("EMFX.NodeWindowPlugin.NodeHierarchyWidget.HierarchyWidget")->GetTreeWidget();
        EXPECT_TRUE(treeWidget) << "Joint Outliner Hierarchy not found";

        // Select the node containing the mesh
        QTreeWidgetItem* meshNodeItem = treeWidget->invisibleRootItem()->child(0);
        EXPECT_TRUE(meshNodeItem) << "Character Node not found";

        QTreeWidgetItem* rootItem = meshNodeItem->child(0);
        EXPECT_TRUE(rootItem) << "Root node not found";
        EXPECT_TRUE(rootItem->data(0, Qt::EditRole) == "rootJoint");
        EXPECT_TRUE(rootItem->data(1, Qt::EditRole) == "Mesh");

        for (int i = 1; i < numJoints; ++i)
        {
            rootItem = rootItem->child(0);
            EXPECT_TRUE(rootItem);
            EXPECT_TRUE(rootItem->data(0, Qt::EditRole) == AZStd::string::format("joint%i", i).c_str());
            EXPECT_TRUE(rootItem->data(1, Qt::EditRole) == "Node");
        }
    }
}   // namespace EMotionFX
