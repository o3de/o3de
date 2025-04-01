/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AzCore/std/algorithm.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>

#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/Plugins/ColliderWidgets/RagdollOutlinerNotificationHandler.h>
#include <Editor/Plugins/ColliderWidgets/RagdollNodeWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/D6JointLimitConfiguration.h>
#include <Tests/Mocks/PhysicsSystem.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>
#include <UI/SkeletonOutlinerTestFixture.h>

namespace EMotionFX
{
    class CopyPasteRagdollCollidersFixture : public SkeletonOutlinerTestFixture
    {
    public:
        void SetUp() override
        {
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
                .WillRepeatedly([]([[maybe_unused]] const AZ::TypeId& jointLimitTypeId,
                                   [[maybe_unused]] const AZ::Quaternion& parentWorldRotation,
                                   [[maybe_unused]] const AZ::Quaternion& childWorldRotation,
                                   [[maybe_unused]] const AZ::Vector3& axis,
                                   [[maybe_unused]] const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
                                { return AZStd::make_unique<D6JointLimitConfiguration>(); });

            SkeletonOutlinerTestFixture::SetUp();
        }

      // todo teardown m_join.reset()
    protected:
        virtual bool ShouldReflectPhysicSystem() override { return true; }

        Physics::MockPhysicsSystem m_physicsSystem;
        Physics::MockPhysicsInterface m_physicsInterface;
        Physics::MockJointHelpersInterface m_jointHelpers;
    };

    TEST_F(CopyPasteRagdollCollidersFixture, CanCopyCollider)
    {
        using ::testing::_;

        SetUpPhysics();

        const Physics::RagdollConfiguration& ragdollConfig = m_actor->GetPhysicsSetup()->GetRagdollConfig();
        const Physics::CharacterColliderConfiguration& simulatedObjectConfig = m_actor->GetPhysicsSetup()->GetSimulatedObjectColliderConfig();

        CommandRagdollHelpers::AddJointsToRagdoll(
            m_actor->GetID(),
            {"rootJoint", "joint1", "joint2", "joint3"},
            /*commandGroup =*/nullptr,
            /* executeInsideCommand =*/false,
            /* addDefaultCollider =*/false);
        EXPECT_EQ(ragdollConfig.m_colliders.m_nodes.size(), 0);

        CommandColliderHelpers::AddCollider(
            m_actor->GetID(),
            "rootJoint",
            PhysicsSetup::Ragdoll,
            azrtti_typeid<Physics::BoxShapeConfiguration>(),
            /*commandGroup =*/nullptr,
            /*executeInsideCommand =*/false);
        EXPECT_EQ(ragdollConfig.m_colliders.m_nodes.size(), 1);

        EMStudio::GetMainWindow()->ApplicationModeChanged("Physics");

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorId " + AZStd::to_string(m_actor->GetID()),
                result))
                << result.c_str();
        }

        SkeletonModel* model = m_skeletonOutlinerPlugin->GetModel();
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

        auto* ragdollColliderContainer = GetJointPropertyWidget()->findChild<RagdollNodeWidget*>()->findChild<ColliderContainerWidget*>();
        ASSERT_TRUE(ragdollColliderContainer);

        // Copy the Box collider
        ragdollColliderContainer->CopyCollider(0);

        // Paste Box collider on joint1's ragdoll
        selectionModel.select(joint1Index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        ragdollColliderContainer->PasteCollider(0, false);
        EXPECT_EQ(ragdollConfig.m_colliders.m_nodes.size(), 2);

        const auto* joint1ColliderConfig = AZStd::find_if(
            begin(ragdollConfig.m_colliders.m_nodes),
            end(ragdollConfig.m_colliders.m_nodes),
            [](const Physics::CharacterColliderNodeConfiguration& config) { return config.m_name == "joint1"; });
        ASSERT_NE(joint1ColliderConfig, end(ragdollConfig.m_colliders.m_nodes));
        EXPECT_EQ(joint1ColliderConfig->m_shapes.size(), 1);
        EXPECT_EQ(joint1ColliderConfig->m_shapes[0].second->GetShapeType(), Physics::ShapeType::Box);

        EMStudio::GetMainWindow()->ApplicationModeChanged("SimulatedObjects");

        // Paste Box collider on joint1's simulated object
        auto* simulatedObjectPlugin = EMStudio::GetPluginManager()->FindActivePlugin<SimulatedObjectWidget>();
        ASSERT_TRUE(simulatedObjectPlugin) << "Simulated object plugin not found.";

        QDockWidget* simulatedObjectInspectorDock = EMStudio::GetMainWindow()->findChild<QDockWidget*>(
            "EMFX.SimulatedObjectWidget.SimulatedObjectInspectorDock");
        ASSERT_TRUE(simulatedObjectInspectorDock);

        auto* simulatedObjectColliderContainer = simulatedObjectInspectorDock->findChild<ColliderContainerWidget*>();
        ASSERT_TRUE(simulatedObjectColliderContainer);

        EXPECT_EQ(simulatedObjectConfig.m_nodes.size(), 0);
        simulatedObjectColliderContainer->PasteCollider(0, false);
        EXPECT_EQ(simulatedObjectConfig.m_nodes.size(), 1);
        EXPECT_THAT(simulatedObjectConfig.m_nodes[0].m_name, StrEq("joint1"));
        EXPECT_EQ(simulatedObjectConfig.m_nodes[0].m_shapes.size(), 1);
        EXPECT_EQ(simulatedObjectConfig.m_nodes[0].m_shapes[0].second->GetShapeType(), Physics::ShapeType::Box);
    }
} // namespace EMotionFX
