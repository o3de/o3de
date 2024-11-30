/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/array.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionData/UniformMotionData.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/TestAssetCode/MotionEvent.h>

namespace EMotionFX
{
    struct SyncParam
    {
        void (*m_eventFactoryA)(MotionEventTrack* track) = MakeNoEvents;
        void (*m_eventFactoryB)(MotionEventTrack* track) = MakeNoEvents;

        // 2.0 seconds of simulation, 0.1 increments, 21 playtimes
        AZStd::array<float, 21> m_expectedPlayTimeA {};
        AZStd::array<float, 21> m_expectedPlayTimeB {};

        // Expected play times will be calculated based on motion event and duration from AnimGraphNode::SyncUsingSyncTracks().
        AnimGraphObject::ESyncMode m_syncMode = AnimGraphObject::ESyncMode::SYNCMODE_DISABLED;
        float m_weightParam = 0.0f;
        bool m_reverseMotion = false;
    };

    class SyncingSystemFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<SyncParam>
    {
    public:
        void ConstructGraph() override
        {
            const SyncParam param = GetParam();
            m_syncMode = param.m_syncMode;
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();
            /*
            Inside blend tree:
            +-------------+
            |m_motionNodeA|--------+
            +-------------+        |
                                   |
            +-------------+        +------->+------------+       +---------+
            |m_motionNodeB|---------------->|m_blend2Node|------>|finalNode|
            +-------------+        +------->+------------+       +---------+
                                   |
            +-----------------+    |
            |m_weightParamNode|----+
            +-----------------+            
            */

            // Create parameter for animgraph
            Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
            parameter->SetName("blendWeight");
            
            m_blendTreeAnimGraph->AddParameter(parameter);

            // Create nodes in animgraph
            m_motionNodeA = aznew AnimGraphMotionNode();
            m_motionNodeB = aznew AnimGraphMotionNode();
            m_blend2Node = aznew BlendTreeBlend2Node();
            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();

            m_blendTree->AddChildNode(m_motionNodeA);
            m_blendTree->AddChildNode(m_motionNodeB);
            m_blendTree->AddChildNode(parameterNode);
            m_blendTree->AddChildNode(m_blend2Node);
            m_blendTree->AddChildNode(finalNode);

            // Connect the nodes in animgraph
            m_blend2Node->AddConnection(m_motionNodeA, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::PORTID_INPUT_POSE_A);
            m_blend2Node->AddConnection(m_motionNodeB, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::PORTID_INPUT_POSE_B);
            m_blend2Node->AddUnitializedConnection(parameterNode, 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(m_blend2Node, BlendTreeBlend2Node::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blend2Node->SetSyncMode(m_syncMode);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);

            // Add motion to motion nodes
            Motion* motionA = aznew Motion("testSkeletalMotionA");
            Motion* motionB = aznew Motion("testSkeletalMotionB");
            UniformMotionData* dataA = aznew UniformMotionData();
            UniformMotionData* dataB = aznew UniformMotionData();
            UniformMotionData::InitSettings settings;
            settings.m_numSamples = 2;
            settings.m_sampleRate = 1.0f;
            dataA->Init(settings);
            settings.m_sampleRate = 0.5f;
            dataB->Init(settings);
            motionA->SetMotionData(dataA);
            motionB->SetMotionData(dataB);
            EXPECT_FLOAT_EQ(motionA->GetDuration(), 1.0f);
            EXPECT_FLOAT_EQ(motionB->GetDuration(), 2.0f);
            motionA->GetEventTable()->AutoCreateSyncTrack(motionA);
            motionB->GetEventTable()->AutoCreateSyncTrack(motionB);
            m_syncTrackA = motionA->GetEventTable()->GetSyncTrack();
            m_syncTrackB = motionB->GetEventTable()->GetSyncTrack();

            MotionSet::MotionEntry* motionEntryA = aznew MotionSet::MotionEntry(motionA->GetName(), motionA->GetName(), motionA);
            MotionSet::MotionEntry* motionEntryB = aznew MotionSet::MotionEntry(motionB->GetName(), motionB->GetName(), motionB);
            m_motionSet->AddMotionEntry(motionEntryA);
            m_motionSet->AddMotionEntry(motionEntryB);
            m_motionNodeA->AddMotionId("testSkeletalMotionA");
            m_motionNodeB->AddMotionId("testSkeletalMotionB");
        }

    public:
        AnimGraphObject::ESyncMode m_syncMode;
        AnimGraphMotionNode* m_motionNodeA = nullptr;
        AnimGraphMotionNode* m_motionNodeB = nullptr;
        BlendTreeBlend2Node* m_blend2Node = nullptr;
        BlendTree* m_blendTree = nullptr;
        AnimGraphSyncTrack* m_syncTrackA = nullptr;
        AnimGraphSyncTrack* m_syncTrackB = nullptr;
    };

