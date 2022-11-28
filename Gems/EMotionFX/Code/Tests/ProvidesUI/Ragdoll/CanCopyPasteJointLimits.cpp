/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <gtest/gtest.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Physics/SystemBus.h>

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>
#include <Editor/Plugins/ColliderWidgets/RagdollOutlinerNotificationHandler.h>
#include <Editor/Plugins/ColliderWidgets/RagdollNodeWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/D6JointLimitConfiguration.h>
#include <Tests/Mocks/PhysicsSystem.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/UI/SkeletonOutlinerTestFixture.h>

namespace EMotionFX
{
    class CopyPasteRagdollJointLimitsFixture : public SkeletonOutlinerTestFixture
    {
    public:
        void SetUp() override
        {
            using testing::_;

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

            SkeletonOutlinerTestFixture::SetUp();
        }
    protected:
        virtual bool ShouldReflectPhysicSystem() override { return true; }

        Physics::MockJointHelpersInterface m_jointHelpers;
    };

    TEST_F(CopyPasteRagdollJointLimitsFixture, TestJointLimits)
    {
        SetUpPhysics();


        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"rootJoint", "joint1", "joint2", "joint3"});
        const Physics::RagdollConfiguration& ragdollConfig = m_actor->GetPhysicsSetup()->GetRagdollConfig();
        ASSERT_EQ(ragdollConfig.m_nodes.size(), 4);
        AZStd::shared_ptr<D6JointLimitConfiguration> rootJointLimit = AZStd::rtti_pointer_cast<D6JointLimitConfiguration>(ragdollConfig.FindNodeConfigByName("rootJoint")->m_jointConfig);
        ASSERT_TRUE(rootJointLimit);
        // Set the initial joint limits on the rootJoint to be something different
        rootJointLimit->m_swingLimitY = 1.0f;
        rootJointLimit->m_swingLimitZ = 100.0f;

        auto skeletonOutlinerPlugin = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
        ASSERT_TRUE(skeletonOutlinerPlugin) << "Skeleton outliner plugin not found.";

        SkeletonModel* model = skeletonOutlinerPlugin->GetModel();
        const QModelIndex rootIndex = model->index(0, 0, model->index(0, 0));
        const QModelIndex joint1Index = model->index(0, 0, rootIndex);
        const QModelIndex joint2Index = model->index(0, 0, joint1Index);
        const QModelIndex joint3Index = model->index(0, 0, joint2Index);
        EXPECT_TRUE(rootIndex.isValid());
        EXPECT_TRUE(joint1Index.isValid());
        EXPECT_TRUE(joint2Index.isValid());
        EXPECT_TRUE(joint3Index.isValid());
        EXPECT_EQ(rootIndex.data(SkeletonModel::COLUMN_NAME).toString(), QString("rootJoint"));
        EXPECT_EQ(joint1Index.data(SkeletonModel::COLUMN_NAME).toString(), QString("joint1"));
        EXPECT_EQ(joint2Index.data(SkeletonModel::COLUMN_NAME).toString(), QString("joint2"));
        EXPECT_EQ(joint3Index.data(SkeletonModel::COLUMN_NAME).toString(), QString("joint3"));

        // Select the rootJoint
        QItemSelectionModel& selectionModel = model->GetSelectionModel();
        selectionModel.select(rootIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        auto nodeWidget = GetJointPropertyWidget()->findChild<RagdollNodeWidget*>();
        ASSERT_TRUE(nodeWidget);
        EXPECT_FALSE(nodeWidget->HasCopiedJointLimits());

        auto jointLimitWidget = GetJointPropertyWidget()->findChild<RagdollJointLimitWidget*>();
        ASSERT_TRUE(jointLimitWidget);
        {
            // Copy the joint limits for the rootJoint
            jointLimitWidget->contextMenuRequested({});
            auto copyAction = jointLimitWidget->findChild<QAction*>("EMFX.RagdollJointLimitWidget.CopyJointLimitsAction");
            ASSERT_TRUE(copyAction);
            copyAction->trigger();
            // Process events so that the menu is destroyed
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        EXPECT_TRUE(nodeWidget->HasCopiedJointLimits());

        // Select joint1
        selectionModel.select(joint1Index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        // Verify initial state of joint1's limits
        const AZStd::shared_ptr<D6JointLimitConfiguration> joint1Limit =
            AZStd::rtti_pointer_cast<D6JointLimitConfiguration>(ragdollConfig.FindNodeConfigByName("joint1")->m_jointConfig);
        EXPECT_EQ(joint1Limit->m_swingLimitY, 45.0f);
        EXPECT_EQ(joint1Limit->m_swingLimitZ, 45.0f);

        // Paste joint limits from rootJoint
        {
            jointLimitWidget->contextMenuRequested({});
            auto pasteAction = jointLimitWidget->findChild<QAction*>("EMFX.RagdollJointLimitWidget.PasteJointLimitsAction");
            ASSERT_TRUE(pasteAction);
            pasteAction->trigger();
            // Process events so that the menu is destroyed
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        EXPECT_EQ(joint1Limit->m_swingLimitY, 1.0f);
        EXPECT_EQ(joint1Limit->m_swingLimitZ, 100.0f);

        // Select joint2 and joint3. They have to be selected independently
        // because they have different parents
        selectionModel.select(joint2Index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        selectionModel.select(joint3Index, QItemSelectionModel::Select | QItemSelectionModel::Rows);

        const AZStd::shared_ptr<D6JointLimitConfiguration> joint2Limit =
            AZStd::rtti_pointer_cast<D6JointLimitConfiguration>(ragdollConfig.FindNodeConfigByName("joint2")->m_jointConfig);
        EXPECT_EQ(joint2Limit->m_swingLimitY, 45.0f);
        EXPECT_EQ(joint2Limit->m_swingLimitZ, 45.0f);
        const AZStd::shared_ptr<D6JointLimitConfiguration> joint3Limit =
            AZStd::rtti_pointer_cast<D6JointLimitConfiguration>(ragdollConfig.FindNodeConfigByName("joint3")->m_jointConfig);
        EXPECT_EQ(joint3Limit->m_swingLimitY, 45.0f);
        EXPECT_EQ(joint3Limit->m_swingLimitZ, 45.0f);

        QTreeView* skeletonTreeView = skeletonOutlinerPlugin->GetDockWidget()->findChild<QTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");
        ASSERT_TRUE(skeletonTreeView);
        {
            skeletonTreeView->customContextMenuRequested({});
            auto pasteAction = skeletonOutlinerPlugin->GetDockWidget()->findChild<QAction*>("EMFX.RagdollNodeInspectorPlugin.PasteJointLimitsAction");
            ASSERT_TRUE(pasteAction);
            pasteAction->trigger();
            // Process events so that the menu is destroyed
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        EXPECT_EQ(joint2Limit->m_swingLimitY, 1.0f);
        EXPECT_EQ(joint2Limit->m_swingLimitZ, 100.0f);
        EXPECT_EQ(joint3Limit->m_swingLimitY, 1.0f);
        EXPECT_EQ(joint3Limit->m_swingLimitZ, 100.0f);
    }
} // namespace EMotionFX
