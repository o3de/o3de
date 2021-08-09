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
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    TEST_F(AnimGraphFixture, AnimGraphParameterCondition_UndoRemoveParameterTest)
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

        AnimGraphParameterCondition* condition = aznew AnimGraphParameterCondition();
        transition->AddCondition(condition);

        m_animGraph->InitAfterLoading();

        // Add parameter to the anim graph and the instance.
        const AZStd::string parameterName = "Parameter1";
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            newParameter->SetName(parameterName);
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(),
                newParameter.get(),
                InvalidIndex);

            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        }

        condition->SetParameterName(parameterName);

        // 1. Remove the parameter including unlinking it from the parameter condition.
        MCore::CommandGroup commandGroup;
        CommandSystem::BuildRemoveParametersCommandGroup(m_animGraph.get(), /*parameterNamesToBeRemoved=*/{ parameterName }, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result)) << result.c_str();
        EXPECT_EQ(m_animGraph->GetNumParameters(), 0) << "The parameter should not be present anymore.";
        EXPECT_EQ(condition->GetParameterName(), "") << "The condition should not be linked to the removed parameter anymore.";

        // 2. Undo.
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_EQ(m_animGraph->GetNumParameters(), 1) << "The parameter should be back again.";
        condition = azdynamic_cast<AnimGraphParameterCondition*>(transition->GetCondition(0));
        EXPECT_EQ(condition->GetParameterName(), parameterName) << "The condition should be linked to the parameter again.";

        // 3. Redo.
        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        condition = azdynamic_cast<AnimGraphParameterCondition*>(transition->GetCondition(0));
        EXPECT_EQ(m_animGraph->GetNumParameters(), 0) << "The parameter should not be present anymore.";
        EXPECT_EQ(condition->GetParameterName(), "") << "The condition should not be linked to the removed parameter anymore.";
    }
} // namespace EMotionFX