    // Play speed test for different sync modes with different weight on blend2Node
    TEST_P(SyncingSystemFixture, SyncingSystemPlaySpeedTests)
    {
        const SyncParam param = GetParam();
        param.m_eventFactoryA(m_syncTrackA);
        param.m_eventFactoryB(m_syncTrackB);
        GetEMotionFX().Update(0.0f);

        MCore::AttributeFloat* weightParam = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);
        weightParam->SetValue(param.m_weightParam);

        // Test reverse motion
        m_motionNodeA->SetReverse(param.m_reverseMotion);
        m_motionNodeB->SetReverse(param.m_reverseMotion);
        uint32 playTimeIndex = 0;
        const float tolerance = 0.00001f;
        Simulate(2.0f/*simulationTime*/, 10.0f/*expectedFps*/, 0.0f/*fpsVariance*/,
            /*preCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance) {},
            /*postCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance) {},
            /*preUpdateCallback*/[](AnimGraphInstance*, float, float, int) {},
            /*postUpdateCallback*/[this, &playTimeIndex, &tolerance, &param](AnimGraphInstance* animGraphInstance, [[maybe_unused]] float time, [[maybe_unused]] float timeDelta, [[maybe_unused]] int frame)
            {
                const float motionPlaySpeedA = m_motionNodeA->ExtractCustomPlaySpeed(animGraphInstance);
                const float durationA = m_motionNodeA->GetDuration(animGraphInstance);
                const float statePlaySpeedA = m_motionNodeA->GetPlaySpeed(animGraphInstance);
                const float motionPlaySpeedB = m_motionNodeB->ExtractCustomPlaySpeed(animGraphInstance);
                const float durationB = m_motionNodeB->GetDuration(animGraphInstance);
                const float statePlaySpeedB = m_motionNodeB->GetPlaySpeed(animGraphInstance);

                if (m_blend2Node->GetSyncMode() == AnimGraphObject::SYNCMODE_DISABLED)
                {
                    // We don't blend playspeeds when sync is disabled, the motion nodes should keep their playspeeds.
                    EXPECT_EQ(motionPlaySpeedA, statePlaySpeedA) << "Motion playspeeds should match the set playspeed in the motion node throughout blending.";
                    EXPECT_EQ(motionPlaySpeedB, statePlaySpeedB) << "Motion playspeeds should match the set playspeed in the motion node throughout blending.";
                }
                else if(m_blend2Node->GetSyncMode() == AnimGraphObject::SYNCMODE_CLIPBASED)
                {
                    float factorA;
                    float factorB;
                    float interpolatedSpeedA;
                    AZStd::tie(interpolatedSpeedA, factorA, factorB) = AnimGraphNode::SyncPlaySpeeds(
                        motionPlaySpeedA, durationA, motionPlaySpeedB, durationB, param.m_weightParam);
                    EXPECT_FLOAT_EQ(statePlaySpeedA, interpolatedSpeedA * factorA) << "Motion playspeeds should match the set playspeed in the motion node throughout blending.";
                }
                else if(m_blend2Node->GetSyncMode() == AnimGraphObject::SYNCMODE_TRACKBASED)
                {
                    const float motionPlayTimeA = m_motionNodeA->GetCurrentPlayTime(animGraphInstance);
                    const float motionPlayTimeB = m_motionNodeB->GetCurrentPlayTime(animGraphInstance);

                    EXPECT_NEAR(motionPlayTimeA, param.m_expectedPlayTimeA[playTimeIndex], tolerance) << "Motion node A playtime should match the expected playtime.";
                    EXPECT_NEAR(motionPlayTimeB, param.m_expectedPlayTimeB[playTimeIndex], tolerance) << "Motion node B playtime should match the expected playtime.";
                    playTimeIndex++;
                }
            }
        );
    }

