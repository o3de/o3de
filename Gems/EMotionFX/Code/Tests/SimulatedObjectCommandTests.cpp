/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActorFixture.h"
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>


namespace EMotionFX
{
    class SimulatedObjectCommandTests
        : public ActorFixture
    {
    public:
        size_t CountSimulatedObjects(const Actor* actor)
        {
            return actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects();
        }

        size_t CountSimulatedJoints(const Actor* actor, size_t objectIndex)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            const SimulatedObject* object = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            return object->GetNumSimulatedJoints();
        }

        size_t CountChildJoints(const Actor* actor, size_t objectIndex, size_t jointIndex)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            const SimulatedObject* object = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            const SimulatedJoint* joint = object->FindSimulatedJointBySkeletonJointIndex(jointIndex);
            return joint->CalculateNumChildSimulatedJoints();
        }

        size_t CountRootJoints(const Actor* actor, size_t objectIndex)
        {
            const AZStd::shared_ptr<SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
            const SimulatedObject* object = simulatedObjectSetup->GetSimulatedObject(objectIndex);
            return object->GetNumSimulatedRootJoints();
        }
    };


    TEST_F(SimulatedObjectCommandTests, EvaluateSimulatedObjectCommands)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const uint32 actorId = GetActor()->GetID();
        const AZStd::vector<AZStd::string> jointNames = GetTestJointNames();

        // 1. Add simulated object.
        const AZStd::string serializedBeforeAdd = SerializeSimulatedObjectSetup(GetActor());
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        const AZStd::string serializedAfterAdd = SerializeSimulatedObjectSetup(GetActor());
        EXPECT_EQ(3, CountSimulatedObjects(GetActor()));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(0, CountSimulatedObjects(GetActor()));
        EXPECT_EQ(serializedBeforeAdd, SerializeSimulatedObjectSetup(GetActor()));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(3, CountSimulatedObjects(GetActor()));
        EXPECT_EQ(serializedAfterAdd, SerializeSimulatedObjectSetup(GetActor()));

        // 2. Remove simulated object.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actorId, 2, &commandGroup);
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actorId, 0, &commandGroup);
        CommandSimulatedObjectHelpers::RemoveSimulatedObject(actorId, 0, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)); 
        EXPECT_EQ(0, CountSimulatedObjects(GetActor()));
        EXPECT_EQ(serializedBeforeAdd, SerializeSimulatedObjectSetup(GetActor()));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(3, CountSimulatedObjects(GetActor()));
        EXPECT_EQ(serializedAfterAdd, SerializeSimulatedObjectSetup(GetActor()));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(0, CountSimulatedObjects(GetActor()));
        EXPECT_EQ(serializedBeforeAdd, SerializeSimulatedObjectSetup(GetActor()));

        // 3. Add simulated joints.
        // 3.1 Add a simulated object first to put in the simulated joints.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId);
        const AZStd::string serialized3_1 = SerializeSimulatedObjectSetup(GetActor());

        // 3.2 Add simulated joints.
        // Joint hierarchy as follow:
        // l_upLeg
        //      --l_upLegRoll
        //      --l_loLeg
        //             --l_ankle
        //                    --l_ball
        const Skeleton* skeleton = GetActor()->GetSkeleton();
        const size_t l_upLegIdx = skeleton->FindNodeByName("l_upLeg")->GetNodeIndex();
        const size_t l_upLegRollIdx = skeleton->FindNodeByName("l_upLegRoll")->GetNodeIndex();
        const size_t l_loLegIdx = skeleton->FindNodeByName("l_loLeg")->GetNodeIndex();
        const size_t l_ankleIdx = skeleton->FindNodeByName("l_ankle")->GetNodeIndex();
        const size_t l_ballIdx = skeleton->FindNodeByName("l_ball")->GetNodeIndex();
        CommandSimulatedObjectHelpers::AddSimulatedJoints(actorId, {l_upLegIdx, l_upLegRollIdx, l_loLegIdx, l_ankleIdx, l_ballIdx}, 0, false, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        const AZStd::string serialized3_2 = SerializeSimulatedObjectSetup(GetActor());
        EXPECT_EQ(5, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(2, CountChildJoints(GetActor(), 0, l_upLegIdx));
        EXPECT_EQ(0, CountChildJoints(GetActor(), 0, l_upLegRollIdx));
        EXPECT_EQ(1, CountChildJoints(GetActor(), 0, l_loLegIdx));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(0, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serialized3_1, SerializeSimulatedObjectSetup(GetActor()));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(5, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serialized3_2, SerializeSimulatedObjectSetup(GetActor()));

        // 4 Remove simulated joints.
        // 4.1 Test sparse chain.
        EXPECT_EQ(1, CountRootJoints(GetActor(), 0));
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, {l_loLegIdx}, 0, false, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(2, CountRootJoints(GetActor(), 0));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(1, CountRootJoints(GetActor(), 0));

        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(2, CountRootJoints(GetActor(), 0));

        EXPECT_TRUE(commandManager.Undo(result));

        // 4.2 Test removing all the joints.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, { l_upLegIdx, l_upLegRollIdx, l_loLegIdx, l_ankleIdx, l_ballIdx }, 0, false, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(0, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serialized3_1, SerializeSimulatedObjectSetup(GetActor()));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(5, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serialized3_2, SerializeSimulatedObjectSetup(GetActor()));

        // 4.3 Test removing the root joint and children.
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, { l_upLegIdx }, 0, true, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(0, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serialized3_1, SerializeSimulatedObjectSetup(GetActor()));

        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(5, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serialized3_2, SerializeSimulatedObjectSetup(GetActor()));
    }

    TEST_F(SimulatedObjectCommandTests, SimulatedObjectCommands_UndoRemoveJointTest)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        const uint32 actorId = GetActor()->GetID();
        const AZStd::vector<AZStd::string> jointNames = GetTestJointNames();

        // 1. Add simulated object
        CommandSimulatedObjectHelpers::AddSimulatedObject(actorId, AZStd::nullopt, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        const AZStd::string serializedBase = SerializeSimulatedObjectSetup(GetActor());
        const size_t simulatedObjectIndex = 0;

        // 2. Add r_upLeg simulated joints
        const Skeleton* skeleton = GetActor()->GetSkeleton();
        const size_t r_upLegIdx = skeleton->FindNodeByName("r_upLeg")->GetNodeIndex();
        const size_t r_loLegIdx = skeleton->FindNodeByName("r_loLeg")->GetNodeIndex();
        CommandSimulatedObjectHelpers::AddSimulatedJoints(actorId, { r_upLegIdx, r_loLegIdx }, 0, false);
        EXPECT_EQ(2, CountSimulatedJoints(GetActor(), 0));
        const AZStd::string serializedUpLeg = SerializeSimulatedObjectSetup(GetActor());

        // 3. Remove the r_loLeg simulated joint
        AZStd::vector<SimulatedJoint*> jointsToBeRemoved;
        commandGroup.RemoveAllCommands();
        CommandSimulatedObjectHelpers::RemoveSimulatedJoints(actorId, { r_upLegIdx }, simulatedObjectIndex, true, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(0, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serializedBase, SerializeSimulatedObjectSetup(GetActor()));

        // 4. Undo
        // This recreates r_loLeg and r_loLeg but won't add all other children recursively as only these two joints got aded in step 3.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(2, CountSimulatedJoints(GetActor(), 0));
        EXPECT_EQ(serializedUpLeg, SerializeSimulatedObjectSetup(GetActor()));
    }
} // namespace EMotionFX
