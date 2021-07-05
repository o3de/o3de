
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeInfo.h>
#include <AzTest/AzTest.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphParameterAction.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class AnimGraphParameterActionTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            // 1. Add two state machines.
            m_node1 = aznew AnimGraphStateMachine();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_node1);
            m_animGraph->GetRootStateMachine()->SetEntryState(m_node1);

            m_node2 = aznew AnimGraphStateMachine();
            m_animGraph->GetRootStateMachine()->AddChildNode(m_node2);

            // 2. Add a transition and add a time condition.
            AnimGraphStateTransition* transition = AddTransition(m_node1, m_node2, 1.0f);
            AnimGraphTimeCondition* condition = aznew AnimGraphTimeCondition();
            condition->SetCountDownTime(0.1f);
            transition->AddCondition(condition);

            // 3. Add a parameter action.
            m_parameterAction = aznew AnimGraphParameterAction();
            TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();
            actionSetup.AddAction(m_parameterAction);
        }

        AnimGraphNode* m_node1 = nullptr;
        AnimGraphNode* m_node2 = nullptr;
        AnimGraphParameterAction* m_parameterAction = nullptr;
    };

    TEST_F(AnimGraphParameterActionTests, AnimGraphParameterActionTests_FloatParameter)
    {
        FloatParameter* parameter = aznew FloatSliderParameter();
        parameter->SetName("testFloat");
        parameter->SetDefaultValue(0.0f);
        m_animGraph->AddParameter(parameter);
        m_animGraphInstance->AddMissingParameterValues();

        const float triggerValue = 100.0f;
        m_parameterAction->SetParameterName("testFloat");
        m_parameterAction->SetTriggerValue(triggerValue);

        float outValue;
        EXPECT_TRUE(m_animGraphInstance->GetParameterValueAsFloat("testFloat", &outValue));
        EXPECT_NE(outValue, triggerValue);

        GetEMotionFX().Update(0.5f);

        EXPECT_TRUE(m_animGraphInstance->GetParameterValueAsFloat("testFloat", &outValue));
        EXPECT_EQ(outValue, triggerValue);

        GetEMotionFX().Update(1.0f);

        EXPECT_TRUE(m_animGraphInstance->GetParameterValueAsFloat("testFloat", &outValue));
        EXPECT_EQ(outValue, triggerValue)
            << "Expect the value get changed after the parameter action triggered.";
    }

    TEST_F(AnimGraphFixture, AnimGraphParameterAction_MoveParameterTest)
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

        m_animGraph->InitAfterLoading();

        CommandSystem::AddTransitionAction(transition, azrtti_typeid<AnimGraphParameterAction>());
        TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();
        ASSERT_EQ(actionSetup.GetNumActions(), 1) << "Something went wrong adding the parameter action to the transition.";
        AnimGraphParameterAction* action = azdynamic_cast<AnimGraphParameterAction*>(actionSetup.GetAction(0));

        // Add first parameter.
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            newParameter->SetName("Parameter1");
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(), newParameter.get(), MCORE_INVALIDINDEX32);
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        }

        // Add second parameter.
        const AZStd::string parameterName = "Parameter2";
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            newParameter->SetName(parameterName);
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(), newParameter.get(), MCORE_INVALIDINDEX32);
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        }

        action->SetParameterName(parameterName);
        action->Reinit();

        AZ::Outcome<size_t> parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        EXPECT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 1) << "Parameter2 should be at the 2nd position.";

        // 1. Move Parameter2 from the 2nd place to the 1st place.
        commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
            m_animGraph->GetID(),
            parameterName.c_str(),
            0);
        EXPECT_TRUE(commandManager.ExecuteCommand(commandString, result)) << result.c_str();
        parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        EXPECT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 0) << "Parameter2 should now be at the 1st position.";
        EXPECT_EQ(parameterIndex.GetValue(), action->GetParameterIndex().GetValue()) << "The action should now refer to the 1st parameter in the anim graph.";

        // 2. Undo.
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        EXPECT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 1) << "Parameter2 should now be back at the 2nd position.";
        EXPECT_EQ(parameterIndex.GetValue(), action->GetParameterIndex().GetValue()) << "The action should now refer to the 2nd parameter in the anim graph.";

        // 3. Redo.
        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
        EXPECT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 0) << "Parameter2 should now be back at the 1st position.";
        EXPECT_EQ(parameterIndex.GetValue(), action->GetParameterIndex().GetValue()) << "The action should now refer to the 1st parameter in the anim graph.";
    }
}
