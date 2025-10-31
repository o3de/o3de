/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Serialization/Locale.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class AnimGraphConditionCommandTestFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            
            AnimGraphStateMachine* node1 = aznew AnimGraphStateMachine();
            m_rootStateMachine->AddChildNode(node1);
            m_rootStateMachine->SetEntryState(node1);

            AnimGraphStateMachine* node2 = aznew AnimGraphStateMachine();
            m_rootStateMachine->AddChildNode(node2);

            m_transition = AddTransition(node1, node2, 1.0f);
        }

    public:
        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_F(AnimGraphConditionCommandTestFixture, AdjustConditionCommandTests)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        AZStd::string result;
        CommandSystem::CommandManager commandManager;

        // Add time condition.
        CommandSystem::CommandAddTransitionCondition* addConditionCommand = aznew CommandSystem::CommandAddTransitionCondition(
            m_animGraph->GetID(),
            m_transition->GetId(),
            azrtti_typeid<AnimGraphTimeCondition>());
        EXPECT_TRUE(commandManager.ExecuteCommand(addConditionCommand, result)) << result.c_str();
        EXPECT_EQ(m_transition->GetNumConditions(), 1) << "There should be a single condition.";
        const AZ::u32 conditionIndex = 0;
        AnimGraphTimeCondition* timeCondition = azdynamic_cast<AnimGraphTimeCondition*>(m_transition->GetCondition(conditionIndex));
        EXPECT_EQ(timeCondition->GetCountDownTime(), 1.0f) << "Condition should have the default value for the count-down time.";

        // 1. Adjust attribute of the time condition.
        const AZStd::string attributesString = AZStd::string::format("-countDownTime %f", 2.0f);
        CommandSystem::CommandAdjustTransitionCondition* adjustConditionCommand = aznew CommandSystem::CommandAdjustTransitionCondition(
            m_animGraph->GetID(),
            m_transition->GetId(),
            conditionIndex,
            attributesString);
        EXPECT_TRUE(commandManager.ExecuteCommand(adjustConditionCommand, result));

        // 2. Undo.
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_EQ(timeCondition->GetCountDownTime(), 1.0f) << "Condition should be back at the default count-down time.";

        // 3. Redo.
        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_EQ(timeCondition->GetCountDownTime(), 2.0f) << "Condition should hold the adjusted count-down time.";
    }
} // namespace EMotionFX
