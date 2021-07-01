/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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


namespace EMotionFX
{
    using RagdollCommandTests = ActorFixture;

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

        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_shldr"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftShoulder
            )
        );

        const AZStd::string serializedBeforeHandAdded = SerializePhysicsSetup(m_actor.get());

        // Adding l_hand should add l_upArm and l_loArm as well
        commandGroup.RemoveAllCommands();
        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        const AZStd::string serializedAfterHandAdded = SerializePhysicsSetup(m_actor.get());

        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );

        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftShoulder
            )
        );
        EXPECT_THAT(SerializePhysicsSetup(m_actor.get()), StrEq(serializedBeforeHandAdded));

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );
        EXPECT_THAT(SerializePhysicsSetup(m_actor.get()), StrEq(serializedAfterHandAdded));
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

        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );

        // l_shldr should already be in the ragdoll, so adding it should do nothing
        commandGroup.RemoveAllCommands();
        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_shldr"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(
                StrEq(),
                jointsToLeftHand
            )
        );

        // Undo here undoes the addition of l_hand
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_TRUE(GetRagdollJointNames(m_actor.get()).empty());

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
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
        EXPECT_TRUE(commandManager.ExecuteCommand(aznew CommandAddRagdollJoint(m_actor->GetID(), "l_shldr"), result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(StrEq(), AZStd::vector<AZStd::string>{"l_shldr"})
        );

        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
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
        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        // Removing the left shoulder should remove the elbow, wrist, and hand
        // as well
        commandGroup.RemoveAllCommands();
        CommandRagdollHelpers::RemoveJointsFromRagdoll(m_actor->GetID(), {"l_shldr"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        EXPECT_THAT(
            GetRagdollJointNames(m_actor.get()),
            testing::UnorderedPointwise(StrEq(), jointsToSpine3)
        );
    }

    TEST_F(RagdollCommandTests, RemoveJointThenUndoRestoresColliders)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        CommandRagdollHelpers::AddJointsToRagdoll(m_actor->GetID(), {"l_hand"}, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();

        const AZStd::string serializedBeforeSphereAdded = SerializePhysicsSetup(m_actor.get());

        CommandColliderHelpers::AddCollider(m_actor->GetID(), "l_hand",
            PhysicsSetup::Ragdoll, azrtti_typeid<Physics::SphereShapeConfiguration>());

        const AZStd::string serializedAfterSphereAdded = SerializePhysicsSetup(m_actor.get());

        EXPECT_THAT(serializedAfterSphereAdded, ::testing::Not(StrEq(serializedBeforeSphereAdded)));

        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();

        EXPECT_THAT(SerializePhysicsSetup(m_actor.get()), StrEq(serializedBeforeSphereAdded));

        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();

        EXPECT_THAT(SerializePhysicsSetup(m_actor.get()), StrEq(serializedAfterSphereAdded));
    }
} // namespace EMotionFX
