/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class AnimGraphTransitionCommandsFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();

            /*
                +---+    +---+    +---+
                | A |    | B |    | C |
                +-+-+    +-+-+    +-+-+
                  ^        ^        ^
                  |        |        |
                  |    +---+---+    |
                  +----+ Start +----+
                       +-------+
            */
            AnimGraphNode* stateStart = aznew AnimGraphMotionNode();
            m_rootStateMachine->AddChildNode(stateStart);
            stateStart->SetName("Start");
            m_rootStateMachine->SetEntryState(stateStart);

            AnimGraphNode* stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            AnimGraphNode* stateB = m_motionNodeAnimGraph->GetMotionNodeB();
            AnimGraphNode* stateC = aznew AnimGraphMotionNode();
            stateC->SetName("C");
            m_rootStateMachine->AddChildNode(stateC);

            m_transitionLeft = AddTransition(stateStart, stateA, 1.0f);
            m_transitionLeft->SetCanBeInterrupted(true);

            m_transitionMiddle = AddTransition(stateStart, stateB, 1.0f);
            m_transitionMiddle->SetCanBeInterrupted(true);
            m_transitionMiddle->SetCanInterruptOtherTransitions(true);

            m_transitionRight = AddTransition(stateStart, stateC, 1.0f);
            m_transitionRight->SetCanInterruptOtherTransitions(true);

            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_motionNodeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        void TearDown() override
        {
            AnimGraphFixture::TearDown();
        }

        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphStateTransition* m_transitionLeft = nullptr;
        AnimGraphStateTransition* m_transitionMiddle = nullptr;
        AnimGraphStateTransition* m_transitionRight = nullptr;
    };


    TEST_F(AnimGraphTransitionCommandsFixture, RemoveTransitionPartOfCanBeInterruptedByTransitionIdsTest)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;
        const AZStd::vector<AZ::u64> onlyRightId = { m_transitionRight->GetId() };

        // 1. Adjust can be interrupted by transition ids for the left transition
        AZStd::vector<AZ::u64> canBeInterruptedByTransitionIds;
        canBeInterruptedByTransitionIds.emplace_back(m_transitionMiddle->GetId());
        canBeInterruptedByTransitionIds.emplace_back(m_transitionRight->GetId());

        // Serialize the attribute into a string so we can pass it as a command parameter.
        const AZStd::string attributesString = AZStd::string::format("-canBeInterruptedByTransitionIds {%s}",
            MCore::ReflectionSerializer::Serialize(&canBeInterruptedByTransitionIds).GetValue().c_str());

        // Construct the command and let it adjust the can be interrupted by transition id mask.
        CommandSystem::AdjustTransition(m_transitionLeft,
            /*isDisabled*/AZStd::nullopt, /*sourceNode*/AZStd::nullopt, /*targetNode*/AZStd::nullopt,
            /*startOffsetX*/AZStd::nullopt, /*startOffsetY*/AZStd::nullopt, /*endOffsetX*/AZStd::nullopt, /*endOffsetY*/AZStd::nullopt,
            attributesString, /*serializedMembers=*/AZStd::nullopt,
            &commandGroup);

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), canBeInterruptedByTransitionIds)
            << "The can be interrupted by transition ids list should contain both, the middle as well as the right transition ids";

        // 2. Remove the middle transition that was part of the can be interrupted by transition ids list
        //    and make sure it got removed from there.
        commandGroup.RemoveAllCommands();
        {
            AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
            CommandSystem::DeleteStateTransition(&commandGroup, m_transitionMiddle, transitionList);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), onlyRightId)
            << "The middle transition should be removed from the can be interrupted by list.";

        // 3. Same for the right transition
        commandGroup.RemoveAllCommands();
        {
            AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
            CommandSystem::DeleteStateTransition(&commandGroup, m_transitionRight, transitionList);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds().empty(), true)
            << "Both transitions should be removed from the can be interrupted by list.";

        // 4. Undo remove right transition
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), onlyRightId)
            << "The middle transition should be back in the can be interrupted by list again.";

        // 5. Undo remove middle transition
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), canBeInterruptedByTransitionIds)
            << "Both transitions should be back int the can be interrupted by list again.";

        // 6. Redo remove middle transition
        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), onlyRightId)
            << "The middle transition should be removed from the can be interrupted by list.";

        // 7. Redo remove middle transition
        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds().empty(), true)
            << "Both transitions should be removed from the can be interrupted by list.";
    }


    TEST_F(AnimGraphTransitionCommandsFixture, RecoveringCanBeInterruptedByTransitionIdsAfterRemoveTest)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;
        MCore::CommandGroup commandGroup;

        // 1. Adjust can be interrupted by transition ids for the left transition
        AZStd::vector<AZ::u64> canBeInterruptedByTransitionIds;
        canBeInterruptedByTransitionIds.emplace_back(m_transitionMiddle->GetId());
        canBeInterruptedByTransitionIds.emplace_back(m_transitionRight->GetId());

        // Serialize the attribute into a string so we can pass it as a command parameter.
        const AZStd::string attributesString = AZStd::string::format("-canBeInterruptedByTransitionIds {%s}",
            MCore::ReflectionSerializer::Serialize(&canBeInterruptedByTransitionIds).GetValue().c_str());

        // Construct the command and let it adjust the can be interrupted by transition id mask.
        CommandSystem::AdjustTransition(m_transitionLeft,
            /*isDisabled*/AZStd::nullopt, /*sourceNode*/AZStd::nullopt, /*targetNode*/AZStd::nullopt,
            /*startOffsetX*/AZStd::nullopt, /*startOffsetY*/AZStd::nullopt, /*endOffsetX*/AZStd::nullopt, /*endOffsetY*/AZStd::nullopt,
            attributesString, /*serializedMembers=*/AZStd::nullopt,
            &commandGroup);

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result))
            << "The can be interrupted by transition ids list should contain both, the middle as well as the right transition ids";

        // 2. Remove the transition that we just modified
        commandGroup.RemoveAllCommands();
        {
            AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
            CommandSystem::DeleteStateTransition(&commandGroup, m_transitionLeft, transitionList);
        }

        EXPECT_TRUE(commandManager.ExecuteCommandGroup(commandGroup, result));

        // 3. Undo remove transition
        EXPECT_TRUE(commandManager.Undo(result));

        // Point m_transitionLeft to the newly created transition after undo.
        const AnimGraphConnectionId connectionId = AnimGraphConnectionId::CreateFromString(result);
        m_transitionLeft = m_motionNodeAnimGraph->RecursiveFindTransitionById(connectionId);

        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), canBeInterruptedByTransitionIds)
            << "The transition should be back and the can be interrupted by transition ids contain the middle as well as the right transition ids.";

        // 4. Undo adjust can be interrupted by transition ids
        EXPECT_TRUE(commandManager.Undo(result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds().empty(), true)
            << "The can be interrupted by list should be back empty.";

        // 5. Redo adjust can be interrupted by transition ids
        EXPECT_TRUE(commandManager.Redo(result));
        EXPECT_EQ(m_transitionLeft->GetCanBeInterruptedByTransitionIds(), canBeInterruptedByTransitionIds)
            << "The can be interrupted by transition ids list should contain both, the middle as well as the right transition ids";

        // 6. Redo remove middle transition
        EXPECT_TRUE(commandManager.Redo(result));
    }

    TEST_F(AnimGraphTransitionCommandsFixture, CreateWildCardUndo)
    {
        AZStd::string result;
        CommandSystem::CommandManager commandManager;     

        size_t numTransitions = m_rootStateMachine->GetNumTransitions();
        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %d -sourceNode \"\" -targetNode \"A\" -sourcePort 0 -targetPort 0 -startOffsetX 0 -startOffsetY 0 -endOffsetX 0 -endOffsetY 0 -transitionType \"%s\"", 
            m_motionNodeAnimGraph->GetID(),
            azrtti_typeid<EMotionFX::AnimGraphStateTransition>().ToString<AZStd::string>().c_str());
        ASSERT_TRUE(commandManager.ExecuteCommand(commandString, result, /*addToHistory=*/true))
            << "The command execution failed";

        ASSERT_EQ(m_rootStateMachine->GetNumTransitions(), numTransitions + 1)
            << "The wildcard transition doesn't seem to be added.";

        ASSERT_TRUE(commandManager.Undo(result));
        ASSERT_EQ(m_rootStateMachine->GetNumTransitions(), numTransitions)
            << "Undo of the wild card creation seem to not have removed the transition.";

        ASSERT_TRUE(commandManager.Redo(result));
        ASSERT_EQ(m_rootStateMachine->GetNumTransitions(), numTransitions + 1)
            << "The wildcard transition doesn't seem to be added on Redo.";
    }
} // namespace EMotionFX
