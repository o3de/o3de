/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/TwoStringEventData.h>
#include <EMotionFX/Source/AnimGraphSyncTrack.h>
#include <EMotionFX/Source/MotionEventTable.h>

namespace EMotionFX
{
    struct EventFilteringTestParam
    {
        AnimGraphObject::EEventMode     m_eventMode;
        float                           m_motionTime;
        float                           m_testDuration;       // Maximum of time this test will be run.
        AZStd::pair<float, float>       m_eventTimeRange;
        float                           m_blendWeight;
        int                             m_eventTriggerTimes;
    };

    // Use this event handler to test if the on event is called.
    class EventFilteringEventHandler
        : public EMotionFX::EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(EventFilteringEventHandler, Integration::EMotionFXAllocator);

        virtual const AZStd::vector<EventTypes> GetHandledEventTypes() const
        {
            return { EVENT_TYPE_ON_EVENT };
        }

        MOCK_METHOD1(OnEvent, void(const EMotionFX::EventInfo& emfxInfo));
    };

    class AnimGraphNodeEventFilterTestFixture : public AnimGraphFixture
        , public ::testing::WithParamInterface<EventFilteringTestParam>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            /*
                +----------+
                | Motion 1 +-----------+
                +----------+           |
                                       |
                +----------+           >+---------+               +-------+
                | Motion 2 +----------->| Blend 2 +-------------->+ Final |
                +----------+            |         |               +-------+
                                       >+---------+
                                       |
                                       |
                +-------------+        |
                | Const Float +--------+
                +-------------+
            */
            m_blend2Node = aznew BlendTreeBlend2Node();
            m_blendTree->AddChildNode(m_blend2Node);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_blend2Node, BlendTreeBlend2Node::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            for (size_t i = 0; i < 2; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                motionNode->SetName(AZStd::string::format("MotionNode%zu", i).c_str());
                m_blendTree->AddChildNode(motionNode);
                m_blend2Node->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, aznumeric_caster(i));
                m_motionNodes.push_back(motionNode);
            }

            m_blend2Node->SetSyncMode(AnimGraphObject::SYNCMODE_CLIPBASED);

            m_floatNode = aznew BlendTreeFloatConstantNode();
            m_blendTree->AddChildNode(m_floatNode);
            m_blend2Node->AddConnection(m_floatNode, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);

            const EventFilteringTestParam& param = GetParam();
            // Setup the motion and motion event.
            for (size_t i = 0; i < 2; ++i)
            {
                const AZStd::string motionId = AZStd::string::format("Motion%zu", i);
                Motion* motion = aznew Motion(motionId.c_str());
                motion->SetMotionData(aznew NonUniformMotionData());
                motion->GetMotionData()->SetDuration(param.m_motionTime);
                m_motions.emplace_back(motion);
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                m_motionNodes[i]->AddMotionId(motionId.c_str());
                m_motionNodes[i]->RecursiveOnChangeMotionSet(m_animGraphInstance, m_motionSet); // Trigger create motion instance.
                m_motionNodes[i]->PickNewActiveMotion(m_animGraphInstance);

                // Add motion event for each motion.
                motion->GetEventTable()->AutoCreateSyncTrack(motion);
                AnimGraphSyncTrack* syncTrack = motion->GetEventTable()->GetSyncTrack();
                AZStd::shared_ptr<const TwoStringEventData> data = GetEMotionFX().GetEventManager()->FindOrCreateEventData<TwoStringEventData>(motionId.c_str(), "params");
                syncTrack->AddEvent(param.m_eventTimeRange.first, param.m_eventTimeRange.second, data);
            }

            m_eventHandler = aznew EventFilteringEventHandler();
            GetEMotionFX().GetEventManager()->AddEventHandler(m_eventHandler);
        }

        void TearDown() override
        {
            GetEMotionFX().GetEventManager()->RemoveEventHandler(m_eventHandler);
            delete m_eventHandler;
            AnimGraphFixture::TearDown();
        }

    public:
        std::vector<AnimGraphMotionNode*> m_motionNodes;
        std::vector<Motion*> m_motions;
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_floatNode = nullptr;
        BlendTreeBlend2Node* m_blend2Node = nullptr;
        EventFilteringEventHandler* m_eventHandler = nullptr;
    };

    TEST_P(AnimGraphNodeEventFilterTestFixture, EventFilterTests)
    {
        const EventFilteringTestParam& param = GetParam();
        m_floatNode->SetValue(param.m_blendWeight);
        m_blend2Node->SetEventMode(param.m_eventMode);

        // Calling update first to make sure unique data is created.
        GetEMotionFX().Update(0.0f);

        // Expect the event handler will call different number of times based on the filtering mode, motion event range and test duration.
        EXPECT_CALL(*m_eventHandler, OnEvent(testing::_))
            .Times(param.m_eventTriggerTimes);

        // Update emfx to trigger the event firing.
        float totalTime = 0.0f;
        const float deltaTime = 0.1f;
        while (totalTime <= param.m_testDuration)
        {
            GetEMotionFX().Update(deltaTime);
            totalTime += deltaTime;
        }
    }

    std::vector<EventFilteringTestParam> EventFilteringTestData
    {
        // General test for event filtering mode.
        {
            AnimGraphObject::EEventMode::EVENTMODE_BOTHNODES,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            4                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_LEADERONLY,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            2                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_FOLLOWERONLY,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            2                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.0f,                   /* blend weight*/
            2                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            2                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            1.0f,                   /* blend weight*/
            2                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_NONE,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            0                       /* event trigger time*/
        },

        // Test if motion even will fire when motion loops.
        {
            AnimGraphObject::EEventMode::EVENTMODE_BOTHNODES,
            1.0f,                   /* motion time */
            3.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            12                      /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            1.0f,                   /* motion time */
            3.0f,                   /* test duration */
            {0.25f, 0.75f},         /* event time(ranged) */
            0.5f,                   /* blend weight*/
            6                       /* event trigger time*/
        },

        // Test with different motion range and duration
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            1.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {0.5f, 1.5f},           /* event time(ranged) */
            0.5f,                   /* blend weight*/
            1                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            1.0f,                   /* motion time */
            3.0f,                   /* test duration */
            {0.5f, 1.5f},           /* event time(ranged) */
            0.5f,                   /* blend weight*/
            3                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            3.0f,                   /* motion time */
            1.0f,                   /* test duration */
            {1.5f, 2.5f},           /* event time(ranged) */
            0.5f,                   /* blend weight*/
            0                       /* event trigger time*/
        },
        {
            AnimGraphObject::EEventMode::EVENTMODE_MOSTACTIVE,
            3.0f,                   /* motion time */
            30.0f,                  /* test duration */
            {1.5f, 2.5f},           /* event time(ranged) */
            0.5f,                   /* blend weight*/
            20                      /* event trigger time*/
        }
    };

    INSTANTIATE_TEST_CASE_P(EventFilterTests,
        AnimGraphNodeEventFilterTestFixture,
        ::testing::ValuesIn(EventFilteringTestData));
} // namespace EMotionFX
