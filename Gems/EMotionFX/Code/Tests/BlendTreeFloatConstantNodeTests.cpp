/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Random.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <limits.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class BlendTreeFloatConstantNodeFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            // Add nodes to blend tree.
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            m_blend2Node = aznew BlendTreeBlend2Node();
            m_blendTree->AddChildNode(m_blend2Node);
            m_floatConstantNode = aznew BlendTreeFloatConstantNode();
            m_blendTree->AddChildNode(m_floatConstantNode);
            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            m_blendTree->AddChildNode(bindPoseNode);

            // Connect the nodes.
            m_blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_A);
            m_blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_B);
            m_blend2Node->AddConnection(m_floatConstantNode, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(m_blend2Node, BlendTreeBlend2Node::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    protected:
        BlendTreeFloatConstantNode* m_floatConstantNode = nullptr;
        BlendTreeBlend2Node* m_blend2Node = nullptr;
        BlendTree* m_blendTree = nullptr;
    };

    TEST_F(BlendTreeFloatConstantNodeFixture, NodeOutputsCorrectFloat)
    {
        // Test max float constant node value.
        m_floatConstantNode->SetValue(FLT_MAX);
        GetEMotionFX().Update(1.0f / 60.0f);
        EXPECT_FLOAT_EQ(m_floatConstantNode->
            GetOutputFloat(m_animGraphInstance, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT)->GetValue(), FLT_MAX);

        // Test min float constant node value.
        m_floatConstantNode->SetValue(FLT_MIN);
        GetEMotionFX().Update(1.0f / 60.0f);
        EXPECT_FLOAT_EQ(m_floatConstantNode->
            GetOutputFloat(m_animGraphInstance, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT)->GetValue(), FLT_MIN);

        // Test 10 random float constant node value in range of [-5,5).
        AZ::SimpleLcgRandom random;
        random.SetSeed(875960);
        for (AZ::u32 i = 0; i < 10; ++i)
        {
            const float randomFloat = random.GetRandomFloat() * 10.0f - 5.0f;
            m_floatConstantNode->SetValue(randomFloat);
            GetEMotionFX().Update(1.0f / 60.0f);
            EXPECT_FLOAT_EQ(m_floatConstantNode->GetOutputFloat(m_animGraphInstance,
                BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT)->GetValue(), randomFloat);
        }
    }
} // end namespace EMotionFX
