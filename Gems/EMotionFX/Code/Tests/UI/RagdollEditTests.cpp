/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QtTest>
#include <QTreeView>
#include <QPushButton>

#include <Editor/Plugins/ColliderWidgets/RagdollOutlinerNotificationHandler.h>
#include <Editor/Plugins/ColliderWidgets/RagdollNodeWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/ColliderHelpers.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/UI/SkeletonOutlinerTestFixture.h>

#include <Editor/ReselectingTreeView.h>
#include <Tests/D6JointLimitConfiguration.h>
#include <Tests/Mocks/PhysicsSystem.h>

namespace EMotionFX
{
    class RagdollEditTestsFixture : public SkeletonOutlinerTestFixture
    {
    public:
        void SetUp() override
        {
            //first setup expect_call, then run SetUp
            using ::testing::_;

            EXPECT_CALL(m_jointHelpers, GetSupportedJointTypeIds)
                .WillRepeatedly(testing::Return(AZStd::vector<AZ::TypeId>{ azrtti_typeid<D6JointLimitConfiguration>() }));

            EXPECT_CALL(m_jointHelpers, GetSupportedJointTypeId(_))
                .WillRepeatedly(
                    [](AzPhysics::JointType jointType) -> AZStd::optional<const AZ::TypeId>
                    {
                        if (jointType == AzPhysics::JointType::D6Joint)
                        {
                            return azrtti_typeid<D6JointLimitConfiguration>();
                        }
                        return AZStd::nullopt;
                    });

            EXPECT_CALL(m_jointHelpers, ComputeInitialJointLimitConfiguration(_, _, _, _, _))
                .WillRepeatedly(
                    []([[maybe_unused]] const AZ::TypeId& jointLimitTypeId,
                       [[maybe_unused]] const AZ::Quaternion& parentWorldRotation,
                       [[maybe_unused]] const AZ::Quaternion& childWorldRotation,
                       [[maybe_unused]] const AZ::Vector3& axis,
                       [[maybe_unused]] const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
                    {
                        return AZStd::make_unique<D6JointLimitConfiguration>();
                    });

            UIFixture::SetUp();
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            UIFixture::TearDown();
        }

    protected:
        virtual bool ShouldReflectPhysicSystem() override { return true; }

        Physics::MockPhysicsSystem m_physicsSystem;
        Physics::MockPhysicsInterface m_physicsInterface;
        Physics::MockJointHelpersInterface m_jointHelpers;
    };


    TEST_F(RagdollEditTestsFixture, RagdollAddJoint)
    {
        const int numJoints = 6;
        RecordProperty("test_case_id", "C3122249");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "RagdollEditTestsActor");

        CreateSkeletonAndModelIndices();

        EXPECT_EQ(m_indexList.size(), numJoints);

        // Find the 3rd joint after the RootJoint in the TreeView and select it
        SelectIndexes(m_indexList, m_treeView, 3, 3);

        QTreeView* treeView = GetAddCollidersTreeView();
        auto index = treeView->model()->index(2, 0);
        treeView->clicked(index);

        // Check the node is in the ragdoll
        EXPECT_TRUE(ColliderHelpers::NodeHasRagdoll(m_indexList[3]));
    }

    TEST_F(RagdollEditTestsFixture, RagdollAddJointsAndRemove)
    {
        const int numJoints = 8;
        RecordProperty("test_case_id", "C3122248");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "RagdollEditTestsActor");

        CreateSkeletonAndModelIndices();
        EXPECT_EQ(m_indexList.size(), numJoints);

        // Select four Indexes
        SelectIndexes(m_indexList, m_treeView, 3, 6);

        // Bring up the contextMenu
        const QRect rect = m_treeView->visualRect(m_indexList[5]);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(m_treeView, rect);

        QMenu* contextMenu = m_skeletonOutlinerPlugin->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* ragdollAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(ragdollAction, contextMenu, "Ragdoll"));
        QMenu* ragdollMenu = ragdollAction->menu();
        EXPECT_TRUE(ragdollMenu);
        QAction* addToRagdollAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addToRagdollAction, ragdollMenu, "Add to ragdoll"));
        addToRagdollAction->trigger();

        // Check the nodes are in the ragdoll
        EXPECT_TRUE(ColliderHelpers::NodeHasRagdoll(m_indexList[3]));
        EXPECT_TRUE(ColliderHelpers::NodeHasRagdoll(m_indexList[4]));
        EXPECT_TRUE(ColliderHelpers::NodeHasRagdoll(m_indexList[5]));
        EXPECT_TRUE(ColliderHelpers::NodeHasRagdoll(m_indexList[6]));

        // Remove context menu as it is rebuild below
        delete contextMenu;

        const QRect rect2 = m_treeView->visualRect(m_indexList[4]);
        EXPECT_TRUE(rect2.isValid());
        BringUpContextMenu(m_treeView, rect2);

        // Find the "Remove from ragdoll" entry and trigger it.
        contextMenu = m_skeletonOutlinerPlugin->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(ragdollAction, contextMenu, "Ragdoll"));
        ragdollMenu = ragdollAction->menu();
        QAction* removeFromRagdollAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(removeFromRagdollAction, ragdollMenu, "Remove from ragdoll"));
        removeFromRagdollAction->trigger();

        // Check the nodes are removed from ragdoll
        EXPECT_FALSE(ColliderHelpers::NodeHasRagdoll(m_indexList[3]));
        EXPECT_FALSE(ColliderHelpers::NodeHasRagdoll(m_indexList[4]));
        EXPECT_FALSE(ColliderHelpers::NodeHasRagdoll(m_indexList[5]));
        EXPECT_FALSE(ColliderHelpers::NodeHasRagdoll(m_indexList[6]));
    }
}
