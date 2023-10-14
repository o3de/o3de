/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionSet.h>
#include <Tests/AnimGraphEventHandlerCounter.h>
#include <Tests/AnimGraphFixture.h>


namespace EMotionFX
{
    struct AnimGraphStateMachine_InterruptionTestData
    {
        // Graph construction data.
        float m_transitionLeftBlendTime;
        float m_transitionLeftCountDownTime;
        float m_transitionMiddleBlendTime;
        float m_transitionMiddleCountDownTime;
        float m_transitionRightBlendTime;
        float m_transitionRightCountDownTime;

        // Per frame checks.
        struct ActiveObjectsAtFrame
        {
            AZ::u32 m_frameNr;

            bool m_stateA;
            bool m_stateB;
            bool m_stateC;
            bool m_transitionLeft;
            bool m_transitionMiddle;
            bool m_transitionRight;

            AZ::u32 m_numStatesEntering;
            AZ::u32 m_numStatesEntered;
            AZ::u32 m_numStatesExited;
            AZ::u32 m_numStatesEnded;
            AZ::u32 m_numTransitionsStarted;
            AZ::u32 m_numTransitionsEnded;
        };

        AZStd::fixed_vector<ActiveObjectsAtFrame, 16> m_activeObjectsAtFrame;
    };

    class AnimGraphStateMachine_InterruptionFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<AnimGraphStateMachine_InterruptionTestData>
    {
    public:
        void ConstructGraph() override
        {
            const AnimGraphStateMachine_InterruptionTestData param = GetParam();
            AnimGraphFixture::ConstructGraph();

            /*
                +---+    +---+    +---+
                | A |    | B |    | C |
                +-+-+    +-+-+    +-+-+
                  ^        ^        ^
                  |        |        |
                  |    +---+---+    |
                  +----+ Start +----+
                       +-------+
            */
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();

            AnimGraphNode* stateStart = aznew AnimGraphMotionNode();
            m_rootStateMachine->AddChildNode(stateStart);
            m_rootStateMachine->SetEntryState(stateStart);

            AnimGraphNode* stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            AnimGraphNode* stateB = m_motionNodeAnimGraph->GetMotionNodeB();
            AnimGraphNode* stateC = aznew AnimGraphMotionNode();
            stateC->SetName("C");
            m_rootStateMachine->AddChildNode(stateC);

            AnimGraphStateTransition* transitionLeft = AddTransitionWithTimeCondition(stateStart,
                stateA,
                param.m_transitionLeftBlendTime/*blendTime*/,
                param.m_transitionLeftCountDownTime)/*countDownTime*/;
            transitionLeft->SetCanBeInterrupted(true);

            AnimGraphStateTransition* transitionMiddle = AddTransitionWithTimeCondition(stateStart,
                stateB,
                param.m_transitionMiddleBlendTime,
                param.m_transitionMiddleCountDownTime);
            transitionMiddle->SetCanBeInterrupted(true);
            transitionMiddle->SetCanInterruptOtherTransitions(true);

            AnimGraphStateTransition* transitionRight = AddTransitionWithTimeCondition(stateStart,
                stateC,
                param.m_transitionRightBlendTime,
                param.m_transitionRightCountDownTime);
            transitionRight->SetCanInterruptOtherTransitions(true);

            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_motionNodeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);

            m_eventHandler = aznew AnimGraphEventHandlerCounter();
            m_animGraphInstance->AddEventHandler(m_eventHandler);
        }

        void TearDown() override
        {
            m_animGraphInstance->RemoveEventHandler(m_eventHandler);
            delete m_eventHandler;
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Destroy();
                m_animGraphInstance = nullptr;
            }
            m_motionNodeAnimGraph.reset();

