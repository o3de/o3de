/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorUpdateScheduler.h>
#include <EMotionFX/Source/MultiThreadScheduler.h>
#include <Tests/SystemComponentFixture.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    // This turned into an assert and is now being catched in the actual code. Skip this test, as we don't test and return at runtime anymore.
    TEST_F(SystemComponentFixture, DISABLED_InsertActorInstanceTwice)
    {
        ActorUpdateScheduler* baseScheduler = GetEMotionFX().GetActorManager()->GetScheduler();
        ASSERT_EQ(baseScheduler->GetType(), MultiThreadScheduler::TYPE_ID) << "Expected multi thread scheduler.";
        MultiThreadScheduler* scheduler = static_cast<MultiThreadScheduler*>(baseScheduler);

        // Create the actor (internally creates an actor instance for the static AABB calculation and removes it again).
        AZStd::unique_ptr<JackNoMeshesActor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        EXPECT_EQ(scheduler->GetNumScheduleSteps(), 0)
            << "Expected an empty scheduler as the temporarily created actor instance got destroyed again.";

        // Create an actor instance and make sure it is in the scheduler.
        ActorInstance* actorInstance = ActorInstance::Create(actor.get());
        EXPECT_EQ(scheduler->GetNumScheduleSteps(), 1) << "The actor instance should be part of the scheduler.";
        EXPECT_EQ(scheduler->GetScheduleStep(0).mActorInstances.size(), 1) << "The step should hold exactly one actor instance.";
        EXPECT_EQ(scheduler->GetScheduleStep(0).mActorInstances[0], actorInstance) << "The actor instance should be part of the step.";

        // Insert the actor instance manually again and make sure there is no duplicate.
        scheduler->RecursiveInsertActorInstance(actorInstance);
        EXPECT_EQ(scheduler->GetNumScheduleSteps(), 1) << "The actor instance should be part of the scheduler.";
        EXPECT_EQ(scheduler->GetScheduleStep(0).mActorInstances.size(), 1) << "The step should hold exactly one actor instance.";
        EXPECT_EQ(scheduler->GetScheduleStep(0).mActorInstances[0], actorInstance) << "The actor instance should be part of the step.";

        actorInstance->Destroy();
    }
} // namespace EMotionFX
