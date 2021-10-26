/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <MCore/Source/AttributeFloat.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class DeferredInitBasicFixture
        : public AnimGraphFixture
    {
    public:
        void SetUp()
        {
            AnimGraphFixture::SetUp();

            // Add value parameter after m_animGraphInstance is created.
            AddValueParameter(azrtti_typeid<FloatSliderParameter>(), "weightParam");
            BlendTreeParameterNode* paramNode = aznew BlendTreeParameterNode();
            m_blendTree->AddChildNode(paramNode);
            paramNode->InitAfterLoading(m_animGraph.get());
            paramNode->InvalidateUniqueData(m_animGraphInstance);
            m_blend2Node->AddConnection(paramNode, static_cast<uint16>(paramNode->FindOutputPortByName("weightParam")->m_portId), BlendTreeBlend2Node::PORTID_INPUT_WEIGHT);
        }

        void ConstructGraph()
        {
            AnimGraphFixture::ConstructGraph();

            /*
            Inside rootStateMachine:
                +------------+       +--------------+
                | motionNode |------>| blendTreeNode|
                +------------+       +--------------+

            Inside blendTreeNode:
                +-----------+
                |motionNodeA|---+
                +-----------+   |
                                +-->+----------+
                +-----------+       |          |     +---------+
                |motionNodeB|------>|blend2Node|---->|finalNode|
                +-----------+       |          |     +---------+
                                +-->+----------+
                  +---------+   |
                  |paramNode|---+
                  +---------+
            */
            m_stateStart = aznew AnimGraphMotionNode();
            m_blendTree = aznew BlendTree();
            AnimGraphMotionNode* motionNodeA = aznew AnimGraphMotionNode();
            AnimGraphMotionNode* motionNodeB = aznew AnimGraphMotionNode();
            m_blend2Node = aznew BlendTreeBlend2Node();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();

            m_blendTree->AddChildNode(motionNodeA);
            m_blendTree->AddChildNode(motionNodeB);
            m_blendTree->AddChildNode(m_blend2Node);
            m_blendTree->AddChildNode(finalNode);

            m_blend2Node->AddConnection(motionNodeA, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::PORTID_INPUT_POSE_A);
            m_blend2Node->AddConnection(motionNodeB, AnimGraphMotionNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::PORTID_INPUT_POSE_B);

            finalNode->AddConnection(m_blend2Node, BlendTreeBlend2Node::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_rootStateMachine->AddChildNode(m_stateStart);
            m_rootStateMachine->AddChildNode(m_blendTree);
            AddTransitionWithTimeCondition(m_stateStart, m_blendTree, 0.0f, 0.1f);

            m_rootStateMachine->SetEntryState(m_stateStart);
        }

    protected:
        AnimGraphMotionNode* m_stateStart = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeBlend2Node* m_blend2Node = nullptr;
    };

    TEST_F(DeferredInitBasicFixture, DeferredInitTwoStateTests)
    {
        size_t expectedUniqueDatas = 0;
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), expectedUniqueDatas) << "AnimGraph should not initialize nodes without update.";
        GetEMotionFX().Update(0.0f);

        // Root state machine, motion node, and time condition should be initialized.
        expectedUniqueDatas += 3;
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 3) << "Only root state machine, motion node, state transition time condition should be initialized.";

        // Activate the blend tree node by matching time condition.
        GetEMotionFX().Update(0.1f);

        // State transition, blend tree node, motion node A and B, parameter node, blend 2 node, and blend tree final node are initialized.
        expectedUniqueDatas += 7;
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), expectedUniqueDatas) << " Seven new unique data should be added.";

        ParamSetValue<MCore::AttributeFloat, float>("weightParam", 1.0f);
        GetEMotionFX().Update(0.1f);
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 10) << "Number of initialized unique data should not change.";
    }

    ///////////////////////////////////////////////////////////////////////////
    class DeferredInitBlendNNodeFixture
        : public AnimGraphFixture
    {
    public:
        // Create a blend N nodes as well as 5 motion inputs and directly connects the input motions with the blend N node.
        BlendTreeBlendNNode* CreateBlendNNode(BlendTree* blendTree, BlendTreeParameterNode* parameterNode, const char* blendNNodeName)
        {
            BlendTreeBlendNNode* blendNNode = aznew BlendTreeBlendNNode();
            blendNNode->SetName(blendNNodeName);
            blendTree->AddChildNode(blendNNode);

            const uint16 motionNodeCount = 5;
            for (uint16 i = 0; i < motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                motionNode->SetName(AZStd::string::format("Motion %i (%s)", i, blendNNodeName).c_str());
                blendTree->AddChildNode(motionNode);
                blendNNode->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
            }
            blendNNode->UpdateParamWeights();
            blendNNode->SetParamWeightsEquallyDistributed(-1.0f, 1.0f);

            blendNNode->AddUnitializedConnection(parameterNode, 0, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
            blendNNode->SetSyncMode(AnimGraphObject::ESyncMode::SYNCMODE_DISABLED);
            return blendNNode;
        }

        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();

            Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
            parameter->SetName("parameter_test");
            m_animGraph->AddParameter(parameter);

            AnimGraphMotionNode* entryMotionNode = aznew AnimGraphMotionNode();
            BlendTree* testBlendTree = aznew BlendTree();
            m_rootStateMachine->AddChildNode(entryMotionNode);
            m_rootStateMachine->AddChildNode(testBlendTree);

            AddTransitionWithTimeCondition(entryMotionNode, testBlendTree, 0.0f, 0.1f);
            m_rootStateMachine->SetEntryState(entryMotionNode);

            // Inside the blend tree.
            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            testBlendTree->AddChildNode(parameterNode);

            BlendTreeBlendNNode* blendNNode = aznew BlendTreeBlendNNode();
            testBlendTree->AddChildNode(blendNNode);

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            testBlendTree->AddChildNode(finalNode);

            blendNNode->AddUnitializedConnection(parameterNode, 0, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
            finalNode->AddConnection(blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            // Creates 5x blend N nodes as input for the blend N node created here. Each of these five blend N nodes have 5x input motions.
            for (uint16 i = 0; i < 5; ++i)
            {
                BlendTreeBlendNNode* inputNode = CreateBlendNNode(testBlendTree, parameterNode, AZStd::string::format("InputBlendNode%i", i).c_str());
                blendNNode->AddConnection(inputNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
            }
        }
    };

    TEST_F(DeferredInitBlendNNodeFixture, DeferredInitBlendNNodeTests)
    {
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), 0) << "AnimGraph should not initialize nodes without update.";
        MCore::AttributeFloat* testParameter = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);

        // Entry state active.
        GetEMotionFX().Update(0.0f);
        size_t expectedUniqueDatas = 3;
        size_t numInputBlendNNodes = 5;
        size_t numInputMotionNodes = 5;
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), expectedUniqueDatas)
            << "Only the root state machine, the entry motion node as well as the time condition should have their unique data allocated.";

        // Transitioning to blend tree.
        GetEMotionFX().Update(0.1f);

        // Transition towards blend tree and blend tree node.
        expectedUniqueDatas += 2;

        // Final node, parameter node, blend N node; 5x blend N nodes; 5x input motions of the first, currently active input blend N.
        expectedUniqueDatas += 3 + numInputBlendNNodes + numInputMotionNodes;
        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), expectedUniqueDatas) << "Two of the blend N input nodes as well as everything in the root state machine should have their unique datas allocated.";

        // Changing the weight to activate more of the blend N inputs step by step.
        for (int i = 1; i < 5; ++i)
        {
            // Original param weight was 0, so i starts at 1 to avoid test redundancy.
            const float weight = 0.25f * i;
            testParameter->SetValue(weight);
            GetEMotionFX().Update(0.1f);

            expectedUniqueDatas += numInputMotionNodes;
            EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), expectedUniqueDatas)
                << "Five new motion node unique datas should be allocated.";
        }

        EXPECT_EQ(m_animGraphInstance->CalcNumAllocatedUniqueDatas(), m_animGraphInstance->GetAnimGraph()->GetNumObjects())
            << "All objects should have their unique datas allocated.";
    }
} // end namespace EMotionFX
