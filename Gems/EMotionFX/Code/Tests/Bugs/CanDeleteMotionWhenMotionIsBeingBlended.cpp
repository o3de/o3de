/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <Tests/Printers.h>
#include <Tests/UI/UIFixture.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanDeleteMotionWhenMotionIsBeingBlended)
    {
        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1);

        Motion* xmotion = aznew Motion("xmotion");
        auto xMotionData = aznew NonUniformMotionData();
        xmotion->SetMotionData(xMotionData);
        xmotion->SetFileName("xmotion.motion");
        const size_t xJointIndex = xMotionData->AddJoint("Joint", Transform::CreateIdentity(), Transform::CreateIdentity());
        xMotionData->AllocateJointPositionSamples(xJointIndex, 2);
        xMotionData->SetJointPositionSample(xJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
        xMotionData->SetJointPositionSample(xJointIndex, 1, {1.0f, AZ::Vector3(1.0f, 0.0f, 0.0f)});
        xMotionData->UpdateDuration();
        xmotion->GetEventTable()->AutoCreateSyncTrack(xmotion);
        xmotion->GetEventTable()->GetSyncTrack()->AddEvent(0.0f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("leftFoot", ""));
        xmotion->GetEventTable()->GetSyncTrack()->AddEvent(0.5f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("rightFoot", ""));

        Motion* ymotion = aznew Motion("ymotion");
        ymotion->SetFileName("ymotion.motion");
        auto yMotionData = aznew NonUniformMotionData();
        ymotion->SetMotionData(yMotionData);
        const size_t yJointIndex = yMotionData->AddJoint("Joint", Transform::CreateIdentity(), Transform::CreateIdentity());
        yMotionData->AllocateJointPositionSamples(yJointIndex, 2);
        yMotionData->SetJointPositionSample(yJointIndex, 0, {0.0f, AZ::Vector3(0.0f, 0.0f, 0.0f)});
        yMotionData->SetJointPositionSample(yJointIndex, 1, {1.0f, AZ::Vector3(0.0f, 1.0f, 0.0f)});
        yMotionData->UpdateDuration();
        ymotion->GetEventTable()->AutoCreateSyncTrack(ymotion);
        ymotion->GetEventTable()->GetSyncTrack()->AddEvent(0.25f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("leftFoot", ""));
        ymotion->GetEventTable()->GetSyncTrack()->AddEvent(0.75f, GetEventManager().FindOrCreateEventData<TwoStringEventData>("rightFoot", ""));

        // Create anim graph
        AnimGraphMotionNode* motionX = aznew AnimGraphMotionNode();
        motionX->SetName("xmotion");
        motionX->SetMotionIds({"xmotion"});

        AnimGraphMotionNode* motionY = aznew AnimGraphMotionNode();
        motionY->SetName("ymotion");
        motionY->SetMotionIds({"ymotion"});

        AnimGraphTimeCondition* condition = aznew AnimGraphTimeCondition();
        condition->SetCountDownTime(0.0f);

        AnimGraphStateTransition* transition = aznew AnimGraphStateTransition();
        transition->SetSourceNode(motionX);
        transition->SetTargetNode(motionY);
        transition->SetBlendTime(5.0f);
        transition->AddCondition(condition);
        transition->SetSyncMode(AnimGraphStateTransition::SYNCMODE_TRACKBASED);

        AnimGraphStateMachine* rootState = aznew AnimGraphStateMachine();
        rootState->SetEntryState(motionX);
        rootState->AddChildNode(motionX);
        rootState->AddChildNode(motionY);
        rootState->AddTransition(transition);

        AZStd::unique_ptr<AnimGraph> animGraph = AZStd::make_unique<AnimGraph>();
        animGraph->SetRootStateMachine(rootState);

        animGraph->InitAfterLoading();

        // Create Motion Set
        MotionSet::MotionEntry* motionEntryX = aznew MotionSet::MotionEntry(xmotion->GetName(), xmotion->GetName(), xmotion);
        MotionSet::MotionEntry* motionEntryY = aznew MotionSet::MotionEntry(ymotion->GetName(), ymotion->GetName(), ymotion);

        AZStd::unique_ptr<MotionSet> motionSet = AZStd::make_unique<MotionSet>();
        motionSet->SetID(0);
        motionSet->AddMotionEntry(motionEntryX);
        motionSet->AddMotionEntry(motionEntryY);

        // Create ActorInstance
        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        // Create AnimGraphInstance
        AnimGraphInstance* animGraphInstance = AnimGraphInstance::Create(animGraph.get(), actorInstance, motionSet.get());
        actorInstance->SetAnimGraphInstance(animGraphInstance);

        GetEMotionFX().Update(0.0f);
        GetEMotionFX().Update(0.5f);
        GetEMotionFX().Update(2.0f);

        EXPECT_THAT(
            rootState->GetActiveStates(animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), AZStd::vector<AnimGraphNode*>{motionX, motionY})
        );

        AZStd::string result;
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("MotionSetRemoveMotion -motionSetID 0 -motionIds xmotion", result)) << result.c_str();
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("RemoveMotion -filename xmotion.motion", result)) << result.c_str();

        GetEMotionFX().Update(0.5f);

        EXPECT_THAT(
            rootState->GetActiveStates(animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), AZStd::vector<AnimGraphNode*>{motionX, motionY})
        );

        actorInstance->Destroy();
    }
} // namespace EMotionFX
