/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphFixture.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/BlendTreeGetTransformNode.h>
#include <EMotionFX/Source/BlendTreeSetTransformNode.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/BlendTreeRotationMath2Node.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>


namespace EMotionFX
{
    class BlendTreeRotationMath2NodeTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            m_getTransformNode = aznew BlendTreeGetTransformNode();
            m_rotationMathNode = aznew BlendTreeRotationMath2Node();
            m_setTransformNode = aznew BlendTreeSetTransformNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();

            m_blendTree->AddChildNode(bindPoseNode);
            m_blendTree->AddChildNode(m_getTransformNode);
            m_blendTree->AddChildNode(m_rotationMathNode);
            m_blendTree->AddChildNode(m_setTransformNode);
            m_blendTree->AddChildNode(finalNode);

            m_getTransformNode->AddUnitializedConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_setTransformNode->AddUnitializedConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_rotationMathNode->AddUnitializedConnection(m_getTransformNode, BlendTreeGetTransformNode::OUTPUTPORT_ROTATION, BlendTreeRotationMath2Node::INPUTPORT_X);
            m_setTransformNode->AddUnitializedConnection(m_rotationMathNode, BlendTreeRotationMath2Node::OUTPUTPORT_RESULT_QUATERNION, BlendTreeSetTransformNode::INPUTPORT_ROTATION);
            finalNode->AddUnitializedConnection(m_setTransformNode, BlendTreeSetTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        BlendTree* m_blendTree = nullptr;
        BlendTreeGetTransformNode* m_getTransformNode = nullptr;
        BlendTreeRotationMath2Node* m_rotationMathNode = nullptr;
        BlendTreeSetTransformNode* m_setTransformNode = nullptr;
    };

    TEST_F(BlendTreeRotationMath2NodeTests, EvalauteTranslationBlending)
    {
        const char* firstNodeName = m_actor->GetSkeleton()->GetNode(0)->GetName();

        m_getTransformNode->SetJointName(firstNodeName);
        m_getTransformNode->InvalidateUniqueData(m_animGraphInstance);

        m_setTransformNode->SetJointName(firstNodeName);
        m_setTransformNode->InvalidateUniqueData(m_animGraphInstance);

        AZ::Quaternion expectedRotation = AZ::Quaternion::CreateRotationY(MCore::Math::pi * 0.25f);

        m_rotationMathNode->SetDefaultValue(expectedRotation);

        Evaluate();
        Transform outputRoot = GetOutputTransform();
        Transform expected = Transform::CreateIdentity();
        expected.Set(AZ::Vector3::CreateZero(), expectedRotation);
        bool success = AZ::IsClose(expected.m_rotation.GetW(), outputRoot.m_rotation.GetW(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetX(), outputRoot.m_rotation.GetX(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetY(), outputRoot.m_rotation.GetY(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetZ(), outputRoot.m_rotation.GetZ(), 0.0001f);

        m_rotationMathNode->SetMathFunction(EMotionFX::BlendTreeRotationMath2Node::MATHFUNCTION_INVERSE_MULTIPLY);
        
        expectedRotation = expectedRotation.GetInverseFull();
        Evaluate();
        outputRoot = GetOutputTransform();
        expected.Identity();
        expected.Set(AZ::Vector3::CreateZero(), expectedRotation);
        success = success && AZ::IsClose(expected.m_rotation.GetW(), outputRoot.m_rotation.GetW(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetX(), outputRoot.m_rotation.GetX(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetY(), outputRoot.m_rotation.GetY(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetZ(), outputRoot.m_rotation.GetZ(), 0.0001f);


        ASSERT_TRUE(success);
    }

} // end namespace EMotionFX
