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

#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphEntryNode.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{

    class CanDeleteExitNodeAfterItHasBeenActiveFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            m_entryNode = aznew AnimGraphEntryNode();
            m_entryNode->SetName("EntryNode");

            m_exitNode = aznew AnimGraphExitNode();
            m_exitNode->SetName("ExitNode");

            m_stateMachine = aznew AnimGraphStateMachine();
            m_stateMachine->SetName("StateMachine");
            m_stateMachine->AddChildNode(m_entryNode);
            m_stateMachine->AddChildNode(m_exitNode);
            m_stateMachine->SetEntryState(m_entryNode);

            m_motionNode = aznew AnimGraphMotionNode();
            m_motionNode->SetName("MotionNode");

            m_rootStateMachine->AddChildNode(m_motionNode);
            m_rootStateMachine->AddChildNode(m_stateMachine);
            m_rootStateMachine->SetEntryState(m_stateMachine);

            AddTransitionWithTimeCondition(m_entryNode, m_exitNode, 0.0f, 1.0f);
            AddTransitionWithTimeCondition(m_motionNode, m_stateMachine, 0.0f, 1.0f);
            AddTransitionWithTimeCondition(m_stateMachine, m_motionNode, 0.0f, 1.0f);
        }

        static void ExpectNodeRefCountsAreZero(const AZStd::vector<const AnimGraphNodeData*>& nodeDatas, int lineNumber)
        {
            for (const AnimGraphNodeData* nodeData : nodeDatas)
            {
                EXPECT_EQ(nodeData->GetRefDataRefCount(), 0u) << "for node " << nodeData->GetNode()->GetName() << ", line " << lineNumber;
                EXPECT_EQ(nodeData->GetPoseRefCount(), 0u) << "for node " << nodeData->GetNode()->GetName() << ", line " << lineNumber;
            }
        }

        AnimGraphStateMachine* m_stateMachine;
        AnimGraphMotionNode* m_motionNode;
        AnimGraphEntryNode* m_entryNode;
        AnimGraphExitNode* m_exitNode;
    };

    TEST_F(CanDeleteExitNodeAfterItHasBeenActiveFixture, CanDeleteExitNodeAfterItHasBeenActive)
    {
        RecordProperty("test_case_id", "C15441569");

        CommandSystem::CommandManager manager;

        AZStd::vector<const AnimGraphNodeData*> nodeDatas{
            m_animGraphInstance->FindOrCreateUniqueNodeData(m_stateMachine),
            m_animGraphInstance->FindOrCreateUniqueNodeData(m_motionNode),
            m_animGraphInstance->FindOrCreateUniqueNodeData(m_entryNode),
            m_animGraphInstance->FindOrCreateUniqueNodeData(m_exitNode),
        };

        GetEMotionFX().Update(0.0f);

        EXPECT_THAT(
            m_rootStateMachine->GetActiveStates(m_animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), {m_stateMachine})
        );
        EXPECT_THAT(
            m_stateMachine->GetActiveStates(m_animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), {m_entryNode})
        );
        ExpectNodeRefCountsAreZero(nodeDatas, __LINE__);

        GetEMotionFX().Update(1.0f);

        // After 1 second, the root state machine should have transitioned
        // completely from the child state machine to the motion node. The
        // child state machine should be completely in the exit state.
        EXPECT_THAT(
            m_rootStateMachine->GetActiveStates(m_animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), {m_motionNode})
        );
        EXPECT_THAT(
            m_stateMachine->GetActiveStates(m_animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), {m_exitNode})
        );
        ExpectNodeRefCountsAreZero(nodeDatas, __LINE__);

        nodeDatas.erase(AZStd::find(nodeDatas.begin(), nodeDatas.end(), m_animGraphInstance->FindOrCreateUniqueNodeData(m_exitNode)));
        MCore::CommandGroup group;
        CommandSystem::DeleteNodes(&group, m_animGraph.get(), {m_exitNode}, true);
        AZStd::string result;
        ASSERT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, result, false)) << result.c_str();

        GetEMotionFX().Update(1.0f);

        // After 2 seconds, the root state machine should be back at the child
        // state machine, and the child state machine back at its entry state.
        EXPECT_THAT(
            m_rootStateMachine->GetActiveStates(m_animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), {m_stateMachine})
        );
        EXPECT_THAT(
            m_stateMachine->GetActiveStates(m_animGraphInstance),
            ::testing::Pointwise(::testing::Eq(), {m_entryNode})
        );
        ExpectNodeRefCountsAreZero(nodeDatas, __LINE__);
    }
} // namespace EMotionFX
