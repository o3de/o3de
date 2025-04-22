/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphParameterAction.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/MotionSet.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    class AnimGraphActionCommandsFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();
            m_stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            m_stateB = m_motionNodeAnimGraph->GetMotionNodeB();
            m_transition = AddTransition(m_stateA, m_stateB, 1.0f);
            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void TearDown() override
        {
            m_motionNodeAnimGraph.reset();
            AnimGraphFixture::TearDown();
        }

        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphNode* m_stateA = nullptr;
        AnimGraphNode* m_stateB = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_F(AnimGraphActionCommandsFixture, AnimGraphActionCommandTests_AddTransitionAction)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const AZStd::string serializedOriginal = SerializeAnimGraph();

        // 1. Add transition action.
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>());
        const AZStd::string serializedAfterAddSingle = SerializeAnimGraph();
        EXPECT_EQ(1, m_transition->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly one transition action.";

        // 2. Add multiple transition actions in a command group.
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>(), AZStd::nullopt, AZStd::nullopt, &commandGroup);
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>(), AZStd::nullopt, AZStd::nullopt, &commandGroup);
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>(), AZStd::nullopt, AZStd::nullopt, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        commandGroup.RemoveAllCommands();
        EXPECT_EQ(4, m_transition->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly four transition actions.";

        // 3. Undo add multiple transition actions.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(1, m_transition->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly one transition action left.";
        EXPECT_EQ(serializedAfterAddSingle, SerializeAnimGraph());

        // 4. Undo add transition action.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(0, m_transition->GetTriggerActionSetup().GetNumActions())
            << "There should be no transition action left anymore.";
        EXPECT_EQ(serializedOriginal, SerializeAnimGraph());
    }

    TEST_F(AnimGraphActionCommandsFixture, AnimGraphActionCommandTests_UndoRemoveTransitionWithAction)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;

        // 1. Add transition action.
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>());
        const AZStd::string serializedAfterAddAction = SerializeAnimGraph();
        EXPECT_EQ(1, m_transition->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly one transition action.";

        // 2. Remove the whole transition including the action.
        {
            AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
            CommandSystem::DeleteStateTransition(&commandGroup, m_transition, transitionList);
        }
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        commandGroup.RemoveAllCommands();
        EXPECT_EQ(0, m_rootStateMachine->GetNumTransitions()) << "The transition A->B should be gone.";

        // 3. Undo remove transition.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(serializedAfterAddAction, SerializeAnimGraph());
    }

    TEST_F(AnimGraphActionCommandsFixture, AnimGraphActionCommandTests_AddStateAction)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const AZStd::string serializedOriginal = SerializeAnimGraph();

        // 1. Add state action.
        CommandSystem::AddStateAction(m_stateA, azrtti_typeid<AnimGraphParameterAction>());
        const AZStd::string serializedAfterAddSingle = SerializeAnimGraph();
        EXPECT_EQ(1, m_stateA->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly one state action.";

        // 2. Add multiple state actions in a command group.
        CommandSystem::AddStateAction(m_stateA, azrtti_typeid<AnimGraphParameterAction>(), AZStd::nullopt, AZStd::nullopt, &commandGroup);
        CommandSystem::AddStateAction(m_stateA, azrtti_typeid<AnimGraphParameterAction>(), AZStd::nullopt, AZStd::nullopt, &commandGroup);
        CommandSystem::AddStateAction(m_stateA, azrtti_typeid<AnimGraphParameterAction>(), AZStd::nullopt, AZStd::nullopt, &commandGroup);
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        commandGroup.RemoveAllCommands();
        EXPECT_EQ(4, m_stateA->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly four state actions.";

        // 3. Undo add multiple state actions.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(1, m_stateA->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly one state action left.";
        EXPECT_EQ(serializedAfterAddSingle, SerializeAnimGraph());

        // 4. Undo add state action.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(0, m_stateA->GetTriggerActionSetup().GetNumActions())
            << "There should be no state action left anymore.";
        EXPECT_EQ(serializedOriginal, SerializeAnimGraph());
    }

    // TODO: There is a reflection issue with MCore::ReflectionSerializer::SerializeMembersExcept that is used
    // in the remove node command. AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> is not reflected.
    TEST_F(AnimGraphActionCommandsFixture, DISABLED_AnimGraphActionCommandTests_UndoRemoveStateWithAction)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;

        // 1. Add state action.
        CommandSystem::AddStateAction(m_stateA, azrtti_typeid<AnimGraphParameterAction>());
        const AZStd::string serializedAfterAddAction = SerializeAnimGraph();
        EXPECT_EQ(1, m_stateA->GetTriggerActionSetup().GetNumActions())
            << "There should be exactly one state action.";

        // 2. Remove the whole state including the action.
        CommandSystem::DeleteNodes(&commandGroup, m_motionNodeAnimGraph.get(), { m_stateA });
        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        commandGroup.RemoveAllCommands();
        EXPECT_EQ(nullptr, m_motionNodeAnimGraph->RecursiveFindNodeByName("A")) << "State A should be gone.";

        // 3. Undo remove state.
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(serializedAfterAddAction, SerializeAnimGraph());
    }
} // namespace EMotionFX
