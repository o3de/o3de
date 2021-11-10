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
#include <Editor/Plugins/Ragdoll/RagdollNodeInspectorPlugin.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>

#include <Tests/D6JointLimitConfiguration.h>
#include <Tests/Mocks/PhysicsSystem.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    class CopyPasteRagdollCollidersFixture : public UIFixture
    {
    public:
        void SetUp() override
        {
            using ::testing::_;

            UIFixture::SetUp();

            AZ::SerializeContext* serializeContext = GetSerializeContext();

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
                .WillRepeatedly([]([[maybe_unused]] const AZ::TypeId& jointLimitTypeId,
                                   [[maybe_unused]] const AZ::Quaternion& parentWorldRotation,
                                   [[maybe_unused]] const AZ::Quaternion& childWorldRotation,
                                   [[maybe_unused]] const AZ::Vector3& axis,
                                   [[maybe_unused]] const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
                                { return AZStd::make_unique<D6JointLimitConfiguration>(); });
        }

    private:
        Physics::MockPhysicsSystem m_physicsSystem;
        Physics::MockPhysicsInterface m_physicsInterface;
        Physics::MockJointHelpersInterface m_jointHelpers;
    };

#if AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_EDITOR_TESTS
    TEST_F(CopyPasteRagdollCollidersFixture, DISABLED_CanCopyCollider)
#else
    TEST_F(CopyPasteRagdollCollidersFixture, CanCopyCollider)
#endif // AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_EDITOR_TESTS
    {
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, 4);
        const Actor* actor = actorAsset->GetActor();

        const Physics::RagdollConfiguration& ragdollConfig = actor->GetPhysicsSetup()->GetRagdollConfig();
        const Physics::CharacterColliderConfiguration& simulatedObjectConfig = actor->GetPhysicsSetup()->GetSimulatedObjectColliderConfig();

        CommandRagdollHelpers::AddJointsToRagdoll(
            actor->GetID(),
            {"rootJoint", "joint1", "joint2", "joint3"},
            /*commandGroup =*/nullptr,
            /* executeInsideCommand =*/false,
            /* addDefaultCollider =*/false);
        EXPECT_EQ(ragdollConfig.m_colliders.m_nodes.size(), 0);

        CommandColliderHelpers::AddCollider(
            actor->GetID(),
            "rootJoint",
            PhysicsSetup::Ragdoll,
            azrtti_typeid<Physics::BoxShapeConfiguration>(),
            /*commandGroup =*/nullptr,
            /*executeInsideCommand =*/false);
        EXPECT_EQ(ragdollConfig.m_colliders.m_nodes.size(), 1);

        EMStudio::GetMainWindow()->ApplicationModeChanged("Physics");

        {
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorId " + AZStd::to_string(actor->GetID()),
                result))
                << result.c_str();
        }

        auto* ragdollPlugin = EMStudio::GetPluginManager()->FindActivePlugin<RagdollNodeInspectorPlugin>();
        ASSERT_TRUE(ragdollPlugin) << "Ragdoll plugin not found.";
        ragdollPlugin->Init();

        auto* skeletonOutlinerPlugin = EMStudio::GetPluginManager()->FindActivePlugin<SkeletonOutlinerPlugin>();
        ASSERT_TRUE(skeletonOutlinerPlugin) << "Skeleton outliner plugin not found.";

        SkeletonModel* model = skeletonOutlinerPlugin->GetModel();
        const QModelIndex rootIndex = model->index(0, 0);
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

        auto* ragdollColliderContainer = ragdollPlugin->GetDockWidget()->findChild<ColliderContainerWidget*>();
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
