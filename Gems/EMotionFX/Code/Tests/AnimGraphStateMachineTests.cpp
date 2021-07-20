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
#include <Tests/AnimGraphFixture.h>


namespace EMotionFX
{
#ifdef ENABLE_SINGLEFRAME_MULTISTATETRANSITIONING
    class AnimGraphStateMachine_MultiplePassesSingleFrameFixture : public AnimGraphFixture
        , public ::testing::WithParamInterface</*numStates*/int>
    {
    public:
        void ConstructGraph() override
        {
            const int numStates = GetParam();
            AnimGraphFixture::ConstructGraph();

            /*
                +-------+    +---+    +---+             +---+
                | Start |--->| A |--->| B |---> ... --->| N |
                +-------+    +---+    +---+             +---+
                BlendTime & CountDownTime = 0.0      m_lastState
            */
            AnimGraphNode* stateStart = aznew AnimGraphMotionNode();
            m_rootStateMachine->AddChildNode(stateStart);
            m_rootStateMachine->SetEntryState(stateStart);

            const char startChar = 'A';
            AnimGraphNode* prevState = stateStart;
            for (int i = 0; i < numStates; ++i)
            {
                AnimGraphNode* state = aznew AnimGraphMotionNode();
                state->SetName(AZStd::string(1, startChar + i).c_str());
                m_rootStateMachine->AddChildNode(state);
                AddTransitionWithTimeCondition(prevState, state, /*blendTime*/0.0f, /*countDownTime*/0.0f);
                prevState = state;
            }

            m_lastState = prevState;
        }

        AnimGraphNode* m_lastState = nullptr;
    };

    TEST_P(AnimGraphStateMachine_MultiplePassesSingleFrameFixture, TestAnimGraphStateMachine_MultiplePassesSingleFrameTest)
    {
        Simulate(/*simulationTime*/0.0f, /*expectedFps*/60.0f, /*fpsVariance*/0.0f,
            /*preCallback*/[this](AnimGraphInstance* animGraphInstance)
            {
            },
            /*postCallback*/[this](AnimGraphInstance* animGraphInstance)
            {
                // Check if we transitioned through the whole state machine and are at the last state.
                const AnimGraphNode* currentState = this->m_rootStateMachine->GetCurrentState(animGraphInstance);
                EXPECT_EQ(this->m_lastState, currentState);
            },
            /*preUpdateCallback*/[this](AnimGraphInstance*, float, float, int) {},
            /*postUpdateCallback*/[this](AnimGraphInstance*, float, float, int) {}
        );
    }

    std::vector<int> animGraphStateMachinePassesTestData
    {
        1/*numStates*/,
        2,
        3,
        8,
        static_cast<int>(AnimGraphStateMachine::GetMaxNumPasses())
    };

    INSTANTIATE_TEST_CASE_P(TestAnimGraphStateMachine_MultiplePassesSingleFrameTest,
         AnimGraphStateMachine_MultiplePassesSingleFrameFixture,
         ::testing::ValuesIn(animGraphStateMachinePassesTestData)
     );
#endif
} // EMotionFX
