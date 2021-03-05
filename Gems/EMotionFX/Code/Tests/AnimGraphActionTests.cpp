/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphSymbolicFollowerParameterAction.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class AnimGraphActionFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            AnimGraphNode* stateA = aznew AnimGraphBindPoseNode();
            m_rootStateMachine->AddChildNode(stateA);
            m_rootStateMachine->SetEntryState(stateA);

            AnimGraphNode* stateB = aznew AnimGraphBindPoseNode();
            m_rootStateMachine->AddChildNode(stateB);

            m_transition = AddTransitionWithTimeCondition(stateA, stateB, /*blendTime=*/1.0f, /*countDownTime=*/1.0f);
        }

        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_F(AnimGraphActionFixture, AnimGraphSymbolicFollowerParameterAction_TriggerWithEmptyParameterName)
    {
        CommandSystem::CommandManager commandManager;

        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphSymbolicFollowerParameterAction>());
        EXPECT_EQ(m_transition->GetTriggerActionSetup().GetNumActions(), 1)
            << "There should be exactly one transition action.";

        AnimGraphSymbolicFollowerParameterAction* action = azdynamic_cast<AnimGraphSymbolicFollowerParameterAction*>(m_transition->GetTriggerActionSetup().GetAction(0));
        ASSERT_NE(action, nullptr) << "Action not a valid symbolic follower parameter action.";

        GetEMotionFX().Update(1.0f);
    }
} // namespace EMotionFX
