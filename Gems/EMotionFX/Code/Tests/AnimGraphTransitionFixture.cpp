/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/array.h>
#include "AnimGraphTransitionFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>

namespace EMotionFX
{
    void AnimGraphTransitionFixture::SetUp()
    {
        SystemComponentFixture::SetUp();

        // This test sets up an anim graph with 2 motions, each of which is 1
        // second long. There is a transition from the first to the second that
        // triggers when the first is complete, and takes 0.5 seconds to
        // transition, and an identical one going the other way. During the
        // transition, the weights of the motion states should add up to 1.
        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1);

        m_animGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
        m_motionNodeA = m_animGraph->GetMotionNodeA();
        m_motionNodeB = m_animGraph->GetMotionNodeB();

        AnimGraphMotionCondition* condition0 = aznew AnimGraphMotionCondition();
        condition0->SetMotionNodeId(m_motionNodeA->GetId());
        condition0->SetTestFunction(AnimGraphMotionCondition::FUNCTION_HASENDED);

        AnimGraphStateTransition* transition0 = aznew AnimGraphStateTransition();
        transition0->SetSourceNode(m_motionNodeA);
        transition0->SetTargetNode(m_motionNodeB);
        transition0->SetBlendTime(0.5f);
        transition0->AddCondition(condition0);

        AnimGraphMotionCondition* condition1 = aznew AnimGraphMotionCondition();
        condition1->SetMotionNodeId(m_motionNodeB->GetId());
        condition1->SetTestFunction(AnimGraphMotionCondition::FUNCTION_HASENDED);

        AnimGraphStateTransition* transition1 = aznew AnimGraphStateTransition();
        transition1->SetSourceNode(m_motionNodeB);
        transition1->SetTargetNode(m_motionNodeA);
        transition1->SetBlendTime(0.5f);
        transition1->AddCondition(condition1);

        m_stateMachine = m_animGraph->GetRootStateMachine();
        m_stateMachine->AddTransition(transition0);
        m_stateMachine->AddTransition(transition1);
        m_stateMachine->InitAfterLoading(m_animGraph.get());

        m_motionSet = aznew MotionSet();
        m_motionSet->SetName("testMotionSet");
        for (int i = 0; i < 2; ++i)
        {
            // The motion set keeps track of motions by their name. Each motion
            // within the motion set must have a unique name.
            AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", i);
            Motion* motion = aznew Motion(motionId.c_str());
            motion->SetMotionData(aznew NonUniformMotionData());
            motion->GetMotionData()->SetDuration(1.0f);
            MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
            m_motionSet->AddMotionEntry(motionEntry);

            AnimGraphMotionNode* motionNode = (i == 0) ? m_motionNodeA : m_motionNodeB;
            motionNode->SetName(motionId.c_str());
            motionNode->AddMotionId(motionId.c_str());
        }

        m_actorInstance = ActorInstance::Create(m_actor.get());

        m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance, m_motionSet);

        m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
    }

    void AnimGraphTransitionFixture::TearDown()
    {
        delete m_motionSet;
        m_motionSet = nullptr;
        if (m_actorInstance)
        {
            m_actorInstance->Destroy();
        }
        m_actor.reset();
        m_animGraph.reset();
        SystemComponentFixture::TearDown();
    }
} // namespace EMotionFX
