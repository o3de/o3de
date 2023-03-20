/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphVector2Condition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    TEST_F(AnimGraphFixture, AnimGraphVector2Condition_MoveParameterTest)
    {
        AZStd::string result;
        AZStd::string commandString;
        CommandSystem::CommandManager commandManager;

        AnimGraphStateMachine* node1 = aznew AnimGraphStateMachine();
        m_animGraph->GetRootStateMachine()->AddChildNode(node1);
        m_animGraph->GetRootStateMachine()->SetEntryState(node1);

        AnimGraphStateMachine* node2 = aznew AnimGraphStateMachine();
        m_animGraph->GetRootStateMachine()->AddChildNode(node2);

        AnimGraphStateTransition* transition = AddTransition(node1, node2, 1.0f);

        AnimGraphVector2Condition* condition = aznew AnimGraphVector2Condition();
        transition->AddCondition(condition);

        m_animGraph->InitAfterLoading();

        // Add float slider parameter.
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            newParameter->SetName("Float Slider Parameter");
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(), newParameter.get(), InvalidIndex);
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        }

        // Add Vector2 parameter.
        const AZStd::string parameterName = "Vector2 Parameter";
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<Vector2Parameter>()));
            newParameter->SetName(parameterName);
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(), newParameter.get(), InvalidIndex);
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        }

        condition->SetParameterName(parameterName);
        condition->Reinit();

        AZ::Outcome<size_t> parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        ASSERT_TRUE(parameterIndex.IsSuccess());
        EXPECT_EQ(parameterIndex.GetValue(), 1) << "The Vector2 parameter should be at the 2nd position.";

        // 1. Move the Vector2 parameter from the 2nd place to the 1st place.
        commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
            m_animGraph->GetID(),
            parameterName.c_str(),
            0);
        EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        ASSERT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 0) << "The Vector2 parameter should now be at the 1st position.";
        EXPECT_EQ(parameterIndex.GetValue(), condition->GetParameterIndex().GetValue()) << "The Vector2 condition should now refer to the 1st parameter in the anim graph.";

        // 2. Undo.
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        ASSERT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 1) << "The Vector2 parameter should now be back at the 2nd position.";
        EXPECT_EQ(parameterIndex.GetValue(), condition->GetParameterIndex().GetValue()) << "The Vector2 condition should now refer to the 2nd parameter in the anim graph.";

        // 3. Redo.
        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        ASSERT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 0) << "The Vector2 parameter should now be back at the 1st position.";
        EXPECT_EQ(parameterIndex.GetValue(), condition->GetParameterIndex().GetValue()) << "The Vector2 condition should now refer to the 1st parameter in the anim graph.";
    }
} // namespace EMotionFX
