/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Tests/SystemComponentFixture.h>
#include <TestAssetCode/ActorFactory.h>
#include <TestAssetCode/SimpleActors.h>

#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <EMotionFX/Source/MotionSystem.h>

namespace EMotionFX
{
    using MotionLayerSystemFixture = SystemComponentFixture;

    TEST_F(MotionLayerSystemFixture, MotionInstanceDestroyedAfterMotionEnds)
    {
        auto actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5);
        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        Motion* motion1 = aznew Motion("motion1");
        motion1->SetMotionData(aznew UniformMotionData());
        motion1->GetMotionData()->SetDuration(10.0f);
        
        Motion* motion2 = aznew Motion("motion2");
        motion2->SetMotionData(aznew UniformMotionData());
        motion2->GetMotionData()->SetDuration(10.0f);

        MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        PlayBackInfo playBackInfo;
        playBackInfo.mBlendInTime = 1.0f;
        playBackInfo.mBlendOutTime = 1.0f;
        playBackInfo.mNumLoops = 1;
        playBackInfo.mPlayNow = false;
        playBackInfo.mFreezeAtLastFrame = false;

        // Add 2 motions to the queue
        const MotionInstance* motionInstance1 = motionSystem->PlayMotion(motion1, &playBackInfo);
        const MotionInstance* motionInstance2 = motionSystem->PlayMotion(motion2, &playBackInfo);

        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);
        EXPECT_FLOAT_EQ(motionInstance1->GetWeight(), 0.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 0.0f);
        EXPECT_FLOAT_EQ(motionInstance1->GetCurrentTime(), 0.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetCurrentTime(), 0.0f);

        GetEMotionFX().Update(1.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);
        EXPECT_FLOAT_EQ(motionInstance1->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 0.0f);
        EXPECT_FLOAT_EQ(motionInstance1->GetCurrentTime(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetCurrentTime(), 0.0f);

        GetEMotionFX().Update(8.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 2);
        EXPECT_FLOAT_EQ(motionInstance1->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 0.0f);
        EXPECT_FLOAT_EQ(motionInstance1->GetCurrentTime(), 9.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetCurrentTime(), 0.0f);

        GetEMotionFX().Update(0.5f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 2);
        EXPECT_FLOAT_EQ(motionInstance1->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 0.5f);
        EXPECT_FLOAT_EQ(motionInstance1->GetCurrentTime(), 9.5f);
        EXPECT_FLOAT_EQ(motionInstance2->GetCurrentTime(), 0.5f);

        GetEMotionFX().Update(0.5f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetCurrentTime(), 1.0f);

        GetEMotionFX().Update(8.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetCurrentTime(), 9.0f);

        GetEMotionFX().Update(1.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 0);

        motion2->Destroy();
        motion1->Destroy();
        actorInstance->Destroy();
    }

    TEST_F(MotionLayerSystemFixture, TransitionsBetweenMotions)
    {
        auto actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5);
        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        Motion* walk = aznew Motion("walk");
        walk->SetMotionData(aznew UniformMotionData());
        walk->GetMotionData()->SetDuration(10.0f);
        Motion* run = aznew Motion("run");
        run->SetMotionData(aznew UniformMotionData());
        run->GetMotionData()->SetDuration(10.0f);

        MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        PlayBackInfo playBackInfo;
        playBackInfo.mBlendInTime = 1.0f;
        playBackInfo.mBlendOutTime = 1.0f;
        playBackInfo.mNumLoops = EMFX_LOOPFOREVER;
        playBackInfo.mPlayNow = true;

        const MotionInstance* walkInstance = motionSystem->PlayMotion(walk, &playBackInfo);

        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(0.5f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);
        EXPECT_FLOAT_EQ(walkInstance->GetWeight(), 1.0f);

        const MotionInstance* runInstance = motionSystem->PlayMotion(run, &playBackInfo);
        GetEMotionFX().Update(0.25f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 2);
        EXPECT_FLOAT_EQ(walkInstance->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(runInstance->GetWeight(), 0.25f);

        GetEMotionFX().Update(0.25f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 2);
        EXPECT_FLOAT_EQ(walkInstance->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(runInstance->GetWeight(), 0.5f);

        GetEMotionFX().Update(0.25f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 2);
        EXPECT_FLOAT_EQ(walkInstance->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(runInstance->GetWeight(), 0.75f);

        GetEMotionFX().Update(0.25f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);
        EXPECT_FLOAT_EQ(runInstance->GetWeight(), 1.0f);

        run->Destroy();
        walk->Destroy();
        actorInstance->Destroy();
    }

    TEST_F(MotionLayerSystemFixture, StopAllMotionsRemovesAllMotionInstances)
    {
        auto actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(5);
        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        Motion* motion1 = aznew Motion("motion1");
        motion1->SetMotionData(aznew UniformMotionData());
        motion1->GetMotionData()->SetDuration(10.0f);

        Motion* motion2 = aznew Motion("motion2");
        motion2->SetMotionData(aznew UniformMotionData());
        motion2->GetMotionData()->SetDuration(10.0f);

        MotionSystem* motionSystem = actorInstance->GetMotionSystem();

        PlayBackInfo playBackInfo;
        playBackInfo.mBlendInTime = 1.0f;
        playBackInfo.mBlendOutTime = 1.0f;
        playBackInfo.mNumLoops = 1;
        playBackInfo.mPlayNow = false;
        playBackInfo.mFreezeAtLastFrame = false;

        // Add 2 motions to the queue
        const MotionInstance* motionInstance1 = motionSystem->PlayMotion(motion1, &playBackInfo);
        const MotionInstance* motionInstance2 = motionSystem->PlayMotion(motion2, &playBackInfo);

        GetEMotionFX().Update(0.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 1);

        GetEMotionFX().Update(9.0f);
        GetEMotionFX().Update(0.5f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 2);
        EXPECT_FLOAT_EQ(motionInstance1->GetWeight(), 1.0f);
        EXPECT_FLOAT_EQ(motionInstance2->GetWeight(), 0.5f);

        motionSystem->StopAllMotions();

        // Wait for them to blend out
        GetEMotionFX().Update(1.0f);
        EXPECT_EQ(motionSystem->GetNumMotionInstances(), 0);

        motion2->Destroy();
        motion1->Destroy();
        actorInstance->Destroy();
    }
} // namespace EMotionFX
