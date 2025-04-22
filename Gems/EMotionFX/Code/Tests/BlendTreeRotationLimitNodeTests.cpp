
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <gtest/gtest_prod.h>
#include "AnimGraphFixture.h"
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
#include <EMotionFX/Source/BlendTreeRotationLimitNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>



namespace EMotionFX
{

    class BlendTreeRotationLimitNodeTests : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            BlendTreeRotationLimitNode* testBlendTreeRotationLimitNode = aznew BlendTreeRotationLimitNode();
            m_getTransformNode = aznew BlendTreeGetTransformNode();
            m_rotationMathNode = aznew BlendTreeRotationMath2Node();
            m_setTransformNode = aznew BlendTreeSetTransformNode();

            m_rotationMathNode->SetMathFunction(EMotionFX::BlendTreeRotationMath2Node::MATHFUNCTION_INVERSE_MULTIPLY);
            testBlendTreeRotationLimitNode->SetRotationLimitsX(-180.0f, 180.0f);
            testBlendTreeRotationLimitNode->SetRotationLimitsY(-180.0f, 180.0f);
            testBlendTreeRotationLimitNode->SetRotationLimitsZ(-45.0f, 45.0f);
            testBlendTreeRotationLimitNode->SetTwistAxis(EMotionFX::ConstraintTransformRotationAngles::EAxis::AXIS_Z);

            m_blendTree->AddChildNode(bindPoseNode);
            m_blendTree->AddChildNode(m_getTransformNode);
            m_blendTree->AddChildNode(m_rotationMathNode);
            m_blendTree->AddChildNode(m_setTransformNode);
            m_blendTree->AddChildNode(testBlendTreeRotationLimitNode);
            m_blendTree->AddChildNode(finalNode);

            m_getTransformNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_setTransformNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_rotationMathNode->AddConnection(m_getTransformNode, BlendTreeGetTransformNode::OUTPUTPORT_ROTATION, BlendTreeRotationMath2Node::INPUTPORT_Y);
            testBlendTreeRotationLimitNode->AddConnection(m_rotationMathNode, BlendTreeRotationMath2Node::OUTPUTPORT_RESULT_QUATERNION, BlendTreeRotationLimitNode::INPUTPORT_ROTATION);
            m_setTransformNode->AddConnection(testBlendTreeRotationLimitNode, BlendTreeRotationLimitNode::OUTPUTPORT_RESULT_QUATERNION, BlendTreeSetTransformNode::INPUTPORT_ROTATION);
            finalNode->AddConnection(m_setTransformNode, BlendTreeSetTransformNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

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

    TEST_F(BlendTreeRotationLimitNodeTests, RotationLimitTest)
    {
        const char* firstNodeName = m_actor->GetSkeleton()->GetNode(0)->GetName();

        m_getTransformNode->SetJointName(firstNodeName);
        m_getTransformNode->InvalidateUniqueData(m_animGraphInstance);

        m_setTransformNode->SetJointName(firstNodeName);
        m_setTransformNode->InvalidateUniqueData(m_animGraphInstance);

        AZ::Quaternion expectedRotation = AZ::Quaternion::CreateRotationZ(MCore::Math::pi * 0.25f);
        AZ::Quaternion desiredRotation = AZ::Quaternion::CreateRotationZ(MCore::Math::pi * 0.5f);

        m_rotationMathNode->SetDefaultValue(desiredRotation);

        Evaluate();
        Transform outputRoot = GetOutputTransform();

        Transform expected = Transform::CreateIdentity();
        expected.Set(AZ::Vector3::CreateZero(), expectedRotation);
        bool success = AZ::IsClose(expected.m_rotation.GetW(), outputRoot.m_rotation.GetW(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetX(), outputRoot.m_rotation.GetX(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetY(), outputRoot.m_rotation.GetY(), 0.0001f);
        success = success && AZ::IsClose(expected.m_rotation.GetZ(), outputRoot.m_rotation.GetZ(), 0.0001f);

        ASSERT_TRUE(success);
    }

} // end namespace EMotionFX
