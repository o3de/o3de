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

#include <Editor/Plugins/Ragdoll/RagdollNodeInspectorPlugin.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>

#include <Editor/ReselectingTreeView.h>
#include <Tests/D6JointLimitConfiguration.h>
#include <Tests/Mocks/PhysicsSystem.h>

namespace EMotionFX
{
    class RagdollEditTestsFixture : public UIFixture
    {
      public:
        void SetUp() override
        {
            using ::testing::_;

            SetupQtAndFixtureBase();

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            Physics::MockPhysicsSystem::Reflect(serializeContext); // Required by Ragdoll plugin to fake PhysX Gem is available
            D6JointLimitConfiguration::Reflect(serializeContext);

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
                    []([[maybe_unused]] const AZ::TypeId& jointLimitTypeId, [[maybe_unused]] const AZ::Quaternion& parentWorldRotation,
                       [[maybe_unused]] const AZ::Quaternion& childWorldRotation, [[maybe_unused]] const AZ::Vector3& axis,
                       [[maybe_unused]] const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
                    {
                        return AZStd::make_unique<D6JointLimitConfiguration>();
                    });

            SetupPluginWindows();
        }

        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            UIFixture::TearDown();
        }

        void CreateSkeletonAndModelIndices()
        {
            // Select the newly created actor
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorID 0", result)) << result.c_str();

            // Change the Editor mode to Physics
            EMStudio::GetMainWindow()->ApplicationModeChanged("Physics");

            // Get the SkeletonOutlinerPlugin and find its treeview
            m_skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
            EXPECT_TRUE(m_skeletonOutliner);
            m_treeView = m_skeletonOutliner->GetDockWidget()->findChild<ReselectingTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");

            m_indexList.clear();
            m_treeView->RecursiveGetAllChildren(m_treeView->model()->index(0, 0), m_indexList);
        }

    protected:
        QModelIndexList m_indexList;
        ReselectingTreeView* m_treeView;
        EMotionFX::SkeletonOutlinerPlugin* m_skeletonOutliner;
        Physics::MockJointHelpersInterface m_jointHelpers;
    };


#if AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_EDITOR_TESTS
    TEST_F(RagdollEditTestsFixture, DISABLED_RagdollAddJoint)
#else
    TEST_F(RagdollEditTestsFixture, RagdollAddJoint)
#endif // AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_EDITOR_TESTS
    {
        const int numJoints = 6;
        RecordProperty("test_case_id", "C3122249");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "RagdollEditTestsActor");

        CreateSkeletonAndModelIndices();

        EXPECT_EQ(m_indexList.size(), numJoints);

        // Find the RagdollNodeInspectorPlugin and its button
        auto ragdollNodeInspector = static_cast<EMotionFX::RagdollNodeInspectorPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::RagdollNodeInspectorPlugin::CLASS_ID));
        EXPECT_TRUE(ragdollNodeInspector) << "Ragdoll plugin not found!";

        auto addAddToRagdollButton = ragdollNodeInspector->GetDockWidget()->findChild<QPushButton*>("EMFX.RagdollNodeWidget.PushButton.RagdollAddRemoveButton");
        EXPECT_TRUE(addAddToRagdollButton) << "Add to ragdoll button not found!";

        // Find the 3rd joint after the RootJoint in the TreeView and select it
        SelectIndexes(m_indexList, m_treeView, 3, 3);

        // Send the left button click directly to the button
        QTest::mouseClick(addAddToRagdollButton, Qt::LeftButton);

        // Check the node is in the ragdoll
        EXPECT_TRUE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[3]));
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

        QMenu* contextMenu = m_skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        QAction* ragdollAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(ragdollAction, contextMenu, "Ragdoll"));
        QMenu* ragdollMenu = ragdollAction->menu();
        EXPECT_TRUE(ragdollMenu);
        QAction* addToRagdollAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addToRagdollAction, ragdollMenu, "Add to ragdoll"));
        addToRagdollAction->trigger();

        // Check the nodes are in the ragdoll
        EXPECT_TRUE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[3]));
        EXPECT_TRUE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[4]));
        EXPECT_TRUE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[5]));
        EXPECT_TRUE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[6]));

        // Remove context menu as it is rebuild below
        delete contextMenu;

        const QRect rect2 = m_treeView->visualRect(m_indexList[4]);
        EXPECT_TRUE(rect2.isValid());
        BringUpContextMenu(m_treeView, rect2);

        // Find the "Remove from ragdoll" entry and trigger it.
        contextMenu = m_skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        EXPECT_TRUE(contextMenu);
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(ragdollAction, contextMenu, "Ragdoll"));
        ragdollMenu = ragdollAction->menu();
        QAction* removeFromRagdollAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(removeFromRagdollAction, ragdollMenu, "Remove from ragdoll"));
        removeFromRagdollAction->trigger();

        // Check the nodes are removed from ragdoll
        EXPECT_FALSE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[3]));
        EXPECT_FALSE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[4]));
        EXPECT_FALSE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[5]));
        EXPECT_FALSE(RagdollNodeInspectorPlugin::IsNodeInRagdoll(m_indexList[6]));
    }
}
