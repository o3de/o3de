
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
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>

namespace EMotionFX
{
    struct AnimGraphStateMachineSyncParam
    {
        float m_playSpeedA;
        float m_durationA;
        float m_playSpeedB;
        float m_durationB;
        bool m_syncEnabled;
    };

    class AnimGraphStateMachineSyncFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<AnimGraphStateMachineSyncParam>
    {
    public:
        void ConstructGraph() override
        {
            const AnimGraphStateMachineSyncParam param = GetParam();
            AnimGraphFixture::ConstructGraph();
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();
            /*
                +---+       +---+
              =>| A |<----->| B |
                +-+-+       +-+-+
            */
            m_stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            m_stateB = m_motionNodeAnimGraph->GetMotionNodeB();

            m_transition = AddTransitionWithTimeCondition(m_stateA,
                m_stateB,
                1.0f/*blendTime*/,
                0.0f/*countDownTime*/);

            if (param.m_syncEnabled)
            {
                m_transition->SetSyncMode(AnimGraphObject::SYNCMODE_CLIPBASED);
            }
            else
            {
                m_transition->SetSyncMode(AnimGraphObject::SYNCMODE_DISABLED);
            }

            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void SetUpMotionNode(const char* motionId, float playSpeed, float duration, AnimGraphMotionNode* motionNode)
        {
            Motion* motion = aznew Motion(motionId);
            motion->SetMotionData(aznew NonUniformMotionData());
            motion->GetMotionData()->SetDuration(duration);

            MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
            m_motionSet->AddMotionEntry(motionEntry);

            motionNode->AddMotionId(motionId);
            motionNode->SetMotionPlaySpeed(playSpeed);
            motionNode->RecursiveOnChangeMotionSet(m_animGraphInstance, m_motionSet);
            motionNode->PickNewActiveMotion(m_animGraphInstance, static_cast<AnimGraphMotionNode::UniqueData*>(motionNode->FindOrCreateUniqueNodeData(m_animGraphInstance)));
        }

        void SetUp() override
        {
            const AnimGraphStateMachineSyncParam param = GetParam();
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_motionNodeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);

            SetUpMotionNode("testMotionA", param.m_playSpeedA, param.m_durationA, m_stateA);
            SetUpMotionNode("testMotionB", param.m_playSpeedB, param.m_durationB, m_stateB);

            GetEMotionFX().Update(0.0f);
        }

        void TearDown() override
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Destroy();
                m_animGraphInstance = nullptr;
            }
            m_motionNodeAnimGraph.reset();
            AnimGraphFixture::TearDown();
        }

    public:
        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphMotionNode* m_stateA = nullptr;
        AnimGraphMotionNode* m_stateB = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_P(AnimGraphStateMachineSyncFixture, PlayspeedTests)
    {
        bool transitioned = false;
        Simulate(2.0f/*simulationTime*/, 10.0f/*expectedFps*/, 0.0f/*fpsVariance*/,
            /*preCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance){},
            /*postCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance){},
            /*preUpdateCallback*/[](AnimGraphInstance*, float, float, int){},
            /*postUpdateCallback*/[this, &transitioned](AnimGraphInstance* animGraphInstance, [[maybe_unused]] float time, [[maybe_unused]] float timeDelta, [[maybe_unused]] int frame)
            {
                if (m_rootStateMachine->IsTransitionActive(m_transition, animGraphInstance))
                {
                    const float weight = m_transition->GetBlendWeight(animGraphInstance);
                    const float motionPlaySpeedA = m_stateA->ExtractCustomPlaySpeed(animGraphInstance);
                    const float durationA = m_stateA->GetDuration(animGraphInstance);
                    const float statePlaySpeedA = m_stateA->GetPlaySpeed(animGraphInstance);
                    const float motionPlaySpeedB = m_stateB->ExtractCustomPlaySpeed(animGraphInstance);
                    const float durationB = m_stateB->GetDuration(animGraphInstance);
                    const float statePlaySpeedB = m_stateB->GetPlaySpeed(animGraphInstance);

                    if (m_transition->GetSyncMode() == AnimGraphObject::SYNCMODE_DISABLED)
                    {
                        // We don't blend playspeeds when sync is disabled, the source and the target states should keep their playspeeds.
                        EXPECT_EQ(motionPlaySpeedA, statePlaySpeedA) << "Motion playspeeds should match the set playspeed in the motion node throughout transitioning.";
                        EXPECT_EQ(motionPlaySpeedB, statePlaySpeedB);
                    }
                    else
                    {
                        float factorA;
                        float factorB;
                        float interpolatedSpeedA;
                        AZStd::tie(interpolatedSpeedA, factorA, factorB) = AnimGraphNode::SyncPlaySpeeds(
                            motionPlaySpeedA, durationA, motionPlaySpeedB, durationB, weight);
                        EXPECT_FLOAT_EQ(statePlaySpeedA, interpolatedSpeedA * factorA);
                    }

                    transitioned = true;
                }

                // Check the parent state machine playspeed.
                const AZStd::vector<AnimGraphNode*>& activeStates = m_rootStateMachine->GetActiveStates(animGraphInstance);
                EXPECT_TRUE(activeStates.size() > 0);
                const float stateMachinePlaySpeed = m_rootStateMachine->GetPlaySpeed(animGraphInstance);
                const float activeStatePlaySpeed = activeStates[0]->GetPlaySpeed(animGraphInstance);
                EXPECT_FLOAT_EQ(stateMachinePlaySpeed, activeStatePlaySpeed) <<
                    "The parent state machine playspeed equals the active state's in case we're not transitioning. In the transitioning case the current state playspeed is passed upwards.";
            }
        );

        EXPECT_TRUE(transitioned) << "The transition did not trigger and run. Test was unable to verify playspeeds.";
    }

    std::vector<AnimGraphStateMachineSyncParam> animGraphStateMachineSyncTestData
    {
        // Tests with syncing disabled
        {
            /*playSpeedA=*/0.3f,
            /*durationA=*/1.0f,
            /*playSpeedB=*/1.0f,
            /*durationB=*/1.0f,
            /*syncEnabled=*/false
        },
        {
            2.0f,
            0.5f,
            3.0f,
            1.0f,
            false
        },
        {
            5.0f,
            3.0f,
            2.0f,
            0.5f,
            false
        },
        // Tests with syncing
        {
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            true
        },
        {
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            true
        },
        {
            0.3f,
            0.5f,
            1.0f,
            2.0f,
            true
        },
        {
            3.0f,
            0.5f,
            1.0f,
            2.0f,
            true
        },
        {
            2.0f,
            3.0f,
            3.0f,
            0.5f,
            true
        }
    };

    INSTANTIATE_TEST_CASE_P(AnimGraphStateMachineSyncTests,
        AnimGraphStateMachineSyncFixture,
        ::testing::ValuesIn(animGraphStateMachineSyncTestData)
    );
} // namespace EMotionFX