            AnimGraphFixture::TearDown();
        }

        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphEventHandlerCounter* m_eventHandler = nullptr;
    };

    TEST_P(AnimGraphStateMachine_InterruptionFixture, AnimGraphStateMachine_InterruptionTest)
    {
        // Defer enter entry state on state machine update.
        m_eventHandler->m_numStatesEntering -= 1;
        m_eventHandler->m_numStatesEntered -= 1;
        m_eventHandler->m_numStatesExited -= 1;
        m_eventHandler->m_numStatesEnded -= 1;

        Simulate(20.0f/*simulationTime*/, 60.0f/*expectedFps*/, 0.0f/*fpsVariance*/,
            /*preCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance){},
            /*postCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance){},
            /*preUpdateCallback*/[](AnimGraphInstance*, float, float, int) {},
            /*postUpdateCallback*/[this](AnimGraphInstance* animGraphInstance, [[maybe_unused]] float time, [[maybe_unused]] float timeDelta, int frame)
            {
                const auto& activeObjectsAtFrame = GetParam().m_activeObjectsAtFrame;
                const AnimGraphStateMachine* stateMachine = this->m_rootStateMachine;
                const AZStd::vector<AnimGraphNode*>& activeStates = stateMachine->GetActiveStates(animGraphInstance);
                const AZStd::vector<AnimGraphStateTransition*>& activeTransitions = stateMachine->GetActiveTransitions(animGraphInstance);

                AnimGraphStateMachine_InterruptionTestData::ActiveObjectsAtFrame compareAgainst;

                compareAgainst.m_stateA = AZStd::find_if(activeStates.begin(), activeStates.end(),
                    [](AnimGraphNode* element) -> bool { return element->GetNameString() == "A"; }) != activeStates.end();
                compareAgainst.m_stateB = AZStd::find_if(activeStates.begin(), activeStates.end(),
                    [](AnimGraphNode* element) -> bool { return element->GetNameString() == "B"; }) != activeStates.end();
                compareAgainst.m_stateC = AZStd::find_if(activeStates.begin(), activeStates.end(),
                    [](AnimGraphNode* element) -> bool { return element->GetNameString() == "C"; }) != activeStates.end();

                compareAgainst.m_transitionLeft = AZStd::find_if(activeTransitions.begin(), activeTransitions.end(),
                    [](AnimGraphStateTransition* element) -> bool { return element->GetTargetNode()->GetNameString() == "A"; }) != activeTransitions.end();
                compareAgainst.m_transitionMiddle = AZStd::find_if(activeTransitions.begin(), activeTransitions.end(),
                    [](AnimGraphStateTransition* element) -> bool { return element->GetTargetNode()->GetNameString() == "B"; }) != activeTransitions.end();
                compareAgainst.m_transitionRight = AZStd::find_if(activeTransitions.begin(), activeTransitions.end(),
                    [](AnimGraphStateTransition* element) -> bool { return element->GetTargetNode()->GetNameString() == "C"; }) != activeTransitions.end();

                for (const AnimGraphStateMachine_InterruptionTestData::ActiveObjectsAtFrame& activeObjects : activeObjectsAtFrame)
                {
                    if (activeObjects.m_frameNr == static_cast<AZ::u32>(frame))
                    {
                        // Check which states and transitions are active and compare it to the expected ones.
                        EXPECT_THAT(activeObjects.m_stateA, ::testing::Eq(compareAgainst.m_stateA))
                            << "State A expected to be " << (activeObjects.m_stateA ? "active." : "inactive.");
                        EXPECT_THAT(activeObjects.m_stateB, ::testing::Eq(compareAgainst.m_stateB))
                            << "State B expected to be " << (activeObjects.m_stateB ? "active." : "inactive.");
                        EXPECT_THAT(activeObjects.m_stateC, ::testing::Eq(compareAgainst.m_stateC))
                            << "State C expected to be " << (activeObjects.m_stateB ? "active." : "inactive.");
                        EXPECT_THAT(activeObjects.m_transitionLeft, ::testing::Eq(compareAgainst.m_transitionLeft))
                            << "Transition Start->A expected to be " << (activeObjects.m_transitionLeft ? "active." : "inactive.");
                        EXPECT_THAT(activeObjects.m_transitionMiddle, ::testing::Eq(compareAgainst.m_transitionMiddle))
                            << "Transition Start->B expected to be " << (activeObjects.m_transitionMiddle ? "active." : "inactive.");
                        EXPECT_THAT(activeObjects.m_transitionRight, ::testing::Eq(compareAgainst.m_transitionRight))
                            << "Transition Start->C expected to be " << (activeObjects.m_transitionRight ? "active." : "inactive.");

                        // Check anim graph events.
                        EXPECT_EQ(this->m_eventHandler->m_numStatesEntering, activeObjects.m_numStatesEntering)
                            << this->m_eventHandler->m_numStatesEntering << " states entering while " << activeObjects.m_numStatesEntering << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numStatesEntered, activeObjects.m_numStatesEntered)
                            << this->m_eventHandler->m_numStatesEntered << " states entered while " << activeObjects.m_numStatesEntered << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numStatesExited, activeObjects.m_numStatesExited)
                            << this->m_eventHandler->m_numStatesExited << " states exited while " << activeObjects.m_numStatesExited << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numStatesEnded, activeObjects.m_numStatesEnded)
                            << this->m_eventHandler->m_numStatesEnded << " states ended while " << activeObjects.m_numStatesEnded << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numTransitionsStarted, activeObjects.m_numTransitionsStarted)
                            << this->m_eventHandler->m_numTransitionsStarted << " transitions started while " << activeObjects.m_numTransitionsStarted << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numTransitionsEnded, activeObjects.m_numTransitionsEnded)
                            << this->m_eventHandler->m_numTransitionsEnded << " transitions ended while " << activeObjects.m_numTransitionsEnded << " are expected.";


                    }
                }
            }
        );
    }

    AZStd::array animGraphStateMachineInterruptionTestData
    {
        // Start transition Start->A, interrupt with Start->B while Start->A is still transitioning
        // Interrupt with Start->C while the others keep transitioning till Start->C is done
        AnimGraphStateMachine_InterruptionTestData{
            10.0f/*transitionLeftBlendTime*/,
            1.0f/*transitionLeftCountDownTime*/,
            10.0f/*transitionMiddleBlendTime*/,
            2.0f/*transitionMiddleCountDownTime*/,
            5.0/*transitionRightBlendTime*/,
            3.0f/*transitionRightCountDownTime*/,
            {
                {
                    0/*_frameNr*/,
                    false/*stateA*/, false/*stateB*/, false/*stateC*/,
                    false/*transitionLeft*/, false/*transitionMiddle*/, false/*transitionRight*/,
                    0/*numStatesEntering*/, 0/*numStatesEntered*/,
                    0/*numStatesExited*/, 0/*numStatesEnded*/,
                    0/*numTransitionsStarted*/, 0/*numTransitionsEnded*/
                },
                {
                    60, // Start transition: Start->A
                    true, false, false,
                    true, false, false,
                    1, 0,
                    1, 0,
                    1, 0
                },
                {
                    90,
                    true, false, false,
                    true, false, false,
                    1, 0,
                    1, 0,
                    1, 0
                },
                {
                    120, // Interrupt transition: Start->A and start transition Start->B
                    true, true, false,
                    true, true, false,
                    2, 0,
                    2, 0,
                    2, 0
                },
                {
                    150,
                    true, true, false,
                    true, true, false,
                    2, 0,
                    2, 0,
                    2, 0
                },
                {
                    300, // Interrupt transition: Start->B and start transition Start->C
                    true, true, true,
                    true, true, true,
                    3, 0,
                    3, 0,
                    3, 0
                },
                {
                    330,
                    true, true, true,
                    true, true, true,
                    3, 0,
                    3, 0,
                    3, 0
                },
                {
                    480,
                    false, false, true,
                    false, false, false,
                    3, 3,
                    3, 3,
                    3, 3
                }
            }
        },
        // Start transition Start->A and let Start->B/C interrupt it
        // Start->A/B finishes and holds the target state active while Start->C is finishing
        AnimGraphStateMachine_InterruptionTestData{
            2.0f/*transitionLeftBlendTime*/,
            1.0f/*transitionLeftCountDownTime*/,
            3.0f/*transitionMiddleBlendTime*/,
            2.0f/*transitionMiddleCountDownTime*/,
            10.0/*transitionRightBlendTime*/,
            4.0f/*transitionRightCountDownTime*/,
            {
                {
                    0,
                    false, false, false,
                    false, false, false,
                    0, 0,
                    0, 0,
                    0, 0
                },
                {
                    60, // Start transition: Start->A
                    true, false, false,
                    true, false, false,
                    1, 0,
                    1, 0,
                    1, 0
                },
                {
                    90,
                    true, false, false,
                    true, false, false,
                    1, 0,
                    1, 0,
                    1, 0
                },
                {
                    120, // Interrupt transition: Start->A and start transition Start->B
                    true, true, false,
                    true, true, false,
                    2, 0,
                    2, 0,
                    2, 0
                },
                {
                    150,
                    true, true, false,
                    true, true, false,
                    2, 0,
                    2, 0,
                    2, 0
                },
                {
                    180, // Start->A finishes and stays on the transition stack to keep the target state active
                    true, true, false,
                    true, true, false,
                    2, 0,
                    2, 0,
                    2, 0
                },
                {
                    240, // Interrupt transition Start->B with Start->C
                    true, true, true,
                    true, true, true,
                    3, 0,
                    3, 0,
                    3, 0
                },
                {
                    300, // Transition Start->B finishes and stays on the transition stack to keep the target state active
                    true, true, true,
                    true, true, true,
                    3, 0,
                    3, 0,
                    3, 0
                },
                {
                    330,
                    true, true, true,
                    true, true, true,
                    3, 0,
                    3, 0,
                    3, 0
                },
                {
                    840, // Latest active transition finishes and cleared the transition stack
                    false, false, true,
                    false, false, false,
                    3, 3,
                    3, 3,
                    3, 3
                }
            }
        }
    };

    INSTANTIATE_TEST_CASE_P(AnimGraphStateMachine_InterruptionTest,
        AnimGraphStateMachine_InterruptionFixture,
            ::testing::ValuesIn(animGraphStateMachineInterruptionTestData)
        );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct AnimGraphStateMachine_InterruptionPropertiesTestData
    {
        float m_transitionLeftBlendTime;
        float m_transitionLeftCountDownTime;
        float m_transitionRightBlendTime;
        float m_transitionRightCountDownTime;
        AnimGraphStateTransition::EInterruptionMode m_interruptionMode;
        float m_maxBlendWeight;
        AnimGraphStateTransition::EInterruptionBlendBehavior m_interruptionBlendBehavior;
    };

    class AnimGraphStateMachine_InterruptionPropertiesFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<AnimGraphStateMachine_InterruptionPropertiesTestData>
    {
    public:
        void ConstructGraph() override
        {
            const AnimGraphStateMachine_InterruptionPropertiesTestData param = GetParam();
            AnimGraphFixture::ConstructGraph();

            /*
                +---+             +---+
                | A |             | B |
                +-+-+             +-+-+
                  ^                 ^
                  |    +---+---+    |
                  +----+ Start +----+
                       +-------+
            */
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();

            AnimGraphNode* stateStart = aznew AnimGraphMotionNode();
            m_rootStateMachine->AddChildNode(stateStart);
            m_rootStateMachine->SetEntryState(stateStart);

            AnimGraphNode* stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            AnimGraphNode* stateB = m_motionNodeAnimGraph->GetMotionNodeB();

            // Start->A (can be interrupted)
            m_transitionLeft = AddTransitionWithTimeCondition(stateStart,
                    stateA,
                    param.m_transitionLeftBlendTime,
                    param.m_transitionLeftCountDownTime);
            m_transitionLeft->SetCanBeInterrupted(true);
            m_transitionLeft->SetInterruptionMode(param.m_interruptionMode);
            m_transitionLeft->SetMaxInterruptionBlendWeight(param.m_maxBlendWeight);
            m_transitionLeft->SetInterruptionBlendBehavior(param.m_interruptionBlendBehavior);

            // Start->B (interrupting transition)
            m_transitionRight = AddTransitionWithTimeCondition(stateStart,
                    stateB,
                    param.m_transitionRightBlendTime,
                    param.m_transitionRightCountDownTime);
            m_transitionRight->SetCanInterruptOtherTransitions(true);

            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_motionNodeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
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

        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphStateTransition* m_transitionLeft = nullptr;
        AnimGraphStateTransition* m_transitionRight = nullptr;
    };

    TEST_P(AnimGraphStateMachine_InterruptionPropertiesFixture, AnimGraphStateMachine_InterruptionPropertiesTest)
    {
        bool prevGotInterrupted = false;
        float prevBlendWeight = 0.0f;

        Simulate(2.0f /*simulationTime*/, 10.0f /*expectedFps*/, 0.0f /*fpsVariance*/,
            /*preCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance) {},
            /*postCallback*/[]([[maybe_unused]] AnimGraphInstance* animGraphInstance) {},
            /*preUpdateCallback*/[](AnimGraphInstance*, float, float, int) {},
            /*postUpdateCallback*/[this, &prevGotInterrupted, &prevBlendWeight](AnimGraphInstance* animGraphInstance, [[maybe_unused]] float time, [[maybe_unused]] float timeDelta, [[maybe_unused]] int frame)
            {
                const float maxInterruptionBlendWeight = m_transitionLeft->GetMaxInterruptionBlendWeight();
                const bool gotInterrupted = m_transitionLeft->GotInterrupted(animGraphInstance);
                const bool gotInterruptedThisFrame = gotInterrupted && !prevGotInterrupted;
                const float blendWeight = m_transitionLeft->GetBlendWeight(animGraphInstance);

                if (m_transitionLeft->GetInterruptionMode() == AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight)
                {
                    if (blendWeight > maxInterruptionBlendWeight)
                    {
                        EXPECT_FALSE(gotInterruptedThisFrame) << "No interruption should be possible anymore at blend weight " << blendWeight << ".";
                    }
                    else
                    {
                        if (m_transitionRight->CheckIfIsReady(animGraphInstance))
                        {
                            EXPECT_TRUE(gotInterrupted) << "Interruption should have happened.";
                        }
                    }
                }

                if (gotInterrupted && !gotInterruptedThisFrame &&
                    m_transitionLeft->GetInterruptionBlendBehavior() == AnimGraphStateTransition::EInterruptionBlendBehavior::Stop)
                {
                    EXPECT_FLOAT_EQ(prevBlendWeight, blendWeight) << "The blend weight should not change anymore after the transition got interrupted when using blend behavior stop.";
                }

                prevGotInterrupted = gotInterrupted;
                prevBlendWeight = blendWeight;
            }
            );
    }

    AZStd::array animGraphStateMachineInterruptionPropertiesTestData
    {
        // Enable right transition at 0.5 while this is over the max blend weight already, don't allow interruption.
        AnimGraphStateMachine_InterruptionPropertiesTestData{
            1.0f /*transitionLeftBlendTime*/,
            0.0f /*transitionLeftCountDownTime*/,
            1.0f /*transitionRightBlendTime*/,
            0.5f /*transitionRightCountDownTime*/,
            AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight /*interruptionMode*/,
            0.1f /*maxBlendWeight*/,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Continue /*interruptionBlendBehavior*/
        },
        // Right transition ready after 0.5 while still in range for the max blend weight, interruption expected.
        AnimGraphStateMachine_InterruptionPropertiesTestData{
            1.0f,
            0.0f,
            1.0f,
            0.5f,
            AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight,
            0.6f,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Continue
        },
        // Interruption always allowed.
        AnimGraphStateMachine_InterruptionPropertiesTestData{
            0.5f,
            0.0f,
            0.5f,
            0.2f,
            AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight,
            1.0f,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Continue
        },
        // Test if interrupted transitions stop transitioning with blend behavior set to stop.
        AnimGraphStateMachine_InterruptionPropertiesTestData{
            1.0f,
            0.0f,
            1.0f,
            0.5f,
            AnimGraphStateTransition::EInterruptionMode::AlwaysAllowed,
            0.0f,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Stop
        },
    };

    INSTANTIATE_TEST_CASE_P(AnimGraphStateMachine_InterruptionPropertiesTest,
        AnimGraphStateMachine_InterruptionPropertiesFixture,
            ::testing::ValuesIn(animGraphStateMachineInterruptionPropertiesTestData)
        );
} // EMotionFX
