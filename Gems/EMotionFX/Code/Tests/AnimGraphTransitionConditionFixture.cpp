/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphTransitionConditionFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>

namespace EMotionFX
{
    void AnimGraphTransitionConditionFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        // This test sets up an anim graph with 2 motions, each of which is 1
        // second long. There is a transition from the first to the second that
        // triggers when the first is complete, and takes 0.5 seconds to
        // transition, and an identical one going the other way. During the
        // transition, the weights of the motion states should add up to 1.
        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1);

        m_animGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
        m_stateMachine = m_animGraph->GetRootStateMachine();
        m_motionNodeA = m_animGraph->GetMotionNodeA();
        m_motionNodeB = m_animGraph->GetMotionNodeB();

        m_transition = aznew AnimGraphStateTransition();
        m_transition->SetSourceNode(m_motionNodeA);
        m_transition->SetTargetNode(m_motionNodeB);
        m_stateMachine->AddTransition(m_transition);

        m_motionSet = aznew MotionSet("testMotionSet");
        for (int nodeIndex = 0; nodeIndex < 2; ++nodeIndex)
        {
            // The motion set keeps track of motions by their name. Each motion
            // within the motion set must have a unique name.
            AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", nodeIndex);
            Motion* motion = aznew Motion(motionId.c_str());
            motion->SetMotionData(aznew NonUniformMotionData());
            motion->GetMotionData()->SetDuration(1.0f);

            // 0.73 seconds would trigger frame 44 when sampling at 60 fps. The
            // event will be seen as triggered inside a motion condition, but a
            // frame later, at frame 45.
            motion->GetEventTable()->AddTrack(MotionEventTrack::Create("TestEventTrack", motion));
            AZStd::shared_ptr<const TwoStringEventData> data = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestEvent", "TestParameter");
            AZStd::shared_ptr<const TwoStringEventData> rangeData = EMotionFX::GetEventManager().FindOrCreateEventData<TwoStringEventData>("TestRangeEvent", "TestParameter");
            motion->GetEventTable()->FindTrackByName("TestEventTrack")->AddEvent(0.73f, data);
            motion->GetEventTable()->FindTrackByName("TestEventTrack")->AddEvent(0.65f, 0.95f, rangeData);

            MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
            m_motionSet->AddMotionEntry(motionEntry);

            AnimGraphMotionNode* motionNode = (nodeIndex == 0) ? m_motionNodeA : m_motionNodeB;
            motionNode->SetName(motionId.c_str());
            motionNode->AddMotionId(motionId.c_str());

            // Disable looping of the motion nodes.
            motionNode->SetLoop(false);
        }

        // Allow subclasses to create any additional nodes before the animgraph is activated.
        AddNodesToAnimGraph();

        m_animGraph->InitAfterLoading();

        m_actorInstance = ActorInstance::Create(m_actor.get());

        m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance, m_motionSet);
        m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
    }

    void AnimGraphTransitionConditionFixture::TearDown()
    {
        delete m_motionSet;
        m_motionSet = nullptr;
        if (m_actorInstance)
        {
            m_actorInstance->Destroy();
            m_actorInstance = nullptr;
        }
        m_actor.reset();
        m_animGraph.reset();

        SystemComponentFixture::TearDown();
    }
} // namespace EMotionFX
