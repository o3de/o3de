/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionSet.h>
#include <Tests/AnimGraphEventHandlerCounter.h>
#include <Tests/AnimGraphFixture.h>


namespace EMotionFX
{
    struct AnimGraphEventTestParm
    {
        int m_numStates;
        float m_transitionBlendTime;
        float m_conditionCountDownTime;
        float m_simulationTime;
        float m_expectedFps;
        float m_fpsVariance;
    };

    class AnimGraphEventTestFixture : public AnimGraphFixture
        , public ::testing::WithParamInterface<AnimGraphEventTestParm>
    {
    public:
        void ConstructGraph() override
        {
            const AnimGraphEventTestParm& params = GetParam();
            AnimGraphFixture::ConstructGraph();

            /*
                +-------+    +---+    +---+             +---+
                | Start |--->| A |--->| B |---> ... --->| N |
                +-------+    +---+    +---+             +---+
            */
            AnimGraphNode* stateStart = aznew AnimGraphMotionNode();
            stateStart->SetName("Start");
            m_rootStateMachine->AddChildNode(stateStart);
            m_rootStateMachine->SetEntryState(stateStart);

            const char startChar = 'A';
            AnimGraphNode* prevState = stateStart;
            for (int i = 0; i < params.m_numStates; ++i)
            {
                AnimGraphNode* state = aznew AnimGraphMotionNode();
                state->SetName(AZStd::string(1, static_cast<char>(startChar + i)).c_str());
                m_rootStateMachine->AddChildNode(state);
                AddTransitionWithTimeCondition(prevState, state, /*blendTime*/params.m_transitionBlendTime, /*countDownTime*/params.m_conditionCountDownTime);
                prevState = state;
            }
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();

            MotionSet::MotionEntry* motionEntry = AddMotionEntry("testMotion", 1.0);

            // Assign a motion to all our motion nodes
            const size_t numStates = m_rootStateMachine->GetNumChildNodes();
            for (size_t i = 0; i < numStates; ++i)
            {
                AnimGraphMotionNode* motionNode = azdynamic_cast<AnimGraphMotionNode*>(m_rootStateMachine->GetChildNode(i));
                if (motionNode)
                {
                    motionNode->AddMotionId(motionEntry->GetId());
                }
            }

            m_eventHandler = aznew AnimGraphEventHandlerCounter();
            m_animGraphInstance->AddEventHandler(m_eventHandler);
        }

        void TearDown() override
        {
            m_animGraphInstance->RemoveEventHandler(m_eventHandler);
            delete m_eventHandler;

            AnimGraphFixture::TearDown();
        }

        void SimulateTest(float simulationTime, float expectedFps, float fpsVariance)
        {
            Simulate(simulationTime, expectedFps, fpsVariance,
                /*preCallback*/[](AnimGraphInstance*)
                {
                },
                /*postCallback*/[this](AnimGraphInstance*)
                {
                    const int numStates = GetParam().m_numStates;
                    const int numExpectedEvents = numStates + 1 /*numDeferEnter*/; // +1 because we defer the entering to entrance state at the beginning of update.
                    EXPECT_EQ(this->m_eventHandler->m_numStatesEntering, numExpectedEvents);
                    EXPECT_EQ(this->m_eventHandler->m_numStatesEntered, numExpectedEvents);
                    EXPECT_EQ(this->m_eventHandler->m_numStatesExited, numExpectedEvents);
                    EXPECT_EQ(this->m_eventHandler->m_numStatesEnded, numExpectedEvents);
                    EXPECT_EQ(this->m_eventHandler->m_numTransitionsStarted, numStates);
                    EXPECT_EQ(this->m_eventHandler->m_numTransitionsEnded, numStates);
                },
                /*preUpdateCallback*/[](AnimGraphInstance*, float, float, int) {},
                /*postUpdateCallback*/[](AnimGraphInstance*, float, float, int) {});

            const int numStates = GetParam().m_numStates;
            if (numStates > 1)
            {
                // Rewind state machine and check the event numbers again
                m_rootStateMachine->Rewind(m_animGraphInstance);
                GetEMotionFX().Update(1.0f/expectedFps);

                const int numExpectedEvents = numStates + 1 /*rewind*/ + 2 /*numDeferEnter*/;
                EXPECT_EQ(this->m_eventHandler->m_numStatesEntering, numExpectedEvents);
                EXPECT_EQ(this->m_eventHandler->m_numStatesEntered, numExpectedEvents);
                EXPECT_EQ(this->m_eventHandler->m_numStatesExited, numExpectedEvents);
                EXPECT_EQ(this->m_eventHandler->m_numStatesEnded, numExpectedEvents);
                EXPECT_EQ(this->m_eventHandler->m_numTransitionsStarted, numStates);
                EXPECT_EQ(this->m_eventHandler->m_numTransitionsEnded, numStates);
            }
        }

        AnimGraphEventHandlerCounter* m_eventHandler = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<AnimGraphEventTestParm> animGraphEventTestData
    {
        // Stable framerate
        {
            1,/*m_numStates*/
            1.0f,/*m_transitionBlendTime*/
            1.0f,/*m_conditionCountDownTime*/
            20.0f,/*m_simulationTime*/
            60.0f,/*m_expectedFps*/
            0.0f/*m_fpsVariance*/
        },
        {
            2,
            1.0f,
            1.0f,
            20.0f,
            60.0f,
            0.0f
        },
        {
            3,
            1.0f,
            1.0f,
            60.0f,
            60.0f,
            0.0f
        },
        {
            8,
            0.1f,
            0.1f,
            60.0f,
            60.0f,
            0.0f
        },
        {
            16,
            1.0f,
            1.0f,
            60.0f,
            60.0f,
            0.0f
        },
        // Unstable framerates
        {
            16,
            1.0f,
            1.0f,
            60.0f,
            30.0f,
            1.0f
        },
        {
            16,
            1.0f,
            1.0f,
            60.0f,
            10.0f,
            1.0f
        }
    };

    TEST_P(AnimGraphEventTestFixture, TestAnimGraphEvents)
    {
        const AnimGraphEventTestParm& params = GetParam();
        SimulateTest(params.m_simulationTime, params.m_expectedFps, params.m_fpsVariance);
    }

    INSTANTIATE_TEST_SUITE_P(TestAnimGraphEvents,
         AnimGraphEventTestFixture,
         ::testing::ValuesIn(animGraphEventTestData));
} // EMotionFX
