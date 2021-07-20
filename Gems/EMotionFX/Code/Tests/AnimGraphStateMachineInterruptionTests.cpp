/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        float transitionLeftBlendTime;
        float transitionLeftCountDownTime;
        float transitionMiddleBlendTime;
        float transitionMiddleCountDownTime;
        float transitionRightBlendTime;
        float transitionRightCountDownTime;

        // Per frame checks.
        struct ActiveObjectsAtFrame
        {
            AZ::u32 frameNr;

            bool stateA;
            bool stateB;
            bool stateC;
            bool transitionLeft;
            bool transitionMiddle;
            bool transitionRight;

            AZ::u32 numStatesEntering;
            AZ::u32 numStatesEntered;
            AZ::u32 numStatesExited;
            AZ::u32 numStatesEnded;
            AZ::u32 numTransitionsStarted;
            AZ::u32 numTransitionsEnded;
        };

        std::vector<ActiveObjectsAtFrame> activeObjectsAtFrame;
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
                param.transitionLeftBlendTime/*blendTime*/,
                param.transitionLeftCountDownTime)/*countDownTime*/;
            transitionLeft->SetCanBeInterrupted(true);

            AnimGraphStateTransition* transitionMiddle = AddTransitionWithTimeCondition(stateStart,
                stateB,
                param.transitionMiddleBlendTime,
                param.transitionMiddleCountDownTime);
            transitionMiddle->SetCanBeInterrupted(true);
            transitionMiddle->SetCanInterruptOtherTransitions(true);

            AnimGraphStateTransition* transitionRight = AddTransitionWithTimeCondition(stateStart,
                stateC,
                param.transitionRightBlendTime,
                param.transitionRightCountDownTime);
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
            /*preCallback*/[this]([[maybe_unused]] AnimGraphInstance* animGraphInstance){},
            /*postCallback*/[this]([[maybe_unused]] AnimGraphInstance* animGraphInstance){},
            /*preUpdateCallback*/[this](AnimGraphInstance*, float, float, int) {},
            /*postUpdateCallback*/[this](AnimGraphInstance* animGraphInstance, [[maybe_unused]] float time, [[maybe_unused]] float timeDelta, int frame)
            {
                const std::vector<AnimGraphStateMachine_InterruptionTestData::ActiveObjectsAtFrame>& activeObjectsAtFrame = GetParam().activeObjectsAtFrame;
                const AnimGraphStateMachine* stateMachine = this->m_rootStateMachine;
                const AZStd::vector<AnimGraphNode*>& activeStates = stateMachine->GetActiveStates(animGraphInstance);
                const AZStd::vector<AnimGraphStateTransition*>& activeTransitions = stateMachine->GetActiveTransitions(animGraphInstance);

                AnimGraphStateMachine_InterruptionTestData::ActiveObjectsAtFrame compareAgainst;

                compareAgainst.stateA = AZStd::find_if(activeStates.begin(), activeStates.end(),
                    [](AnimGraphNode* element) -> bool { return element->GetNameString() == "A"; }) != activeStates.end();
                compareAgainst.stateB = AZStd::find_if(activeStates.begin(), activeStates.end(),
                    [](AnimGraphNode* element) -> bool { return element->GetNameString() == "B"; }) != activeStates.end();
                compareAgainst.stateC = AZStd::find_if(activeStates.begin(), activeStates.end(),
                    [](AnimGraphNode* element) -> bool { return element->GetNameString() == "C"; }) != activeStates.end();

                compareAgainst.transitionLeft = AZStd::find_if(activeTransitions.begin(), activeTransitions.end(),
                    [](AnimGraphStateTransition* element) -> bool { return element->GetTargetNode()->GetNameString() == "A"; }) != activeTransitions.end();
                compareAgainst.transitionMiddle = AZStd::find_if(activeTransitions.begin(), activeTransitions.end(),
                    [](AnimGraphStateTransition* element) -> bool { return element->GetTargetNode()->GetNameString() == "B"; }) != activeTransitions.end();
                compareAgainst.transitionRight = AZStd::find_if(activeTransitions.begin(), activeTransitions.end(),
                    [](AnimGraphStateTransition* element) -> bool { return element->GetTargetNode()->GetNameString() == "C"; }) != activeTransitions.end();

                for (const auto& activeObjects : activeObjectsAtFrame)
                {
                    if (activeObjects.frameNr == frame)
                    {
                        // Check which states and transitions are active and compare it to the expected ones.
                        EXPECT_EQ(activeObjects.stateA, compareAgainst.stateA)
                            << "State A expected to be " << (activeObjects.stateA ? "active." : "inactive.");
                        EXPECT_EQ(activeObjects.stateB, compareAgainst.stateB)
                            << "State B expected to be " << (activeObjects.stateB ? "active." : "inactive.");
                        EXPECT_EQ(activeObjects.stateC, compareAgainst.stateC)
                            << "State C expected to be " << (activeObjects.stateB ? "active." : "inactive.");
                        EXPECT_EQ(activeObjects.transitionLeft, compareAgainst.transitionLeft)
                            << "Transition Start->A expected to be " << (activeObjects.transitionLeft ? "active." : "inactive.");
                        EXPECT_EQ(activeObjects.transitionMiddle, compareAgainst.transitionMiddle)
                            << "Transition Start->B expected to be " << (activeObjects.transitionMiddle ? "active." : "inactive.");
                        EXPECT_EQ(activeObjects.transitionRight, compareAgainst.transitionRight)
                            << "Transition Start->C expected to be " << (activeObjects.transitionRight ? "active." : "inactive.");

                        // Check anim graph events.
                        EXPECT_EQ(this->m_eventHandler->m_numStatesEntering, activeObjects.numStatesEntering)
                            << this->m_eventHandler->m_numStatesEntering << " states entering while " << activeObjects.numStatesEntering << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numStatesEntered, activeObjects.numStatesEntered)
                            << this->m_eventHandler->m_numStatesEntered << " states entered while " << activeObjects.numStatesEntered << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numStatesExited, activeObjects.numStatesExited)
                            << this->m_eventHandler->m_numStatesExited << " states exited while " << activeObjects.numStatesExited << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numStatesEnded, activeObjects.numStatesEnded)
                            << this->m_eventHandler->m_numStatesEnded << " states ended while " << activeObjects.numStatesEnded << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numTransitionsStarted, activeObjects.numTransitionsStarted)
                            << this->m_eventHandler->m_numTransitionsStarted << " transitions started while " << activeObjects.numTransitionsStarted << " are expected.";
                        EXPECT_EQ(this->m_eventHandler->m_numTransitionsEnded, activeObjects.numTransitionsEnded)
                            << this->m_eventHandler->m_numTransitionsEnded << " transitions ended while " << activeObjects.numTransitionsEnded << " are expected.";
                    }
                }
            }
        );
    }

    std::vector<AnimGraphStateMachine_InterruptionTestData> animGraphStateMachineInterruptionTestData
    {
        // Start transition Start->A, interrupt with Start->B while Start->A is still transitioning
        // Interrupt with Start->C while the others keep transitioning till Start->C is done
        {
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
        {
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
        float transitionLeftBlendTime;
        float transitionLeftCountDownTime;
        float transitionRightBlendTime;
        float transitionRightCountDownTime;
        AnimGraphStateTransition::EInterruptionMode interruptionMode;
        float maxBlendWeight;
        AnimGraphStateTransition::EInterruptionBlendBehavior interruptionBlendBehavior;
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
                    param.transitionLeftBlendTime,
                    param.transitionLeftCountDownTime);
            m_transitionLeft->SetCanBeInterrupted(true);
            m_transitionLeft->SetInterruptionMode(param.interruptionMode);
            m_transitionLeft->SetMaxInterruptionBlendWeight(param.maxBlendWeight);
            m_transitionLeft->SetInterruptionBlendBehavior(param.interruptionBlendBehavior);

            // Start->B (interrupting transition)
            m_transitionRight = AddTransitionWithTimeCondition(stateStart,
                    stateB,
                    param.transitionRightBlendTime,
                    param.transitionRightCountDownTime);
            m_transitionRight->SetCanInterruptOtherTransitions(true);

            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_motionNodeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
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
            /*preCallback*/[this]([[maybe_unused]] AnimGraphInstance* animGraphInstance) {},
            /*postCallback*/[this]([[maybe_unused]] AnimGraphInstance* animGraphInstance) {},
            /*preUpdateCallback*/[this](AnimGraphInstance*, float, float, int) {},
            /*postUpdateCallback*/[this, &prevGotInterrupted, &prevBlendWeight](AnimGraphInstance* animGraphInstance, [[maybe_unused]] float time, [[maybe_unused]] float timeDelta, [[maybe_unused]] int frame)
            {
                const AnimGraphStateMachine_InterruptionPropertiesTestData param = GetParam();

                const AnimGraphStateTransition::EInterruptionMode interruptionMode = m_transitionLeft->GetInterruptionMode();
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

    std::vector<AnimGraphStateMachine_InterruptionPropertiesTestData> animGraphStateMachineInterruptionPropertiesTestData
    {
        // Enable right transition at 0.5 while this is over the max blend weight already, don't allow interruption.
        {
            1.0f /*transitionLeftBlendTime*/,
            0.0f /*transitionLeftCountDownTime*/,
            1.0f /*transitionRightBlendTime*/,
            0.5f /*transitionRightCountDownTime*/,
            AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight /*interruptionMode*/,
            0.1f /*maxBlendWeight*/,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Continue /*interruptionBlendBehavior*/
        },
        // Right transition ready after 0.5 while still in range for the max blend weight, interruption expected.
        {
            1.0f,
            0.0f,
            1.0f,
            0.5f,
            AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight,
            0.6f,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Continue
        },
        // Interruption always allowed.
        {
            0.5f,
            0.0f,
            0.5f,
            0.2f,
            AnimGraphStateTransition::EInterruptionMode::MaxBlendWeight,
            1.0f,
            AnimGraphStateTransition::EInterruptionBlendBehavior::Continue
        },
        // Test if interrupted transitions stop transitioning with blend behavior set to stop.
        {
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
