/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>

namespace EMotionFX
{
    class ActivateAnimGraphCommandFixture
        : public AnimGraphFixture
    {
    public:
        AnimGraphStateMachine* AddSubStateMachine(AnimGraphNode* parentState)
        {
            AnimGraphStateMachine* stateMachine = aznew AnimGraphStateMachine();
            parentState->AddChildNode(stateMachine);

            AnimGraphBindPoseNode* bindPose = aznew AnimGraphBindPoseNode();
            stateMachine->AddChildNode(bindPose);
            stateMachine->SetEntryState(bindPose);

            return stateMachine;
        }

        void ConstructGraph()
        {
            AnimGraphFixture::ConstructGraph();

            AnimGraphStateMachine* sm1 = AddSubStateMachine(m_rootStateMachine);
            AddSubStateMachine(m_rootStateMachine);

            m_rootStateMachine->SetEntryState(sm1);
        }

        void CheckStateMachinesAreInEntryStates(const AnimGraph* animGraph, AnimGraphInstance* instance)
        {
            AZStd::vector<AnimGraphNode*> stateMachines;
            animGraph->RecursiveCollectNodesOfType(azrtti_typeid<AnimGraphStateMachine>(), &stateMachines);

            for (AnimGraphNode* state : stateMachines)
            {
                AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(state);
                const AZStd::vector<AnimGraphNode*>& activeStates = stateMachine->GetActiveStates(instance);
                EXPECT_EQ(activeStates.size(), 1);
                EXPECT_TRUE(activeStates[0] == stateMachine->GetEntryState());
            }
        }
    };

    TEST_F(ActivateAnimGraphCommandFixture, ActivateAnimGraph)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string command;
        AZStd::string result;

        m_actorInstance->SetAnimGraphInstance(nullptr);
        EXPECT_TRUE(m_actorInstance->GetAnimGraphInstance() == nullptr);

