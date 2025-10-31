/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphSimpleStateAction.h>
#include <EMotionFX/Source/AnimGraphSymbolicFollowerParameterAction.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <LmbrCentral/Scripting/SimpleStateComponentBus.h>
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

    TEST_F(AnimGraphActionFixture, AnimGraphSimpleStateAction_BasicTests)
    {
        CommandSystem::CommandManager commandManager;

        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphSimpleStateAction>());
        EXPECT_EQ(m_transition->GetTriggerActionSetup().GetNumActions(), 1) << "There should be exactly one transition action.";

        // Setup the simple state action
        AnimGraphSimpleStateAction* action =
            azdynamic_cast<AnimGraphSimpleStateAction*>(m_transition->GetTriggerActionSetup().GetAction(0));
        ASSERT_NE(action, nullptr) << "Action not a valid simple state action.";

        GetEMotionFX().Update(1.0f);
    }
} // namespace EMotionFX
