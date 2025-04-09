/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <MCore/Source/ReflectionSerializer.h>
#include "AnimGraphFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/Transform.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    void AnimGraphFixture::SetUp()
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        SystemComponentFixture::SetUp();
        AnimGraphFactory::ReflectTestTypes(GetSerializeContext());

        // This fixture sets up the basic pieces to test animation graphs:
        // 1) will create an actor with a root node (joint) at the origin
        // 2) will create an empty animation graph
        // 3) will create an empty motion set
        // 4) will instantiate the actor and animation graph
        {
            ConstructActor();
            ASSERT_TRUE(m_actor) << "Construct actor did not build a valid actor.";
            m_actor->ResizeTransformData();
            m_actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/ false, /*convertUnitType=*/ false);
        }
        {
            m_motionSet = aznew MotionSet("testMotionSet");
        }
        {
            ConstructGraph();
            m_animGraph->InitAfterLoading();
        }
        {
            m_actorInstance = ActorInstance::Create(m_actor.get());
            m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance, m_motionSet);
            m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
            m_animGraphInstance->IncreaseReferenceCount(); // Two owners now, the test and the actor instance
            m_animGraphInstance->RecursiveInvalidateUniqueDatas();
        }
    }
    
    void AnimGraphFixture::ConstructGraph()
    {
        m_animGraph = AnimGraphFactory::Create<EmptyAnimGraph>();
        m_rootStateMachine = m_animGraph->GetRootStateMachine();
    }

    void AnimGraphFixture::ConstructActor()
    {
        m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1);
    }

    AZStd::string AnimGraphFixture::SerializeAnimGraph() const
    {
        if (!m_animGraph.get())
        {
            return AZStd::string();
        }

        return MCore::ReflectionSerializer::Serialize(m_animGraph.get()).GetValue();
    }

    void AnimGraphFixture::TearDown()
    {
        if (m_animGraphInstance)
        {
            m_animGraphInstance->Destroy();
            m_animGraphInstance = nullptr;
        }
        if (m_actorInstance)
        {
            m_actorInstance->Destroy();
            m_actorInstance = nullptr;
        }
        if (m_motionSet)
        {
            delete m_motionSet;
            m_motionSet = nullptr;
        }

        m_blendTreeAnimGraph.reset();
        m_animGraph.reset();
        m_actor.reset();

        SystemComponentFixture::TearDown();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    void AnimGraphFixture::Evaluate()
    {
        m_actorInstance->UpdateTransformations(0.0);
    }

    const Transform& AnimGraphFixture::GetOutputTransform(uint32 nodeIndex)
    {
        const Pose* pose = m_actorInstance->GetTransformData()->GetCurrentPose();
        return pose->GetModelSpaceTransform(nodeIndex);
    }

    void AnimGraphFixture::AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name)
    {
        Parameter* parameter = ParameterFactory::Create(typeId);
        parameter->SetName(name);
        m_animGraph->AddParameter(parameter);
        m_animGraphInstance->AddMissingParameterValues();
    }

    AnimGraphStateTransition* AnimGraphFixture::AddTransition(AnimGraphNode* source, AnimGraphNode* target, float time)
    {
        AnimGraphStateMachine* parentSM = azdynamic_cast<AnimGraphStateMachine*>(target->GetParentNode());
        if (!parentSM)
        {
            return nullptr;
        }

        AnimGraphStateTransition* transition = aznew AnimGraphStateTransition();

        transition->SetSourceNode(source);
        transition->SetTargetNode(target);
        transition->SetBlendTime(time);

        parentSM->AddTransition(transition);
        return transition;
    }

    AnimGraphTimeCondition* AnimGraphFixture::AddTimeCondition(AnimGraphStateTransition* transition, float countDownTime)
    {
        AnimGraphTimeCondition* condition = aznew AnimGraphTimeCondition();

        condition->SetCountDownTime(countDownTime);
        transition->AddCondition(condition);

        return condition;
    }

    AnimGraphStateTransition* AnimGraphFixture::AddTransitionWithTimeCondition(AnimGraphNode* source, AnimGraphNode* target, float blendTime, float countDownTime)
    {
        AnimGraphStateTransition* transition = AddTransition(source, target, blendTime);
        AddTimeCondition(transition, countDownTime);
        return transition;
    }

    MotionSet::MotionEntry* AnimGraphFixture::AddMotionEntry(const AZStd::string& motionId, float motionMaxTime)
    {
        Motion* motion = aznew Motion(motionId.c_str());
        motion->SetMotionData(aznew NonUniformMotionData());
        motion->GetMotionData()->SetDuration(motionMaxTime);
        MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
        m_motionSet->AddMotionEntry(motionEntry);
        return motionEntry;
    }

    void AnimGraphFixture::Simulate(float simulationTime, float expectedFps, float fpsVariance,
        SimulateCallback preCallback,
        SimulateCallback postCallback,
        SimulateFrameCallback preUpdateCallback,
        SimulateFrameCallback postUpdateCallback)
    {
        AZ::SimpleLcgRandom random;
        random.SetSeed(875960);

        const float minFps = expectedFps - fpsVariance / 2.0f;
        const float maxFps = expectedFps + fpsVariance / 2.0f;

        int frame = 0;
        float time = 0.0f;

        preCallback(m_animGraphInstance);

        // Make sure to update at least once and to have a valid internal state and everything is initialized on the first frame.
        preUpdateCallback(m_animGraphInstance, time, 0.0f, frame);
        GetEMotionFX().Update(0.0f);
        postUpdateCallback(m_animGraphInstance, time, 0.0f, frame);

        while (time < simulationTime)
        {
            const float randomFps = fabs(minFps + random.GetRandomFloat() * (maxFps - minFps));
            float timeDelta = 0.0f;
            if (randomFps > 0.1f)
            {
                timeDelta = 1.0f / randomFps;
            }
            time += timeDelta;
            frame++;

            preUpdateCallback(m_animGraphInstance, time, timeDelta, frame);
            GetEMotionFX().Update(timeDelta);
            postUpdateCallback(m_animGraphInstance, time, timeDelta, frame);
        }

        postCallback(m_animGraphInstance);
    }
}