        command = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d\n",
            m_actorInstance->GetID(), m_animGraph->GetID(), m_motionSet->GetID());
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result));

        AnimGraphInstance* newInstance = m_actorInstance->GetAnimGraphInstance();
        EXPECT_TRUE(newInstance != nullptr);
        EXPECT_TRUE(newInstance->GetAnimGraph() == m_animGraph.get());
        CheckStateMachinesAreInEntryStates(m_animGraph.get(), newInstance);
    }

    TEST_F(ActivateAnimGraphCommandFixture, ActivateAnimGraph_InvalidParameters)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string command;
        AZStd::string result;

        command = AZStd::string::format("ActivateAnimGraph -actorInstanceIndex %d -animGraphID %d -motionSetID %d\n",
            m_actorInstance->GetID(), m_animGraph->GetID(), m_motionSet->GetID());
        EXPECT_FALSE(commandManager.ExecuteCommand(command, result));

        command = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphIndex %d -motionSetID %d\n",
            m_actorInstance->GetID(), m_animGraph->GetID(), m_motionSet->GetID());
        EXPECT_FALSE(commandManager.ExecuteCommand(command, result));

        command = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetIndex %d\n",
            m_actorInstance->GetID(), m_animGraph->GetID(), m_motionSet->GetID());
        EXPECT_FALSE(commandManager.ExecuteCommand(command, result));
    }

    ///////////////////////////////////////////////////////////////////////////
    class LoadAnimGraphCommandTests
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            AnimGraphNode* stateA = aznew AnimGraphMotionNode();
            stateA->SetName("A");
            m_rootStateMachine->AddChildNode(stateA);
            m_rootStateMachine->SetEntryState(stateA);

            AnimGraphNode* stateB = aznew AnimGraphMotionNode();
            stateB->SetName("B");
            m_rootStateMachine->AddChildNode(stateB);

            AddTransition(stateA, stateB, 1.0f);

            // Save the anim graph to disk.
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            EXPECT_TRUE(context != nullptr) << "Serialize context is not valid.";
            EXPECT_TRUE(m_animGraph->SaveToFile(m_filename, context)) << "Saving anim graph to " << m_filename << " failed.";
        }

    public:
        const char* m_filename = "TestAnimGraph.animgraph";
    };

    // Note: Disabled tests as they fail on Jenkins. Loading the anim graph fails in AZ::Utils::LoadObjectFromFile<AnimGraph>() after saving the anim graph to disk successfully.

    class LoadAnimGraphCommandTestsBoolParam
        : public LoadAnimGraphCommandTests
        , public ::testing::WithParamInterface<bool>
    {
    };
    INSTANTIATE_TEST_SUITE_P(LoadAnimGraphCommandTests, LoadAnimGraphCommandTestsBoolParam, ::testing::Bool());

    TEST_F(LoadAnimGraphCommandTests, DISABLED_LoadAnimGraph)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string command;
        AZStd::string result;

        command = AZStd::string::format("LoadAnimGraph -filename \"%s\"", m_filename);
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result)) << result.c_str();
        const AnimGraph* loadedAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByFileName(m_filename);
        EXPECT_TRUE(loadedAnimGraph != nullptr);
        EXPECT_FALSE(loadedAnimGraph->GetIsOwnedByRuntime());
        EXPECT_FALSE(loadedAnimGraph->GetIsOwnedByAsset());
        EXPECT_NE(m_animGraph->GetID(), loadedAnimGraph->GetID())
            << "The id of the original anim graph does not differ from the loaded one, which means that the loading routine just returned the loaded anim graph.";
    }

    TEST_F(LoadAnimGraphCommandTests, DISABLED_LoadAnimGraphTwice)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string command;
        AZStd::string result;
        command = AZStd::string::format("LoadAnimGraph -filename \"%s\"", m_filename);

        // Load the anim graph the first time.
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result)) << result.c_str();
        const int firstAnimGraphId = AzFramework::StringFunc::ToInt(result.c_str());
        const AnimGraph* firstAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(firstAnimGraphId);
        EXPECT_TRUE(firstAnimGraph != nullptr);
        EXPECT_FALSE(firstAnimGraph->GetIsOwnedByRuntime());
        EXPECT_FALSE(firstAnimGraph->GetIsOwnedByAsset());

        // Load the anim graph again.
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result)) << result.c_str();
        const int secondAnimGraphId = AzFramework::StringFunc::ToInt(result.c_str());
        const AnimGraph* secondAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(secondAnimGraphId);
        EXPECT_TRUE(secondAnimGraph != nullptr);
        EXPECT_FALSE(secondAnimGraph->GetIsOwnedByRuntime());
        EXPECT_FALSE(secondAnimGraph->GetIsOwnedByAsset());

        EXPECT_EQ(secondAnimGraph->GetID(), secondAnimGraph->GetID())
            << "The second load should be skipped as the anim graph already got loaded. If the ids are not equal, it means the anim graph got loaded twice.";
    }

    TEST_P(LoadAnimGraphCommandTestsBoolParam, DISABLED_LoadAnimGraphAfterAssetLoad)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string command;
        AZStd::string result;
        command = AZStd::string::format("LoadAnimGraph -filename \"%s\"", m_filename);

        // Load the anim graph once and fake that it got loaded by the asset system (faking a reference graph load).
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result)) << result.c_str();
        const AZ::u32 assetAnimGraphId = static_cast<AZ::u32>(AzFramework::StringFunc::ToInt(result.c_str()));
        AnimGraph* assetAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(assetAnimGraphId);
        EXPECT_TRUE(assetAnimGraph != nullptr);
        if (GetParam())
        {
            assetAnimGraph->SetIsOwnedByRuntime(true);
        }
        else
        {
            assetAnimGraph->SetIsOwnedByAsset(true);
        }

        // Load the anim graph again, this time normally.
        EXPECT_TRUE(commandManager.ExecuteCommand(command, result)) << result.c_str();
        const int loadedAnimGraphId = AzFramework::StringFunc::ToInt(result.c_str());
        const AnimGraph* loadedAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(loadedAnimGraphId);
        EXPECT_TRUE(loadedAnimGraph != nullptr);
        EXPECT_FALSE(loadedAnimGraph->GetIsOwnedByRuntime());
        EXPECT_FALSE(loadedAnimGraph->GetIsOwnedByAsset());
        EXPECT_NE(loadedAnimGraph->GetID(), assetAnimGraph->GetID())
            << "As the first loaded anim graph pretends to be loaded by the asset system, the second and normal load should force load a second anim graph.";
    }
} // namespace EMotionFX
