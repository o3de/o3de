/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphSyncTrack.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeRerouteNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>

namespace EMotionFX
{
    class BlendTreeRerouteNodeFixture : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            BlendTree* tree = m_blendTreeAnimGraph->GetBlendTreeNode();

            auto* finalNode = aznew BlendTreeFinalNode();
            auto* blend = aznew BlendTreeBlend2Node();
            m_source = aznew BlendTreeFloatConstantNode();
            m_reroute = aznew BlendTreeRerouteNode();

            m_poseReroute = aznew BlendTreeRerouteNode();
            m_poseReroute->SetDataTypeId(AttributePose::TYPE_ID);
            auto* poseA = aznew AnimGraphBindPoseNode();
            m_poseB = aznew AnimGraphBindPoseNode();

            for (AnimGraphNode* node : AZStd::vector<AnimGraphNode*>{finalNode, blend, m_source, m_reroute, poseA, m_poseB, m_poseReroute})
            {
                tree->AddChildNode(node);
            }

            m_reroute->AddConnection(m_source, 0, 0);
            m_poseReroute->AddConnection(poseA, 0, 0);
            blend->AddConnection(m_poseReroute, 0, BlendTreeBlend2Node::INPUTPORT_POSE_A);
            blend->AddConnection(m_poseB, 0, BlendTreeBlend2Node::INPUTPORT_POSE_B);
            blend->AddConnection(m_reroute, 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(blend, 0, 0);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    protected:
        BlendTreeFloatConstantNode* m_source = nullptr;
        BlendTreeRerouteNode* m_reroute = nullptr;
        BlendTreeRerouteNode* m_poseReroute = nullptr;
        AnimGraphBindPoseNode* m_poseB = nullptr;
    };

    TEST_F(BlendTreeRerouteNodeFixture, ValueIsReadyDuringUpdate)
    {
        m_source->SetValue(0.75f);
        m_reroute->PerformUpdate(m_animGraphInstance, 1.0f / 60.0f);

        EXPECT_FLOAT_EQ(m_source->GetOutputFloat(m_animGraphInstance, 0)->GetValue(), 0.75f);
        EXPECT_FLOAT_EQ(m_reroute->GetOutputFloat(m_animGraphInstance, 0)->GetValue(), 0.75f);
    }

    TEST_F(BlendTreeRerouteNodeFixture, BlendWeightsMatchCurrentInput)
    {
        for (float value : {0.75f, 0.25f, 1.0f, 0.0f})
        {
            m_source->SetValue(value);
            GetEMotionFX().Update(1.0f / 60.0f);

            EXPECT_FLOAT_EQ(m_poseReroute->FindOrCreateUniqueNodeData(m_animGraphInstance)->GetLocalWeight(), 1.0f - value);
            EXPECT_FLOAT_EQ(m_poseB->FindOrCreateUniqueNodeData(m_animGraphInstance)->GetLocalWeight(), value);
        }
    }

    TEST_F(BlendTreeRerouteNodeFixture, DisconnectedValueReturnsToDefaultDuringUpdate)
    {
        m_source->SetValue(0.75f);
        GetEMotionFX().Update(1.0f / 60.0f);

        m_reroute->RemoveConnection(m_reroute->GetConnections().front());
        GetEMotionFX().Update(1.0f / 60.0f);

        EXPECT_FLOAT_EQ(m_reroute->GetOutputFloat(m_animGraphInstance, 0)->GetValue(), 0.0f);
        EXPECT_FLOAT_EQ(m_poseReroute->FindOrCreateUniqueNodeData(m_animGraphInstance)->GetLocalWeight(), 1.0f);
        EXPECT_FLOAT_EQ(m_poseB->FindOrCreateUniqueNodeData(m_animGraphInstance)->GetLocalWeight(), 0.0f);
    }

    TEST_F(BlendTreeRerouteNodeFixture, DisconnectedPoseReturnsToBindPose)
    {
        GetEMotionFX().Update(1.0f / 60.0f);

        m_poseReroute->RemoveConnection(m_poseReroute->GetConnections().front());
        GetEMotionFX().Update(1.0f / 60.0f);

        const Pose& pose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        const Pose& bindPose = *m_actor->GetBindPose();
        ASSERT_EQ(pose.GetNumTransforms(), bindPose.GetNumTransforms());
        for (size_t i = 0; i < pose.GetNumTransforms(); ++i)
        {
            EXPECT_EQ(pose.GetLocalSpaceTransform(i), bindPose.GetLocalSpaceTransform(i));
        }
    }
}

namespace EMotionFX
{
    class RerouteMotionEventHandler : public EventHandler
    {
    public:
        const AZStd::vector<EventTypes> GetHandledEventTypes() const override
        {
            return {EVENT_TYPE_ON_EVENT};
        }

