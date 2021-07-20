
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeInfo.h>
#include <AzTest/AzTest.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    void ValidateTagCondition(AnimGraphTagCondition* tagCondition)
    {
        AnimGraph* animGraph = tagCondition->GetAnimGraph();
        ASSERT_NE(animGraph, nullptr);

        const AZStd::vector<size_t>& parameterIndices = tagCondition->GetTagParameterIndices();
        const AZStd::vector<AZStd::string> parameterNames = tagCondition->GetParameters();
        const size_t numParameterNames = parameterNames.size();
        ASSERT_EQ(parameterIndices.size(), numParameterNames) << "Indices are not up to date with the parameter names.";

        for (size_t i = 0; i < numParameterNames; ++i)
        {
            const AZStd::string& parameterName = parameterNames[i];
            AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndexByName(parameterName);
            EXPECT_TRUE(parameterIndex.IsSuccess()) << "Parameter " << parameterName.c_str() << " does not exist in the anim graph.";
            EXPECT_EQ(parameterIndex.GetValue(), parameterIndices[i]) << "Index for parameter " << parameterName.c_str() << "out of date.";
        }
    }

    TEST_F(AnimGraphFixture, AnimGraphTagCondition_MoveAndRemoveParameterTest)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string commandResult;
        AZStd::string commandString;

        AnimGraphStateMachine* node1 = aznew AnimGraphStateMachine();
        m_animGraph->GetRootStateMachine()->AddChildNode(node1);
        m_animGraph->GetRootStateMachine()->SetEntryState(node1);

        AnimGraphStateMachine* node2 = aznew AnimGraphStateMachine();
        m_animGraph->GetRootStateMachine()->AddChildNode(node2);
        AnimGraphStateTransition* transition = AddTransition(node1, node2, 1.0f);

        m_animGraph->InitAfterLoading();

        // Add tag condition.
        CommandSystem::CommandAddTransitionCondition* addConditionCommand = aznew CommandSystem::CommandAddTransitionCondition(
            m_animGraph->GetID(), transition->GetId(), azrtti_typeid<AnimGraphTagCondition>());
        EXPECT_TRUE(commandManager.ExecuteCommand(addConditionCommand, commandResult)) << commandResult.c_str();
        ASSERT_EQ(transition->GetNumConditions(), 1) << "Something went wrong adding the tag condition to the transition.";
        AnimGraphTagCondition* tagCondition = azdynamic_cast<AnimGraphTagCondition*>(transition->GetCondition(0));
        ASSERT_NE(tagCondition, nullptr) << "The condition should be a tag condition.";

        // Add a float slider parameter.
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            newParameter->SetName("Parameter1");
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(), newParameter.get());
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, commandResult)) << commandResult.c_str();
        }

        // Add several tag parameters.
        const AZStd::vector<AZStd::string> tagParameterNames = {"Tag1", "Tag2", "Tag3"};
        for (const AZStd::string& tagParameterName : tagParameterNames)
        {
            AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<TagParameter>()));
            newParameter->SetName(tagParameterName);
            CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph.get(), newParameter.get());
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, commandResult)) << commandResult.c_str();
        }

        const AZStd::vector<AZStd::string> initialConditionTags = {"Tag1", "Tag3"};
        tagCondition->SetTags(initialConditionTags);
        tagCondition->Reinit();
        ValidateTagCondition(tagCondition);

        // Remove Tag1
        {
            MCore::CommandGroup removeParameterGroup;
            CommandSystem::BuildRemoveParametersCommandGroup(m_animGraph.get(), { "Tag1" }, &removeParameterGroup);
            EXPECT_TRUE(commandManager.ExecuteCommandGroup(removeParameterGroup, commandResult)) << commandResult.c_str();
            EXPECT_THAT(tagCondition->GetParameters(), ::testing::ElementsAre("Tag3"))
                << "The condition should only be linked to Tag3 after removing Tag1";
            ValidateTagCondition(tagCondition);

            // Undo.
            EXPECT_TRUE(commandManager.Undo(commandResult)) << commandResult.c_str();
            EXPECT_EQ(tagCondition->GetParameters(), initialConditionTags);
            ValidateTagCondition(tagCondition);

            // Redo.
            EXPECT_TRUE(commandManager.Redo(commandResult)) << commandResult.c_str();
            EXPECT_THAT(tagCondition->GetParameters(), ::testing::ElementsAre("Tag3"))
                << "The condition should only be linked to Tag3 after removing Tag1";
            ValidateTagCondition(tagCondition);
        }

        // Move Tag3 up.
        {
            const AZStd::string parameterName = "Tag3";
            AZ::Outcome<size_t> parameterIndex = m_animGraph->FindValueParameterIndexByName(parameterName);
            EXPECT_TRUE(parameterIndex.IsSuccess() && parameterIndex.GetValue() == 2) << "Tag3 should be at the 3rd position after removing Tag1.";

            // Move Tag3 from the 3rd place to the 1st place.
            commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d",
                m_animGraph->GetID(),
                parameterName.c_str(),
                0);
            EXPECT_TRUE(commandManager.ExecuteCommand(commandString, commandResult)) << commandResult.c_str();
            ValidateTagCondition(tagCondition);

            // Undo.
            EXPECT_TRUE(commandManager.Undo(commandResult)) << commandResult.c_str();
            ValidateTagCondition(tagCondition);

            // Redo.
            EXPECT_TRUE(commandManager.Redo(commandResult)) << commandResult.c_str();
            ValidateTagCondition(tagCondition);
        }
    }
}
