/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeFloatConditionNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <limits.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    struct BlendTreeFloatConditionNodeTestData
    {
        float m_xInputFloat;
        float m_yInputFloat;
        bool m_expectedOutputForOneInputX[6];
        bool m_expectedOutputForOneInputY[6];
        bool m_expectedOutputForTwoInput[6];
    };

    class BlendTreeFloatConditionNodeFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<testing::tuple<bool, BlendTreeFloatConditionNodeTestData>>
    {
    public:
        void ConstructGraph() override
        {
            m_useBoolOutput = testing::get<0>(GetParam());
            m_param = testing::get<1>(GetParam());
            AnimGraphFixture::ConstructGraph();

            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            // Add nodes to blend tree.
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            BlendTreeBlend2Node* blend2Node = aznew BlendTreeBlend2Node();
            m_blendTree->AddChildNode(blend2Node);
            m_floatConditionNode = aznew BlendTreeFloatConditionNode();
            m_floatConditionNode->SetDefaultValue(0.0f);
            m_floatConditionNode->SetTrueResult(1.0f);
            m_floatConditionNode->SetFalseResult(0.0f);
            m_blendTree->AddChildNode(m_floatConditionNode);
            m_floatConstantNodeOne = aznew BlendTreeFloatConstantNode();
            m_blendTree->AddChildNode(m_floatConstantNodeOne);
            m_floatConstantNodeOne->SetValue(m_param.m_xInputFloat);
            m_floatConstantNodeTwo = aznew BlendTreeFloatConstantNode();
            m_blendTree->AddChildNode(m_floatConstantNodeTwo);
            m_floatConstantNodeTwo->SetValue(m_param.m_yInputFloat);
            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            m_blendTree->AddChildNode(bindPoseNode);

            // Connect the nodes.
            blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_A);
            blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_B);
            blend2Node->AddConnection(m_floatConditionNode, 
                m_useBoolOutput ? BlendTreeFloatConditionNode::PORTID_OUTPUT_BOOL : BlendTreeFloatConditionNode::PORTID_OUTPUT_VALUE, 
                BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(blend2Node, BlendTreeBlend2Node::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    protected:
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_floatConstantNodeOne = nullptr;
        BlendTreeFloatConstantNode* m_floatConstantNodeTwo = nullptr;
        BlendTreeFloatConditionNode* m_floatConditionNode = nullptr;
        BlendTreeFloatConditionNodeTestData m_param;
        bool m_useBoolOutput;
        const std::vector <BlendTreeFloatConditionNode::EFunction> m_eFuncs
        {
            BlendTreeFloatConditionNode::FUNCTION_EQUAL,
            BlendTreeFloatConditionNode::FUNCTION_NOTEQUAL,
            BlendTreeFloatConditionNode::FUNCTION_GREATER,
            BlendTreeFloatConditionNode::FUNCTION_LESS,
            BlendTreeFloatConditionNode::FUNCTION_GREATEROREQUAL,
            BlendTreeFloatConditionNode::FUNCTION_LESSOREQUAL
        };
    };

    std::vector<BlendTreeFloatConditionNodeTestData> blendTreeConditionNodeConditionTestData
    {
        /* 
        *    {
        *         X input node value, 
        *         Y input node value, 
        *         {expected outputs with respect to eFuncs for only x input}, 
        *         {expected outputs with respect to eFuncs for only y input}, 
        *         {expected outputs with respect to eFuncs for two inputs}
        *    } 
        */
        {
            1.0f,
            5.0f,
            {false, true, true, false, true, false},
            {false, true, false, true, false, true},
            {false, true, false, true, false, true}
        },
        {
            1.0f,
            1.0f,
            {false, true, true, false, true, false},
            {false, true, false, true, false, true},
            {true, false, false, false, true, true}
        },
        {
            1.0f,
            -1.0f,
            {false, true, true, false, true, false},
            {false, true, true, false, true, false},
            {false, true, true, false, true, false}
        }
    };

    TEST_P(BlendTreeFloatConditionNodeFixture, BlendTreeFloatConditionNode_NoInputNodeConditionTest)
    {
        // Node output correct value/bool under different condition functions without any input node connection
        for (AZ::u32 i = 0; i < 6; ++i)
        {
            m_floatConditionNode->SetFunction(m_eFuncs[i]);
            GetEMotionFX().Update(1.0f / 60.0f);
            // Default output for no input is false
            EXPECT_FLOAT_EQ(m_floatConditionNode->GetOutputFloat(m_animGraphInstance,
                m_useBoolOutput ? BlendTreeFloatConditionNode::PORTID_OUTPUT_BOOL : BlendTreeFloatConditionNode::PORTID_OUTPUT_VALUE)->
                GetValue(), 0.0f) << "(No input) Expected output: false";
        }
    };

    TEST_P(BlendTreeFloatConditionNodeFixture, BlendTreeFloatConditionNode_OneInputNodeConditionTest)
    {
        // Node output correct value/bool under different condition functions with one input node connection
        BlendTreeConnection* xInputPortConnection = m_floatConditionNode->
            AddConnection(m_floatConstantNodeOne, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeFloatConditionNode::PORTID_INPUT_X);
        // Test with only X input node
        for (AZ::u32 i = 0; i < 6; ++i) 
        {
            m_floatConditionNode->SetFunction(m_eFuncs[i]);
            GetEMotionFX().Update(1.0f / 60.0f);
                
            EXPECT_FLOAT_EQ(m_floatConditionNode->GetOutputFloat(m_animGraphInstance, 
                m_useBoolOutput ? BlendTreeFloatConditionNode::PORTID_OUTPUT_BOOL : BlendTreeFloatConditionNode::PORTID_OUTPUT_VALUE)->
                GetValue(), m_param.m_expectedOutputForOneInputX[i] ? 1.0f : 0.0f)
                << "(With only X input) expected ouput: " << m_param.m_expectedOutputForOneInputX[i];
        }
        
        m_floatConditionNode->RemoveConnection(xInputPortConnection);
        BlendTreeConnection* yInputPortConnection = m_floatConditionNode->
            AddConnection(m_floatConstantNodeTwo, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeFloatConditionNode::PORTID_INPUT_Y);
        // Test with only Y input node
        for (AZ::u32 i = 0; i < 6; ++i)
        {
            m_floatConditionNode->SetFunction(m_eFuncs[i]);
            GetEMotionFX().Update(1.0f / 60.0f);
            EXPECT_FLOAT_EQ(m_floatConditionNode->GetOutputFloat(m_animGraphInstance,
                m_useBoolOutput ? BlendTreeFloatConditionNode::PORTID_OUTPUT_BOOL : BlendTreeFloatConditionNode::PORTID_OUTPUT_VALUE)->
                GetValue(), m_param.m_expectedOutputForOneInputY[i] ? 1.0f : 0.0f)
                << "(With only Y input) expected ouput: " << m_param.m_expectedOutputForOneInputY[i];
        }
        m_floatConditionNode->RemoveConnection(yInputPortConnection);
    };

    TEST_P(BlendTreeFloatConditionNodeFixture, BlendTreeFloatConditionNode_TwoInputNodeConditionTest)
    {
        // Node output correct value/bool under different condition functions with two input node connection
        m_floatConditionNode->AddConnection(m_floatConstantNodeOne, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeFloatConditionNode::PORTID_INPUT_X);
        m_floatConditionNode->AddConnection(m_floatConstantNodeTwo, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeFloatConditionNode::PORTID_INPUT_Y);
        for (AZ::u32 i = 0; i < 6; ++i)
        {
            m_floatConditionNode->SetFunction(m_eFuncs[i]);
            GetEMotionFX().Update(1.0f / 60.0f);
            EXPECT_FLOAT_EQ(m_floatConditionNode->GetOutputFloat(m_animGraphInstance,
                m_useBoolOutput ? BlendTreeFloatConditionNode::PORTID_OUTPUT_BOOL : BlendTreeFloatConditionNode::PORTID_OUTPUT_VALUE)->
                GetValue(), m_param.m_expectedOutputForTwoInput[i] ? 1.0f : 0.0f)
                << "(With X and Y input) expected ouput: " << m_param.m_expectedOutputForTwoInput[i];
        }
    };

    INSTANTIATE_TEST_CASE_P(BlendTreeFloatConditionNode_ConditionTest,
        BlendTreeFloatConditionNodeFixture,
        ::testing::Combine(
            ::testing::Bool(),
            ::testing::ValuesIn(blendTreeConditionNodeConditionTestData)
        )
    );
} // end namespace EMotionFX