        void OnEvent(const EventInfo&) override
        {
            ++m_events;
        }

        size_t m_events = 0;
    };

    class RerouteMotionTest
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<size_t>
    {
    protected:
        void ConstructActor() override
        {
            m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(2);
            m_actor->SetMotionExtractionNodeIndex(0);
        }

        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            auto* motion = aznew Motion("Moving");
            auto* data = aznew NonUniformMotionData();
            motion->SetMotionData(data);

            for (size_t i = 0; i < 2; ++i)
            {
                const Transform bind = m_actor->GetBindPose()->GetLocalSpaceTransform(i);
                const size_t joint = data->AddJoint(m_actor->GetSkeleton()->GetNode(i)->GetName(), bind, bind);
                data->AllocateJointPositionSamples(joint, 2);

                AZ::Vector3 offset(0.0f, 1.0f, 0.0f);
                if (i == 1)
                {
                    offset = AZ::Vector3(2.0f, 0.0f, 0.0f);
                }

                data->SetJointPositionSample(joint, 0, {0.0f, bind.m_position});
                data->SetJointPositionSample(joint, 1, {1.0f, bind.m_position + offset});
            }

            motion->UpdateDuration();
            motion->GetEventTable()->AutoCreateSyncTrack(motion);
            m_syncTrack = motion->GetEventTable()->GetSyncTrack();
            m_syncTrack->AddEvent(0.25f, GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>("Step", ""));
            m_motionSet->AddMotionEntry(aznew MotionSet::MotionEntry("Moving", "Moving", motion));

            auto* tree = aznew BlendTree();
            m_rootStateMachine->AddChildNode(tree);
            m_rootStateMachine->SetEntryState(tree);

            m_source = aznew AnimGraphMotionNode();
            m_source->AddMotionId("Moving");
            m_source->SetMotionExtraction(true);
            tree->AddChildNode(m_source);

            m_output = m_source;
            for (size_t i = 0; i < GetParam(); ++i)
            {
                auto* reroute = aznew BlendTreeRerouteNode();
                reroute->SetDataTypeId(AttributePose::TYPE_ID);
                tree->AddChildNode(reroute);
                reroute->AddConnection(m_output, 0, 0);
                m_output = reroute;
            }

            auto* final = aznew BlendTreeFinalNode();
            tree->AddChildNode(final);
            final->AddConnection(m_output, 0, 0);
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_actorInstance->SetMotionExtractionEnabled(true);
            GetEMotionFX().GetEventManager()->AddEventHandler(&m_events);
        }

        void TearDown() override
        {
            GetEMotionFX().GetEventManager()->RemoveEventHandler(&m_events);
            AnimGraphFixture::TearDown();
        }

        AnimGraphMotionNode* m_source = nullptr;
        AnimGraphNode* m_output = nullptr;
        AnimGraphSyncTrack* m_syncTrack = nullptr;
        RerouteMotionEventHandler m_events;
    };

    TEST_P(RerouteMotionTest, PreservesMovingPoseEventsTrajectoryAndSync)
    {
        GetEMotionFX().Update(0.0f);
        for (size_t i = 0; i < 6; ++i)
        {
            GetEMotionFX().Update(0.1f);
        }

        const auto& pose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        const AZ::Vector3 expectedJointPosition = m_actor->GetBindPose()->GetLocalSpaceTransform(1).m_position + AZ::Vector3(1.2f, 0.0f, 0.0f);
        EXPECT_TRUE(pose.GetLocalSpaceTransform(1).m_position.IsClose(expectedJointPosition, 0.001f));
        EXPECT_NEAR(m_actorInstance->GetLocalSpacePosition().GetY(), 0.6f, 0.001f);
        EXPECT_EQ(m_events.m_events, 1);

        const AnimGraphNodeData* source = m_source->FindOrCreateUniqueNodeData(m_animGraphInstance);
        const AnimGraphNodeData* output = m_output->FindOrCreateUniqueNodeData(m_animGraphInstance);
        EXPECT_NEAR(source->GetCurrentPlayTime(), 0.6f, 0.001f);
        EXPECT_FLOAT_EQ(output->GetCurrentPlayTime(), source->GetCurrentPlayTime());
        EXPECT_FLOAT_EQ(output->GetDuration(), source->GetDuration());
        EXPECT_EQ(output->GetSyncTrack(), m_syncTrack);
        EXPECT_EQ(m_syncTrack->GetNumEvents(), 1);
    }

    INSTANTIATE_TEST_SUITE_P(DirectAndRouted, RerouteMotionTest, ::testing::Values(size_t{0}, size_t{1}, size_t{3}));
}