    std::vector<SyncParam> SyncTestData
    {
        {
            /*.eventFactoryA     =*/ MakeNoEvents,
            /*.eventFactoryB     =*/ MakeNoEvents,
            /*.expectedPlayTimeA =*/ {},
            /*.expectedPlayTimeB =*/ {},
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_DISABLED,
            /*.weightParam       =*/ 0.0f,
            /*.reverseMotion     =*/ true
        },
        {
            /*.eventFactoryA     =*/ MakeNoEvents,
            /*.eventFactoryB     =*/ MakeNoEvents,
            /*.expectedPlayTimeA =*/ {},
            /*.expectedPlayTimeB =*/ {},
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_CLIPBASED,
            /*.weightParam       =*/ 0.25f,
            /*.reverseMotion     =*/ false
        },
        {
            /*.eventFactoryA     =*/ MakeNoEvents,
            /*.eventFactoryB     =*/ MakeOneEvent,
            /*.expectedPlayTimeA =*/ {},
            /*.expectedPlayTimeB =*/ {},
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_CLIPBASED,
            /*.weightParam       =*/ 0.5f,
            /*.reverseMotion     =*/ true
        },
        {
            /*.eventFactoryA     =*/ MakeOneEvent,
            /*.eventFactoryB     =*/ MakeOneEvent,
            /*.expectedPlayTimeA =*/ { 1.0f, 0.0625f, 0.125f, 0.1875f, 0.25f, 0.3125f, 0.375f, 0.4375f, 0.5f, 0.5625f, 0.625f, 0.6875f, 0.75f, 0.8125f, 0.875f, 0.9375f, 1.0f, 0.0625f, 0.125f, 0.1875f, 0.25f },
            /*.expectedPlayTimeB =*/ { 1.75f, 1.875f, 2.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f, 1.0f, 1.125f, 1.25f, 1.375f, 1.5f, 1.625f, 1.75f, 1.875f, 2.0f, 0.125f, 0.25f },
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_TRACKBASED,
            /*.weightParam       =*/ 0.75f,
            /*.reverseMotion     =*/ false
        },
        {
            /*.eventFactoryA     =*/ MakeOneEvent,
            /*.eventFactoryB     =*/ MakeTwoEvents,
            /*.expectedPlayTimeA =*/ { 1.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f },
            /*.expectedPlayTimeB =*/ { 0.625f, 0.725f, 0.325f, 0.425f, 0.525f, 0.625f, 0.725f, 0.325f, 0.425f, 0.525f, 0.625f, 0.725f, 0.325f, 0.425f, 0.525f, 0.625f, 0.725f, 0.325f, 0.425f, 0.525f, 0.625f },
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_TRACKBASED,
            /*.weightParam       =*/ 1.0f,
            /*.reverseMotion     =*/ true
        },
        {
            /*.eventFactoryA     =*/ MakeOneEvent,
            /*.eventFactoryB     =*/ MakeThreeEvents,
            /*.expectedPlayTimeA =*/ { 1.0f, 0.15f, 0.3f, 0.45f, 0.6f, 0.75f, 0.9f, 0.05f, 0.2f, 0.35f, 0.5f, 0.65f, 0.8f, 0.95f, 0.1f, 0.25f, 0.4f, 0.55f, 0.7f, 0.85f, 1.0f },
            /*.expectedPlayTimeB =*/ { 0.625f, 0.7f, 0.275f, 0.35f, 0.425f, 0.5f, 0.575f, 0.65f, 0.725f, 0.3f, 0.375f, 0.45f, 0.525f, 0.6f, 0.675f, 0.75f, 0.325f, 0.4f, 0.475f, 0.55f, 0.625f },
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_TRACKBASED,
            /*.weightParam       =*/ 0.5f,
            /*.reverseMotion     =*/ false
        },
        {
            /*.eventFactoryA     =*/ MakeTwoEvents,
            /*.eventFactoryB     =*/ MakeThreeEvents,
            /*.expectedPlayTimeA =*/ { 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.4875f, 0.575f, 0.6625f, 0.75f, 0.8375f, 0.9375f, 0.0375f, 0.1375f, 0.2375f, 0.3375f, 0.4375f },
            /*.expectedPlayTimeB =*/ { 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.35f, 1.55f, 1.725f, 1.9f, 0.075f, 0.25f, 0.3375f, 0.4375f, 0.5375f, 0.6375f, 0.7375f, 0.8375f, 0.9375f },
            /*.syncMode          =*/ AnimGraphObject::ESyncMode::SYNCMODE_TRACKBASED,
            /*.weightParam       =*/ 0.25f,
            /*.reverseMotion     =*/ true
        }
    };

    INSTANTIATE_TEST_CASE_P(SyncingSystem, SyncingSystemFixture,
        ::testing::ValuesIn(SyncTestData));
} // end namespace EMotionFX
