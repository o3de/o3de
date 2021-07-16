/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ActorFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>

namespace EMotionFX
{
    TEST_F(ActorFixture, CloneActorInstanceCommand)
    {
        ActorManager* actorManager = GetEMotionFX().GetActorManager();
        CommandSystem::CommandManager commandManager;

        EXPECT_EQ(actorManager->GetNumActorInstances(), 1);
        CommandSystem::CloneActorInstance(m_actorInstance);
        EXPECT_EQ(actorManager->GetNumActorInstances(), 2);

        actorManager->GetActorInstance(1)->Destroy();
        EXPECT_EQ(actorManager->GetNumActorInstances(), 1);
    }

    TEST_F(ActorFixture, CloneActorInstanceCommandGroup)
    {
        ActorManager* actorManager = GetEMotionFX().GetActorManager();
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;
        AZStd::string result;

        EXPECT_EQ(actorManager->GetNumActorInstances(), 1);
        CommandSystem::CloneActorInstance(m_actorInstance, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_EQ(actorManager->GetNumActorInstances(), 2);

        actorManager->GetActorInstance(1)->Destroy();
        EXPECT_EQ(actorManager->GetNumActorInstances(), 1);
    }

    TEST_F(ActorFixture, CreateActorInstancesAndUndo)
    {
        ActorManager* actorManager = GetEMotionFX().GetActorManager();
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;
        AZStd::string result;

        // 1. Clone the actor instance, now we have two actor instances.
        EXPECT_EQ(actorManager->GetNumActorInstances(), 1);
        CommandSystem::CloneActorInstance(m_actorInstance, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_EQ(actorManager->GetNumActorInstances(), 2);

        // 2. Delete the two actor instances using commands.
        commandGroup.Clear();
        commandGroup.AddCommandString(AZStd::string::format("RemoveActorInstance -actorInstanceID %i", actorManager->GetActorInstance(0)->GetID()));
        commandGroup.AddCommandString(AZStd::string::format("RemoveActorInstance -actorInstanceID %i", actorManager->GetActorInstance(1)->GetID()));
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_EQ(actorManager->GetNumActorInstances(), 0);

        // 3. Undo step 2
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(actorManager->GetNumActorInstances(), 2);

        // 4. Delete both actor instances, and free up the pointer to prevent double deletion.
        actorManager->GetActorInstance(1)->Destroy();
        actorManager->GetActorInstance(0)->Destroy();
        m_actorInstance = nullptr;
    }
} // namespace EMotionFX
