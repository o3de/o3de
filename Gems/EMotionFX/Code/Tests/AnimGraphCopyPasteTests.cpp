/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphParameterAction.h>
#include <EMotionFX/Source/AnimGraphFollowerParameterAction.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphSymbolicFollowerParameterAction.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/AnimGraphFixture.h>


namespace EMotionFX
{
    class AnimGraphSimpleCopyPasteFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<bool>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            /*
                +---+            +---+
                | A |--Actions-->| B |
                +---+            +---+
                +---+
                | C |
                +---+
            */
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();

            m_stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            m_stateB = m_motionNodeAnimGraph->GetMotionNodeB();

            m_stateC = aznew AnimGraphStateMachine();
            m_stateC->SetName("C");
            m_rootStateMachine->AddChildNode(m_stateC);

            m_transition = AddTransition(m_stateA, m_stateB, 1.0f);
            m_motionNodeAnimGraph->InitAfterLoading();
        }

        bool CompareParameterAction(AnimGraphTriggerAction* actionA, AnimGraphTriggerAction* actionB)
        {
            AnimGraphParameterAction* paramterActionA = static_cast<AnimGraphParameterAction*>(actionA);
            AnimGraphParameterAction* paramterActionB = static_cast<AnimGraphParameterAction*>(actionB);
            return (paramterActionA->GetTriggerValue() == paramterActionB->GetTriggerValue())
                && (paramterActionA->GetParameterName() == paramterActionB->GetParameterName());
        }

        void TearDown() override
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Destroy();
                m_animGraphInstance = nullptr;
            }
            m_motionNodeAnimGraph.reset();

            AnimGraphFixture::TearDown();
        }

        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphNode* m_stateA = nullptr;
        AnimGraphNode* m_stateB = nullptr;
        AnimGraphStateMachine* m_stateC = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_P(AnimGraphSimpleCopyPasteFixture, AnimGraphCopyPasteTests_CopyAndPasteTransitionActions)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        // 1. Add transition actions.
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphParameterAction>());
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphFollowerParameterAction>());
        CommandSystem::AddTransitionAction(m_transition, azrtti_typeid<AnimGraphSymbolicFollowerParameterAction>());
        EXPECT_EQ(3, m_transition->GetTriggerActionSetup().GetNumActions()) << "There should be exactly three transition actions.";

        // 2. Cut & paste both states
        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);
        nodesToCopy.emplace_back(m_stateB);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(1, m_rootStateMachine->GetNumTransitions())
                << "As we only had one transition before the cut & paste operation, there should be exactly one now, too.";

            AnimGraphStateTransition* newTransition = m_rootStateMachine->GetTransition(0);
            const TriggerActionSetup& actionSetup = newTransition->GetTriggerActionSetup();
            EXPECT_EQ(3, actionSetup.GetNumActions()) << "There should be three transition actions again.";
        }
        else
        {
            const size_t numTransitions = m_rootStateMachine->GetNumTransitions();
            EXPECT_EQ(2, numTransitions)
                << "After copy & paste, there should be two transitions.";

            for (size_t i = 0; i < numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = m_rootStateMachine->GetTransition(i);
                const TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();
                EXPECT_EQ(3, actionSetup.GetNumActions()) << "There should be three transition actions for both transitions.";
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    class AnimGraphTransitionConditionCopyPasteFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<bool>
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

        AZStd::vector<AZ::TypeId> GetConditionTypeIds() const
        {
            AZStd::vector<AZ::TypeId> result;
            AnimGraphObjectFactory objectFactory;
            const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = objectFactory.GetUiObjectPrototypes();
            for (EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
            {
                EMotionFX::AnimGraphTransitionCondition* conditionPrototype = azdynamic_cast<EMotionFX::AnimGraphTransitionCondition*>(objectPrototype);
                if (conditionPrototype &&
                    (azrtti_typeid<>(conditionPrototype) != azrtti_typeid<AnimGraphTransitionCondition>()))
                {
                    AZ::TypeId type = azrtti_typeid<>(conditionPrototype);
                    AZ::Debug::Platform::OutputToDebugger({}, AZStd::string::format("Condition: Name=%s, Type=%s\n", conditionPrototype->GetPaletteName(), type.ToString<AZStd::string>().c_str()).c_str());
                    result.emplace_back(azrtti_typeid<>(conditionPrototype));
                }
            }
            return result;
        }

        void VerifyTransition(AnimGraphStateTransition* transition)
        {
            const AZStd::vector<AZ::TypeId> conditionTypeIds = GetConditionTypeIds();
            const size_t numConditionTypes = conditionTypeIds.size();
            const size_t numConditions = transition->GetNumConditions();
            EXPECT_EQ(numConditions, numConditionTypes) << "We should have a condition for each prototype type.";

            for (size_t i = 0; i < numConditions; ++i)
            {
                const EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);
                EXPECT_EQ(azrtti_typeid<>(condition), conditionTypeIds[i])
                    << "The conditions on the transition should have the same order as the prototypes.";
            }
        }

        void VerifyAfterOperation()
        {
            const AZStd::vector<AZ::TypeId> conditionTypeIds = GetConditionTypeIds();
            const bool cutMode = GetParam();
            if (cutMode)
            {
                EXPECT_EQ(m_rootStateMachine->GetNumTransitions(), 1)
                    << "As we only had one transition before the cut & paste operation, there should be exactly one now, too.";

                AnimGraphStateTransition* newTransition = m_rootStateMachine->GetTransition(0);
                VerifyTransition(newTransition);
            }
            else
            {
                const size_t numTransitions = m_rootStateMachine->GetNumTransitions();
                EXPECT_EQ(2, numTransitions)
                    << "After copy & paste, there should be two transitions.";

                for (size_t i = 0; i < numTransitions; ++i)
                {
                    AnimGraphStateTransition* transition = m_rootStateMachine->GetTransition(i);
                    VerifyTransition(transition);
                }
            }
        }

        void TearDown() override
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Destroy();
                m_animGraphInstance = nullptr;
            }
            m_motionNodeAnimGraph.reset();

            AnimGraphFixture::TearDown();
        }


        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphNode* m_stateA = nullptr;
        AnimGraphNode* m_stateB = nullptr;
        AnimGraphStateTransition* m_transition = nullptr;
    };

    TEST_P(AnimGraphTransitionConditionCopyPasteFixture, AnimGraphCopyPasteTests_CopyAndPasteTransitionConditions)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        // 1. Add transition conditions.
        const AZStd::vector<AZ::TypeId> conditionTypeIds = GetConditionTypeIds();
        EXPECT_TRUE(conditionTypeIds.size() > 0) << "There are no transition conditions registered in the object factory.";

        for (const AZ::TypeId& conditionTypeId : conditionTypeIds)
        {
            CommandSystem::CommandAddTransitionCondition* addConditionCommand = aznew CommandSystem::CommandAddTransitionCondition(
                m_motionNodeAnimGraph->GetID(), m_transition->GetId(), conditionTypeId);
            EXPECT_TRUE(commandManager.ExecuteCommand(addConditionCommand, result)) << result.c_str();
        }
        VerifyTransition(m_transition);

        // 2. Copy/cut & paste both states
        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);
        nodesToCopy.emplace_back(m_stateB);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));
        VerifyAfterOperation();

        // 3. Undo.
        EXPECT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_EQ(m_rootStateMachine->GetNumTransitions(), 1) << "We should be back at only the original transition again.";
        VerifyTransition(m_rootStateMachine->GetTransition(0));

        // 4. Redo.
        EXPECT_TRUE(commandManager.Redo(result)) << result.c_str();
        VerifyAfterOperation();
    }

    INSTANTIATE_TEST_CASE_P(AnimGraphCopyPasteTests,
        AnimGraphTransitionConditionCopyPasteFixture,
        ::testing::Bool());

    ///////////////////////////////////////////////////////////////////////////

    TEST_P(AnimGraphSimpleCopyPasteFixture, AnimGraphCopyPasteTests_TransitionIds)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();
        const AnimGraphConnectionId oldtransitionId = m_transition->GetId();

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);
        nodesToCopy.emplace_back(m_stateB);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(1, m_rootStateMachine->GetNumTransitions())
                << "As we only had one transition before the cut & paste operation, there should be exactly one now, too.";

            AnimGraphStateTransition* newTransition = m_rootStateMachine->GetTransition(0);
            EXPECT_TRUE(newTransition->GetId() == oldtransitionId) << "There cut & pasted transition should have the same id.";
        }
        else
        {
            const size_t numTransitions = m_rootStateMachine->GetNumTransitions();
            EXPECT_EQ(2, numTransitions)
                << "After copy & paste, there should be two transitions.";

            for (size_t i = 0; i < numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = m_rootStateMachine->GetTransition(i);

                if (transition == m_transition)
                {
                    continue;
                }

                EXPECT_FALSE(transition->GetId() == oldtransitionId) << "There copied transition should have another id. Transition ids need to be unique.";
            }
        }
    }

    TEST_P(AnimGraphSimpleCopyPasteFixture, AnimGraphCopyPasteTests_CopyAndPasteToAStateMachine)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        // 1. Copy the nodeA and nodeB to nodeC(statemachine) 
        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);
        nodesToCopy.emplace_back(m_stateB);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_stateC,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(1, m_rootStateMachine->GetNumChildNodes());
            EXPECT_EQ(0, m_rootStateMachine->GetNumTransitions());

            EXPECT_EQ(2, m_stateC->GetNumChildNodes());
            EXPECT_EQ(1, m_stateC->GetNumTransitions());

            EXPECT_EQ("A", m_stateC->GetChildNode(0)->GetNameString());
            EXPECT_EQ("B", m_stateC->GetChildNode(1)->GetNameString());
        }
        else
        {
            EXPECT_EQ(3, m_rootStateMachine->GetNumChildNodes());
            EXPECT_EQ(1, m_rootStateMachine->GetNumTransitions());

            EXPECT_EQ(2, m_stateC->GetNumChildNodes());
            EXPECT_EQ(1, m_stateC->GetNumTransitions());
        }

        /*      After 1. Cut == true
                +--------------------------+
                | C                        |
                |  +---+            +---+  |
                |  | A2|--Actions-->| B2|  |
                |  +---+            +---+  |
                |                          |
                +--------------------------+
        */

        /*      After 1. Cut == false
                +---+            +---+
                | A |--Actions-->| B |
                +---+            +---+
                +--------------------------+
                | C                        |
                |  +---+            +---+  |
                |  | A2|--Actions-->| B2|  |
                |  +---+            +---+  |
                |                          | 
                +--------------------------+
        */

        // 2. Copy and paste the nodeC(state machine).
        commandGroup.Clear();
        nodesToCopy.clear();
        nodesToCopy.emplace_back(m_stateC);

        CommandSystem::AnimGraphCopyPasteData copyPasteData2;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/false,
            copyPasteData2,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(2, m_rootStateMachine->GetNumChildNodes());
            EXPECT_EQ(0, m_rootStateMachine->GetNumTransitions());
            EXPECT_EQ(6, m_rootStateMachine->RecursiveCalcNumNodes());
        }
        else
        {
            EXPECT_EQ(4, m_rootStateMachine->GetNumChildNodes());
            EXPECT_EQ(1, m_rootStateMachine->GetNumTransitions());
            EXPECT_EQ(8, m_rootStateMachine->RecursiveCalcNumNodes());
        }
    }

    TEST_P(AnimGraphSimpleCopyPasteFixture, AnimGraphCopyPasteTests_TriggerActions)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        // Add transition actions to the node.
        AnimGraphParameterAction* action1 = aznew AnimGraphParameterAction();
        action1->SetParameterName("action1Param");
        action1->SetTriggerValue(5.8f);

        AnimGraphParameterAction* action2 = aznew AnimGraphParameterAction();
        action2->SetParameterName("action2Param");
        action2->SetTriggerValue(8.5f);

        TriggerActionSetup& actionSetup = m_stateA->GetTriggerActionSetup();
        actionSetup.AddAction(action1);
        actionSetup.AddAction(action2);
        m_stateA->InitTriggerActions();

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        nodesToCopy.emplace_back(m_stateA);

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        if (cutMode)
        {
            EXPECT_EQ(3, m_rootStateMachine->GetNumChildNodes()) << "After the cut + copy, the total node number remain the same.";

            AnimGraphNode* copiedNode = m_rootStateMachine->GetChildNode(2);

            const TriggerActionSetup& actionSetup2 = copiedNode->GetTriggerActionSetup();
            EXPECT_EQ(2, actionSetup2.GetNumActions());
            AnimGraphParameterAction* action1_1 = static_cast<AnimGraphParameterAction*>(actionSetup2.GetAction(0));
            AnimGraphParameterAction* action2_1 = static_cast<AnimGraphParameterAction*>(actionSetup2.GetAction(1));

            EXPECT_TRUE(action1_1 && action2_1) << "After cut/copy, the new action setup should has two parameter actions.";
            EXPECT_TRUE(action1_1->GetParameterName() == "action1Param");
            EXPECT_EQ(action1_1->GetTriggerValue(), 5.8f);
            EXPECT_TRUE(action2_1->GetParameterName() == "action2Param");
            EXPECT_EQ(action2_1->GetTriggerValue(), 8.5f);
        }
        else
        {
            EXPECT_EQ(4, m_rootStateMachine->GetNumChildNodes()) << "After the copy, the total node number should increase by one.";

            AnimGraphNode* copiedNode = m_rootStateMachine->GetChildNode(3);
            EXPECT_NE(copiedNode, m_stateA) << "Make sure the fourth node is the newly copied node, not the original node.";

            const TriggerActionSetup& actionSetup2 = copiedNode->GetTriggerActionSetup();
            EXPECT_EQ(2, actionSetup2.GetNumActions());
            EXPECT_TRUE(CompareParameterAction(actionSetup2.GetAction(0), action1)) << "After copy, the action should be the same as the original node.";
            EXPECT_TRUE(CompareParameterAction(actionSetup2.GetAction(1), action2)) << "After copy, the action should be the same as the original node.";
        }
    }

    INSTANTIATE_TEST_CASE_P(AnimGraphCopyPasteTests,
        AnimGraphSimpleCopyPasteFixture,
        ::testing::Bool());

    ///////////////////////////////////////////////////////////////////////////

    class AnimGraphCopyPasteFixture_CanBeInterruptedBy
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<bool>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            /*
                +---+     +---+
                | A |---->| B |
                +---+     +---+
                  |
                  v
                +---+
                | C |
                +---+
            */
            m_motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_rootStateMachine = m_motionNodeAnimGraph->GetRootStateMachine();

            m_stateA = m_motionNodeAnimGraph->GetMotionNodeA();
            m_stateB = m_motionNodeAnimGraph->GetMotionNodeB();

            m_stateC = aznew AnimGraphMotionNode();
            m_stateC->SetName("C");
            m_rootStateMachine->AddChildNode(m_stateC);
            
            m_transitionAB = AddTransition(m_stateA, m_stateB, 1.0f);
            m_transitionAC = AddTransition(m_stateA, m_stateC, 1.0f);

            AZStd::vector<AnimGraphConnectionId> canBeInterruptedBy = { m_transitionAC->GetId() };
            m_transitionAB->SetCanBeInterruptedBy(canBeInterruptedBy);
            m_motionNodeAnimGraph->InitAfterLoading();
        }

        void TearDown() override
        {
            if (m_animGraphInstance)
            {
                m_animGraphInstance->Destroy();
                m_animGraphInstance = nullptr;
            }
            m_motionNodeAnimGraph.reset();

            AnimGraphFixture::TearDown();
        }

    public:
        AZStd::unique_ptr<TwoMotionNodeAnimGraph> m_motionNodeAnimGraph;
        AnimGraphNode* m_stateA = nullptr;
        AnimGraphNode* m_stateB = nullptr;
        AnimGraphNode* m_stateC = nullptr;
        AnimGraphStateTransition* m_transitionAB = nullptr;
        AnimGraphStateTransition* m_transitionAC = nullptr;
    };

    TEST_P(AnimGraphCopyPasteFixture_CanBeInterruptedBy, CopyCanBeInterruptedWithTransitionIds)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy = { m_stateA, m_stateB, m_stateC };

        CommandSystem::AnimGraphCopyPasteData copyPasteData;
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_rootStateMachine,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        // Check if the can be interrupted by other transition ids are valid.
        size_t numTransitionsChecked = 0;
        const size_t numTransitions = m_rootStateMachine->GetNumTransitions();
        for (size_t i = 0; i < numTransitions; ++i)
        {
            const AnimGraphStateTransition* transition = m_rootStateMachine->GetTransition(i);
            const AZStd::vector<AZ::u64>& canBeInterruptedByTransitionIds = transition->GetCanBeInterruptedByTransitionIds();
            if (!canBeInterruptedByTransitionIds.empty())
            {
                for (AZ::u64 interruptionCandidateTransitionId : canBeInterruptedByTransitionIds)
                {
                    const AnimGraphStateTransition* interruptionCandidate = m_rootStateMachine->FindTransitionById(interruptionCandidateTransitionId);
                    EXPECT_TRUE(interruptionCandidate != nullptr) <<
                        "In case the interruption transition candidate cannot be found something is wrong with the transition id relinking when copy/cut & pasting.";

                    if (interruptionCandidate)
                    {
                        EXPECT_FALSE(interruptionCandidate == transition) <<
                            "The interruption candidate cannot be the interruption itself. Something went wrong with the transition id relinking.";

                        EXPECT_TRUE((transition->GetSourceNode() == interruptionCandidate->GetSourceNode()) || transition->GetIsWildcardTransition() || interruptionCandidate->GetIsWildcardTransition()) <<
                            "The source nodes of the transition and the interruption candidate have to be the same, unless either of them is a wildcard.";
                    }
                }

                numTransitionsChecked++;
            }
        }

        if (cutMode)
        {
            EXPECT_EQ(2, numTransitions) << "There should be exactly the same amount of transitions as before the operation.";
            EXPECT_EQ(1, numTransitionsChecked) << "Only one transition should hold interruption candidates.";
        }
        else
        {
            EXPECT_EQ(4, numTransitions) << "After copy & paste, there should be four transitions.";
            EXPECT_EQ(2, numTransitionsChecked) << "Two transitions should hold interruption candidates.";
        }
    }

    INSTANTIATE_TEST_CASE_P(CopyPasteTests,
        AnimGraphCopyPasteFixture_CanBeInterruptedBy,
        ::testing::Bool());

    ///////////////////////////////////////////////////////////////////////////
    
    class AnimGraphCopyPasteFixture_NodeTriggerValue
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<bool>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();
            m_blendTree->SetName("TestBlendTree");
            /*
                +---------+
                |bindPoseA|----+
                +---------+    |    +------+       +-----+
                               +--->|blendN|------>|final|
                               +--->|      |       +-----+
                +---------+    |    +------+
                |bindPoseB|----+
                +---------+
                +------------+
                |TestBindPose|
                +------------+
            */

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_bindPoseNodeA = aznew AnimGraphBindPoseNode();
            m_bindPoseNodeB = aznew AnimGraphBindPoseNode();
            m_testBindPoseNode = aznew AnimGraphBindPoseNode();
            m_testConnection = AZStd::make_unique<EMotionFX::BlendTreeConnection>(m_testBindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlendNNode::PORTID_INPUT_POSE_2);
            m_blendNNode = aznew BlendTreeBlendNNode();

            m_bindPoseNodeA->SetName("A");
            m_bindPoseNodeB->SetName("B");
            m_testBindPoseNode->SetName("TestBindPoseNode");
            m_blendNNode->SetName("C");
            finalNode->SetName("D");

            m_blendTree->AddChildNode(m_bindPoseNodeA);
            m_blendTree->AddChildNode(m_bindPoseNodeB);
            m_blendTree->AddChildNode(m_testBindPoseNode);
            m_blendTree->AddChildNode(m_blendNNode);
            m_blendTree->AddChildNode(finalNode);

            m_blendNNode->AddConnection(m_bindPoseNodeA, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlendNNode::PORTID_INPUT_POSE_0);
            m_blendNNode->AddConnection(m_bindPoseNodeB, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlendNNode::PORTID_INPUT_POSE_1);
            finalNode->AddConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendNNode->UpdateParamWeights();
            m_blendNNode->SetParamWeightsEquallyDistributed(-1.0f, 1.0f);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    public:
        AZStd::unique_ptr<EMotionFX::BlendTreeConnection> m_testConnection;
        AnimGraphBindPoseNode* m_bindPoseNodeA = nullptr;
        AnimGraphBindPoseNode* m_bindPoseNodeB = nullptr;
        AnimGraphBindPoseNode* m_testBindPoseNode = nullptr;
        BlendTreeBlendNNode* m_blendNNode = nullptr;
        BlendTree* m_blendTree = nullptr;
    };

    TEST_P(AnimGraphCopyPasteFixture_NodeTriggerValue, PastedNodesHaveSameTriggerValue)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        MCore::CommandGroup commandGroup;
        const bool cutMode = GetParam();

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy = { m_bindPoseNodeA, m_bindPoseNodeB, m_blendNNode };
        CommandSystem::AnimGraphCopyPasteData copyPasteData;

        // Copy and paste m_bindPoseNodeA, m_bindPoseNodeB, m_blendNNode in the blend tree.
        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup,
            /*targetParentNode=*/m_blendTree,
            nodesToCopy,
            /*posX=*/0,
            /*posY=*/0,
            /*cutMode=*/cutMode,
            copyPasteData,
            /*ignoreTopLevelConnections=*/false);
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));

        const AZStd::vector<EMotionFX::BlendNParamWeight> &originalNodeParamWeights = m_blendNNode->GetParamWeights();
        const size_t numParamWeights = originalNodeParamWeights.size();

        if (cutMode)
        {
            // Node 0(bindPoseA), 1(bindPoseB), 3(blendN), 4(final) are the existing nodes.
            EXPECT_EQ(5, m_blendTree->GetNumChildNodes()) << "After cut and paste, the total node number of nodes in the blend tree should stay the same.";

            // After cut and paste, node 0, 1, 2 should be placed in index 2, 3, 4.
            // Node 3(test bind pose node) is now placed in index 0.
            EXPECT_TRUE(strcmp(m_blendTree->GetChildNode(0)->GetName(), "TestBindPoseNode") == 0) << "Test bind pose node should now place in index 0.";

            BlendTreeBlendNNode* cutBlendNNode = azrtti_cast<BlendTreeBlendNNode*>(m_blendTree->GetChildNode(4));

            // Check the ports are properly connected among the pasted nodes.
            EXPECT_TRUE(cutBlendNNode->CheckIfIsInputPortConnected(BlendTreeBlendNNode::INPUTPORT_POSE_0));
            EXPECT_TRUE(cutBlendNNode->CheckIfIsInputPortConnected(BlendTreeBlendNNode::INPUTPORT_POSE_1));

            const AZStd::vector<EMotionFX::BlendNParamWeight> &cutNodeParamWeights = cutBlendNNode->GetParamWeights();
            
            EXPECT_TRUE(cutNodeParamWeights.size() == numParamWeights) << "Number of cut and pasted parameter weights should be the same as the number of original parameter weights.";
            for (uint32 i = 0; i < numParamWeights; i++)
            {
                EXPECT_TRUE(originalNodeParamWeights[i].GetWeightRange() == cutNodeParamWeights[i].GetWeightRange()) <<
                    "Parameter weights in the cut and pasted blend n node should be equal to the parameter weights in original blend n node.";
            }
        }
        else
        {
            EXPECT_EQ(8, m_blendTree->GetNumChildNodes()) << "After copy and paste, the total node number of nodes in the blend tree should increase by 3.";

            BlendTreeBlendNNode* copiedBlendNNode = azrtti_cast<BlendTreeBlendNNode*> (m_blendTree->GetChildNode(7));

            EXPECT_TRUE(copiedBlendNNode->CheckIfIsInputPortConnected(BlendTreeBlendNNode::INPUTPORT_POSE_0));
            EXPECT_TRUE(copiedBlendNNode->CheckIfIsInputPortConnected(BlendTreeBlendNNode::INPUTPORT_POSE_1));

            const AZStd::vector<EMotionFX::BlendNParamWeight> &copiedNodeParamWeights = copiedBlendNNode->GetParamWeights();

            EXPECT_TRUE(copiedNodeParamWeights.size() == numParamWeights) << "Number of copied parameter weights should be the same as the number of original parameter weights.";
            for (uint32 i = 0; i < numParamWeights; i++)
            {
                EXPECT_TRUE(originalNodeParamWeights[i].GetWeightRange() == copiedNodeParamWeights[i].GetWeightRange()) <<
                    "Parameter weights in copied blend n node should be equal to the parameter weights in original blend n node.";
            }
        }

        BlendTreeBlendNNode* testBlendNNode = azrtti_cast<BlendTreeBlendNNode*> (m_blendTree->GetChildNode(cutMode ? 4:7));
        const AZStd::vector<EMotionFX::BlendNParamWeight> &testNodeParamWeights = testBlendNNode->GetParamWeights();

        commandGroup.Clear();
        CommandSystem::CreateNodeConnection(&commandGroup, testBlendNNode, m_testConnection.get());
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result));
        EXPECT_TRUE(testBlendNNode->HasConnectionAtInputPort(BlendTreeBlendNNode::PORTID_INPUT_POSE_2)) <<
            "New connection should be created.";

        for (uint32 i = 0; i < m_blendNNode->GetParamWeights().size(); i++)
        {
            EXPECT_TRUE(originalNodeParamWeights[i].GetWeightRange() == testNodeParamWeights[i].GetWeightRange()) <<
                "Existing parameter weights should not be affected by adding a new connection.";
        }

        // Adding a new connection will call update param for blend N node,
        // the new connection will have the same value as its previous connection.
        EXPECT_TRUE(testNodeParamWeights[2].GetWeightRange() == 1) <<
            "New connection's parameter weight should be the weight value of 1.";
    }

    INSTANTIATE_TEST_CASE_P(AnimGraphCopyPasteTests,
        AnimGraphCopyPasteFixture_NodeTriggerValue,
        ::testing::Bool());
} // namespace EMotionFX
