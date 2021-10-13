/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzFramework/Physics/ShapeConfiguration.h"
#include <ActorFixture.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <Tests/D6JointLimitConfiguration.h>
#include <Tests/Mocks/PhysicsSystem.h>

namespace EMotionFX
{
    class RagdollCommandTests : public ActorFixture
    {
    public:
        void SetUp() override
        {
            using ::testing::_;

            ActorFixture::SetUp();

            D6JointLimitConfiguration::Reflect(GetSerializeContext());

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
    protected:
        Physics::MockJointHelpersInterface m_jointHelpers;
    };

    AZStd::vector<AZStd::string> GetRagdollJointNames(const Actor* actor)
    {
        const PhysicsSetup* physicsSetup = actor->GetPhysicsSetup().get();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        AZStd::vector<AZStd::string> result;
        result.reserve(ragdollConfig.m_nodes.size());

        for (const auto& nodeConfig : ragdollConfig.m_nodes)
        {
            result.emplace_back(nodeConfig.m_debugName);
        }

        return result;
    }

    TEST_F(RagdollCommandTests, AddJointLowerInHierarchy)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZStd::vector<AZStd::string> jointsToLeftShoulder {
            "jack_root",
            "Bip01__pelvis",
            "spine1",
            "spine2",
            "spine3",
            "l_shldr",
        };

        const AZStd::vector<AZStd::string> jointsToLeftHand {
            "jack_root",
            "Bip01__pelvis",
            "spine1",
            "spine2",
            "spine3",
            "l_shldr",
            "l_upArm",
            "l_loArm",
            "l_hand",
        };

        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_shldr"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftShoulder
            )
        );

        const AZStd::string serializedBeforeHandAdded = SerializePhysicsSetup(GetActor());

        // Adding l_hand should add l_upArm and l_loArm as well
        commandGroup.RemoveAllCommands();
        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        const AZStd::string serializedAfterHandAdded = SerializePhysicsSetup(GetActor());

        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );

        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftShoulder
            )
        );
        EXPECT_THAT(SerializePhysicsSetup(GetActor()), StrEq(serializedBeforeHandAdded));

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );
        EXPECT_THAT(SerializePhysicsSetup(GetActor()), StrEq(serializedAfterHandAdded));
    }

    TEST_F(RagdollCommandTests, AddJointHigherInHierarchy)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZStd::vector<AZStd::string> jointsToLeftHand {
            "jack_root",
            "Bip01__pelvis",
            "spine1",
            "spine2",
            "spine3",
            "l_shldr",
            "l_upArm",
            "l_loArm",
            "l_hand",
        };

        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );

        // l_shldr should already be in the ragdoll, so adding it should do nothing
        commandGroup.RemoveAllCommands();
        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_shldr"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );

        // Undo here undoes the addition of l_hand
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_TRUE(GetRagdollJointNames(GetActor()).empty());

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );
    }

    TEST_F(RagdollCommandTests, AddJointAddsAllTheWayToTheRoot)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZStd::vector<AZStd::string> jointsToLeftHand {
            "jack_root",
            "Bip01__pelvis",
            "spine1",
            "spine2",
            "spine3",
            "l_shldr",
            "l_upArm",
            "l_loArm",
            "l_hand",
        };

        // Add a joint to the ragdoll that does not make a chain all the way to the root
        EXPECT_TRUE(commandManager.ExecuteCommand(aznew CommandAddRagdollJoint(GetActor()->GetID(), "l_shldr"), result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(StrEq(), AZStd::vector<AZStd::string>{"l_shldr"})
        );

        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(StrEq(), jointsToLeftHand)
        );
    }

    TEST_F(RagdollCommandTests, RemoveJointRemovesChildren)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const AZStd::vector<AZStd::string> jointsToSpine3 {
            "jack_root",
            "Bip01__pelvis",
            "spine1",
            "spine2",
            "spine3",
        };

        // Add joints from the root to the left hand
        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        // Removing the left shoulder should remove the elbow, wrist, and hand
        // as well
        commandGroup.RemoveAllCommands();
        CommandRagdollHelpers::RemoveJointsFromRagdoll(GetActor()->GetID(), {"l_shldr"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_THAT(
            GetRagdollJointNames(GetActor()),
            testing::UnorderedPointwise(StrEq(), jointsToSpine3)
        );
    }

    TEST_F(RagdollCommandTests, RemoveJointThenUndoRestoresColliders)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        CommandRagdollHelpers::AddJointsToRagdoll(GetActor()->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        const AZStd::string serializedBeforeSphereAdded = SerializePhysicsSetup(GetActor());

        CommandColliderHelpers::AddCollider(GetActor()->GetID(), "l_hand",
            PhysicsSetup::Ragdoll, azrtti_typeid<Physics::SphereShapeConfiguration>());

        const AZStd::string serializedAfterSphereAdded = SerializePhysicsSetup(GetActor());

        EXPECT_THAT(serializedAfterSphereAdded, ::testing::Not(StrEq(serializedBeforeSphereAdded)));

        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();

        EXPECT_THAT(SerializePhysicsSetup(GetActor()), StrEq(serializedBeforeSphereAdded));

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();

        EXPECT_THAT(SerializePhysicsSetup(GetActor()), StrEq(serializedAfterSphereAdded));
    }
} // namespace EMotionFX
