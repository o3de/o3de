
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphFixture.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeTransformNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/Printers.h>


namespace EMotionFX
{
    class BlendTreeTransformNodeTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            m_transformNode = aznew BlendTreeTransformNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();

            m_blendTree->AddChildNode(m_transformNode);
            m_blendTree->AddChildNode(finalNode);
            m_blendTree->AddChildNode(parameterNode);

            m_transformNode->SetTargetNodeName("rootJoint");
            m_transformNode->SetMinTranslation(AZ::Vector3::CreateZero());
            m_transformNode->SetMaxTranslation(AZ::Vector3(10.0f, 0.0f, 0.0f));

            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
                parameter->SetName("translate_amount");
                m_blendTreeAnimGraph->AddParameter(parameter);
            }

            finalNode->AddUnitializedConnection(m_transformNode, BlendTreeTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_transformNode->AddUnitializedConnection(parameterNode, 0, BlendTreeTransformNode::PORTID_INPUT_TRANSLATE_AMOUNT);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        BlendTree* m_blendTree;
        BlendTreeTransformNode* m_transformNode;
    };

    // Basic test that just evaluates the node. Since the node is not doing anything,
    // The pose should not be affected. 
    TEST_F(BlendTreeTransformNodeTests, Evaluate)
    {
        Evaluate();
        ASSERT_EQ(Transform::CreateIdentity(), GetOutputTransform());
    }

    // Set some values to validate that the node is transforming the root
    TEST_F(BlendTreeTransformNodeTests, EvalauteTranslationBlending)
    {
        MCore::AttributeFloat* translate_amount = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);

        translate_amount->SetValue(0.0f);
        Evaluate();
        Transform outputRoot = GetOutputTransform();
        Transform expected = Transform::CreateIdentity();
        ASSERT_EQ(expected, outputRoot);

        translate_amount->SetValue(0.5f);
        Evaluate();
        expected.m_position = AZ::Vector3(5.0f, 0.0f, 0.0f);
        outputRoot = GetOutputTransform();
        ASSERT_EQ(expected, outputRoot);

        translate_amount->SetValue(1.0f);
        Evaluate();
        expected.m_position = AZ::Vector3(10.0f, 0.0f, 0.0f);
        outputRoot = GetOutputTransform();
        ASSERT_EQ(expected, outputRoot);
    }

} // end namespace EMotionFX
